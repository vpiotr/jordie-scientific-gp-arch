/////////////////////////////////////////////////////////////////////////////
// Name:        GasmOperatorInit.cpp
// Project:     scLib
// Purpose:     Initialization operator for GP
// Author:      Piotr Likus
// Modified by:
// Created:     29/12/2009
/////////////////////////////////////////////////////////////////////////////

//sc
#include "sc/rand.h"

//other
#include "sgp/GasmOperatorInit.h"
#include "sgp/GasmOperator.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

using namespace dtp;

// ----------------------------------------------------------------------------
// sgpGasmOperatorInitEntity
// ----------------------------------------------------------------------------
sgpGasmOperatorInitEntity::sgpGasmOperatorInitEntity()
{
  m_minSize = 1; m_maxSize = 5;
  m_maxBlockCount = 1;
  m_supportedDataTypes = SGP_GASM_DEF_DATA_TYPES;
  m_activeIslandId = 0;
}

sgpGasmOperatorInitEntity::~sgpGasmOperatorInitEntity()
{
}

void sgpGasmOperatorInitEntity::setFunctions(const sgpFunctionMapColn &functions)
{
  m_functions = functions;
}

void sgpGasmOperatorInitEntity::getFunctions(sgpFunctionMapColn &functions)
{
  functions = m_functions;
}

void sgpGasmOperatorInitEntity::setSizeRange(uint minSize, uint maxSize)
{
  m_minSize = minSize;
  m_maxSize = maxSize;
}

void sgpGasmOperatorInitEntity::getSizeRange(uint &minSize, uint &maxSize)
{
  minSize = m_minSize;
  maxSize = m_maxSize;
}

void sgpGasmOperatorInitEntity::setMaxBlockCount(uint value)
{
  m_maxBlockCount = value;
}

uint sgpGasmOperatorInitEntity::getMaxBlockCount()
{
  return m_maxBlockCount;
}

void sgpGasmOperatorInitEntity::setDefaultBlockMeta(const scDataNode &value)
{ 
  m_defaultBlockMeta = value;
}

void sgpGasmOperatorInitEntity::getDefaultBlockMeta(scDataNode &value)
{
  value = m_defaultBlockMeta;
}

void sgpGasmOperatorInitEntity::setVMachine(sgpVMachine *machine)
{
  m_machine = machine;
}

void sgpGasmOperatorInitEntity::setSupportedDataTypes(uint mask)
{
  m_supportedDataTypes = mask;
}

void sgpGasmOperatorInitEntity::setInfoBlockMeta(const sgpGaGenomeMetaList &metaList)
{
  m_infoBlockMeta = metaList;
}  

uint sgpGasmOperatorInitEntity::getActiveIslandId()
{
  return m_activeIslandId;
}

void sgpGasmOperatorInitEntity::setActiveIslandId(uint islandId)
{
  m_activeIslandId = islandId;
}

void sgpGasmOperatorInitEntity::setIslandTool(sgpEntityIslandToolIntf *value)
{
  m_islandTool = value;
}

void sgpGasmOperatorInitEntity::buildRandomEntity(sgpEntityBase &output)
{
  uint blockLimit = 1;
  if (m_maxBlockCount > 1)
    blockLimit = randomInt(1, m_maxBlockCount);

  std::auto_ptr<scDataNode> blockCodeGuard;  
  sgpProgramCode prg;  
  uint defInputCount = 0;
  if (m_defaultBlockMeta.size() > 0)
    defInputCount = m_defaultBlockMeta[0].size();
  uint useInputCount;  

  for(uint i=0; i != blockLimit; ) {
    blockCodeGuard.reset(new scDataNode());
    if (i == 0) {
      blockCodeGuard->addItem(m_defaultBlockMeta);
      useInputCount = defInputCount;
    } else {
      blockCodeGuard->addItem(scDataNode(ict_parent));
      useInputCount = 0;
    }  
          
    buildRandomBlock(useInputCount, prg.getBlockCount(), *blockCodeGuard);
    assert(blockCodeGuard->getElementType(0) == vt_parent);
    if (blockCodeGuard->size() > 1) {
      prg.addBlock(blockCodeGuard.release());
      i++;
    }  
  }

  scDataNode &fullCode = prg.getFullCode();
  output.setGenomeAsNode(fullCode);
  
  if (m_infoBlockMeta.size()) { 
    scDataNode infoBlock;    
    if (!m_infoBlockMeta.empty())
      sgpGaOperatorInit::buildRandomGenome(m_infoBlockMeta, infoBlock); 
    sgpEntityForGasm *outputGasm = dynamic_cast<sgpEntityForGasm *>(&output);
    outputGasm->setInfoBlock(infoBlock);
    m_islandTool->setIslandId(*outputGasm, m_activeIslandId);
  }        

}

void sgpGasmOperatorInitEntity::addInfoVarPart(scDataNode &infoBlock)
{ // empty here
}

// generate 1..n random instructions with arguments
void sgpGasmOperatorInitEntity::buildRandomBlock(uint inputCount, uint blockIndex, scDataNode &output)
{
  uint instrLimit = randomInt(m_minSize, m_maxSize);
  sgpGaGenome newCodeRaw;
  scDataNode newCode, newCell;
  sgpGasmRegSet writtenRegs;
  
  if (inputCount > 0)
    for(uint i=SGP_REGB_INPUT,epos=SGP_REGB_INPUT+inputCount;i!=epos;i++)
      writtenRegs.insert(i);
      
  sgpGasmDataNodeTypeSet dataNodeTypes;
  sgpGasmCodeProcessor::castTypeSet(m_supportedDataTypes, dataNodeTypes);
  sgpGasmProbList emptyProbs;  
      
  for(uint i=0; i!=instrLimit; i++)
  {
    for(uint tryCnt=500; tryCnt>0; tryCnt--) {
      if (sgpGasmCodeProcessor::buildRandomInstr(m_functions, writtenRegs, m_supportedDataTypes, dataNodeTypes, emptyProbs, m_machine, 
        SC_NULL, &blockIndex, newCodeRaw)) 
      {
        if (verifyArgs(newCodeRaw, writtenRegs)) {
          for(uint j=0, eposj = newCodeRaw.size(); j != eposj; j++) {
            newCell = newCodeRaw[j];
            output.addItem(newCell);
          }
          newCodeRaw.clear();        
          break;  
        }
      }
      newCodeRaw.clear();        
    }
  }
}

bool sgpGasmOperatorInitEntity::verifyArgs(sgpGaGenome &newCode, sgpGasmRegSet &writtenRegs) 
{
  uint instrCode = newCode[0].getAsUInt();
  uint instrCodeRaw, argCount;
  bool res = false;
  
  sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
  sgpFunction *func = 
    ::getFunctorForInstrCode(m_functions, instrCodeRaw);
  if (func != SC_NULL) {      
    scDataNode args;
    
    sgpGasmCodeProcessor::getArguments(newCode, 1, argCount, args);
    if (m_machine->verifyArgs(args, func)) 
      res = true;
    if (res) {
      // update written regs
      scDataNode argMeta;
      uint ioMode;
      
      if (func->getArgMeta(argMeta)) {
        for(uint i=0,epos=SC_MIN(uint(argMeta.size()), uint(args.size()));i!=epos;i++)
        {
          ioMode = sgpVMachine::getArgMetaParamUInt(argMeta, i, GASM_ARG_META_IO_MODE);
          if ((ioMode & gatfOutput) != 0) {
            if (sgpVMachine::getArgType(args[i]) == gatfRegister) {
              uint regNo = sgpVMachine::getRegisterNo(args[i]);
              writtenRegs.insert(regNo);
            }
          }
        }
      }
    }  
  }
  return res;      
}


// ----------------------------------------------------------------------------
// sgpGasmOperatorInit
// ----------------------------------------------------------------------------
sgpGasmOperatorInit::sgpGasmOperatorInit()
{
  m_minSize = 1; m_maxSize = 5;
  m_infoVarPartMinSize = m_infoVarPartMaxSize = 0;
  m_maxBlockCount = 1;
  m_supportedDataTypes = SGP_GASM_DEF_DATA_TYPES;
}

sgpGasmOperatorInit::~sgpGasmOperatorInit()
{
}

void sgpGasmOperatorInit::init()
{
  initOperatorInitEntity();
}

void sgpGasmOperatorInit::initOperatorInitEntity()
{
  if (getOperatorInitEntity() != SC_NULL)
    return;

  std::auto_ptr<sgpGasmOperatorInitEntity> operInitEntity(new sgpGasmOperatorInitEntity);
  operInitEntity->setMetaInfo(this->getMetaInfo());
  operInitEntity->setDefaultBlockMeta(m_defaultBlockMeta);
  operInitEntity->setFunctions(m_functions);
  operInitEntity->setInfoBlockMeta(m_infoBlockMeta);
  operInitEntity->setMaxBlockCount(m_maxBlockCount);
  operInitEntity->setSizeRange(m_minSize, m_maxSize);
  operInitEntity->setSupportedDataTypes(m_supportedDataTypes);
  operInitEntity->setVMachine(m_machine);

  m_operatorInitEntity.reset(operInitEntity.release());

  setOperatorInitEntity(m_operatorInitEntity.get());
}

void sgpGasmOperatorInit::setFunctions(const sgpFunctionMapColn &functions)
{
  m_functions = functions;
}

void sgpGasmOperatorInit::getFunctions(sgpFunctionMapColn &functions)
{
  functions = m_functions;
}

void sgpGasmOperatorInit::setSizeRange(uint minSize, uint maxSize)
{
  m_minSize = minSize;
  m_maxSize = maxSize;
}

void sgpGasmOperatorInit::getSizeRange(uint &minSize, uint &maxSize)
{
  minSize = m_minSize;
  maxSize = m_maxSize;
}

void sgpGasmOperatorInit::setInfoVarPartSizeRange(uint minSize, uint maxSize)
{
  m_infoVarPartMinSize = minSize;
  m_infoVarPartMaxSize = maxSize;
}

void sgpGasmOperatorInit::getInfoVarPartSizeRange(uint &minSize, uint &maxSize)
{
  minSize = m_infoVarPartMinSize;
  maxSize = m_infoVarPartMaxSize;
}

sgpGaOperatorInitEntity *sgpGasmOperatorInit::getOperatorInitEntity()
{
  return m_operatorInitEntity.get();
}

void sgpGasmOperatorInit::checkSizeLimits()
{
  if ((m_maxSize < m_minSize) || (m_maxSize == 0))
    throw scError("Size range incorrect"); 
}

void sgpGasmOperatorInit::setMaxBlockCount(uint value)
{
  m_maxBlockCount = value;
}

uint sgpGasmOperatorInit::getMaxBlockCount()
{
  return m_maxBlockCount;
}

void sgpGasmOperatorInit::setDefaultBlockMeta(const scDataNode &value)
{ 
  m_defaultBlockMeta = value;
}

void sgpGasmOperatorInit::getDefaultBlockMeta(scDataNode &value)
{
  value = m_defaultBlockMeta;
}

void sgpGasmOperatorInit::setVMachine(sgpVMachine *machine)
{
  m_machine = machine;
}

void sgpGasmOperatorInit::setSupportedDataTypes(uint mask)
{
  m_supportedDataTypes = mask;
}

void sgpGasmOperatorInit::setInfoBlockMeta(const sgpGaGenomeMetaList &metaList)
{
  m_infoBlockMeta = metaList;
}  

void sgpGasmOperatorInit::execute(uint limit, sgpGaGeneration &newGeneration)
{
  if ((m_maxSize < m_minSize) || (m_maxSize == 0))
    throw scError("Size range incorrect");

  inherited::execute(limit, newGeneration);
}

