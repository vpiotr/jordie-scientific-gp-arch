/////////////////////////////////////////////////////////////////////////////
// Name:        GasmVMachine.h
// Project:     scLib
// Purpose:     Virtual machine for Gasm language. 
//              Includes memory management and interpreter.
// Author:      Piotr Likus
// Modified by:
// Created:     31/01/2009
/////////////////////////////////////////////////////////////////////////////
#include "sc/defs.h"

#include <limits>

#include "base/algorithm.h"

#include "perf/log.h"

#include "sgp/GasmVMachine.h"
#include "sc/utils.h"
#include "sgp/GasmFunLib.h"

#define USE_INSTR_CACHE 
#define USE_GASM_STATS

#ifdef USE_GASM_STATS
#include "perf/counter.h"
#endif

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

using namespace dtp;
using namespace base;

//#define DEBUG_VMACHINE

void addFunctionToList(uint instrCode, sgpFunction *funct, sgpFunctionMapColn &list)
{
  list.insert(std::make_pair(instrCode, sgpFunctionTransporter(funct)));
}

sgpFunction *getFunctorForInstrCode(const sgpFunctionMapColn &functions,
  uint instrCodeRaw)
{
  sgpFunctionMapColnIterator p = const_cast<sgpFunctionMapColn &>(functions).find(instrCodeRaw);
  sgpFunction *functor;
  
  if (p == functions.end()) {
    return SC_NULL;
  } 
  
  sgpFunctionTransporter transporter = p->second;
  functor = &(*transporter);
  return functor;
}

// ----------------------------------------------------------------------------
// sgpProgramState
// ----------------------------------------------------------------------------    

void sgpProgramState::reset()
{
  clearRegisters();
  resetWarmWay();
}

void sgpProgramState::resetWarmWay()
{
  activeBlockNo = 0;
  activeCellNo = 1;
  nextCellNo = 0;
  nextBlockNo = -1;
  flags = 0;

  definedRegs.clear();
  callStack.clear();
  globalVars.clear();
  references.clear();
  reserve.clear();
  notes.clear();
  valueStack.clear();
}

void sgpProgramState::clearRegisters()
{
  activeRegs.clear();
  modifiedRegs.clear();
}

void sgpProgramState::markRegAsModified(uint regNo)
{
  modifiedRegs.insert(regNo);
}

void sgpProgramState::restoreRegisters(const scDataNode &src)
{
  if (!modifiedRegs.empty())
  {  
     scDataNode element;
     uint regNoEpos = std::min<uint>(src.size(), activeRegs.size());
     
     for(sgpRegUSet::const_iterator it = modifiedRegs.begin(), epos = modifiedRegs.end();
         it != epos;
         ++it)
     {
       if (*it < regNoEpos)
       {
         src.getElement(*it, element);       
         activeRegs.setElement(*it, element);
       }  
     }    
     modifiedRegs.clear();
  } else if (activeRegs.empty())
  {
     activeRegs.clear();
     activeRegs.copyFrom(src);
  }
}

// ----------------------------------------------------------------------------
// sgpProgramCode
// ----------------------------------------------------------------------------    

sgpProgramCode::sgpProgramCode()
{  
  m_maxBlockLength = 0;
}

sgpProgramCode::sgpProgramCode(const sgpProgramCode &rhs)
{
  m_maxBlockLength = 0;
  m_code = rhs.m_code;
  codeChanged();
}

sgpProgramCode::~sgpProgramCode()
{
}

void sgpProgramCode::codeChanged()
{
  // force update of max block length
  m_maxBlockLength = 0;
}

cell_size_t sgpProgramCode::getMaxBlockLength()
{
  if (m_maxBlockLength == 0)
  {
    for(uint i = 0,epos = m_code.size(); i != epos; i++)
      if (m_code[i].size() > m_maxBlockLength)
        m_maxBlockLength = m_code[i].size();
  }
  return m_maxBlockLength;
}

sgpProgramCode &sgpProgramCode::operator=(const sgpProgramCode &rhs)
{
  if (&rhs != this) {
    m_code = rhs.m_code;    
    codeChanged();
  }  
  return *this;
}

void sgpProgramCode::clear()
{
  m_code.clear();
  codeChanged();
}
  
uint sgpProgramCode::getBlockCount() const
{
  return m_code.size();
}

scDataNode &sgpProgramCode::getBlock(uint blockNo)
{
  return m_code[blockNo];
}

void sgpProgramCode::getBlock(uint blockNo, scDataNode &output)
{
  m_code.getElement(blockNo, output);
}

void sgpProgramCode::getBlockMetaInfo(uint blockNo, scDataNode &inputArgs, scDataNode &outputArgs) const
{
  inputArgs.clear();
  outputArgs.clear();
  
  if (blockNo < m_code.size()) {
    const scDataNode &blockCode = m_code[blockNo];    
    //scDataNode blockCode;
    //m_code.getElement(blockNo, blockCode);    
    
    scDataNode metaInfo;
    if (blockCode.size() > 0) {
      blockCode.getElement(0, metaInfo);

      if (metaInfo.size() > 0) 
        metaInfo.getElement(0, inputArgs);
        
      if (metaInfo.size() > 1) 
        metaInfo.getElement(1, outputArgs);        
    }
  }
}

void sgpProgramCode::getBlockMetaInfoForInput(uint blockNo, scDataNode &inputArgs) const
{
  scDataNode dummyValue;
  getBlockMetaInfo(blockNo, inputArgs, dummyValue);
}

void sgpProgramCode::getBlockMetaInfoForOutput(uint blockNo, scDataNode &outputArgs) const
{
  scDataNode dummyValue;
  getBlockMetaInfo(blockNo, dummyValue, outputArgs);
}

void sgpProgramCode::getBlockMetaInfo(uint blockNo, scDataNode &output) const
{
  output.clear();
  if (blockNo < m_code.size()) {
    const scDataNode &blockCode = m_code[blockNo];    
    if (blockCode.size() > 0) {
      blockCode.getElement(0, output);
    }  
  }    
}

void sgpProgramCode::setDefBlockMetaInfo(uint blockNo)
{
  setBlockMetaInfo(blockNo, scDataNode(), scDataNode());
}

void sgpProgramCode::setBlockMetaInfo(uint blockNo, const scDataNode &inputArgs, const scDataNode &outputArgs)
{
  if (blockNo < m_code.size()) {
    scDataNode metaInfo;
    metaInfo.addItem(inputArgs);
    metaInfo.addItem(outputArgs);
    setBlockMetaInfo(blockNo, metaInfo);
  }  
}

void sgpProgramCode::setBlockMetaInfo(uint blockNo, const scDataNode &metaInfo)
{
  if (blockNo < m_code.size()) {
    scDataNode &blockCode = m_code[blockNo];
    if (!blockCode.size()) { 
      blockCode.addItem(scDataNode());      
    }    

    blockCode.setElement(0, metaInfo);
  }  
}

bool sgpProgramCode::getBlockCodePart(uint blockNo, cell_size_t beginPos, cell_size_t count, scDataNode &output) const
{
  bool res;
  output.clear();
  
  if (blockNo >= m_code.size()) {
    res = false;
  } else {
    const scDataNode &blockCode = m_code[blockNo];    
    //scDataNode blockCode;
    //m_code.getElement(blockNo, blockCode);    
    
    scDataNode element;

    if (!output.isArray() && !output.isParent())
    {
      output.setAsParent();
    }
    
    for(int i=0,epos = blockCode.size(); i!=epos; i++)
    {
      blockCode.getElement(i, element);
      if (output.isArray())
        output.addItem(element);
      else
        output.addChild(new scDataNode(element));  
    }
    res = true;
  } // if block ok  
  return res;
}

// returns only code
void sgpProgramCode::getBlockCode(uint blockNo, scDataNode &output) const
{
  output = m_code[blockNo];
  //m_code.getElement(blockNo, output);
  output.eraseElement(0);  
}

void sgpProgramCode::setBlockCode(uint blockNo, const scDataNode &code)
{
  scDataNode inputArgs;
  scDataNode outputArgs;
  getBlockMetaInfo(blockNo, inputArgs, outputArgs);
  setBlock(blockNo, inputArgs, outputArgs, code);
}

void sgpProgramCode::clearBlock(uint blockNo)
{
  scDataNode nullBlock;
  m_code.setElement(blockNo, nullBlock);
}

void sgpProgramCode::getBlockCell(uint blockNo, uint cellOffset, scDataNode &output)
{
  m_code[blockNo].getElement(cellOffset, output);
}

scDataNode *sgpProgramCode::cloneBlockCell(uint blockNo, uint cellOffset)
{
  return m_code[blockNo].cloneElement(cellOffset); 
}

void sgpProgramCode::addBlock()
{
  if (!m_code.isContainer())
    m_code.setAsList();
    
  std::auto_ptr<scDataNode> newBlockGuard(new scDataNode());
  newBlockGuard->setAsList();
  m_code.addChild(newBlockGuard.release());
  
  codeChanged();
}

void sgpProgramCode::addBlock(scDataNode *block, scDataNode *metaInfo)
{
  std::auto_ptr<scDataNode> blockGuard(block);
  std::auto_ptr<scDataNode> metaGuard(metaInfo);
  
  uint blockNo = m_code.size();
  addBlock();

  if(metaGuard.get() != SC_NULL)
    m_code[blockNo].addItem(*metaGuard);
    
  m_code[blockNo].addItemList(*blockGuard);
  codeChanged();
}

void sgpProgramCode::addBlock(const scDataNode &inputArgs, const scDataNode &outputArgs, const scDataNode &code)
{
  uint blockNo = m_code.size();
  addBlock();
  setBlockMetaInfo(blockNo, inputArgs, outputArgs);
  m_code[blockNo].addItemList(code);
  codeChanged();
}

void sgpProgramCode::setBlock(uint blockNo, const scDataNode &inputArgs, const scDataNode &outputArgs, const scDataNode &code)
{
  m_code[blockNo].clear();
  setBlockMetaInfo(blockNo, inputArgs, outputArgs);
  m_code[blockNo].addItemList(code);
  codeChanged();
}

void sgpProgramCode::eraseLastBlock()
{
  if (m_code.size() > 0) 
    m_code.eraseElement(m_code.size() - 1);
  codeChanged();
}

void sgpProgramCode::eraseBlock(uint blockIndex)
{
  if (m_code.size() > blockIndex) 
    m_code.eraseElement(blockIndex);
  codeChanged();
}

scDataNode &sgpProgramCode::getFullCode()
{
  return m_code;
}

void sgpProgramCode::setFullCode(const scDataNode &code)
{
  m_code = code;
  codeChanged();
}

cell_size_t sgpProgramCode::getBlockLength(uint blockNo) const
{
  cell_size_t res;
  if (blockNo < m_code.size()) {
    res = m_code[blockNo].size();
  } else {
    res = 0;
  }    
  return res;
}

cell_size_t sgpProgramCode::getCodeLength() const
{
  cell_size_t res(0);
  for(uint i=0,epos = getBlockCount(); i != epos; ++i)
  {
    res += getBlockLength(i);
  }
  return res;
}

bool sgpProgramCode::blockExists(uint blockNo)
{
  return (blockNo < m_code.size());
}

// ----------------------------------------------------------------------------
// sgpFunction
// ----------------------------------------------------------------------------    
sgpFunction::sgpFunction(): scReferenceCounter()
{    
}

sgpFunction::~sgpFunction()
{
}

void sgpFunction::getArgCount(uint &a_min, uint &a_max) const
{
  a_min = a_max = 0;
}

bool sgpFunction::getArgMeta(scDataNode &output) const
{
  return false;
}

const scDataNode *sgpFunction::getArgMeta() const
{
  return m_argMetaGuard.get();
}

scString sgpFunction::getExportName() const
{
  return getName();
}

uint sgpFunction::getId() const
{
  return 0;
}

void sgpFunction::setMachine(sgpVMachine *machine)
{
  m_machine = machine;
}

uint sgpFunction::getFunctionCost() const
{
  return 1; // constant here
}

bool sgpFunction::isJumpAction()
{
  return false;
}
  
bool sgpFunction::isStrippable()
{
  return true;
}
  
void sgpFunction::getJumpArgs(scDataNode &args)
{ // do nothing
}

void sgpFunction::addArgMeta(uint ioMode, uint argTypeMask, uint dataTypeMask, scDataNode &output) const
{
  std::auto_ptr<scDataNode> guard(new scDataNode());
  guard->addChild(new scDataNode(ioMode));
  guard->addChild(new scDataNode(argTypeMask));
  guard->addChild(new scDataNode(dataTypeMask));

  output.addChild(guard.release());
}

void sgpFunction::prepare()
{
  if (m_argMetaGuard.get() == SC_NULL)
  {
    m_argMetaGuard.reset(new scDataNode());
    if (!getArgMeta(*m_argMetaGuard))
      m_argMetaGuard.reset();
  }  
}

bool sgpFunction::execute(const scDataNode &args, uint &execCost) const
{
  bool res = execute(args);
  execCost = getFunctionCost();
  return res;
}

bool sgpFunction::hasDynamicArgs() const
{
  return false;
}

// ----------------------------------------------------------------------------
// sgpFunctionForExpand
// ----------------------------------------------------------------------------    
sgpFunctionForExpand::sgpFunctionForExpand(): sgpFunction()
{
}
  
sgpFunctionForExpand::~sgpFunctionForExpand()
{
}

// ----------------------------------------------------------------------------
// sgpVMachine
// ----------------------------------------------------------------------------    
sgpVMachine::sgpVMachine()
{
  m_features = sgpGvmFeaturesDefault;
  m_readRegErrorLock = 0;
  
  setErrorLimit(SGP_DEF_ERROR_LIMIT);
  setDynamicRegsLimit(SGP_DEF_DYNAMIC_REGS_LIMIT);
  setDefaultDataType(gdtfInt);
  setSupportedArgTypes(gatfAny);
  setSupportedDataTypes(gdtfAll);
  setMaxStackDepth(SGP_DEF_MAX_STACK_DEPTH);
  setMaxAccessPathLength(SGP_DEF_MAX_ACCESS_PATH_LEN);
  setExtraRegDataTypes(gdtfInt + gdtfByte + gdtfFloat);
  setMaxItemCount(SGP_DEF_ITEM_LIMIT);
  setValueStackLimit(SGP_DEF_VALUE_STACK_LIMIT);
  setBlockSizeLimit(SGP_DEF_BLOCK_SIZE_LIMIT);

  resetProgram();
}

sgpVMachine::~sgpVMachine()
{
}

void sgpVMachine::run(uint instrLimit)
{
  m_instrLimit = instrLimit;
  prepareRun();
  beforeRun();
  intRun();
  afterRun();
}

void sgpVMachine::prepareRun()
{
  m_instrCount = 0;
  m_errorCount = 0;
}

void sgpVMachine::intRun()
{
  cell_size_t runLimit = m_instrLimit;
  bool useLimit = (m_instrLimit > 0);

  scDataNode *args = SC_NULL;
  std::auto_ptr<scDataNode> argsGuard;
  
  uint instrCode;
  uint argCount;
  sgpFunction *functor;
  bool found;
  ulong64 instrKey = 0;
  bool useTrace = ((ggfTraceEnabled & m_features) != 0);
  
  if (isFinished())
    return;
    
  m_instrCount = 0;
  do {
    m_lastOperCost = 0;    
    found = true;

#ifdef USE_INSTR_CACHE    
    if (!m_instrCacheRequired || !getActiveInstrFromCache(instrCode, argCount, args, functor))
    {
#endif
      if (argsGuard.get() != SC_NULL)
      {      
        argsGuard->clear();
      } else {       
        argsGuard.reset(new scDataNode());
      }  
      if (getActiveInstr(instrCode, argCount, *argsGuard.get(), functor)) {
        args = argsGuard.get();        
      } else {  
        found = false;
        argsGuard.reset();
      }  
#ifdef USE_INSTR_CACHE    
    }
#endif
    
    assert(!found || (args != SC_NULL));
    assert(!found || (argCount == 0) || (args->getValueType() != vt_null));
    
    if (found) {    
      m_programState.nextCellNo = m_programState.activeCellNo + (1 + argCount);
      if (useTrace) 
        addToTraceLogInstr(instrCode, *args, functor);

#ifdef USE_INSTR_CACHE    
      if (m_instrCacheRequired) {
        instrKey = calcActiveInstrCacheKey();
      }          
#endif      
      intRunInstr(instrCode, *args, functor);
      
      m_programState.activeCellNo = m_programState.nextCellNo;
    } else {
      m_programState.activeCellNo += (1 + argCount);
    }  
    
#ifdef USE_INSTR_CACHE    
    if (m_instrCacheRequired && (argsGuard.get() != SC_NULL))  
      addInstrToCache(instrKey, instrCode, argCount, argsGuard.release(), functor);
#endif
    
    if (useLimit) runLimit--;
    m_instrCount++;
    m_totalCost += m_lastOperCost;
    if ((m_errorCount > 0) && (m_errorCount == m_errorLimit))
      break; 

    if (isFinished())
    {
      if (!m_programState.callStack.empty())
      { 
        exitBlock();
        m_programState.activeCellNo = m_programState.nextCellNo;
      }  
      checkNextBlock();
    }      

  } while ((runLimit > 0 || !useLimit) && (!isFinished()));
}

void sgpVMachine::checkNextBlock()
{
  if (
       (m_programState.nextBlockNo >= 0) 
       && 
       (m_programState.nextBlockNo != static_cast<int>(m_programState.activeBlockNo))
       &&
       m_programCode.blockExists(m_programState.nextBlockNo)
     )
  {
    scDataNode blockResult = getOutput();
    prepareBlockRun(m_programState.nextBlockNo);   
    setInput(blockResult);
    setOutput(blockResult); // next block is optional
  }
}

bool sgpVMachine::isFinished()
{
  bool res = true;
  if (m_programCode.blockExists(m_programState.activeBlockNo)) {
    res = (m_programState.activeCellNo >= m_programCode.getBlockLength(m_programState.activeBlockNo));
  }
  return res;
}

void sgpVMachine::runInstr(uint code, const scDataNode &args)
{
  uint instrCode;
  uint argCount;
  decodeInstr(code, instrCode, argCount);
  
  sgpFunctionMapColnIterator p = m_functions.find(instrCode);
  sgpFunction *functor;
  argCount = args.size();
  
  if (p == m_functions.end()) {
    argCount = 0;
    functor = SC_NULL;
    handleUnknownInstr(instrCode, scDataNode());
  } else {
    sgpFunctionTransporter transporter = p->second;
    functor = &(*transporter);

    if (!checkArgCount(argCount, functor)) {
      handleWrongParamCount(code);        
    } else {
      // all OK
      intRunInstr(instrCode, args, functor);
    }  
  }  
}

bool sgpVMachine::verifyArgs(const scDataNode &args, sgpFunction *functor)
{
  bool staticTypes;
  uint argNo;
  bool res;
  const scDataNode *argMetaPtr;
  
  res = checkArgCount(args.size(), functor);  

  if (res) {
    argMetaPtr = functor->getArgMeta();
    if (argMetaPtr != SC_NULL) 
      res = checkArgMeta(args, *argMetaPtr, gatfInput+gatfOutput, argNo, staticTypes);
  }
      
  if (res)
    res = checkArgsSupported(args, argNo);  
    
  return res;  
}

void sgpVMachine::intRunInstr(uint code, const scDataNode &args, sgpFunction *functor)
{
  bool argsOk = true;

  assert(functor != SC_NULL);
  
  try {
    if ((ggfValidateArgs & m_features) != 0) 
    {
       ulong64 keyNum = calcActiveInstrCacheKey();
         
       if (!getArgsValidFromCache(keyNum)) {
         const scDataNode *argMetaPtr;
         bool staticTypes = false;
         uint argNo = 0;
         
         argMetaPtr = functor->getArgMeta();
         
         if (argMetaPtr != SC_NULL) {
           argsOk = checkArgMeta(args, *argMetaPtr, gatfInput+gatfOutput, argNo, staticTypes);
           if (!staticTypes) 
             addOperCostById(SGP_OPER_COST_DYNAMIC_TYPE_ARGS);
         }           
         if (argsOk)
           argsOk = checkArgsSupported(args, argNo);  
         if (staticTypes && argsOk)
           setArgsValidInCache(keyNum);  
         if (!argsOk) {
           handleWrongArgumentError(code, args, argNo);
         }  
       } 
    } 

    if (argsOk) {
      uint funCost;
      
      if (functor->execute(args, funCost))
      {
        addOperCost(funCost);
      } else {
        handleInstrError(code, args, GVM_ERROR_FUNCT_FAIL, "Function failed");
        addOperCostById(SGP_OPER_COST_FUNC_FAILED);
      }
    }
  }  
  catch(scError &excp) {
    handleInstrException(excp, code, args, functor);
  }
  catch(const std::exception& e) {
    handleInstrException(e, code, args, functor);
  }
  catch(...) {
    handleInstrException(scString("Unknown"), code, args, functor);
  }
}
  
// add cost specified by ID to last oper cost
void sgpVMachine::addOperCostById(uint a_subOperCostId, uint mult)
{
  uint cost;

  switch (a_subOperCostId) {
    case SGP_OPER_COST_UNK_INSTR:
      cost = 100;
      break;
    case SGP_OPER_COST_EXCEPTION:
      cost = 90;
      break;
    case SGP_OPER_COST_ERROR:  
      cost = 20;
      break;
    case SGP_OPER_COST_ARG_CNT_ERROR:  
      cost = 18;
      break;
    default:
      cost = 1;
      break;
  }
          
  m_lastOperCost += (cost * mult);
}

// add cost specified by value
void sgpVMachine::addOperCost(ulong64 a_cost)
{
  m_lastOperCost += a_cost;
}

// reset after code change
void sgpVMachine::resetProgram()
{
  m_instrCacheRequired = false;
  initRegisterTemplate();
  clearArgValidCache();
  clearInstrCache();
  m_programState.clearRegisters();
  resetWarmWay();
}

// reset data state, no program change was performed since last reset
void sgpVMachine::resetWarmWay()
{
  m_programState.resetWarmWay();
  m_programState.restoreRegisters(m_registerTemplate);
  m_totalCost = 0;
  if ((m_features & ggfNotesEnabled) != 0)
    m_programState.notes = m_notes;
  clearErrors();
  clearTrace();
}

void sgpVMachine::setStartBlockNo(uint blockNo)
{
  m_programState.activeBlockNo = blockNo;
}

void sgpVMachine::prepareBlockRun(uint blockNo)
{
  m_programState.resetWarmWay();
  m_programState.restoreRegisters(m_registerTemplate);
  clearInput();
  clearOutput();
  m_programState.activeBlockNo = blockNo;
  m_programState.activeCellNo = m_programState.nextCellNo = 1;
  m_programState.nextBlockNo = -1;
}

void sgpVMachine::clearInput()
{
#ifdef DEBUG_VMACHINE
  scLog::addDebug("vmach-clr-inp-beg");
#endif  
  scDataNode nullInput;
  for(uint i = SGP_REGB_INPUT; i <= SGP_REGB_INPUT_MAX; i++)
    m_programState.activeRegs.setElement(i, nullInput);
#ifdef DEBUG_VMACHINE
  scLog::addDebug("vmach-clr-inp-end");
#endif  
}

void sgpVMachine::clearOutput()
{
  setOutput(scDataNode());
}

void sgpVMachine::initBlockState(uint blockNo)
{
  m_programState.activeBlockNo = blockNo;
  m_programState.activeCellNo = m_programState.nextCellNo = 1;
  m_programState.definedRegs.clear();
  m_programState.references.clear();

  initRegisters();
}

bool sgpVMachine::getActiveInstrForLog(uint &instrCode, uint &argCount, scString &name)
{
  bool res = false;
  sgpFunction *functor;

  if (m_programCode.blockExists(m_programState.activeBlockNo)) {
    cell_size_t activeCellNo = m_programState.activeCellNo;
    scDataNode code;
    scDataNode cell;

    m_programCode.getBlock(m_programState.activeBlockNo, code);
    code.getElement(activeCellNo, cell);
    uint instrCodeCell = cell.getAsUInt();
    decodeInstr(instrCodeCell, instrCode, argCount);
    
    sgpFunctionMapColnIterator p = m_functions.find(instrCode);

    if (p != m_functions.end()) {
      sgpFunctionTransporter transporter = p->second;
      functor = &(*transporter);
      name = functor->getName();
      res = true;
    }
  }
  
  return res;
}

bool sgpVMachine::getActiveInstr(uint &instrCode, uint &argCount, scDataNode &args, sgpFunction *&functor)
{
  bool res = true;
  cell_size_t activeCellNo = m_programState.activeCellNo;
  scDataNode &code = m_programCode.getBlock(m_programState.activeBlockNo);
  uint instrCodeCell = code.getUInt(activeCellNo);
      
  decodeInstr(instrCodeCell, instrCode, argCount);
  
  sgpFunctionMapColnIterator p = m_functions.find(instrCode);

  args.clear();
  
  if (p == m_functions.end()) {
    functor = SC_NULL;
    handleUnknownInstr(instrCode, scDataNode());
    res = false;
  } else {
    sgpFunctionTransporter transporter = p->second;
    functor = &(*transporter);

    cell_size_t argOffset = activeCellNo + 1;
    cell_size_t codeSize = m_programCode.getBlockLength(m_programState.activeBlockNo);

    if (argOffset + argCount > codeSize) {
      if (argOffset <= codeSize) {
        argCount = codeSize - argOffset;
      } else {
        argCount = 0;  
      }  
    }

    if (!checkArgCount(argCount, functor))    
    {
      handleWrongParamCount(instrCodeCell);
      res = false;
    }    
     
    if (res) {  
      for(uint i=0; i<argCount; i++) {
        args.addChild(
          //code.cloneElement(argOffset + i));
          m_programCode.cloneBlockCell(m_programState.activeBlockNo, argOffset + i));
      }  
    }
  }  
  
  addOperCostById(SGP_OPER_COST_GET_ACT_INSTR);  
  return res;
}

ulong64 sgpVMachine::calcActiveInstrCacheKey()
{
  return
     m_programState.activeCellNo +
      (m_programCode.getMaxBlockLength() * m_programState.activeBlockNo);       
}

bool sgpVMachine::getActiveInstrFromCache(uint &instrCode, uint &argCount, scDataNode *&args, sgpFunction *&functor)
{  
  ulong64 keyNum = calcActiveInstrCacheKey();
  sgpInstructionInfo *ptr = m_instrCache.find(keyNum);
  bool res = (ptr != SC_NULL);
  if (res) {
    ptr->getActiveInstr(instrCode, argCount, args, functor);   
  }
    
  return res; 
}

void sgpVMachine::addInstrToCache(ulong64 keyNum, uint instrCode, uint argCount, scDataNode *args, sgpFunction *functor)
{
  m_instrCache.insert(keyNum, 
    new sgpInstructionInfo(instrCode, argCount, args, functor));
}

void sgpVMachine::clearInstrCache()
{
  m_instrCache.clear();
}
  
void sgpVMachine::getArgValue(const scDataNode &argInfo, scDataNode &output)
{
}

void sgpVMachine::setArgValue(const scDataNode &argInfo, const scDataNode &value)
{
}

uint sgpVMachine::getArgType(const scDataNode &argInfo)
{
  uint res;
  if (argInfo.isParent())
    res = argInfo.getUInt("arg_type", gatfConst);
  else if (argInfo.getValueType() == vt_uint) {
    if (argInfo.getAsUInt() >= (UINT_MAX - SGP_MAX_REG_NO)) 
      res = gatfRegister;
    else
      res = gatfConst;  
  } else 
    res = gatfConst;  
  return res;
}

uint sgpVMachine::getArgDataType(const scDataNode &argInfo)
{
  uint res;
  if (argInfo.isParent())
    res = argInfo.getUInt("data_type", gdtfInt);
  else
    res = gdtfInt;  
  return res;
}

bool sgpVMachine::castValue(const scDataNode &value, sgpGvmDataTypeFlag targetType, scDataNode &output)
{
  uint inType = calcDataType(value);
  
  if (inType == uint(targetType)) {
    output = value;
  } else {   
    initDataNodeAs(targetType, output); 
    if (((inType & gdtfAllBaseXInts) != 0) && ((targetType & gdtfAllBaseFloats) != 0))
    {
      scDataNode tempValue(xdouble(value.getAsInt64()));
      output.assignValueFrom(tempValue);      
    } else if (((inType & gdtfAllBaseFloats) != 0) && ((targetType & gdtfAllBaseXInts) != 0))
    {
      scDataNode tempValue(long64(value.getAsXDouble()));
      output.assignValueFrom(tempValue);      
    } else if (((inType & gdtfAllBaseFloats) != 0) && ((targetType & gdtfBool) != 0))
    {
      bool bValue;
      
      if (inType == gdtfFloat)
        bValue = !(std::fabs(value.getAsFloat()) < std::numeric_limits<float>::epsilon());      
      else if (inType == gdtfDouble)  
        bValue = !(std::fabs(value.getAsDouble()) < std::numeric_limits<double>::epsilon());      
      else  
        bValue = !(std::fabs(value.getAsXDouble()) < std::numeric_limits<long double>::epsilon());      
            
      output.assignValueFrom(scDataNode(bValue));      
    } else if (((inType & gdtfBool) != 0) && ((targetType & gdtfAllBaseFloats) != 0))
    {
      float fValue;
      
      if (value.getAsBool())
        fValue = 1.0;
      else
        fValue = 0.0;
            
      output.assignValueFrom(scDataNode(fValue));      
    } else if (((inType & gdtfAllBaseXInts) != 0) && ((targetType & gdtfBool) != 0))
    {
      bool bValue;
      
      if (inType == gdtfUInt64)
        bValue = (value.getAsUInt64() != 0);
      else
        bValue = (value.getAsInt64() != 0);
            
      output.assignValueFrom(scDataNode(bValue));      
    } else if (((inType & gdtfBool) != 0) && ((targetType & gdtfAllBaseXInts) != 0))
    {
      int iValue;
      
      if (value.getAsBool())
        iValue = 1;
      else
        iValue = 0;
            
      output.assignValueFrom(scDataNode(iValue));      
    } else if (((inType & (gdtfStruct+gdtfArray)) != 0) && ((targetType & (gdtfStruct+gdtfArray)) != 0))
    {
      scDataNode element;
      for(int i=0,epos=value.size(); i!=epos; i++)
      {
        value.getElement(i, element);
        output.addElement(element);
      }     
    } else if (((inType & (gdtfStruct+gdtfArray)) != 0) && ((targetType & (gdtfString)) != 0))
    {
      scDataNode element;
      scString sValue("");
      
      for(int i=0,epos=value.size(); i!=epos; i++)
      {
        value.getElement(i, element);
        sValue += (unsigned char)(element.getAsByte());
      }     
      
      output.setAsString(sValue);      
    } else if (((inType & (gdtfString)) != 0) && ((targetType & (gdtfStruct+gdtfArray)) != 0))
    {
      scDataNode element;
      scString sValue = value.getAsString();
      for(int i=0,epos=sValue.length(); i!=epos; i++)
      {
        output.addElement(scDataNode(byte(sValue[i])));
      }     
    } else { 
      output.assignValueFrom(value);
    }  
  }  
  return true;
}

bool sgpVMachine::isLastRunOk()
{ 
  return (m_errorLog.size() == 0);
}

void sgpVMachine::setProgramState(const sgpProgramState &state)
{
  m_programState = state;
}

void sgpVMachine::getProgramState(sgpProgramState &state) const
{
  state = m_programState;
}

void sgpVMachine::setProgramCode(const sgpProgramCode &code)
{
  m_programCode = code;
  resetProgram();
}

void sgpVMachine::getProgramCode(sgpProgramCode &code) const
{
  code = m_programCode;
}

void sgpVMachine::setFunctionList(const sgpFunctionMapColn &functions)
{
  m_functions.clear();
  sgpFunLib::duplicate(functions, m_functions);

  for(sgpFunctionMapColn::iterator p = m_functions.begin(); p != m_functions.end(); p++)
  {
    p->second->setMachine(this);
    p->second->prepare();
  }
}

uint sgpVMachine::getFeatures()
{
  return m_features;
}

void sgpVMachine::setFeatures(uint features)
{
  m_features = features;
}

void sgpVMachine::getErrorLog(scStringList &output)
{
  output = m_errorLog;
}

void sgpVMachine::getTraceLog(scStringList &output)
{
  output = m_traceLog;
}

void sgpVMachine::setErrorLimit(uint limit)
{
  m_errorLimit = limit;
}

void sgpVMachine::setDynamicRegsLimit(uint limit)
{
  m_dynamicRegsLimit = limit;
}

uint sgpVMachine::getSupportedArgTypes()
{
  return m_supportedArgTypes;
}

void sgpVMachine::setSupportedArgTypes(uint types)
{
  m_supportedArgTypes = types;
}

void sgpVMachine::setSupportedDataTypes(uint types)
{
  m_supportedDataTypes = types;
}

void sgpVMachine::setDefaultDataType(sgpGvmDataTypeFlag a_type)
{
  m_defDataType = a_type;
}

uint sgpVMachine::getMaxStackDepth()
{
  return m_maxStackDepth;
}

void sgpVMachine::setMaxStackDepth(uint value)
{
  m_maxStackDepth = value;
}

uint sgpVMachine::getMaxAccessPathLength()
{
  return m_maxAccessPathLength;
}

void sgpVMachine::setMaxAccessPathLength(uint value)
{
  m_maxAccessPathLength = value;
}

void sgpVMachine::setExtraRegDataTypes(uint value)
{
  m_extraRegDataTypes = value;
}

uint sgpVMachine::getExtraRegDataTypes()
{
  return m_extraRegDataTypes;
}  

void sgpVMachine::setRandomInit(bool value)
{
  m_randomInit = value;
}

bool sgpVMachine::getRandomInit()
{
  return m_randomInit;
}

bool sgpVMachine::getNotes(scDataNode &output)
{
  bool res;
  if ((m_features & ggfNotesEnabled)) {
    output = m_programState.notes;
    res = true;
  } else {
    output.clear();
    res = false;
  }      
  return res;
}

bool sgpVMachine::setNotes(const scDataNode &value)
{
  bool res;
  if ((m_features & ggfNotesEnabled)) {
    m_notes = value;
    m_programState.notes = value;
    res = true;
  } else {
    res = false;
  }      
  return res;
}

void sgpVMachine::setMaxItemCount(uint a_value)
{
  m_maxItemCount = a_value;
}

uint sgpVMachine::getMaxItemCount()
{
  return m_maxItemCount;
}

void sgpVMachine::setValueStackLimit(uint value)
{
  m_valueStackLimit = value;
}

void sgpVMachine::setBlockSizeLimit(uint value)
{
  m_blockSizeLimit = value;
}

scString sgpVMachine::getCurrentAddressCtx()
{
  scString res = 
    toString(m_programState.activeBlockNo)+
    ":"+
    toHexString(m_programState.activeCellNo);
  return res;  
}

scString sgpVMachine::getCurrentCtx()
{
  scString res;

  uint instrCode;
  uint argCount;
  scString name;
  bool activeInstr = false;
  
  try {
    activeInstr = getActiveInstrForLog(instrCode, argCount, name);
  } 
  catch(...) {
  // do nothing - inside log processing
  }  
  
  if (activeInstr) {
    res += ", instr-code: ["+toString(instrCode)+"]";
    res += ",instr-name: ["+name+"]";
    res += ", arg-count: ["+toString(argCount)+"]";
  }
  return res;  
}

scString sgpVMachine::formatErrorMsg(uint msgCode, const scString &msgText)
{
  scString res = getCurrentAddressCtx();
  res += ", msg-code: ["+toString(msgCode)+"], "+msgText+getCurrentCtx();
  return res;
}

void sgpVMachine::handleUnknownInstr(uint instrCode, const scDataNode &args)
{
  addOperCostById(SGP_OPER_COST_UNK_INSTR);
  m_errorCount++;
  if ((m_features & ggfLogErrors) != 0) {
    m_errorLog.push_back(
      formatErrorMsg(GVM_ERROR_UNK_INSTRUCTION, "Unknown instruction: "+toString(instrCode)));
  }  
}

void sgpVMachine::handleError(uint msgCode, const scString &msgText, const scDataNode *context)
{
  handleErrorCost(msgCode);
  if ((m_features & ggfLogErrors) != 0) {
    logError(msgCode, msgText);
  }    
}

void sgpVMachine::logError(uint msgCode, const scString &msgText, const scDataNode *context)
{
  m_errorLog.push_back(
    formatErrorMsg(msgCode, msgText));
}

void sgpVMachine::handleErrorCost(uint errorCode)
{
  addOperCostById(SGP_OPER_COST_ERROR);
  m_errorCount++;
}

void sgpVMachine::handleInstrError(uint instrCode, const scDataNode &args, uint errorCode, const scString &errorMsg)
{
  handleInstrErrorCost(instrCode, errorCode);
  
  if ((m_features & ggfLogErrors) != 0) {
    logInstrError(instrCode, args, errorCode, errorMsg);
  }    
}

void sgpVMachine::logInstrError(uint instrCode, const scDataNode &args, uint errorCode, const scString &errorMsg)
{
  m_errorLog.push_back(
    formatErrorMsg(GVM_ERROR_FUNCT_FAIL, 
      "Execution error, instruction: ["+toString(instrCode)+"], error: ["+toString(errorCode)+"], message: "+errorMsg));
}

void sgpVMachine::handleInstrErrorCost(uint instrCode, uint errorCode)
{
  switch (errorCode) {
    case GVM_ERROR_WRONG_PARAM_COUNT:
      addOperCostById(SGP_OPER_COST_ARG_CNT_ERROR);
      break;
    default:  
      addOperCostById(SGP_OPER_COST_ERROR);
      break;
  }    
  m_errorCount++;
}

void sgpVMachine::handleInstrError(const scString &name, const scDataNode &args, uint errorCode, const scString &errorMsg)
{
  addOperCostById(SGP_OPER_COST_ERROR);
  m_errorCount++;
  if ((m_features & ggfLogErrors) != 0) {
    m_errorLog.push_back(
      formatErrorMsg(GVM_ERROR_FUNCT_FAIL, 
        "Execution error, instruction: ["+name+"], error: ["+toString(errorCode)+"], message: "+errorMsg));
  }    
}

void sgpVMachine::handleInstrException(const std::exception& error, 
  uint code, const scDataNode &args, sgpFunction *functor)
{
  addOperCostById(SGP_OPER_COST_EXCEPTION);
  m_errorCount++;
  
  if ((m_features & ggfLogErrors) != 0) {
    m_errorLog.push_back(
      formatErrorMsg(GVM_ERROR_STD_EXCEPT, 
        "Std exception, instruction: ["+
        toString(code)+
       "], message: "+
       error.what()
      )
    );    
  }
}

void sgpVMachine::handleInstrException(const scError &error, 
  uint code, const scDataNode &args, sgpFunction *functor)
{
  addOperCostById(SGP_OPER_COST_EXCEPTION);
  m_errorCount++;

  if ((m_features & ggfLogErrors) != 0) {
    m_errorLog.push_back(
      formatErrorMsg(GVM_ERROR_SC_EXCEPT, 
        "Exception, instruction: ["+
        toString(code)+
       "], message: "+
       error.what()+
       ", details: "+
       const_cast<scError &>(error).getDetails()
      )
    );    
  }
}

void sgpVMachine::handleInstrException(const scString &errorType, 
  uint code, const scDataNode &args, sgpFunction *functor)
{
  addOperCostById(SGP_OPER_COST_EXCEPTION);
  m_errorCount++;

  if ((m_features & ggfLogErrors) != 0) {
    m_errorLog.push_back(
      formatErrorMsg(GVM_ERROR_SC_EXCEPT, 
        "Exception, "+errorType+", instruction: ["+
        toString(code)+
       "]"
      )
    );    
  }
}  

void sgpVMachine::handleWrongArgumentError(uint instrCode, const scDataNode &args, uint argNo)
{
  handleInstrErrorCost(instrCode, GVM_ERROR_WRONG_PARAMS);

  if ((m_features & ggfLogErrors) != 0) {
    scString msg("Wrong argument: ");
    msg += toString(argNo);
    logInstrError(instrCode, args, GVM_ERROR_WRONG_PARAMS, msg);
  }    
}

void sgpVMachine::handleWrongParamCount(uint instrCode)
{
  handleInstrErrorCost(instrCode, GVM_ERROR_WRONG_PARAM_COUNT);

  if ((m_features & ggfLogErrors) != 0) {
    scString msg("Wrong param count");
    logInstrError(instrCode, scDataNode(), GVM_ERROR_WRONG_PARAM_COUNT, msg);
  }    
}

void sgpVMachine::handleReadDisabled(uint regNo)
{
  handleErrorCost(GVM_ERROR_REG_READ_DISABLED);
  if ((m_features & ggfLogErrors) != 0) {
    logError(GVM_ERROR_REG_READ_DISABLED, "Error - read disabled, reg no.:"+toString(regNo));
  }  
}

void sgpVMachine::handleRegWriteDisabled(uint regNo)
{
  handleErrorCost(GVM_ERROR_REG_WRITE_DISABLED);
  if ((m_features & ggfLogErrors) != 0) {
    logError(GVM_ERROR_REG_WRITE_DISABLED, "Error - write disabled, reg no.:"+toString(regNo));
  }  
}

void sgpVMachine::handleRegWriteTypeError(uint regNo, const scDataNode &value)
{
  handleErrorCost(GVM_ERROR_REG_WRITE_WRONG_TYPE);
  if ((m_features & ggfLogErrors) != 0) {
    logError(GVM_ERROR_REG_WRITE_WRONG_TYPE, 
             "Error - wrong value type for reg write, reg no.:"+
               toString(regNo)+
               ", new value type: "+
               toString(calcDataType(value))
            );
  }  
}

void sgpVMachine::handleRecurrenceForbidden(uint blockNo)
{
  handleErrorCost(GVM_ERROR_RECURRENCE_FORBID);
  if ((m_features & ggfLogErrors) != 0) {
    logError(GVM_ERROR_RECURRENCE_FORBID, 
             "Recurrence forbidden, block: "+toString(blockNo)
    );
  }  
}

void sgpVMachine::handleWrongBlockNo(uint blockNo)
{
  handleErrorCost(GVM_ERROR_CALL_TO_WRONG_BLOCK_NO);
  if ((m_features & ggfLogErrors) != 0) {
    logError(GVM_ERROR_CALL_TO_WRONG_BLOCK_NO, 
             "Call - wrong block no.: "+toString(blockNo)
    );
  }  
}

void sgpVMachine::initRegisters()
{
  m_programState.activeRegs = m_registerTemplate;
}

//- #0 - return value(s) - type depends on block
//- #1..#33 - input params
//- #34..#60 - restricted type accumulators (signed int, unsigned int, float)
//- #61..#249 - variant variables (no definition required)
//-   note: on the end of variant vars build-in-data-type registers can appear depending on 
//      extraRegDataTypes property (in form of blocks of 16 registers)
//- #250..#255 - system variables:
//    #250 - creature state (persistent array)
//    #251 - local variables
//    #252 - current number of input vars
//    #253 - list of active program's code blocks, each as array of cells
//    #255 - processor status: last result #0, last instruction cost: #1...
// NOTE: registers needs to be created using child type
void sgpVMachine::initRegisterTemplate()
{  
  int i, a;
  scDataNode defValue, newValue;
  initDataNodeAs(m_defDataType, defValue);
  
  m_registerTemplate.clear();
  m_registerTemplateSize = 0;
    
  a = 0;
  // 0..33 - i/o (virtual regs, always variant - defined elsewhere)
  for(i = a; i <= a + 33; i++) {
    m_registerTemplate.addChild(new scDataNode());
  }  
  a = a + 34;  
  // 34..36 int
  addRegisterTplBlock(gdtfInt, 3);

  a += 3; 
  // 37..39 int64
  addRegisterTplBlock(gdtfInt64, 3);

  a += 3; 
  // 40..42 byte
  addRegisterTplBlock(gdtfByte, 3);

  a += 3; 
  // 43..45 uint
  addRegisterTplBlock(gdtfUInt, 3);

  a += 3; 
  // 46..48 uint64
  addRegisterTplBlock(gdtfUInt64, 3);

  a += 3; 
  // 49..51 float
  addRegisterTplBlock(gdtfFloat, 3);

  a += 3; 
  // 52..54 double
  addRegisterTplBlock(gdtfDouble, 3);

  a += 3; 
  // 55..57 xdouble
  addRegisterTplBlock(gdtfXDouble, 3);

  a += 3; 
  // 58..60 bool
  addRegisterTplBlock(gdtfBool, 3);
  
  a += 3; 
  // 61..249 variants or extra registers
  uint variantCnt = SGP_REGB_VIRTUAL - a;
  uint extraMask = m_supportedDataTypes & m_extraRegDataTypes;
  // add 16x regs for: int, int64, byte, uint, uint64, float, double, xdouble, bool depending on extraMask
  uint extraTypes[] = {gdtfInt, gdtfInt64, gdtfByte, gdtfUInt, gdtfUInt64, gdtfFloat, gdtfDouble, gdtfXDouble, gdtfBool, 0};
  // count total number of extra regs
  uint totalExtra = 0;
  for(int i=0; i < 255; i++)
  { 
    if (extraTypes[i] == 0) break; 
    if ((extraTypes[i] &  extraMask) != 0) totalExtra++; 
  }
    
  if (variantCnt > totalExtra * 16) {
    variantCnt = variantCnt - totalExtra * 16;
    addRegisterTplBlock(gdtfVariant, variantCnt);
  } else {
    variantCnt = 0;
  }    

  a += variantCnt;

  // add extra blocks
  for(int i=0; i < 255; i++)
  { 
    if (extraTypes[i] == 0) break; 
    if ((extraTypes[i] &  extraMask) != 0) {
      addRegisterTplBlock(sgpGvmDataTypeFlag(extraTypes[i]), 16);
      a += 16;
    }  
  }
    
  // 250..255 - virtual regs - always variants
  for(i = a; i <= a + 5; i++) {
    m_registerTemplate.addChild(new scDataNode());
  } 
  
  m_registerDataTypes.clear();
  m_registerDataTypes.resize(m_registerTemplate.size());
  for(uint i=0, epos = m_registerTemplate.size(); i != epos; i++)
  {
    if (m_registerTemplate[i].isNull())
      m_registerDataTypes[i] = gdtfVariant;
    else  
      m_registerDataTypes[i] = calcDataType(m_registerTemplate[i]);
  }  
  m_registerTemplateSize = m_registerTemplate.size();  
}

void sgpVMachine::addRegisterTplBlock(sgpGvmDataTypeFlag a_type, uint a_size)
{
  scDataNode newValue = buildRegisterValue(a_type);
  for(uint i = 0; i < a_size; i++) 
    m_registerTemplate.addChild(new scDataNode(newValue));  
}

scDataNode sgpVMachine::buildRegisterValue(sgpGvmDataTypeFlag a_type)
{
  scDataNode res;
  if ((a_type & m_supportedDataTypes) != 0) {
    initDataNodeAs(a_type, res);
  } else {
    initDataNodeAs(m_defDataType, res);
  }      
  return res;
}

bool sgpVMachine::canReadRegister(uint regNo) const
{
  if (regNo == 0)
    return false;
  else if (regNo == SGP_REGNO_PROGRAM_CODE)
    return ((m_features & ggfCodeAccessRead) != 0);
  else if (regNo == SGP_REGNO_GLOBAL_VARS)
    return ((m_features & ggfGlobalVarsEnabled) != 0);
  else if (regNo == SGP_REGNO_NOTES)
    return ((m_features & ggfNotesEnabled) != 0);
  else  
    return true;  
}

bool sgpVMachine::canWriteRegister(uint regNo) const
{
  if (regNo >= SGP_REGB_INPUT && regNo <= SGP_REGB_INPUT_MAX)
    return false;
  else if (regNo == SGP_REGNO_PROGRAM_CODE)    
    return ((m_features & ggfCodeAccessWrite) != 0);
  else if (regNo == SGP_REGNO_GLOBAL_VARS)
    return ((m_features & ggfGlobalVarsEnabled) != 0);
  else if (regNo == SGP_REGNO_NOTES)
    return ((m_features & ggfNotesEnabled) != 0);
  else
    return true;  
}

// fast check if register is variant, not precise
bool sgpVMachine::isVariantRegisterFast(uint regNo) const
{
  if (m_extraRegDataTypes == 0) {
    if ((regNo >= SGP_REGB_VARIANTS) && (regNo < SGP_REGB_VIRTUAL))
      return true;
    else
      return false;  
  } else {
    bool res = (getRegisterDefaultDataType(regNo) == gdtfVariant);
    return res;
  } 
}

uint sgpVMachine::getRegisterDefaultDataType(uint regNo) const
{
  if (m_registerTemplateSize > regNo) 
    return m_registerDataTypes[regNo];    
  else
    return gdtfVariant;  
}

// note: input and output is non-static (dynamic)
bool sgpVMachine::isStaticTypeRegister(uint regNo) const
{
  bool res = ((regNo >= SGP_REGB_ACCUMS) && (regNo < SGP_REGB_VARIANTS));
  
  if (!res) {
    if ((regNo >= SGP_REGB_VARIANTS) && (regNo < SGP_REGB_VIRTUAL) && (m_extraRegDataTypes != 0)) {
    // can be extra static register
      uint regType = getRegisterDefaultDataType(regNo);    
      switch (regType) {
        case gdtfNull:
        case gdtfStruct:
        case gdtfArray:
        case gdtfRef:
        case gdtfVariant:
          break;
        default:
          res = true;
      } // switch     
    } // inside variant
  } // not an accum
 return res;
}

bool sgpVMachine::isRegisterTypeMatched(uint regNo, const scDataNode &value, bool ignoreSizeDiff, 
  bool &typeRestricted) const
{
  return isRegisterTypeMatched(regNo, &value, 0, ignoreSizeDiff, typeRestricted);
}

bool sgpVMachine::isRegisterTypeMatched(uint regNo, uint valueTypeSet, bool ignoreSizeDiff, bool &typeRestricted) const
{
  return isRegisterTypeMatched(regNo, SC_NULL, valueTypeSet, ignoreSizeDiff, typeRestricted);
}

bool sgpVMachine::isRegisterTypeMatched(uint regNo, const scDataNode *value, uint valueTypeSet, bool ignoreSizeDiff, bool &typeRestricted) const
{
  bool res;
  uint valueType;
  if (value != SC_NULL)
    valueType = calcDataType(*value);
  else   
    valueType = valueTypeSet;
  
  //if (regNo >= SGP_REGB_VARIANTS || regNo < SGP_REGB_ACCUMS) {
  if (!isStaticTypeRegister(regNo)) {
    if (valueTypeSet != 0)
    // mask provided for check - have to be compatible
      res = ((valueType & gdtfVariant) != 0);
    else
    // new value provided for check for variant register - can be any value
      res = true;  
    typeRestricted = false;
  } else {
    uint regType = m_registerDataTypes[regNo];    
    if ((regType == gdtfNull) || (regType == gdtfVariant)) {
      res = true;
      typeRestricted = false;
    } else {
         
      typeRestricted = true;
      if (!ignoreSizeDiff) 
      {
        res = ((valueType & regType) != 0);
      } else {         
        switch (regType) {
          case gdtfInt:
            res = ((valueType & (gdtfInt + gdtfByte)) != 0);
            break;
          case gdtfInt64:
            res = ((valueType & (gdtfInt64 + gdtfInt + gdtfUInt + gdtfByte)) != 0);
            break;
          case gdtfByte:
            res = ((valueType & (gdtfByte)) != 0);
            break;
          case gdtfUInt:
            res = ((valueType & (gdtfUInt + gdtfByte)) != 0);
            break;
          case gdtfUInt64:
            res = ((valueType & (gdtfUInt64 + gdtfInt + gdtfUInt + gdtfByte)) != 0);
            break;
          case gdtfFloat:
            res = ((valueType & (gdtfFloat)) != 0);
            break;
          case gdtfDouble:
            res = ((valueType & (gdtfDouble + gdtfFloat)) != 0);
            break;
          case gdtfXDouble:
            res = ((valueType & (gdtfXDouble + gdtfDouble + gdtfFloat)) != 0);
            break;
          case gdtfBool:
          case gdtfString:
          case gdtfStruct:
          case gdtfArray:
          case gdtfRef:
            //res = (valueType == regType);
            res = ((valueType & regType) != 0);
            break;
          default:
          // unknown type
            res = false;
        } // switch
      }
    }  
  }  
  
  return res;  
}

// check if register is supported considering it's default data type
bool sgpVMachine::isRegisterTypeAllowed(uint regNo) const
{
  uint dataType;
  bool res = false;
  if (isVariantRegisterFast(regNo)) {
    res = ((m_supportedDataTypes & gdtfVariant) != 0);
  }
  else if (getRegisterDataType(dataType, regNo)) {
    res = ((m_supportedDataTypes & dataType) != 0);
  }
  return res;
}

bool sgpVMachine::isRegisterTypeIo(uint regNo) const
{
  if ((regNo == SGP_REGB_OUTPUT) ||
      (
        (regNo >= SGP_REGB_INPUT) &&
        (regNo <= SGP_REGB_INPUT_MAX)
      )
     )
  {
    return true;
  } else {
    return false;
  }            
}

bool sgpVMachine::isRegisterTypeVirtual(uint regNo) const
{
  if ((regNo >= SGP_REGB_VIRTUAL) && (regNo <= SGP_REGB_VIRTUAL_MAX))
    return true;
  else
    return false;   
}

bool sgpVMachine::isRegisterNo(uint value)
{
  if (value >= UINT_MAX - SGP_MAX_REG_NO)
    return true;
  else
    return false;  
}

uint sgpVMachine::getRegisterNo(const scDataNode &lvalue)
{
  uint regNo;
  if (lvalue.isParent() && lvalue.hasChild("value"))
    regNo = lvalue.getUInt("value");
  else {
    regNo = lvalue.getAsUInt();  
    if (regNo >= UINT_MAX - SGP_MAX_REG_NO)
      regNo = regNo - (UINT_MAX - SGP_MAX_REG_NO);
  }  
  return regNo;  
}

uint sgpVMachine::getRegisterNo(const scDataNodeValue &lvalue)
{
  uint regNo = lvalue.getAsUInt();  
  if (regNo >= UINT_MAX - SGP_MAX_REG_NO)
    regNo = regNo - (UINT_MAX - SGP_MAX_REG_NO);
  return regNo;  
}

void sgpVMachine::setLValue(const scDataNode &lvalue, const scDataNode &value, bool forceCast)
{
  uint argType = getArgType(lvalue);
  switch (argType) { 
    case gatfRegister: {
      uint regNo = getRegisterNo(lvalue);      
      setRegisterValue(regNo, value, false, forceCast);
      break;
    }  
    case gatfRef:
      setReferencedValue(lvalue, value, forceCast);
      break;
    case gatfNull:
    // do nothing
      break;
    default:
      handleError(GVM_ERROR_WRONG_LVALUE_TYPE, "Type: "+toString(argType));
      break;
  } // switch
  addOperCostById(SGP_OPER_COST_SET_LVALUE);  
} // function
  
bool sgpVMachine::getRegisterDataType(uint &dataType, uint regNo, bool ignoreRef) const
{
  scDataNode value;
  bool res = const_cast<sgpVMachine *>(this)->getRegisterValue(regNo, value, ignoreRef);
  dataType = calcDataType(value);
  return res;
}
  
bool sgpVMachine::getRegisterValue(uint regNo, scDataNode &value, bool ignoreRef)
{
  bool res;

  if (!canReadRegister(regNo)) {
    if (!isReadRegErrorLocked() && ((m_features & ggfLogReadErrors) != 0))            
      handleReadDisabled(regNo);
    res = false;
  } else {
    if (regNo < m_programState.activeRegs.size()) {
      m_programState.activeRegs.getElement(regNo, value);
      res = true;  
    } else {
      scString regNoName(toString(regNo));
      if (m_programState.definedRegs.hasChild(regNoName)) {
        value = m_programState.definedRegs[regNoName];
        res = true;
      } else {
        value.clear();
        res = false;
      }
    }
    
    if (!ignoreRef && res && isReferenceEnabled()) {
      uint argType = getArgType(value);
      if (argType == gatfRef) {
        scDataNode midValue(value);
        res = getReferencedValue(midValue, value);
      }
    }
  }
  return res;     
}

scDataNode *sgpVMachine::getRegisterPtr(uint regNo)
{
  scDataNode *res;

  if (regNo < SGP_REGB_VIRTUAL) {
    res = &(m_programState.activeRegs.getChildren().at(regNo));
  } else if (regNo == SGP_REGNO_GLOBAL_VARS) {
    res = &(m_programState.globalVars);
  } else if (regNo == SGP_REGNO_PROGRAM_CODE) {
    res = &(m_programCode.getFullCode());    
  } else {
    res = SC_NULL;
  }
  return res;
}

void sgpVMachine::clearRegisterValue(uint regNo)
{
  scDataNode nullValue;
  setRegisterValue(regNo, nullValue, true);
}

inline bool sgpVMachine::isReferenceEnabled()
{
  if ((gatfRef & m_supportedArgTypes) != 0)
    return true;
  else
    return false;  
}

void sgpVMachine::setRegisterValue(uint regNo, const scDataNode &value, bool ignoreRef, bool forceCast)
{
  bool isRef = false;
  bool valueFound = canReadRegister(regNo);
  bool refsEnabled = isReferenceEnabled();
    
  if (valueFound && refsEnabled) {
    scDataNode oldValue;
    if (getRegisterValue(regNo, oldValue, true))
    {
      if (getArgType(oldValue) == gatfRef) {
        if (ignoreRef) {
          releaseRef(regNo, oldValue);
        } else {
          isRef = true;  
          setReferencedValue(oldValue, value, forceCast);
        } 
      }
    } 
  }
  
  if (!isRef) {
   bool typeRestricted = false;
   
   if (!canWriteRegister(regNo)) {
      //handleError(GVM_ERROR_REG_WRITE_DISABLED, "Error - write disabled, reg no.:"+toString(regNo));
      handleRegWriteDisabled(regNo);
    } else if (!forceCast && !isRegisterTypeMatched(regNo, value, true, typeRestricted)) {  
      //handleError(GVM_ERROR_REG_WRITE_WRONG_TYPE, "Error - wrong value type for reg write, reg no.:"+toString(regNo)+", new value type: "+toString(calcDataType(value)));
      handleRegWriteTypeError(regNo, value);
    } else {    
      if (refsEnabled)
        clearRefsBelow(regNo, scDataNode());
      
      if (regNo < m_programState.activeRegs.size()) {
        if ((ggfTraceEnabled & m_features) != 0) addToTraceLogChange(regNo, value);
        if (typeRestricted || forceCast)
          m_programState.activeRegs[regNo].assignValueFrom(value);
        else
          m_programState.activeRegs[regNo] = value;
      } else if (m_programState.definedRegs.hasChild(toString(regNo))) {
        if ((ggfTraceEnabled & m_features) != 0) addToTraceLogChange(regNo, value);
        m_programState.definedRegs[toString(regNo)] = value;
      } else if ((m_features && ggfDynamicRegsEnabled) != 0) { 
        if ((m_dynamicRegsLimit == 0) || (m_programState.definedRegs.size() < m_dynamicRegsLimit)) {  
          if ((ggfTraceEnabled & m_features) != 0) addToTraceLogChange(regNo, value);
          m_programState.definedRegs.addChild(toString(regNo), new scDataNode(value));
        }  
        else { 
        // limit reached
          handleError(GVM_ERROR_REG_LIMIT_REACHED, "Reg limit reached");
        }  
      } else {
      // dynamic regs not allowed
         handleError(GVM_ERROR_DYN_REGS_DISABLED, "Dynamic regs disabled");
      } // dynamic regs

      if ((m_programState.flags && psfPushNextResult) != 0)
      {
        m_programState.flags ^= psfPushNextResult;
        pushValueToValueStack(value);        
      }  
        
      m_programState.markRegAsModified(regNo);      
    } // set OK
  } // is ref
}

void sgpVMachine::buildRegisterArg(scDataNode &ref, uint regNo)
{
  ref.clear();
  ref.setAsUInt(UINT_MAX - SGP_MAX_REG_NO + regNo);
  //ref.addChild(new scDataNode("arg_type", gatfRegister));
  //ref.addChild(new scDataNode("value", regNo));
}

scDataNode sgpVMachine::buildRegisterArg(uint regNo)
{
  scDataNode res;
  buildRegisterArg(res, regNo);
  return res; 
}

bool sgpVMachine::getReferencedValue(const scDataNode &ref, scDataNode &value)
{
  bool res;
  if (!ref.hasChild("reg_no"))
    res = false; 
  else {  
    uint regNo = ref.getUInt("reg_no");
    if (!canReadRegister(regNo)) {
      if (!isReadRegErrorLocked() && ((m_features & ggfLogReadErrors) != 0))
        handleError(GVM_ERROR_REG_READ_DISABLED, "Error - read disabled, reg no.:"+toString(regNo));
      res = false;
    } else if (!ref.hasChild("path")) {
      res = getRegisterValue(regNo, value);
      addOperCostById(SGP_OPER_COST_GET_REF_VALUE);  
    } else { 
      scDataNode &pathNode = const_cast<scDataNode &>(ref)["path"];
      if ((m_maxAccessPathLength > 0) && (pathNode.size() > m_maxAccessPathLength)) {
        handleError(GVM_ERROR_ACCESS_PATH_TOO_LONG_RD, "RD, Access path too long, reg no.:"+toString(regNo)+", path length: "+toString(pathNode.size()));
        res = false;
      } else {  
        scDataNode *curNode;
        curNode = getRegisterPtr(regNo);      
        if (curNode != SC_NULL)
          res = curNode->getElementByPath(pathNode, value);
        else {
          scDataNode curNode;
          res = getRegisterValue(regNo, curNode);
          if (res)
            res = curNode.getElementByPath(pathNode, value);
        }  
        addOperCostById(SGP_OPER_COST_GET_REF_VALUE, pathNode.size());  
      }
    }  
  }
  
  return res;
}

/// - change referenced value
/// - clear all references connected with the same access path but with longer access path
void sgpVMachine::setReferencedValue(const scDataNode &ref, const scDataNode &value, bool forceCast)
{
  if (ref.hasChild("reg_no")) 
  {
    uint regNo = ref.getUInt("reg_no");
    if ((ggfTraceEnabled & m_features) != 0) addToTraceLogChangeByRef(regNo, value);
    if (!canWriteRegister(regNo)) {
      handleError(GVM_ERROR_REG_WRITE_DISABLED, "Error - write disabled, reg no.:"+toString(regNo));
    } else if (!ref.hasChild("path")) {
      setRegisterValue(regNo, value, false, forceCast);
      addOperCostById(SGP_OPER_COST_SET_REF_VALUE);  
    } else { 
      scDataNode &pathNode = const_cast<scDataNode &>(ref)["path"];
      if ((m_maxAccessPathLength > 0) && (pathNode.size() > m_maxAccessPathLength)) {
        handleError(GVM_ERROR_ACCESS_PATH_TOO_LONG_WR, "WR, Access path too long, reg no.:"+toString(regNo)+", path length: "+toString(pathNode.size()));
      } else if (!pathNode.size()) {
        setRegisterValue(regNo, value, false, forceCast);
        addOperCostById(SGP_OPER_COST_SET_REF_VALUE);  
      } else {          
        scDataNode *curNode;
        curNode = getRegisterPtr(regNo);      
        if (curNode != SC_NULL)
          curNode->setElementByPath(pathNode, value);
        else {
          scDataNode tempValue;
          if (getRegisterValue(regNo, tempValue)) {
            tempValue.setElementByPath(pathNode, value);
            setRegisterValue(regNo, tempValue, false, forceCast);
          } // get value OK
        } // ptr unavailable  
        clearRefsBelow(regNo, pathNode);
        addOperCostById(SGP_OPER_COST_SET_REF_VALUE, pathNode.size());  
      } // non-empty path
   } // path param exists
 } // reg no param exists
} // function      

/// set all references to this register to null (invalidate)
void sgpVMachine::clearRefs(uint regNo)
{
  if (m_programState.references.find(regNo) != m_programState.references.end()) {
    scDataNode &refs = m_programState.references[regNo];
    scString regNoTxt;
    for(scDataNode::iterator p=refs.begin(), epos = refs.end(); p != epos; p++)
    {
      regNoTxt = p->getName();  
      clearRegisterValue(stringToUInt(regNoTxt));
    }  
    refs.getChildren().clear();
  }  
}

void sgpVMachine::clearRefsBelow(uint regNo, const scDataNode &path)
{
  if (m_programState.references.find(regNo) != m_programState.references.end()) {
    scString pathTxt = path.implode(",");
    scString checkPathTxt;
    scDataNode &refs = m_programState.references[regNo];
    scString regNoTxt;
    scStringList toClear;
    scDataNode helper;

    for(scDataNode::iterator p=refs.begin(), epos = refs.end(); p != epos; p++)
    {
      regNoTxt = p->getName();  
      checkPathTxt = p->getAsNode(helper).implode(",");
      if ((checkPathTxt.find(pathTxt) == 0) && (checkPathTxt.length() > pathTxt.length())) {
        clearRegisterValue(stringToUInt(regNoTxt));
      }  
      toClear.push_back(regNoTxt);
    }  
    
    for(scStringList::iterator sl = toClear.begin(), epos = toClear.end(); sl != epos; sl++) {
      regNoTxt = *sl;
      refs.getChildren().erase(regNoTxt);
    }
  }  
}

bool sgpVMachine::isRefInRegister(uint regNo) {
  bool res = false;
  scDataNode currValue;
  if (canReadRegister(regNo) && getRegisterValue(regNo, currValue, true)) {
    if (getArgType(currValue) == gatfRef) 
      res = true;
  }    
  return res;    
}

/// if inputRegNo contains constant - add reference to this reg no
/// if it contains reference - add reference to the final register (use element no. if exists)
void sgpVMachine::addRef(uint outputRegNo, uint inputRegNo, int elementNo, const scString &elementName)
{
  scDataNode currValue;
  uint useRegNo = inputRegNo;
  scDataNode path;
  
  if (canReadRegister(inputRegNo) && getRegisterValue(inputRegNo, currValue, true)) {
    if (getArgType(currValue) == gatfRef) {
      int checkRegNo = currValue.getInt("reg_no", -1); 
      if (checkRegNo > -1) {
        useRegNo = checkRegNo;
        if (currValue.hasChild("path"))        
          path = currValue["path"];
      }    
    }
  }

  if (elementNo > -1)
    path.addChild(new scDataNode(elementNo));
  else if (elementName.length() > 0) 
    path.addChild(new scDataNode(elementName));    
      
  scDataNode refValue, regValue;

  refValue.addChild("reg_no", new scDataNode(useRegNo));
  if (!path.empty())
    refValue.addChild("path", new scDataNode(path));
  refValue.addChild("arg_type", new scDataNode(static_cast<uint>(gatfRef)));

  scString textPath = path.implode(",");

  if (m_programState.references.find(useRegNo) == m_programState.references.end()) 
    m_programState.references.insert(std::make_pair<uint, scDataNode>(useRegNo, scDataNode()));
    
  m_programState.references[useRegNo].addChild(
    toString(outputRegNo), 
    new scDataNode(textPath));
      
  setRegisterValue(outputRegNo, refValue, true);  
}

void sgpVMachine::defineVar(uint outputRegNo, sgpGvmDataTypeFlag a_dataType)
{
  scDataNode value;
  if (getRegisterValue(outputRegNo, value)) {
    handleError(GVM_ERROR_VAR_ALREADY_DEFINED, "Error - var already defined, reg no.:"+toString(outputRegNo));
  } else {
    initDataNodeAs(a_dataType, value);
    m_programState.definedRegs.addChild(toString(outputRegNo), new scDataNode(value));
  }
}

bool sgpVMachine::defineArray(uint outputRegNo, sgpGvmDataTypeFlag a_dataType, cell_size_t a_size)
{
  bool res;
  scDataNode value;
  if (getRegisterValue(outputRegNo, value)) {
    handleError(GVM_ERROR_ARRAY_ALREADY_DEFINED, "Error - array already defined, reg no.:"+toString(outputRegNo));
    res = false;
  } else {
    scDataNode arrNode, tempNode;

    initDataNodeAs(a_dataType, tempNode);
    arrNode.setAsArray(tempNode.getValueType());
    
    m_programState.definedRegs.addChild(toString(outputRegNo), new scDataNode(arrNode));
    res = true;
  }
  return res;
}

void sgpVMachine::defineArrayFromCode(uint outputRegNo, sgpGvmDataTypeFlag a_dataType, cell_size_t a_size)
{
  if (defineArray(outputRegNo, a_dataType, a_size))
  {
    scDataNode code;
    if (m_programCode.getBlockCodePart(m_programState.activeBlockNo, m_programState.nextCellNo, a_size, code))
    {
      scDataNode tempValue, element;
      if (getRegisterValue(outputRegNo, tempValue)) {
        for(int i=0,epos = code.size(); i!=epos; i++)
        {
          code.getElement(i, element);
          tempValue.addItem(element);
        }  
        setRegisterValue(outputRegNo, tempValue);
      }  
    }
    m_programState.nextCellNo += a_size;
  }
}

/// disconnect single reference (unregister)
/// reference info:
///  - target register no
///  - access path (list of element indices - strings or integers)
///  - referencing register no
void sgpVMachine::releaseRef(uint refRegNo, const scDataNode &ref)
{
  if (ref.hasChild("reg_no")) {
    uint regNo = ref.getUInt("reg_no");
    if (m_programState.references.find(regNo) != m_programState.references.end())
      m_programState.references[regNo].getChildren().erase(toString(refRegNo));
  }
}

void sgpVMachine::beforeRun()
{
}

void sgpVMachine::afterRun()
{
}

uint sgpVMachine::encodeInstr(uint code, uint argCount)
{
  uint res;
  res = (argCount & 0x1f);
  res = res << 11;
  res = res | (code & 0x7ff);
  return res;
}

void sgpVMachine::decodeInstr(uint instrCode, uint &rawCode, uint &argCount)
{
  rawCode = (instrCode & 0x7ff);
  argCount = ((instrCode >> 11) & 0x1f);
}

bool sgpVMachine::isEncodedInstrCode(uint instrCode)
{
  return ((instrCode & 0xffff) == instrCode);
}

void sgpVMachine::clearErrors()
{
  m_errorLog.clear();
}

void sgpVMachine::clearTrace()
{
  m_traceLog.clear();
}

cell_size_t sgpVMachine::getInstrCount()
{
  return m_instrCount;
}

bool sgpVMachine::checkArgDataType(const scDataNode &value, uint allowedTypes)
{
  uint argType = calcDataType(value);  
  return ((argType & allowedTypes) != 0);
}

bool sgpVMachine::checkArgType(const scDataNode &value, uint allowedTypes)
{
  uint argType = getArgType(value);  
  return ((argType & allowedTypes) != 0);
}

uint sgpVMachine::calcDataType(const scDataNode &value) const
{
  uint argType = getArgType(value);  
  uint res;
  
  switch (argType) {
    case gatfRegister: {
      uint regNo = getRegisterNo(value);
      if (isStaticTypeRegister(regNo)) {
        res = getRegisterDefaultDataType(regNo);
      } else {  
        scDataNode regValue;
        if (const_cast<sgpVMachine *>(this)->getRegisterValueInternal(regNo, regValue))
          res = calcDataType(regValue);
        else
          res = gdtfNull;  
      } // dynamic reg
      break;        
    }  

    case gatfNull:
      res = gdtfNull;
      break;
      
    case gatfRef: {
      scDataNode refValue;
      if (const_cast<sgpVMachine *>(this)->getReferencedValueInternal(value, refValue)) {
        res = calcDataType(refValue);
      } else {
        res = gdtfNull;
      }               
      break;        
    }
    
    default: // including const 
    {
      scDataNodeValueType nodeValueType;
      if (value.empty() || !value.hasChild("value")) {
        nodeValueType = value.getValueType();
      } else {
        nodeValueType = value.getElementType("value");
      }  
      res = castDataNodeToGasmType(nodeValueType);    
      break;        
   } // case - const  
  } // switch arg type
   
  return res;   
}

bool sgpVMachine::getRegisterValueInternal(uint regNo, scDataNode &regValue)
{
  bool res = true;
  lockReadRegError();
  try {
    if (!getRegisterValue(regNo, regValue)) {
      res = false;
    }  
    unlockReadRegError();  
  }  
  catch(...) {
    unlockReadRegError(); 
    throw; 
  } // catch
  return res;
}

bool sgpVMachine::getReferencedValueInternal(const scDataNode &ref, scDataNode &value)
{
  bool res = true;
  lockReadRegError();
  try {
    if (!getReferencedValue(value, value)) {
      res = false;
    }  
    unlockReadRegError();  
  }  
  catch(...) {
    unlockReadRegError(); 
    throw; 
  } // catch
  return res;
}

sgpGvmDataTypeFlag sgpVMachine::castDataNodeToGasmType(scDataNodeValueType a_type)  
{
  sgpGvmDataTypeFlag res = gdtfNull;
  switch (a_type) {
    case vt_int:
      res = gdtfInt;
      break;
    case vt_uint:
      res = gdtfUInt;
      break;
    case vt_int64:
      res = gdtfInt64;
      break;
    case vt_uint64:
      res = gdtfUInt64;
      break;
    case vt_bool:
      res = gdtfBool;
      break;
    case vt_float:
      res = gdtfFloat;
      break;
    case vt_double:
      res = gdtfDouble;
      break;
    case vt_xdouble:
      res = gdtfXDouble;
      break;
    case vt_parent:
      res = gdtfStruct;
      break;
    case vt_array:
      res = gdtfArray;
      break;
    case vt_null:
      res = gdtfNull;
      break;
    case vt_byte: 
      res = gdtfByte;
      break;
    case vt_string:
      res = gdtfString;
      break;
    case vt_vptr:
      res = gdtfRef;
      break;
    default:
      res = gdtfString;        
      break;          
  } // switch sc datatype   
  return res;
}

void sgpVMachine::initDataNodeAs(sgpGvmDataTypeFlag a_type, scDataNode &output)  
{
  output.clear();
  switch (a_type) {
    case gdtfInt:
      output.setAsInt(0);
      break;
    case gdtfUInt:
      output.setAsUInt(0);
      break;
    case gdtfInt64:
      output.setAsInt64(0);
      break;
    case gdtfUInt64:
      output.setAsUInt64(0);
      break;
    case gdtfBool:
      output.setAsBool(false);
      break;
    case gdtfFloat:
      output.setAsFloat(0.0);
      break;
    case gdtfDouble:
      output.setAsDouble(0.0);
      break;
    case gdtfXDouble:
      output.setAsXDouble(0.0);
      break;
    case gdtfStruct:
      output.setAsParent();
      break;
    case gdtfArray:
      output.setAsArray(vt_datanode);
      break;
    case gdtfNull:
    case gdtfVariant:
      output.setAsNull();
      break;
    case gdtfByte: 
      output.setAsByte(0);
      break;
    case gdtfString:
      output.setAsString("");
      break;
    default: // includes gdtfRef
      throw scError("Incorrect data type for init: "+toString(a_type));
      break;          
  } // switch datatype   
}

scDataNodeValueType sgpVMachine::castGasmToDataNodeType(sgpGvmDataTypeFlag a_type)  
{
  scDataNodeValueType res = vt_null;
  
  switch (a_type) {
    case gdtfInt:
      res = vt_int;
      break;
    case gdtfUInt:
      res = vt_uint;
      break;
    case gdtfInt64:
      res = vt_int64;
      break;
    case gdtfUInt64:
      res = vt_uint64;
      break;
    case gdtfBool:
      res = vt_bool;
      break;
    case gdtfFloat:
      res = vt_float;
      break;
    case gdtfDouble:
      res = vt_double;
      break;
    case gdtfXDouble:
      res = vt_xdouble;
      break;
    case gdtfStruct:
      res = vt_parent;
      break;
    case gdtfArray:
      res = vt_array;
      break;
    case gdtfByte: 
      res = vt_byte;
      break;
    case gdtfString:
      res = vt_string;
      break;
    case gdtfNull:
    case gdtfVariant:
    case gdtfRef:
      res = vt_null;
      break;
    default: // includes gdtfRef
      throw scError("Incorrect data type: "+toString(a_type));
      break;          
  } // switch datatype   
  return res;
}

// prepare list of registers based on a given spec
void sgpVMachine::prepareRegisterSet(sgpGasmRegSet &regSet, uint allowedDataTypes, uint ioMode) const
{
  bool tempRestricted;
  uint useDataTypes = (allowedDataTypes & m_supportedDataTypes);
  
  regSet.clear();
  if ((ioMode & gatfInput) != 0) {
  // input
    // always add block input
    for(uint i=SGP_REGB_INPUT, epos = SGP_REGB_INPUT_MAX + 1; i != epos; i++)
      regSet.insert(i);
    // add templated registers  
    for(uint i = SGP_REGB_INPUT_MAX + 1, epos = m_registerTemplateSize; i != epos; i++)
    {
      if (canReadRegister(i)) {
        if (isRegisterTypeMatched(i, useDataTypes, false, tempRestricted))
          regSet.insert(i);        
      }
    }  
  } else {
  // strict output
    // always add block output
    regSet.insert(SGP_REGNO_OUTPUT);
    // add templated registers  
    for(uint i = SGP_REGB_INPUT_MAX + 1, epos = m_registerTemplateSize; i != epos; i++)
    {
      if (canWriteRegister(i)) {
        if (isRegisterTypeMatched(i, useDataTypes, false, tempRestricted))
          regSet.insert(i);        
      }
    }  
  }
}

bool sgpVMachine::argValidationEnabled()
{
  return ((m_features & ggfValidateArgs) != 0);
}

void sgpVMachine::evaluateArg(const scDataNode &input, scDataNode &output)
{
  addOperCostById(SGP_OPER_COST_EVALUATE_ARG);  
  uint argType = getArgType(input); 
  uint dataType;
   
  switch (argType) { 
    case gatfConst: {
      output = input;

      addOperCostById(SGP_OPER_COST_EVALUATE_CONST);

      dataType = getArgDataType(output);
      if ((dataType & (gdtfArray + gdtfStruct + gdtfString)) != 0) {
        addOperCostById(SGP_OPER_COST_EVALUATE_STRUCT_ARG);
      } else if ((dataType & gdtfVariant) != 0) {
        addOperCostById(SGP_OPER_COST_EVALUATE_VARIANT);
      }  

      break;
    }

    case gatfRegister: {
      uint regNo = getRegisterNo(input);      
      getRegisterValue(regNo, output);

      if ((getArgType(output) & gatfConst) == 0) {
        scDataNode midValue = output;
        evaluateArg(midValue, output);
      } else {
        dataType = getArgDataType(output);
        if ((dataType & (gdtfArray + gdtfStruct + gdtfString)) != 0) {
          addOperCostById(SGP_OPER_COST_EVALUATE_STRUCT_ARG);
        } else if (((dataType & (gdtfVariant)) != 0) || isVariantRegisterFast(regNo)) {
          addOperCostById(SGP_OPER_COST_EVALUATE_VARIANT);
        }  
      } 
      break;
    }  

    case gatfRef:
      getReferencedValue(input, output);
      break;

    case gatfNull: {
      output.clear();
      break;
    }
      
    default:
      handleError(GVM_ERROR_UNDEF_ARG_TYPE, "Type: "+toString(argType));
      break;
  } // switch  
}

ulong64 sgpVMachine::getTotalCost()
{
  return m_totalCost;
}

scDataNode sgpVMachine::getOutput()
{
  assert(!m_programState.activeRegs.empty());
#ifdef DEBUG_VMACHINE
  if (m_programState.activeRegs.empty())
    throw scError("ActiveRegs are empty!");
#endif      
  scDataNode res;
  m_programState.activeRegs.getElement(SGP_REGB_OUTPUT, res);
  return res;
}

void sgpVMachine::setOutput(const scDataNode &value)
{
  assert(!m_programState.activeRegs.empty());
#ifdef DEBUG_VMACHINE
  if (m_programState.activeRegs.empty())
    throw scError("ActiveRegs are empty!");
#endif      
  if (value.size() > 0) { 
    scDataNode element;
    for(int i=0,epos = value.size(), cnt = 0; (i != epos) && (cnt < SGP_REGB_OUTPUT_MAX); i++, cnt++)
    {
      m_programState.activeRegs.setElement(SGP_REGB_OUTPUT + cnt, value.getElement(i));      
    }  
  } else { 
  //not an array
    m_programState.activeRegs.setElement(SGP_REGB_OUTPUT + 0, value);      
  }  
}

void sgpVMachine::setInput(const scDataNode &value)
{
  assert(!m_programState.activeRegs.empty());
#ifdef DEBUG_VMACHINE
  if (m_programState.activeRegs.empty())
    throw scError("ActiveRegs are empty!");
#endif      
  if (value.size() > 0) { 
    scDataNode element;
    for(int i=0,epos = value.size(), cnt = 0; (i != epos) && (cnt < SGP_REGB_INPUT_MAX); i++, cnt++)
    {
      m_programState.activeRegs.setElement(SGP_REGB_INPUT + cnt, value.getElement(i));      
    }  
  } else { 
  //not an array
    m_programState.activeRegs.setElement(SGP_REGB_INPUT + 0, value);      
  }  
}

// optimize code for repeated executions
void sgpVMachine::prepareCode(sgpProgramCode &code) const
{
  if ((gatfRef & m_supportedArgTypes) == 0)
    stripUnusedCodeAfterLastOutWrite(code);
}

void sgpVMachine::stripUnusedCodeAfterLastOutWrite(sgpProgramCode &code) const
{
  scDataNode blockCode;
  cell_size_t instrOffsetForScan, stripPos, endOffset, argOffset;
  uint instrCode, instrCodeRaw, argCount, argCountFound, argEpos;
  uint skipSize, regNo, ioMode, argDataTypes;
  const scDataNode *argMetaPtr;
  scDataNode tempCode;
  sgpFunction *functor; 
  bool codeChanged;
    
  for(uint i = 0, epos = code.getBlockCount(); i != epos; i++)
  {
    code.getBlockCode(i, blockCode);  
    codeChanged = false;

    instrOffsetForScan = 0;
    endOffset = blockCode.size();
    stripPos = endOffset;
    
    while(instrOffsetForScan < endOffset) {
      skipSize = 1;
      
      if (blockCode.getElementType(instrOffsetForScan) == vt_uint) {
        instrCode = blockCode.getUInt(instrOffsetForScan);      
        decodeInstr(instrCode, instrCodeRaw, argCount);
        functor = ::getFunctorForInstrCode(m_functions, instrCodeRaw);
        argEpos = SC_MIN(instrOffsetForScan+1+argCount, endOffset);
        argCountFound = argEpos - (instrOffsetForScan + 1);
        argCount = SC_MIN(argCount, argCountFound);
        if (functor != SC_NULL) 
          argMetaPtr = functor->getArgMeta();
        else
          argMetaPtr = SC_NULL;  
        
        if ((functor != SC_NULL) && (argMetaPtr != SC_NULL))
        {
          skipSize = 1 + argCount;
          
          if (!functor->isStrippable())
          {
            stripPos = instrOffsetForScan + skipSize;
          } else {            
            for(uint argNo=0,epos=SC_MIN(argCount, argMetaPtr->size()); argNo != epos; argNo++)
            {
              argOffset = instrOffsetForScan + 1 + argNo;
              ioMode = sgpVMachine::getArgMetaParamUInt(*argMetaPtr, argNo, GASM_ARG_META_IO_MODE);    
              argDataTypes = sgpVMachine::getArgMetaParamUInt(*argMetaPtr, argNo, GASM_ARG_META_DATA_TYPE);
              argDataTypes &= m_supportedDataTypes;    
              
              if (getArgType(blockCode.getElement(argOffset)) == gatfRegister) {
                if ((ioMode & gatfOutput) != 0) {              
                // output reg
                  regNo = sgpVMachine::getRegisterNo(blockCode.getElement(argOffset));
                  if (regNo == SGP_REGB_OUTPUT) {
                    stripPos = instrOffsetForScan + skipSize;
                  }
                } 
              }  
            } // for
          } // strippable  
        } // if functor OK
      } // if instruction
      instrOffsetForScan += skipSize;
    } // while
    
    if (stripPos < endOffset) {     
      blockCode.eraseFrom(stripPos);
      codeChanged = true;
    }  
    
    if (codeChanged) {
      code.setBlockCode(i, blockCode);
    }
  } // for
}

void sgpVMachine::expandCode(sgpProgramCode &code, uint firstBlockNo, bool validateOrder) const
{
  scDataNode blockCode;
  cell_size_t instrOffsetForScan, endOffset;
  uint instrCode, instrCodeRaw, argCount, argCountFound, argEpos;
  uint skipSize;
  scDataNode tempCode;
  sgpFunction *functor; 
  sgpFunctionForExpand *functionExp;
  bool codeChanged;
  scDataNode cell;
  scDataNode args;
    
  uint blockNo = code.getBlockCount(); 
  
  bool prgChanged = false;
  bool blockChanged;
  
  // expand lowest level first 
  while(blockNo > firstBlockNo) 
  {
    blockNo--;
    code.getBlockCode(blockNo, blockCode);  
    codeChanged = false;
    blockChanged = false;

    instrOffsetForScan = 0;
    endOffset = blockCode.size();
    
    while(instrOffsetForScan < endOffset) {
      skipSize = 1;
      codeChanged = false; 
      
      if (blockCode.getElementType(instrOffsetForScan) == vt_uint) {
        instrCode = blockCode.getUInt(instrOffsetForScan);      
        decodeInstr(instrCode, instrCodeRaw, argCount);
        functor = ::getFunctorForInstrCode(m_functions, instrCodeRaw);
        argEpos = SC_MIN(instrOffsetForScan+1+argCount, endOffset);
        argCountFound = argEpos - (instrOffsetForScan + 1);
        argCount = SC_MIN(argCount, argCountFound);
        if (functor != SC_NULL) {
          functionExp = query_cast<sgpFunctionForExpand>(functor);
        }
        else {
          functionExp = SC_NULL;  
        }
        
        if (functionExp != SC_NULL)
        {
          skipSize = 1 + argCount;

          if (functionExp != SC_NULL) {
            args.clear();
            for(uint i=0; i < argCount; i++) {
              args.addChild(blockCode.cloneElement(instrOffsetForScan + i + 1));
            }  

            if (functionExp->expand(args, code, blockNo, instrOffsetForScan, validateOrder)) 
            {
              code.getBlockCode(blockNo, blockCode);  
              endOffset = blockCode.size();
              codeChanged = true; 
            }  
          }
        } // if functor OK
      } // if instruction
      if (codeChanged) {
        blockChanged = true;
        if ((m_blockSizeLimit > 0) && (endOffset >= m_blockSizeLimit))
          instrOffsetForScan = endOffset;
        else  
          instrOffsetForScan = 0;          
      } else {   
        instrOffsetForScan += skipSize;
      }  
    } // while instr
#ifdef USE_GASM_STATS
  if (blockChanged) {
#ifdef GVM_USE_COUNTERS  
    //m_counters.inc("vmach-expand-block-cnt");
#endif    
    prgChanged = true;
  }  
#endif                  
  } // while block
#ifdef GVM_USE_COUNTERS
  if (prgChanged) {  
    //m_counters.inc("vmach-expand-prg-cnt");
  }  
#endif                  
}

// expand macro instruction, args[0] is output, args[1..k] is input 
// returns <false> if code was not changed
bool sgpVMachine::expandMacro(uint macroNo, const scDataNode &args, sgpProgramCode &programCode, uint blockNo, cell_size_t instrOffset, bool validateOrder)
{
  bool res = true;
  scDataNode blockCode;
  
  programCode.getBlockCode(blockNo, blockCode);  
  
  uint instrCode = blockCode.getUInt(instrOffset);
  uint instrCodeRaw, argCnt;  
  scDataNode macroCode;
    
  decodeInstr(instrCode, instrCodeRaw, argCnt);
  uint delSize = 1 + argCnt;    

  // remove "macro" instruction
  while ((delSize > 0) && (instrOffset < blockCode.size()))
  {
    blockCode.eraseElement(instrOffset);
    delSize--;
  }

  // prepare new code      
  if ((!validateOrder || (macroNo > blockNo)) && (macroNo < programCode.getBlockCount()))
  {
     scDataNode macroInputMeta;
     
     programCode.getBlockMetaInfoForInput(macroNo, macroInputMeta);
     programCode.getBlockCode(macroNo, macroCode);        
     
     if (!args.empty())
     {
       // replace macro's input arguments with provided args
       macroReplaceInputRegsWithArgs(macroInputMeta, args, macroCode);     

       // replace macro's output arguments with provided args[0]
       macroReplaceArgRegInCode(SGP_REGB_OUTPUT, args[0], gatfOutput, 0, macroCode);
     }  
  }    

  // insert new code
  if (!macroCode.empty()) {
    typedef std::vector<scDataNodeValue> scDataVector;
    scDataVector blockCodeVector;
    scDataVector macroCodeVector;
    blockCode.copyTo(blockCodeVector);
    macroCode.copyTo(macroCodeVector);
    
    // insert macro code
    blockCodeVector.insert(blockCodeVector.begin() + instrOffset,
        macroCodeVector.begin(),
        macroCodeVector.end());

    blockCode.copyValueFrom(blockCodeVector);
  }
  
  programCode.setBlockCode(blockNo, blockCode);
  
  return res;
}

// note: first arg is an output arg
void sgpVMachine::macroReplaceInputRegsWithArgs(const scDataNode &macroInputMeta, const scDataNode &args, scDataNode &code)
{
  if (!args.empty())
    for(uint i=0, epos = SC_MIN(macroInputMeta.size(), args.size() - 1); i != epos; i++)
      macroReplaceArgRegInCode(SGP_REGB_INPUT + i, args[i + 1], gatfInput, 0, code);
}

bool sgpVMachine::macroReplaceArgRegInCode(uint oldRegNo, const scDataNode &newValue, uint searchIoMode, 
  cell_size_t instrOffset, scDataNode &blockCode)
{
  bool res = false;
  cell_size_t instrOffsetForScan = instrOffset;
  cell_size_t skipSize, argOffset;  
  cell_size_t endOffset = blockCode.size();
  uint instrCode, instrCodeRaw;
  uint argCount, argEpos, argCountFound;  
  uint ioMode, regNo;
  const scDataNode *argMetaPtr;
  sgpFunction *functor; 
  
  while(instrOffsetForScan < endOffset) {
    skipSize = 1;
    
    if (blockCode.getElementType(instrOffsetForScan) == vt_uint) {
      instrCode = blockCode.getUInt(instrOffsetForScan);      
      decodeInstr(instrCode, instrCodeRaw, argCount);
      functor = ::getFunctorForInstrCode(m_functions, instrCodeRaw);
      argEpos = SC_MIN(instrOffsetForScan+1+argCount, endOffset);
      argCountFound = argEpos - (instrOffsetForScan + 1);
      argCount = SC_MIN(argCount, argCountFound);
      
      if (functor != SC_NULL) 
        argMetaPtr = functor->getArgMeta();
      else
        argMetaPtr = SC_NULL;  
      
      if ((functor != SC_NULL) && (argMetaPtr != SC_NULL))
      {
        skipSize = 1 + argCount;
        
        for(uint argNo=0,epos=SC_MIN(argCount, argMetaPtr->size()); argNo != epos; argNo++)
        {
          argOffset = instrOffsetForScan + 1 + argNo;
          ioMode = sgpVMachine::getArgMetaParamUInt(*argMetaPtr, argNo, GASM_ARG_META_IO_MODE);    
          
          if (getArgType(blockCode.getElement(argOffset)) == gatfRegister) {
            if ((ioMode & searchIoMode) != 0) {              
            // input reg
              regNo = sgpVMachine::getRegisterNo(blockCode.getElement(argOffset));
              if (regNo == oldRegNo) {
                blockCode.setElement(argOffset, newValue);
                res = true;
              }
            } 
          }  
        } // for
      } // if functor OK
    } // if instruction
    instrOffsetForScan += skipSize;
  } // while
  return res;
}

uint sgpVMachine::getArgMetaParamUInt(const scDataNode &meta, uint argNo, uint argMetaId)
{
  if (meta.isArray()) {
    scDataNode metaElement;
    meta.getElement(argNo, metaElement);
    return metaElement.getUInt(argMetaId);
  } else {
    assert(argNo < meta.size());
    return meta[argNo].getUInt(argMetaId);
  } 
}

void sgpVMachine::buildFunctionArgMeta(scDataNode &output, uint ioMode, uint argTypes, uint dataTypes)
{
  output.clear();
  output.addChild(new scDataNode(ioMode));
  output.addChild(new scDataNode(argTypes));
  output.addChild(new scDataNode(dataTypes));
}

bool sgpVMachine::checkArgMeta(const scDataNode &args, const scDataNode &meta, uint ioMode, uint &argNo, bool &staticTypes)
{
  bool res = true;
  uint epos;
  //scDataNode metaElement;
  std::auto_ptr<scDataNode> metaElementGuard; 
  const scDataNode *metaElementPtr = SC_NULL;
  std::auto_ptr<scDataNode> argElementGuard; 
  const scDataNode *argElementPtr = SC_NULL;
  uint argType, dataType;
  static uint regNo;
  bool useElement = args.isArray();
  bool useMetaElement = meta.isArray();
  
  staticTypes = true;
  
  argNo = 0;
  
  if (args.size() < meta.size())
    epos = args.size();
  else
    epos = meta.size();    

  if (epos > 0) {
    if (useElement) {
      argElementGuard.reset(new scDataNode());
      argElementPtr = argElementGuard.get();
    }
    if (useMetaElement) {
      metaElementGuard.reset(new scDataNode());
      metaElementPtr = metaElementGuard.get();
    }
  }  
  
  for(uint i=0; i != epos; i++)
  {
     if (useMetaElement) 
       meta.getElement(i, *metaElementGuard);
     else
       metaElementPtr = &(meta[i]);
     
     if ((*metaElementPtr).size() >= 3) {
       if (((*metaElementPtr).getUInt(GASM_ARG_META_IO_MODE) & ioMode) != 0) { // io mode matched
         if (useElement) 
           args.getElement(i, *argElementGuard);
         else
           argElementPtr = &(args[i]);
           
         argType = getArgType(*argElementPtr);

         if (staticTypes) {
           if (argType == gatfRegister) {
             regNo = getRegisterNo(*argElementPtr);
             if (!isStaticTypeRegister(regNo))
               staticTypes = false;
           } else if ((argType & (gatfConst + gatfNull)) == 0) {
             staticTypes = false;
           }  
         }
         
         if ((argType & (*metaElementPtr).getUInt(GASM_ARG_META_ARG_TYPE)) == 0) {
           res = false;
           argNo = i;
           break;
         } else {  
           dataType = calcDataType(*argElementPtr);
           if ((dataType & (*metaElementPtr).getUInt(GASM_ARG_META_DATA_TYPE)) == 0) {
             res = false;
             argNo = i;
             break;
           } // data type error
         } // data type check
       } // io mode OK
     } // meta info complete
  } // for  
  
  return res;
}

bool sgpVMachine::checkArgsSupported(const scDataNode &args, uint &argNo)
{
  bool res = true;

  std::auto_ptr<scDataNode> argElementGuard; 
  const scDataNode *argElementPtr;

  bool useElement = args.isArray();

  if (useElement && args.size()) {
    argElementGuard.reset(new scDataNode());
    argElementPtr = argElementGuard.get();
  } else {
    argElementPtr = SC_NULL;
  } 
 
  for(int i=0,epos=args.size(); i!=epos; i++)
  {
    if (useElement) 
      args.getElement(i, *argElementGuard);
    else
      argElementPtr = &(args[i]);

    if ((calcDataType(*argElementPtr) & m_supportedDataTypes) == 0)
    {
      res = false;
      argNo = i;
      break;
    }    
    if ((getArgType(*argElementPtr) & m_supportedArgTypes) == 0)
    {
      res = false;
      argNo = i;
      break;
    }
  }  
  return res;
}

bool sgpVMachine::checkArgCount(const scDataNode &args, sgpFunction *functor)
{
  return checkArgCount(args.size(), functor);
}

bool sgpVMachine::checkArgCount(uint argCount, sgpFunction *functor)
{
  uint minArgCount, maxArgCount;
  functor->getArgCount(minArgCount, maxArgCount);

  if ((argCount < minArgCount) || ((maxArgCount > 0) && (argCount > maxArgCount))) 
    return false;
  else
    return true;  
}

void sgpVMachine::lockReadRegError()
{
  m_readRegErrorLock++;
}

void sgpVMachine::unlockReadRegError()
{
  if (m_readRegErrorLock > 0)
    m_readRegErrorLock--;
}

bool sgpVMachine::isReadRegErrorLocked() const
{
  return (m_readRegErrorLock > 0);
}

void sgpVMachine::skipCells(cell_size_t a_value)
{
  m_programState.nextCellNo = m_programState.nextCellNo + a_value;
  if (m_programState.nextCellNo > m_programCode.getBlockLength(m_programState.activeBlockNo))
    handleError(GVM_ERROR_WRONG_SKIP, "Invalid skip size: "+toString(a_value));
}

void sgpVMachine::forceInstrCache()
{
  m_instrCacheRequired = true;
}

void sgpVMachine::getCounters(scDataNode &output)
{
#ifdef GVM_USE_COUNTERS
  m_counters.getAll(output);
#endif  
}

void sgpVMachine::resetCounters()
{
  m_counters.clear();
}

void sgpVMachine::jumpBack(cell_size_t a_value)
{
  // cache can be usefull only with loops - and this function is used for loops
  m_instrCacheRequired = true;
  
  if (m_programState.nextCellNo > a_value)
    m_programState.nextCellNo -= a_value;
  else { 
    m_programState.nextCellNo = 1;
    handleError(GVM_ERROR_WRONG_JUMP_BACK, "Invalid jump size: "+toString(a_value));
  }  
}

bool sgpVMachine::callBlock(uint blockNo, const scDataNode &args)
{
  bool res;
  scDataNode inputArgs;

  res = true;
  if ((m_maxStackDepth > 0) && (m_programState.callStack.size() >= m_maxStackDepth)) {
    handleError(GVM_ERROR_STACK_OVERFLOW, "Stack overflow, call to block: "+toString(blockNo));
    res = false;
  }
  
  if ((blockNo == m_programState.activeBlockNo) && ((m_features & ggfRecurrenceEnabled) == 0)) {
    handleError(GVM_ERROR_RECURRENCE_FORBID, "Recurrence forbidden, block: "+toString(blockNo));
    res = false;
  }
  
  if (res) {
    inputArgs = args;
    if (args.size() > 0) {
      inputArgs.eraseElement(0);
    }  
  }
  
  if (!m_programCode.blockExists(blockNo)) {
    res = false;
    handleError(GVM_ERROR_CALL_TO_WRONG_BLOCK_NO, "Call - wrong block no.: "+toString(blockNo));
  }  
  
  if (res && (m_features & ggfFixedBlockArgCount) != 0) {
    scDataNode inputMeta, outputMeta;
    m_programCode.getBlockMetaInfo(blockNo, inputMeta, outputMeta);
    if (inputArgs.size() != inputMeta.size()) {
      res = false;
      handleError(GVM_ERROR_WRONG_CALL_INPUT_ARG_CNT, "Wrong input value count: "+toString(inputArgs.size())+", for block: "+toString(blockNo));
    }   
  } 
  
  if (res) {
    scDataNode extraValue;

    if (args.size() > 0)
      extraValue = args.getElement(0);
    
    pushBlockState(extraValue);
    initBlockState(blockNo);
    setInput(inputArgs);
    m_programState.activeBlockNo = blockNo;
    m_programState.activeCellNo = m_programState.nextCellNo = 1;
  }  
  return res;  
}

bool sgpVMachine::setNextBlockNo(uint blockNo)
{
  bool res = true;

  if ((blockNo <= m_programState.activeBlockNo) && ((m_features & ggfRecurrenceEnabled) == 0)) {
    handleRecurrenceForbidden(blockNo);
    res = false;
  }

  if (!m_programCode.blockExists(blockNo)) {
    res = false;
    handleWrongBlockNo(blockNo);
  }  
  
  if (res) 
    m_programState.nextBlockNo = static_cast<int>(blockNo);
  return res;
}

bool sgpVMachine::setNextBlockNoRel(uint blockNoOffset)
{ 
  uint blockNo = m_programState.activeBlockNo + blockNoOffset;
  return setNextBlockNo(blockNo);
}

void sgpVMachine::clearNextBlockNo()
{
  m_programState.nextBlockNo = -1;
}

void sgpVMachine::exitBlock()
{
  if (m_programState.callStack.size() > 0) {
    scDataNode outputArg;
    scDataNode outputValue = getOutput();
    popBlockState(outputArg);
    setLValue(outputArg, outputValue);    
  } else {
  // go to end
    m_programState.nextCellNo = m_programCode.getBlockLength(m_programState.activeBlockNo);
  } 
}

// push: block no, active cell, next cell, active regs, defined regs, references
void sgpVMachine::pushBlockState(const scDataNode &extraValue)
{
  std::auto_ptr<scDataNode> guard(new scDataNode());
  guard->addChild(new scDataNode(m_programState.activeBlockNo)); 
  guard->addChild(new scDataNode(m_programState.activeCellNo)); 
  guard->addChild(new scDataNode(m_programState.nextCellNo)); 
  guard->addChild(new scDataNode(m_programState.activeRegs)); 
  guard->addChild(new scDataNode(m_programState.definedRegs)); 

  std::auto_ptr<scDataNode> refsGuard(new scDataNode());
  std::auto_ptr<scDataNode> refsGuardForChild;
  
  for(sgpRegReferences::iterator it = m_programState.references.begin(), epos = m_programState.references.end(); it != epos; ++it)
  {    
    refsGuardForChild.reset(new scDataNode());    
    refsGuardForChild->transferChildrenFrom(it->second);
    refsGuard->addChild(toString(it->first), refsGuardForChild.release());
  }
  
  guard->addChild(refsGuard.release()); 
  guard->addChild(new scDataNode(extraValue)); 
  m_programState.callStack.addChild(guard.release());
}

void sgpVMachine::popBlockState(scDataNode &extraValue)
{
  if (m_programState.callStack.size()) {
    scDataNode oldState = m_programState.callStack.getElement(m_programState.callStack.size() - 1);
    m_programState.callStack.eraseElement(m_programState.callStack.size() - 1);

    m_programState.activeBlockNo = oldState.getUInt(0);
    m_programState.activeCellNo = oldState.getUInt(1);
    m_programState.nextCellNo = oldState.getUInt(2);
    m_programState.activeRegs = oldState.getElement(3);
    m_programState.definedRegs = oldState.getElement(4);
    m_programState.references.clear();
    scDataNode refs;
    std::auto_ptr<scDataNode> refsGuardForChild;

    oldState.getElement(5, refs);
    scDataNode helper;
    
    for(scDataNode::iterator it=refs.begin(), epos = refs.end(); it != epos; ++it)
    {
      m_programState.references.insert(
        std::make_pair<uint, scDataNode>(
          stringToUInt(it->getName()),
          it->getAsNode(helper)
        )
      );  
    }

    extraValue = oldState.getElement(6);
  }
}

void sgpVMachine::pushValue(const scDataNode &value)
{
  m_programState.callStack.addChild(new scDataNode(value));
}

scDataNode sgpVMachine::popValue()
{
  scDataNode res;
  
  if (m_programState.callStack.size() > 0) {
    res = m_programState.callStack.getElement(m_programState.callStack.size() - 1);
    m_programState.callStack.eraseElement(m_programState.callStack.size() - 1);    
  }
  
  return res;
}

//------------------------------------------------------------------------
//--- array
//------------------------------------------------------------------------
ulong64 sgpVMachine::arrayGetSize(uint regNo)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) 
    return ptr->size();
  else {
    handleError(GVM_ERROR_ARRAY_NOT_AVAILABLE, "");
    return 0;  
  }  
}

void sgpVMachine::arrayAddItem(uint regNo, const scDataNode &value)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    if (m_maxItemCount > 0) {
      if (m_maxItemCount <= ptr->size())
      {
        handleError(GVM_ERROR_ARRAY_LIMIT_REACHED, "");
        return;
      }
    }

    if (value.getValueType() != ptr->getValueType()) {
    // cast if needed
      scDataNode tempValue;
      if (ptr->size()) {
        // prepare item from first item of array
        ptr->getElement(0, tempValue);
        tempValue.assignValueFrom(value);
      } else { // no source for type 
        tempValue = value;
      }  
      ptr->addItem(tempValue);
    } else { 
      ptr->addItem(value);
    }  
  }  
  else {
    handleError(GVM_ERROR_ARRAY_NOT_AVAILABLE, "");
  }  
}

void sgpVMachine::arrayEraseItem(uint regNo, ulong64 idx)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    ptr->eraseElement(idx);
  }  
  else {
    handleError(GVM_ERROR_ARRAY_NOT_AVAILABLE, "");
  }  
}

void sgpVMachine::arraySetItem(uint regNo, ulong64 idx, const scDataNode &value)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    if (value.getValueType() != ptr->getValueType()) {
    // cast if needed
      scDataNode tempValue;
      if (ptr->size()) {
        // prepare item from first item of array
        ptr->getElement(0, tempValue);
        tempValue.assignValueFrom(value);
      } else { // no source for type 
        tempValue = value;
      }  
      ptr->setElement(idx, tempValue);
    } else { 
      ptr->setElement(idx, value);
    }  
  }  
  else {
    handleError(GVM_ERROR_ARRAY_NOT_AVAILABLE, "");
  }  
}

void sgpVMachine::arrayGetItem(uint regNo, ulong64 idx, scDataNode &output)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    ptr->getElement(idx, output);
  }  
  else {
    handleError(GVM_ERROR_ARRAY_NOT_AVAILABLE, "");
  }  
}

long64 sgpVMachine::arrayIndexOf(uint regNo, const scDataNode &value)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if ((ptr != SC_NULL) && ptr->isArray()) {
    return ptr->indexOfValue(value);
  }  
  else {
    handleError(GVM_ERROR_ARRAY_NOT_AVAILABLE, "");
    return 0;  
  }  
}

void sgpVMachine::arrayMerge(uint regNo1, uint regNo2, scDataNode &output)
{
  output.clear();
  scDataNode *ptr1 = getRegisterPtr(regNo1);
  scDataNode *ptr2 = getRegisterPtr(regNo2);
  if ((ptr1 != SC_NULL) && (ptr2 != SC_NULL) && ptr1->isArray() && ptr2->isArray()) {
    if (ptr1->getValueType() != ptr2->getValueType())
      handleError(GVM_ERROR_WRONG_ARRAY_TYPE, "");
    else {
      output.setAsArray(ptr1->getValueType());
      output.addItemList(*ptr1);
      output.addItemList(*ptr2);
    }  
  }  
  else {
    handleError(GVM_ERROR_ARRAY_NOT_AVAILABLE, "");
  }  
}

void sgpVMachine::arrayRange(uint regNo, ulong64 aStart, ulong64 aSize, scDataNode &output)
{
  output.clear();
  scDataNode *ptr = getRegisterPtr(regNo);
  if ((ptr != SC_NULL) && ptr->isArray()) {
    ulong64 epos = (aSize > 0)?(aStart+aSize):ptr->size();
    output.setAsArray(ptr->getValueType());
    scDataNode element;
    for(ulong64 i=aStart; i != epos; i++)
    {
      ptr->getElement(i, element);
      output.addItem(element);
    }
  }  
  else {
    handleError(GVM_ERROR_ARRAY_NOT_AVAILABLE, "");
  }  
}

//------------------------------------------------------------------------
//--- struct
//------------------------------------------------------------------------

ulong64 sgpVMachine::structGetSize(uint regNo)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) 
    return ptr->size();
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
    return 0;  
  }  
}

void sgpVMachine::structAddItem(uint regNo, const scDataNode &value)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    if (m_maxItemCount > 0) {
      if (m_maxItemCount <= ptr->size())
      {
        handleError(GVM_ERROR_STRUCT_LIMIT_REACHED, "");
        return;
      }
    }
    ptr->addChild(new scDataNode(value));
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
  }  
}

void sgpVMachine::structAddItem(uint regNo, const scString &aName, const scDataNode &value)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    if (m_maxItemCount > 0) {
      if (m_maxItemCount <= ptr->size())
      {
        handleError(GVM_ERROR_STRUCT_LIMIT_REACHED, "");
        return;
      }
    }
    ptr->addChild(aName, new scDataNode(value));
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
  }  
}

void sgpVMachine::structEraseItem(uint regNo, const scString &idx)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    ptr->eraseElement(ptr->indexOfName(idx));
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
  }  
}

void sgpVMachine::structEraseItem(uint regNo, ulong64 idx)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    ptr->eraseElement(idx);
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
  }  
}

void sgpVMachine::structSetItem(uint regNo, const scString &idx, const scDataNode &value)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    (*ptr)[idx].copyValueFrom(value);
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
  }  
}

void sgpVMachine::structSetItem(uint regNo, ulong64 idx, const scDataNode &value)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    (*ptr)[idx].copyValueFrom(value);
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
  }  
}

void sgpVMachine::structGetItem(uint regNo, const scString &idx, scDataNode &output)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    output = (*ptr)[idx];
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
  }  
}

void sgpVMachine::structGetItem(uint regNo, ulong64 idx, scDataNode &output)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    output = (*ptr)[idx];
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
  }  
}

long64 sgpVMachine::structIndexOf(uint regNo, const scDataNode &value)
{
  long64 res = scDataNode::npos;
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    res = ptr->getChildren().indexOfValue(value);
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
  }    
  return res;
}

void sgpVMachine::structMerge(uint regNo1, uint regNo2, scDataNode &output)
{
  output.clear();
  scDataNode *ptr1 = getRegisterPtr(regNo1);
  scDataNode *ptr2 = getRegisterPtr(regNo2);
  if ((ptr1 != SC_NULL) && (ptr2 != SC_NULL) && ptr1->isParent() && ptr2->isParent()) {
    scDataNode element;

    for(uint i=0,epos=ptr1->size(); i!=epos; i++)
      output.addChild(ptr1->cloneElement(i));

    for(uint i=0,epos=ptr2->size(); i!=epos; i++)
      output.addChild(ptr2->cloneElement(i));
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
  }  
}

void sgpVMachine::structRange(uint regNo, ulong64 aStart, ulong64 aSize, scDataNode &output)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  output.clear();
  if ((ptr != SC_NULL) && ptr->isParent()) {
    ulong64 epos = (aSize > 0)?(aStart+aSize):ptr->size();

    for(ulong64 i=aStart; i != epos; i++)
      output.addChild(new scDataNode((*ptr)[i]));
  }  
  else {
    handleError(GVM_ERROR_ARRAY_NOT_AVAILABLE, "");
  }  
}

void sgpVMachine::structGetItemName(uint regNo, ulong64 aIndex, scString &output)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if ((ptr != SC_NULL) && ptr->isParent()) {
    output = (*ptr).getElementName(aIndex);
  }
  else {
    handleError(GVM_ERROR_ARRAY_NOT_AVAILABLE, "");
    output = "";
  }  
}

//------------------------------------------------------------------------
//--- vector
//------------------------------------------------------------------------

void sgpVMachine::vectorFindMinValue(uint regNo, scDataNode &output)
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    if (ptr->empty()) { // findMinValue(output)
      handleError(GVM_ERROR_STRUCT_NOT_APPLICABLE, "");
      output.clear();
    }  

    scDataNode::iterator it;

    switch(ptr->getElementType()) {
      case vt_byte:
        it = ptr->min_element<byte>();
        break;
      case vt_int:
        it = ptr->min_element<int>();
        break;
      case vt_uint:
        it = ptr->min_element<uint>();
        break;
      case vt_int64:
        it = ptr->min_element<int>();
        break;
      case vt_uint64:
        it = ptr->min_element<uint>();
        break;
      case vt_float:
        it = ptr->min_element<float>();
        break;
      case vt_double:
        it = ptr->min_element<double>();
        break;
      case vt_xdouble:
        it = ptr->min_element<xdouble>();
        break;
      default:
        it = ptr->end();
    }

    output = it->getAsElement();
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
    output.clear();
  }    
}

void sgpVMachine::vectorFindMaxValue(uint regNo, scDataNode &output)        
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    if (ptr->empty()) {
      handleError(GVM_ERROR_STRUCT_NOT_APPLICABLE, "");
      output.clear();
    }  

    scDataNode::iterator it;

    switch(ptr->getElementType()) {
      case vt_byte:
        it = ptr->max_element<byte>();
        break;
      case vt_int:
        it = ptr->max_element<int>();
        break;
      case vt_uint:
        it = ptr->max_element<uint>();
        break;
      case vt_int64:
        it = ptr->max_element<int>();
        break;
      case vt_uint64:
        it = ptr->max_element<uint>();
        break;
      case vt_float:
        it = ptr->max_element<float>();
        break;
      case vt_double:
        it = ptr->max_element<double>();
        break;
      case vt_xdouble:
        it = ptr->max_element<xdouble>();
        break;
      default:
        handleError(GVM_ERROR_STRUCT_NOT_APPLICABLE, "");
        it = ptr->end();
    }

    output = it->getAsElement();

  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
    output.clear();
  }    
}

void sgpVMachine::vectorSum(uint regNo, scDataNode &output)        
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    switch(ptr->getElementType()) {
      case vt_byte:
        output = ptr->accumulate<byte>(0);
        break;
      case vt_int:
        output = ptr->accumulate<int>(0);
        break;
      case vt_uint:
        output = ptr->accumulate<uint>(0);
        break;
      case vt_int64:
        output = ptr->accumulate<int64>(0);
        break;
      case vt_uint64:
        output = ptr->accumulate<uint64>(0);
        break;
      case vt_float:
        output = ptr->accumulate<float>(0.0);
        break;
      case vt_double:
        output = ptr->accumulate<double>(0.0);
        break;
      case vt_xdouble:
        output = ptr->accumulate<xdouble>(0.0);
        break;
      default:
        handleError(GVM_ERROR_STRUCT_NOT_APPLICABLE, "");
        output = 0;
    }
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
    output.clear();
  }    
}

void sgpVMachine::vectorAvg(uint regNo, scDataNode &output)        
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    switch(ptr->getElementType()) {
      case vt_byte:
        output = ptr->avg<byte>(0);
        break;
      case vt_int:
        output = ptr->avg<int>(0);
        break;
      case vt_uint:
        output = ptr->avg<uint>(0);
        break;
      case vt_int64:
        output = ptr->avg<int64>(0);
        break;
      case vt_uint64:
        output = ptr->avg<uint64>(0);
        break;
      case vt_float:
        output = ptr->avg<float>(0.0);
        break;
      case vt_double:
        output = ptr->avg<double>(0.0);
        break;
      case vt_xdouble:
        output = ptr->avg<xdouble>(0.0);
        break;
      default:
        handleError(GVM_ERROR_STRUCT_NOT_APPLICABLE, "");
        output = 0;
    }
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
    output.clear();
  }    
}

void sgpVMachine::vectorSort(uint regNo)        
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    if (!ptr->sort()) {
      handleError(GVM_ERROR_STRUCT_NOT_APPLICABLE, "");
    }  
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
  }    
}

void sgpVMachine::vectorDistinct(uint regNo)        
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    switch(ptr->getElementType()) {
      case vt_byte:
        ptr->sort<byte>();
        ptr->unique();
        break;
      case vt_int:
        ptr->sort<int>();
        ptr->unique();
        break;
      case vt_uint:
        ptr->sort<uint>();
        ptr->unique();
        break;
      case vt_int64:
        ptr->sort<int64>();
        ptr->unique();
        break;
      case vt_uint64:
        ptr->sort<uint64>();
        ptr->unique();
        break;
      case vt_float:
        ptr->sort<float>();
        ptr->unique();
        break;
      case vt_double:
        ptr->sort<double>();
        ptr->unique();
        break;
      case vt_xdouble:
        ptr->sort<xdouble>();
        ptr->unique();
        break;
      default:
        handleError(GVM_ERROR_STRUCT_NOT_APPLICABLE, "");
    }
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
  }    
}

void sgpVMachine::vectorNorm(uint regNo)        
{
  scDataNode *ptr = getRegisterPtr(regNo);
  if (ptr != SC_NULL) {
    switch(ptr->getElementType()) {
      case vt_byte:
        make_normal(ptr->scalarBegin<byte>(), ptr->scalarEnd<byte>(), ptr->begin());
        break;
      case vt_int:
        make_normal(ptr->scalarBegin<int>(), ptr->scalarEnd<int>(), ptr->begin());
        break;
      case vt_uint:
        make_normal(ptr->scalarBegin<uint>(), ptr->scalarEnd<uint>(), ptr->begin());
        break;
      case vt_int64:
        make_normal(ptr->scalarBegin<int64>(), ptr->scalarEnd<int64>(), ptr->begin());
        break;
      case vt_uint64:
        make_normal(ptr->scalarBegin<uint64>(), ptr->scalarEnd<uint64>(), ptr->begin());
        break;
      case vt_float:
        make_normal(ptr->scalarBegin<float>(), ptr->scalarEnd<float>(), ptr->begin());
        break;
      case vt_double:
        make_normal(ptr->scalarBegin<double>(), ptr->scalarEnd<double>(), ptr->begin());
        break;
      case vt_xdouble:
        make_normal(ptr->scalarBegin<xdouble>(), ptr->scalarEnd<xdouble>(), ptr->begin());
        break;
      default:
        handleError(GVM_ERROR_STRUCT_NOT_APPLICABLE, "");
    }
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
  }    
}

void sgpVMachine::vectorDotProduct(uint regNo1, uint regNo2, scDataNode &output)
{
  scDataNode *ptr1 = getRegisterPtr(regNo1);
  scDataNode *ptr2 = getRegisterPtr(regNo2);
  
  if ((ptr1 != SC_NULL) && (ptr2 != SC_NULL)) {
    switch(ptr1->getElementType()) {
      case vt_byte:
        output = dot_product(ptr1->scalarBegin<byte>(), ptr1->scalarEnd<byte>(), ptr2->scalarBegin<byte>());
        break;
      case vt_int:
        output = dot_product(ptr1->scalarBegin<int>(), ptr1->scalarEnd<int>(), ptr2->scalarBegin<int>());
        break;
      case vt_uint:
        output = dot_product(ptr1->scalarBegin<uint>(), ptr1->scalarEnd<uint>(), ptr2->scalarBegin<uint>());
        break;
      case vt_int64:
        output = dot_product(ptr1->scalarBegin<int64>(), ptr1->scalarEnd<int64>(), ptr2->scalarBegin<int64>());
        break;
      case vt_uint64:
        output = dot_product(ptr1->scalarBegin<uint64>(), ptr1->scalarEnd<uint64>(), ptr2->scalarBegin<uint64>());
        break;
      case vt_float:
        output = dot_product(ptr1->scalarBegin<float>(), ptr1->scalarEnd<float>(), ptr2->scalarBegin<float>());
        break;
      case vt_double:
        output = dot_product(ptr1->scalarBegin<double>(), ptr1->scalarEnd<double>(), ptr2->scalarBegin<double>());
        break;
      case vt_xdouble:
        output = dot_product(ptr1->scalarBegin<xdouble>(), ptr1->scalarEnd<xdouble>(), ptr2->scalarBegin<xdouble>());
        break;
      default:
        handleError(GVM_ERROR_STRUCT_NOT_APPLICABLE, "");
    }
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
    output.clear();
  }    
}

void sgpVMachine::vectorStdDev(uint regNo1, scDataNode &output)
{
  scDataNode *ptr = getRegisterPtr(regNo1);
  
  if (ptr != SC_NULL) {
    switch(ptr->getElementType()) {
      case vt_byte: 
      case vt_int: 
      case vt_uint: 
      case vt_int64:
      case vt_uint64:
        output = ptr->std_dev<double>(0.0);
        break;
      case vt_float:
        output = ptr->std_dev<float>(0.0);
        break;
      case vt_double:
        output = ptr->std_dev<double>(0.0);
        break;
      case vt_xdouble:
        output = ptr->std_dev<xdouble>(0.0);
        break;
      default:
        handleError(GVM_ERROR_STRUCT_NOT_APPLICABLE, "");
        output = 0;
    }
  }  
  else {
    handleError(GVM_ERROR_STRUCT_NOT_AVAILABLE, "");
    output.clear();
  }    
}

void sgpVMachine::setPushNextResult()
{
  m_programState.flags |= psfPushNextResult;
}

bool sgpVMachine::pushValueToValueStack(const scDataNode &value)
{
  if (m_valueStackLimit > 0)
  {
    if (m_programState.valueStack.size() >= m_valueStackLimit)
    {
      return false;
    }  
  }
  m_programState.valueStack.addChild(new scDataNode(value));
  return true;  
}

bool sgpVMachine::popValueFromValueStack(scDataNode &output)
{
  if (m_programState.valueStack.empty())
  {
    return false;
  } 
  m_programState.valueStack.getElement(m_programState.valueStack.size() - 1, output);
  return true;
}

void sgpVMachine::addToTraceLogChange(uint regNo, const scDataNode &value)
{
  scString line = 
    formatAddress(m_programState.activeBlockNo, m_programState.activeCellNo)+" ";
  line += "set-register-value #"+toString(regNo)+", ["+value.getAsString()+"]";  
  m_traceLog.push_back(line);  
}

void sgpVMachine::addToTraceLogChangeByRef(uint regNo, const scDataNode &value)
{
  scString line = 
    formatAddress(m_programState.activeBlockNo, m_programState.activeCellNo)+" ";
  line += "set-register-value #"+toString(regNo)+"[], ["+value.getAsString()+"]";  
  m_traceLog.push_back(line);  
}

void sgpVMachine::addToTraceLogInstr(uint instrCode, const scDataNode &args, const sgpFunction *functor)
{
  uint argType;
 
  scString line = 
    formatAddress(m_programState.activeBlockNo, m_programState.activeCellNo)+" ";
 
  line += functor->getName()+"/"+toString(instrCode)+"/ ";
 
  for(uint i=0,epos=args.size(); i!=epos; i++) {
    argType = getArgType(args.getElement(i));   
    if (argType == gatfRegister) {
      line += scString("#")+toString(getRegisterNo(args.getElement(i)))+", ";
    } else if ((argType == gatfNull) || args.getElement(i).isNull()) {
      line += "null, ";
    } else
      line += args.getString(i)+", ";    
  }  
 
  m_traceLog.push_back(line);
}

scString sgpVMachine::formatAddress(uint blockNo, cell_size_t cellAddr) {
  return toHexString(blockNo)+":"+toHexString(cellAddr);
}

bool sgpVMachine::getArgsValidFromCache(ulong64 keyNum)
{
  return 
   (
    (m_validArgCache.find(keyNum) != m_validArgCache.end())
   );
}

void sgpVMachine::setArgsValidInCache(ulong64 keyNum)
{
  if ((ggfCodeAccessWrite & m_features) == 0)
    m_validArgCache.insert(keyNum);
}
    
void sgpVMachine::clearArgValidCache()
{
  m_validArgCache.clear();
}

uint sgpVMachine::blockAdd()
{
  uint res = m_programCode.getBlockCount();
  m_programCode.addBlock();
  return res;
}

bool sgpVMachine::blockInit(uint blockNo)
{
  bool res = (blockNo != m_programState.activeBlockNo);
  if (res) {
    res = m_programCode.blockExists(blockNo);
    if (res) {
      m_programCode.clearBlock(blockNo);      
      m_programCode.setDefBlockMetaInfo(blockNo);
      clearArgValidCache();
      clearInstrCache();
    }
  }   
  return res;
}

bool sgpVMachine::blockErase(uint blockNo)
{
  bool res = (blockNo != m_programState.activeBlockNo);
  if (res) {
    res = (m_programCode.blockExists(blockNo) && (blockNo + 1 == m_programCode.getBlockCount()));
    if (res) {
      m_programCode.eraseLastBlock();
      clearArgValidCache();
      clearInstrCache();
    }
  }   
  return res;
}

cell_size_t sgpVMachine::blockGetSize(uint blockNo)
{
  return m_programCode.getBlockLength(blockNo);
}

uint sgpVMachine::blockGetCount()
{
  return m_programCode.getBlockCount();
}

uint sgpVMachine::blockActiveId()
{
  return m_programState.activeBlockNo;
}

