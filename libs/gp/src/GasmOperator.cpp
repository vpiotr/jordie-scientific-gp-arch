/////////////////////////////////////////////////////////////////////////////
// Name:        GasmOperator.cpp
// Project:     sgpLib
// Purpose:     Operators for evolver for genetic programs
// Author:      Piotr Likus
// Modified by:
// Created:     23/03/2009
/////////////////////////////////////////////////////////////////////////////
#include "sc/defs.h"

//stl
#include <cmath>
#include <climits>
#include <numeric>
#include <queue>
#include <map>
#include <memory>

// for debug
#ifdef _DEBUG
#include <iostream>
using namespace std;
#endif

//sc
#include "sc/rand.h"
#include "sc/smath.h"

//sgp
#include "sgp/GasmVMachine.h"
#include "sgp/GasmEvolver.h"
#include "sgp/GasmOperator.h"
//#include "sgp/GpFitnessFunBase.h"

#include "sgp/GasmFunLibCore.h"
#include "sgp/GasmFunLibMacros.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

#define USE_GASM_OPERATOR_STATS

//#define USE_MUT_MACRO_DEL

#ifdef _DEBUG
//#define VALIDATE_DUPS
#ifdef VALIDATE_DUPS      
#define KEEP_OLD_GENOMES
#endif
#define OPER_DEBUG  
#endif

//#define OPT_SPLIT_ON_GEN_BLOCK  

using namespace dtp;

#undef max

#ifdef OPER_DEBUG  
#include "sgp/GasmLister.h"

void debugTestInstruction(uint instrCode)
{
  uint instrCodeRaw, argCountInCode;
  sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCountInCode);
  if (instrCodeRaw > 200)
    throw scError(scString("Incorrect instruction code: ")+toString(instrCode)+"/"+toString(instrCodeRaw));
}
#endif

double calcBlockDegradFactor(uint blockNo, double factor)
{
  double blockFactor = 1.0;
  for(uint i = blockNo; i > 1; i--) {
    blockFactor *= factor;
  }
  return blockFactor;
}

uint getRandomBitSetItem(uint bitSet)
{
  uint res;
  uint bitCount = countBitOnes(bitSet);
  if (bitCount > 0) {
     uint bitNo = randomUInt(1, bitCount);
     res = getNthActiveBit(bitSet, bitNo - 1);
  }  
  else
    res = 0; // empty data set  
  return res;
}

// select an item index using specified probabilities, handling 0.0 probs is included
uint selectProbItem(const sgpGasmProbList &list)
{
  if (list.size() == 0)
    return 0;
    
  double sum = std::accumulate(list.begin(), list.end(), 0.0);
  double p = randomDouble(0.0, 1.0) * sum;
  double rprob = list[0];
  uint res = 0;
  uint epos = list.size() - 1;
  
  while ((p >= rprob) && (res < epos)) {
    res++;
    rprob += list[res];
  }
  
  return res;
}

uint calcIslandId(uint rawValue, uint islandCount)
{
   return(rawValue % islandCount);                       
}

bool getIslandId(uint &output, const sgpEntityForGasm &workInfo, uint islandLimit)
{
  bool res = true;
  int islandIdx = workInfo.getInfoValuePosInGenome(SGP_INFOBLOCK_IDX_OTHER_PARAM_BASE + SGP_INFOPARAM_IDX_OTHER_PARAM_ISLAND_ID);

  if ((islandIdx < 0) || (islandLimit == 0))
    res = false;
  
  if (res)  
  { 
    workInfo.getInfoUInt(SGP_INFOBLOCK_IDX_OTHER_PARAM_BASE + SGP_INFOPARAM_IDX_OTHER_PARAM_ISLAND_ID, output);
    output = ::calcIslandId(output, islandLimit);
  }
  return res;
}

bool setIslandId(sgpEntityForGasm &workInfo, uint islandId)
{
  bool res = true;
  int islandIdx = workInfo.getInfoValuePosInGenome(SGP_INFOBLOCK_IDX_OTHER_PARAM_BASE + SGP_INFOPARAM_IDX_OTHER_PARAM_ISLAND_ID);

  if (islandIdx < 0)
    res = false;
  
  if (res)  
  { 
    workInfo.setInfoUInt(SGP_INFOBLOCK_IDX_OTHER_PARAM_BASE + SGP_INFOPARAM_IDX_OTHER_PARAM_ISLAND_ID, islandId);
  }
  return res;
}

// returns multi-map (island-id => entity-index)
void prepareIslandMap(const sgpGaGeneration &newGeneration, uint islandLimit, scDataNode &output)
{
  uint islandId;
  const sgpEntityForGasm *workInfo;
  scString islandName;

  for(uint i = newGeneration.beginPos(), epos = newGeneration.endPos(); i != epos; i++)
  {
    workInfo = checked_cast<const sgpEntityForGasm *>(newGeneration.atPtr(i));
    
    if (workInfo->getInfoUInt(SGP_INFOBLOCK_IDX_OTHER_PARAM_BASE + SGP_INFOPARAM_IDX_OTHER_PARAM_ISLAND_ID, islandId))
    {
      islandId = calcIslandId(islandId, islandLimit);                       
    } else {
      islandId = 0;
    }
    islandName = toString(islandId);
    if (!output.hasChild(islandName)) {
      output.addChild(new scDataNode(islandName));
      output[islandName].setAsArray(vt_uint);
    }  
    output[islandName].addItemAsUInt(i);  
  }    
}

template < class T >
T switchedRound(bool doRound, T value)
{
  if (doRound)
    return static_cast<T>(round(value));
  else
    return value;  
}

// ----------------------------------------------------------------------------
// sgpGaGenomeCompareToolForGasm
// ----------------------------------------------------------------------------
// returns difference between two genomes, 0.0 - same, 1.0 - completely different
sgpGaGenomeCompareToolForGasm::sgpGaGenomeCompareToolForGasm(): sgpGaGenomeCompareTool()
{
  setMaxGenomeLength(0);
}

double sgpGaGenomeCompareToolForGasm::calcGenomeDiff(const sgpGaGeneration &newGeneration, uint first, uint second)
{
  return calcGenomeDiff(newGeneration, first, second, std::numeric_limits<unsigned int>::max());
}

double sgpGaGenomeCompareToolForGasm::calcGenomeDiff(const sgpGaGeneration &newGeneration, uint first, uint second, uint genNo)
{
  double ratioForCount;
  double ratioForDiff;
  double ratioForSize;
  double resRatio;
  uint cnt1, cnt2, minCnt, maxSize, minSize;
  uint startGenNo, endGenNo;
  sgpGaGenome gen1, gen2;
  bool singleGen = true;
  
  cnt1 = newGeneration[first].getGenomeCount();
  cnt2 = newGeneration[second].getGenomeCount();

  minCnt = std::min<uint>(cnt1, cnt2);      

  if (genNo == std::numeric_limits<unsigned int>::max())
  {
    startGenNo = 0;
    endGenNo = minCnt;
    singleGen = false;
  } else {
  // just one gen
    startGenNo = genNo;
    endGenNo = genNo + 1;
  }
  
  if (!singleGen) {
    if (cnt1 < cnt2)
      ratioForCount = double(cnt1) / cnt2;
    else
      ratioForCount = double(cnt2) / cnt1;
    ratioForCount = 1.0 - ratioForCount;  
  } else {  
    ratioForCount = 0.0;
  }  
          
  ratioForDiff = 0.0;
  ratioForSize = 0.0;
  
  const sgpEntityForGasm *firstWorkInfo;

  firstWorkInfo = static_cast<const sgpEntityForGasm *>(newGeneration.atPtr(first));
  
  for(uint i = startGenNo, epos = endGenNo; i != epos; i++) 
  {    
    newGeneration[first].getGenome(i, gen1);
    newGeneration[second].getGenome(i, gen2);
    maxSize = std::max<uint>(gen1.size(), gen2.size());
    minSize = std::min<uint>(gen1.size(), gen2.size());
    if (minSize > 0)
    {
      if (firstWorkInfo->isInfoBlock(i))
        ratioForDiff += calcGenomeDiffForValues(gen1, gen2);
      else
        ratioForDiff += calcGenomeDiffForCode(gen1, gen2);  
    }

    if (maxSize > 0)
      ratioForSize += (1.0 - static_cast<double>(minSize)/static_cast<double>(maxSize));
  } // for i 

  if (!singleGen && (minCnt > 0)) {
    minCnt = endGenNo - startGenNo;
    ratioForDiff = ratioForDiff / static_cast<double>(minCnt);
    ratioForSize = ratioForSize / static_cast<double>(minCnt);
  }

  if (singleGen)
    resRatio = (ratioForDiff + ratioForSize) / 2.0;
  else  
    resRatio = (ratioForDiff + ratioForCount + ratioForSize) / 3.0;
  
  return resRatio;
}

double sgpGaGenomeCompareToolForGasm::calcGenomeDiffForValues(const sgpGaGenome &gen1, const sgpGaGenome &gen2) 
{
  double res = 0.0;
  scString str1, str2;
  uint minSize = std::min<uint>(gen1.size(), gen2.size());
  
  for(uint j=0, eposj = minSize; j != eposj; j++) {
    if (gen1[j].getValueType() == vt_string) {
      str1 = gen1[j].getAsString();
      str2 = gen2[j].getAsString();
      if (str1.empty()) {
        if (!str2.empty()) 
          res += (1.0 / static_cast<double>(minSize));
      } else { // str1 not empty  
        res +=  
          (
            (
              static_cast<double>(m_diffFunct.get()->calc(str1, str2))
              / 
              static_cast<double>(str1.length())
            )
            *
            (1.0 / static_cast<double>(minSize))
          );    
      }
    } else {
      if (const_cast<scDataNodeValue &>(gen1[j]) != const_cast<scDataNodeValue &>(gen2[j]))
        res += (1.0 / static_cast<double>(minSize));
    }    
  } // for j
  return res;
}

double sgpGaGenomeCompareToolForGasm::calcGenomeDiffForCode(const sgpGaGenome &gen1, const sgpGaGenome &gen2) const
{
  double res = 0.0;
  sgpGaGenomeMetaList info1, info2;
  
  sgpEntityForGasm::buildMetaForCode(gen1, info1);
  sgpEntityForGasm::buildMetaForCode(gen2, info2);

  uint minSize = std::min<uint>(info1.size(), info2.size());
  uint idx = 0;
  while(idx < minSize)
  {
    if (const_cast<scDataNodeValue &>(gen1[idx]) != const_cast<scDataNodeValue &>(gen2[idx]))
    {
      if (info1[idx].userType != info2[idx].userType)
      {
        res += SGP_XOVER_DIFF_RATIO_META_TYPE;
      }  
      else if (info1[idx].userType == ggtInstrCode)
      {
        res += SGP_XOVER_DIFF_RATIO_INSTR;        
      }        
      else if (info1[idx].userType == ggtRegNo)
      {
        res += SGP_XOVER_DIFF_RATIO_REG_NO;
      }  
      else 
      { 
        res += SGP_XOVER_DIFF_RATIO_ARG_VALUE;
      }  
    }    
    idx++;
  }
  
  res /= static_cast<double>(minSize);
  return res;
}

void sgpGaGenomeCompareToolForGasm::checkMaxGenomeLength(uint value)
{
  if (m_maxGenomeLength < value)  
    setMaxGenomeLength(value * 15 / 10);
}

void sgpGaGenomeCompareToolForGasm::setMaxGenomeLength(uint value)
{
  if (value > 0)
    m_diffFunct.reset(new strDiffFunctor(value));
  else  
    m_diffFunct.reset(new strDiffFunctor(SGP_GASM_DEF_GENOME_MAX_LEN_4_DIFF));   
  m_maxGenomeLength = value;  
}

// ----------------------------------------------------------------------------
// sgpGaGenomeCompareToolForGasm
// ----------------------------------------------------------------------------
sgpDistanceFunctionLinearToTorus::sgpDistanceFunctionLinearToTorus()
{
  m_sourceLength = m_rowLength = 0;
}

sgpDistanceFunctionLinearToTorus::~sgpDistanceFunctionLinearToTorus()
{
}

void sgpDistanceFunctionLinearToTorus::setSourceLength(uint value)
{
  m_sourceLength = value;
  m_rowLength = round(std::sqrt(static_cast<float>(value)));
  if ((m_rowLength == 0) && (value > 0))
    m_rowLength = 1;
}

// f(x) = sqrt(dist-x^2 + dist-y^2) 
void sgpDistanceFunctionLinearToTorus::calcDistanceAbs(uint first, uint second, double &distance)
{
  int distX, distY;
  calcDistanceXY(first, second, distX, distY);
  distance = std::sqrt(static_cast<double>(distX*distX + distY*distY));
}

void sgpDistanceFunctionLinearToTorus::calcDistanceRel(uint first, uint second, double &distance)
{
  double workDist;
  // MAX distance is row length / 2
  double distLimit = static_cast<double>(m_rowLength)/2.0;
  calcDistanceAbs(first, second, workDist);
  if (workDist > distLimit)
    workDist = distLimit;
  workDist = distLimit - workDist;  
  workDist = 1.0 - workDist / distLimit;
  distance = workDist;
}

void sgpDistanceFunctionLinearToTorus::calcDistanceXY(uint first, uint second, int &distX, int &distY)
{
  int x1, y1, x2, y2;
  calcPosOnTorus(first, x1, y1);
  calcPosOnTorus(second, x2, y2);
  distX = tourus_distance(x1, x2, m_rowLength);
  distY = tourus_distance(y1, y2, m_rowLength);
}

void sgpDistanceFunctionLinearToTorus::calcPosOnTorus(uint linearPos, int &posX, int &posY)
{
  posX = linearPos % m_rowLength;
  posY = linearPos / m_rowLength;
}

// ----------------------------------------------------------------------------
// sgpGasmCodeProcessor
// ----------------------------------------------------------------------------
// calculate number of duplicated instructions / total no of instructions
double sgpGasmCodeProcessor::calcDupsRatio(const sgpGaGenomeMetaList &info, const sgpGaGenome &genome)
{
  uint dupCnt = 0;
  uint instrCnt = 0;
  cell_size_t instrOffset = 0;
  cell_size_t dupSize, instrSize;
  
  for(cell_size_t i=0,epos=info.size()+1; i!=epos; i++)
  {
    if ((i == epos-1) || (info[i].userType == ggtInstrCode)) {
      instrCnt++;
      if (i != instrOffset) {
      // find at least one duplicate for instruction (instrOffset..i - 1)
        dupSize = 0;
        instrSize = i - instrOffset;
        for(cell_size_t j=0,eposj=info.size()-instrSize; j!=eposj; j++) {
          if ((j < instrOffset) || (j >= instrOffset + instrSize)) {
            dupSize = 0;
            while((const_cast<scDataNodeValue &>(genome[j+dupSize]) == const_cast<scDataNodeValue &>(genome[instrOffset+dupSize]))) {
              dupSize++;
              if (dupSize >= instrSize) 
                break;
            }
            if (dupSize >= instrSize) {
              dupCnt++;
              break;    
            }  
          } // if not inside searched instr  
        } // for j
      } // if not first instr
    } // if instr
  } // for i
  if (instrCnt > 0) 
    return double(dupCnt)/double(instrCnt);
  else
    return 0.0;  
} // function

void sgpGasmCodeProcessor::checkMainNotEmpty(const sgpEntityForGasm *workInfo)
{
  sgpProgramCode code;  
  workInfo->getProgramCode(code);

  scDataNode mainCode;
  code.getBlockCode(SGP_GASM_MAIN_BLOCK_INDEX, mainCode);
  if (mainCode.size() == 0)
    throw scError("Main block empty");
}

void sgpGasmCodeProcessor::castTypeSet(uint supportedGasmTypes, sgpGasmDataNodeTypeSet &dataNodeTypes)
{
  dataNodeTypes.clear();
  
  uint typeId = 1;
  uint leftTypes = supportedGasmTypes;
  while(leftTypes != 0) {
    if ((leftTypes & typeId) != 0) {
      dataNodeTypes.insert(sgpVMachine::castGasmToDataNodeType(sgpGvmDataTypeFlag(typeId)));
      leftTypes = leftTypes ^ typeId;
    } 
    typeId = typeId << 1;
    if (typeId > gdtfMax)
      break;
  }  
}  

bool sgpGasmCodeProcessor::buildRandomInstr(const sgpFunctionMapColn &functions,
  sgpGasmRegSet &writtenRegs, uint supportedDataTypes, sgpGasmDataNodeTypeSet &dataNodeTypes, 
  const sgpGasmProbList &instrCodeProbs,  
  const sgpVMachine *vmachine,
  const sgpEntityForGasm *workInfo,
  const uint *targetBlockIndex,
  sgpGaGenome &newCode)
{  
  uint maxInstrCode = functions.size() - 1;
  scDataNodeValueType randType;
  uint typeNo;  
  bool bArgAsReg;
      
  uint instrCode;
    
  if (instrCodeProbs.empty()) 
    instrCode = randomInt(0, maxInstrCode);
  else
    instrCode = ::selectProbItem(instrCodeProbs);  
    
  uint argCnt = 0;
  uint argNo = 0;
  
  sgpFunctionMapColnIterator p = const_cast<sgpFunctionMapColn &>(functions).find(instrCode);
  sgpFunction *functor;
  
  if (p == functions.end()) {
    return false;
  } 
  
  sgpFunctionTransporter transporter = p->second;
  uint minArgCount, maxArgCount;
  functor = &(*transporter);
  scDataNodeValue newCell;
  uint instrCodeEnc;

  functor->getArgCount(minArgCount, maxArgCount);
  
  scDataNode argMeta;
  scDataNode cellNode;
  uint ioMode;
  sgpGasmRegSet::const_iterator writtenRegsIt;
  sgpGasmDataNodeTypeSet argNodeDataTypes;  
      
  if (functor->hasDynamicArgs())   
  {
  // build special args - context dependent
    sgpProgramCode code;  
    uint blockCount;
    bool canAddCall;

    if (workInfo != SC_NULL) 
    {
      workInfo->getProgramCode(code);
    }  
    
    switch (functor->getId()) {
      case FUNCID_CORE_CALL:
      case FUNCID_MACROS_MACRO:
      case FUNCID_MACROS_MACRO_REL:
      {
      // build random call
        blockCount = code.getBlockCount();
        canAddCall = (blockCount > 1);
        uint firstBlockNo;  
        if (targetBlockIndex != SC_NULL) {
          firstBlockNo = (*targetBlockIndex) + 1;
          canAddCall = canAddCall && (firstBlockNo >= (blockCount - 1)); 
        } else {
          firstBlockNo = 1;
        }  
          
        if (canAddCall) {
          uint randomBlockNo = randomUInt(firstBlockNo, blockCount - 1);
          scDataNode argMetaInput, argMetaOutput;
          code.getBlockMetaInfo(randomBlockNo, argMetaInput, argMetaOutput);
          argCnt = argMetaInput.size() + argMetaOutput.size() + 1;
          
          instrCodeEnc = sgpVMachine::encodeInstr(instrCode, argCnt);    
          newCell.setAsUInt(instrCodeEnc);
          newCode.push_back(newCell);

          newCell.setAsUInt(randomBlockNo);
          newCode.push_back(newCell);
          
          argCnt--; // remove block-no
          std::auto_ptr<scDataNode> newArgMetaGuard;
          
          argMeta.clear();
          
          for(uint i=0, epos = argMetaOutput.size(); i != epos; i++)
          {
            newArgMetaGuard.reset(new scDataNode());
            sgpVMachine::buildFunctionArgMeta(*newArgMetaGuard, gatfOutput, gatfLValue, sgpVMachine::castDataNodeToGasmType(argMetaOutput[i].getValueType()));
            argMeta.addChild(newArgMetaGuard.release());
          }

          for(uint i=0, epos = argMetaInput.size(); i != epos; i++)
          {
            newArgMetaGuard.reset(new scDataNode());
            sgpVMachine::buildFunctionArgMeta(*newArgMetaGuard, gatfInput, gatfAny, sgpVMachine::castDataNodeToGasmType(argMetaInput[i].getValueType()));
            argMeta.addChild(newArgMetaGuard.release());
          }
        }  
      }
      default:
        // do nothing
        ;
    }
    
    if (newCode.empty())
      return false;    
    // else - generate call's args below  
  } else if (!functor->getArgMeta(argMeta)) {
    return false;
  } else {
    argCnt = randomInt(minArgCount, maxArgCount);
    instrCodeEnc = sgpVMachine::encodeInstr(instrCode, argCnt);    
    newCell.setAsUInt(instrCodeEnc);
    newCode.push_back(newCell);
  }  
  
  uint leftCnt = argCnt;
  uint regNo, regIdx;
  // --- generate range selection with probability:
  // 50% for small numbers
  // 37% for medium numbers
  // 13% for large numbers
  // 10% of medium real numbers will be rounded (3.7% of real numbers)
  uint typeRangeSelector = randomInt(0,99);
  bool smallConst = (typeRangeSelector < 50); 
  bool mediumConst = (!smallConst) && (typeRangeSelector < 50 + 37); 
  bool roundConst = ((typeRangeSelector % 10) == 0);
   
  // ----
  uint dataTypes;
  sgpGasmRegSet regSet, workRegs;
  std::set<scDataNodeValueType> allowedDataTypes;
  uint argTypes;
  bool variantAllowed;
    
  while(leftCnt > 0) {
    newCell.setAsNull();
    if (argMeta.size() > argNo) {
      ioMode = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_IO_MODE);
      dataTypes = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_DATA_TYPE);
      argTypes = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_ARG_TYPE);
    } else {
      ioMode = 0; 
      dataTypes = 0; 
      argTypes = 0;
    }  
    if ((ioMode & gatfOutput) != 0)
      bArgAsReg = true;
    else  
      bArgAsReg = ((argTypes & gatfRegister) != 0) && randomFlip(SGP_GASM_RAND_ARG_PROB); 

    if (bArgAsReg) {
      prepareRegisterSet(regSet, dataTypes & supportedDataTypes, ioMode, vmachine);
      
      if ((ioMode & gatfInput) != 0) {
      // input
        // join writtenRegs with regSet
        for(sgpGasmRegSet::const_iterator it = writtenRegs.begin(), epos = writtenRegs.end(); it != epos; ++it)
        {
          if (regSet.find(*it) != regSet.end())
            workRegs.insert(*it);
        }
            
        if (workRegs.size()>0) {     
          regIdx = randomInt(0, workRegs.size() - 1);
          regNo = *(workRegs.begin());
          for(sgpGasmRegSet::const_iterator it=writtenRegs.begin(),epos=writtenRegs.end();it != epos; ++it,regIdx--) 
          {
            if (regIdx == 0) {
              regNo = *it;
              break;
            }
          }  
          sgpVMachine::buildRegisterArg(cellNode, regNo);
          newCell.copyFrom(cellNode);
        }
      } else {
      // output
        newCell.copyFrom(randomRegNoAsValue(regSet));
      }  
    } else {
    // constant
      sgpGasmCodeProcessor::castTypeSet(dataTypes, argNodeDataTypes);
      variantAllowed = ((dataTypes & gdtfVariant) != 0);
  
      allowedDataTypes.clear();
      for(std::set<scDataNodeValueType>::const_iterator it=dataNodeTypes.begin(),epos=dataNodeTypes.end(); it != epos; ++it)
      {  
        if (
             ((*it == vt_null) && variantAllowed)
             ||
             (argNodeDataTypes.find(*it) != argNodeDataTypes.end())
           )
        {     
          allowedDataTypes.insert(*it);
        }  
      }
      
      variantAllowed = variantAllowed && 
          (allowedDataTypes.find(vt_null) != allowedDataTypes.end());  
      
      typeNo = randomInt(0, allowedDataTypes.size() - 1);
      
      randType = vt_null;
      for(std::set<scDataNodeValueType>::const_iterator it=allowedDataTypes.begin(),epos=allowedDataTypes.end(); it != epos; ++it, typeNo--)
      {  
        if (typeNo == 0) {
          randType = *it;
          break;
        } 
      }
      
      switch (randType) {
        case vt_null:
          newCell.setAsNull();
          break;      
        case vt_byte:
          if (smallConst)
            newCell.setAsByte(byte(randomInt(0, 2)));
          else if (mediumConst)
            newCell.setAsByte(byte(randomInt(0, 10)));
          else  
            newCell.setAsByte(byte(randomInt(0, 255)));
          break;      
        case vt_int:
          if (smallConst)
            newCell.setAsInt(randomInt(-2, 2));
          else if (mediumConst)
            newCell.setAsInt(randomInt(-10, 10));
          else  
            newCell.setAsInt(randomInt(INT_MIN, INT_MAX));
          break;      
        case vt_uint: 
          if (smallConst)
            newCell.setAsUInt(randomUInt(0, 2));
          else if (mediumConst)
            newCell.setAsUInt(randomUInt(0, 10));
          else  
            newCell.setAsUInt(randomUInt(0, UINT_MAX));
          break;      
        case vt_int64: 
          if (smallConst)
            newCell.setAsInt64(randomInt(-2, 2));
          else if (mediumConst)
            newCell.setAsInt64(randomInt(-10, 10));
          else  
            newCell.setAsInt64(round(randomDouble(LONG_MIN, LONG_MAX)));
          break;      
        case vt_uint64: 
          if (smallConst)
            newCell.setAsUInt64(randomUInt(0, 2));
          else if (mediumConst)
            newCell.setAsUInt64(randomUInt(0, 10));
          else
            newCell.setAsUInt64(randomUInt(0, ULONG_MAX));
          break;      
        case vt_string: {
          scString val;
          if (smallConst)
            randomString("", 5, val);  
          else  
            randomString("", SGP_GASM_MAX_RAND_STR_LEN, val);  
          newCell.setAsString(val);
          break;      
        }  
        case vt_bool: 
          newCell.setAsBool(randomBool());
          break;      
        case vt_float: 
          if (smallConst)
            newCell.setAsFloat(randomDouble(-1.0, 1.0));
          else if (mediumConst)
            newCell.setAsFloat(switchedRound(roundConst, randomDouble(-10.0, 10.0)));
          else
            newCell.setAsFloat(randomDouble(static_cast<double>(INT_MIN), static_cast<double>(INT_MAX)));
          break;      
        case vt_double: 
          if (smallConst)
            newCell.setAsDouble(randomDouble(-1.0, 1.0));
          else if (mediumConst)
            newCell.setAsDouble(switchedRound(roundConst, randomDouble(-10.0, 10.0)));
          else  
            newCell.setAsDouble(randomDouble(static_cast<double>(INT_MIN), static_cast<double>(INT_MAX)));
          break;      
        case vt_xdouble:
          if (smallConst)          
            newCell.setAsXDouble(randomDouble(-1.0, 1.0));
          else if (mediumConst)
            newCell.setAsXDouble(switchedRound(roundConst, randomDouble(-10.0, 10.0)));
          else
            newCell.setAsXDouble(randomDouble(static_cast<double>(INT_MIN), static_cast<double>(INT_MAX)));
          break;      
        default:
          throw scError("Wrong arg type: "+toString(randType));
      } // switch constant type
    } // if constant
    newCode.push_back(newCell);
    leftCnt--;
    argNo++;
  } // while argCnt

  return true;
}

void sgpGasmCodeProcessor::getArguments(const sgpGaGenome &genome, 
  int start, uint argCount, scDataNode &output)
{
  output.clear();
  std::auto_ptr<scDataNode> cellDataGuard;
  for(uint i=start,epos = std::min<int>(start+argCount, genome.size()); i != epos; i++)
  {
    cellDataGuard.reset(new scDataNode());
    *cellDataGuard = genome[i];
    output.addChild(cellDataGuard.release());
  }  
}

void sgpGasmCodeProcessor::prepareRegisterSet(sgpGasmRegSet &regSet, 
  uint supportedDataTypes, uint ioMode, const sgpVMachine *vmachine)
{
  vmachine->prepareRegisterSet(regSet, supportedDataTypes, ioMode);
}

scDataNodeValue sgpGasmCodeProcessor::randomRegNoAsValue(const sgpGasmRegSet &regSet) 
{
  assert(regSet.size() > 0);
  uint regIndex = randomInt(0, regSet.size() - 1);
  uint regNo = *regSet.begin();
  for(sgpGasmRegSet::const_iterator it=regSet.begin(),epos=regSet.end(); it != epos; ++it, regIndex--)
  {
    if (regIndex == 0) {
      regNo = *it;
      break;
    }
  }
    
  return sgpVMachine::buildRegisterArg(regNo);
}

uint sgpGasmCodeProcessor::randomRegNo(const sgpGasmRegSet &regSet) 
{
  assert(regSet.size() > 0);
  uint regIndex = randomInt(0, regSet.size() - 1);
  uint regNo = *regSet.begin();
  for(sgpGasmRegSet::const_iterator it=regSet.begin(),epos=regSet.end(); it != epos; ++it, regIndex--)
  {
    if (regIndex == 0) {
      regNo = *it;
      break;
    }
  }
    
  return regNo;
}


