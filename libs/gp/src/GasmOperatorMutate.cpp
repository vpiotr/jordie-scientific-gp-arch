/////////////////////////////////////////////////////////////////////////////
// Name:        GasmOperatorMutate.cpp
// Project:     scLib
// Purpose:     Mutation operator for GP virtual machine.
// Author:      Piotr Likus
// Modified by:
// Created:     29/12/2009
/////////////////////////////////////////////////////////////////////////////

#define USE_GASM_OPERATOR_STATS
#define OPT_KEEP_MUT_RATE_IN_MID

//sc
#include "sc/rand.h"
#include "sc/smath.h"

//other
#include "sgp/GasmOperatorMutate.h"
#include "sgp/GasmOperator.h"
#include "sgp/GasmIslandCommon.h"

#include "sgp\ExperimentConst.h"

#ifdef TRACE_ENTITY_BIO
#include "sgp\GpEntityTracer.h"
#endif

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

const uint WRITTEN_REGS_REGNO     = 0;
const uint WRITTEN_REGS_DIST      = 1;
const uint WRITTEN_REGS_DATA_TYPE = 2;

const double SGP_MUT_FADE_MAX_USED_VALUE = 0.9;

using namespace dtp;

// ----------------------------------------------------------------------------
// sgpGasmOperatorMutate
// ----------------------------------------------------------------------------
// construction
sgpGasmOperatorMutate::sgpGasmOperatorMutate(): inherited()
{
  m_probability = m_infoProbability = SGP_GASM_DEF_OPER_PROB_MUTATE;
  m_largeFloatChangeRatio = SGP_GASM_MUT_REAL_LARGE_CHANGE_RATIO;  
  m_smallFloatChangeRatio = SGP_GASM_MUT_REAL_SMALL_CHANGE_RATIO;                                
  m_globalMutRatio = SGP_GASM_MUT_DEF_GLOBAL_RATIO;
  m_changeRatio = 1.0;
  m_changeRateMin = SGP_GASM_MUT_CHANGE_RATE_MIN;
  m_changeRateMax = SGP_GASM_MUT_CHANGE_RATE_MAX;
  m_islandLimit = 0;
  m_maxBlockCount = 0;
  m_experimentParams = SC_NULL;
  m_blockSizeLimit = SGP_GASM_MUT_DEF_BLOCK_SIZE_LIMIT;
  m_blockSizeLimitStep = 0;
  m_yieldSignal = SC_NULL;
  m_islandTool = SC_NULL;
  
  setSupportedDataTypes(SGP_GASM_DEF_DATA_TYPES);
  setInstrCodeChangeSpread(SGP_GASM_DEF_INSTR_CODE_CHANGE_SPREAD);
  setRegNoChangeSpread(SGP_GASM_DEF_REG_NO_CHANGE_SPREAD);
  setFeatures(gmfDefault);
  fillTypeList();
}

sgpGasmOperatorMutate::~sgpGasmOperatorMutate()
{
}

// properties
double sgpGasmOperatorMutate::getProbability()
{
  return m_probability;
}

void sgpGasmOperatorMutate::setProbability(double aValue)
{
  m_probability = aValue;
}

double sgpGasmOperatorMutate::getInfoProbability()
{
  return m_infoProbability;
}

void sgpGasmOperatorMutate::setInfoProbability(double aValue)
{
  m_infoProbability = aValue;
}

void sgpGasmOperatorMutate::setFunctions(const sgpFunctionMapColn &functions)
{
  m_functions = functions;
  
  m_functionNameMap.clear();
  
  for(sgpFunctionMapColn::const_iterator it=functions.begin(),
      epos=functions.end(); 
      it!=epos; 
      it++)
  {
    m_functionNameMap.addChild(it->second->getName(), new scDataNode(it->first));
  }    
}

void sgpGasmOperatorMutate::getFunctions(sgpFunctionMapColn &functions)
{
  functions = m_functions;
}

void sgpGasmOperatorMutate::setVMachine(sgpVMachine *machine)
{
  m_machine = machine;
}

void sgpGasmOperatorMutate::setSupportedDataTypes(uint mask)
{
  m_supportedDataTypes = mask;
  sgpGasmCodeProcessor::castTypeSet(m_supportedDataTypes, m_supportedDataNodeTypes);
}

void sgpGasmOperatorMutate::setYieldSignal(scSignal *value)
{
  m_yieldSignal = value;
}

void sgpGasmOperatorMutate::setIslandTool(sgpEntityIslandToolIntf *value)
{
  m_islandTool = value;
}

void sgpGasmOperatorMutate::getCounters(scDataNode &output)
{
  output = m_counters;
}

void sgpGasmOperatorMutate::setInstrCodeChangeSpread(uint value)
{
  m_instrCodeChangeSpread = value;
  m_instrCodeChangeMin = -int(value/2);
  m_instrCodeChangeMax = int(value/2);
}

void sgpGasmOperatorMutate::setRegNoChangeSpread(uint value)
{
  m_regNoChangeSpread = value;
  m_regNoChangeMin = -int(value/2);
  m_regNoChangeMax = int(value/2);
}

void sgpGasmOperatorMutate::setLargeFloatChangeRatio(double value)
{
  m_largeFloatChangeRatio = value;
}

void sgpGasmOperatorMutate::setSmallFloatChangeRatio(double value)
{
  m_smallFloatChangeRatio = value;
}

void sgpGasmOperatorMutate::fillTypeList()
{
  m_typeListGlobal.resize(SGP_GASM_MUT_PROB_SECTION_GLOBAL_SIZE);
  m_typeListGlobal[SGP_GASM_MUT_PROB_IDX_GLOBAL_DELETE] = gmtDelete;
  m_typeListGlobal[SGP_GASM_MUT_PROB_IDX_GLOBAL_REPLACE] = gmtReplace;
  m_typeListGlobal[SGP_GASM_MUT_PROB_IDX_GLOBAL_INSERT] = gmtInsert;
  m_typeListGlobal[SGP_GASM_MUT_PROB_IDX_GLOBAL_BLOCK] = gmtBlock;
  
  m_typeListValue.resize(SGP_GASM_MUT_PROB_SECTION_VALUE_SIZE);
  m_typeListValue[SGP_GASM_MUT_PROB_IDX_VALUE_VALUE] = gmtValue;
  m_typeListValue[SGP_GASM_MUT_PROB_IDX_VALUE_VALUE_REG] = gmtValueReg;
  m_typeListValue[SGP_GASM_MUT_PROB_IDX_VALUE_TYPE_UP] = gmtValueTypeUp;
  m_typeListValue[SGP_GASM_MUT_PROB_IDX_VALUE_TYPE_DOWN] = gmtValueTypeDown;
  m_typeListValue[SGP_GASM_MUT_PROB_IDX_VALUE_SWAP_ARGS] = gmtSwapArgs;
  m_typeListValue[SGP_GASM_MUT_PROB_IDX_VALUE_NEG] = gmtValueNeg;  
  
  m_typeListInstr.resize(SGP_GASM_MUT_PROB_SECTION_INSTR_SIZE);
  m_typeListInstr[SGP_GASM_MUT_PROB_IDX_INSTR_INSTR_CODE] = gmtInstrCode;
  m_typeListInstr[SGP_GASM_MUT_PROB_IDX_INSTR_JOIN_INSTR] = gmtJoinInstr;
  m_typeListInstr[SGP_GASM_MUT_PROB_IDX_INSTR_SPLIT_INSTR] = gmtSplitInstr;
  m_typeListInstr[SGP_GASM_MUT_PROB_IDX_INSTR_LINK_INSTR] = gmtLinkInstr;

  m_typeListRegNo.resize(SGP_GASM_MUT_PROB_SECTION_REGNO_SIZE);
  m_typeListRegNo[SGP_GASM_MUT_PROB_IDX_REGNO_REGNO] = gmtRegNo;
  m_typeListRegNo[SGP_GASM_MUT_PROB_IDX_REGNO_REGNO_ZERO] = gmtRegNoZero;
  m_typeListRegNo[SGP_GASM_MUT_PROB_IDX_REGNO_REG_VALUE] = gmtRegValue;
  m_typeListRegNo[SGP_GASM_MUT_PROB_IDX_REGNO_SWAP_ARGS] = gmtSwapArgs;

  m_typeListMacro.resize(SGP_GASM_MUT_PROB_SECTION_MACRO_SIZE);
  m_typeListMacro[SGP_GASM_MUT_PROB_IDX_MACRO_INS]        = gmtMacroIns;
  m_typeListMacro[SGP_GASM_MUT_PROB_IDX_MACRO_GEN]        = gmtMacroGen;
  m_typeListMacro[SGP_GASM_MUT_PROB_IDX_MACRO_DEL]        = gmtMacroDel;
  m_typeListMacro[SGP_GASM_MUT_PROB_IDX_MACRO_ARG_ADD]    = gmtMacroArgAdd;
  m_typeListMacro[SGP_GASM_MUT_PROB_IDX_MACRO_ARG_DEL]    = gmtMacroArgDel;
}

void sgpGasmOperatorMutate::setTypeProbs(const scDataNode &valueList)
{
  uint vsize = valueList.size();
  m_typeProbs = valueList;

  // global section
  m_typeProbSectionGlobal.resize(SGP_GASM_MUT_PROB_SECTION_GLOBAL_SIZE); 
  for(sgpGasmProbList::iterator it = m_typeProbSectionGlobal.begin(),epos=m_typeProbSectionGlobal.end(); it!=epos; ++it)
    *it = 0.0;

  if (vsize >= SGP_GASM_MUT_PROB_SECTION_GLOBAL + m_typeProbSectionGlobal.size() - 1) {
    for(uint i=0,epos = m_typeProbSectionGlobal.size(); i!=epos; i++) {
      m_typeProbSectionGlobal[i] = valueList.getDouble(SGP_GASM_MUT_PROB_SECTION_GLOBAL + i);
    }    
  }
  
  // value section
  m_typeProbSectionValue.resize(SGP_GASM_MUT_PROB_SECTION_VALUE_SIZE);
  for(sgpGasmProbList::iterator it = m_typeProbSectionValue.begin(),epos=m_typeProbSectionValue.end(); it!=epos; ++it)
    *it = 0.0;

  if (vsize >= SGP_GASM_MUT_PROB_SECTION_VALUE + m_typeProbSectionValue.size() - 1) {
    for(uint i=0,epos = m_typeProbSectionValue.size(); i!=epos; i++) {
      m_typeProbSectionValue[i] = valueList.getDouble(SGP_GASM_MUT_PROB_SECTION_VALUE + i);
    }    
  }

  // instruction section
  m_typeProbSectionInstr.resize(SGP_GASM_MUT_PROB_SECTION_INSTR_SIZE);
  for(sgpGasmProbList::iterator it = m_typeProbSectionInstr.begin(),epos=m_typeProbSectionInstr.end(); it!=epos; ++it)
    *it = 0.0;
  
  if (vsize >= SGP_GASM_MUT_PROB_SECTION_INSTR + m_typeProbSectionInstr.size() - 1) {
    for(uint i=0,epos = m_typeProbSectionInstr.size(); i!=epos; i++) {
      m_typeProbSectionInstr[i] = valueList.getDouble(SGP_GASM_MUT_PROB_SECTION_INSTR + i);
    }    
  }
  
  // regNo section  
  m_typeProbSectionRegNo.resize(SGP_GASM_MUT_PROB_SECTION_REGNO_SIZE);
  for(sgpGasmProbList::iterator it = m_typeProbSectionRegNo.begin(),epos=m_typeProbSectionRegNo.end(); it!=epos; ++it)
    *it = 0.0;

  if (vsize >= SGP_GASM_MUT_PROB_SECTION_REGNO + m_typeProbSectionRegNo.size() - 1) {
    for(uint i=0,epos = m_typeProbSectionRegNo.size(); i!=epos; i++) {
      m_typeProbSectionRegNo[i] = valueList.getDouble(SGP_GASM_MUT_PROB_SECTION_REGNO + i);
    }    
  }

  // macro section  
  m_typeProbSectionMacro.resize(SGP_GASM_MUT_PROB_SECTION_MACRO_SIZE);
  for(sgpGasmProbList::iterator it = m_typeProbSectionMacro.begin(),epos=m_typeProbSectionMacro.end(); it!=epos; ++it)
    *it = 0.0;

  if (vsize >= SGP_GASM_MUT_PROB_SECTION_MACRO + m_typeProbSectionMacro.size() - 1) {
    for(uint i=0,epos = m_typeProbSectionMacro.size(); i!=epos; i++) {
      m_typeProbSectionMacro[i] = valueList.getDouble(SGP_GASM_MUT_PROB_SECTION_MACRO + i);
    }    
  }
}

uint sgpGasmOperatorMutate::getTypeProbsSize() const
{
  return SGP_GASM_MUT_PROB_SECTION_COUNT*SGP_GASM_MUT_PROB_SECTION_SIZE;
}

uint sgpGasmOperatorMutate::getCodeAreaFactorsCount() const
{
  return SGP_GASM_MUT_AREA_COUNT;
}

void sgpGasmOperatorMutate::setMetaForInfoBlock(const sgpGaGenomeMetaList &value)
{
  m_metaForInfoBlock = value;
}

void sgpGasmOperatorMutate::setMaxBlockCount(uint value)
{
  m_maxBlockCount = value;
}

uint sgpGasmOperatorMutate::getMaxBlockCount()
{
  return m_maxBlockCount;
}  

void sgpGasmOperatorMutate::setGenomeChangedTracer(sgpGenomeChangedTracer *tracer)
{
  m_genomeChangedTracer = tracer;
}

uint sgpGasmOperatorMutate::getFeatures()
{
  return m_features;
}

void sgpGasmOperatorMutate::setFeatures(uint value)
{
  m_features = value;
}

void sgpGasmOperatorMutate::setFadingInfoParamSet(const sgpGasmInfoParamSet &paramSet)
{
  m_fadingParamSet = paramSet;
}

void sgpGasmOperatorMutate::setFadingInfoParamSet2(const sgpGasmInfoParamSet &paramSet)
{
  m_fadingParamSet2 = paramSet;
}
  
void sgpGasmOperatorMutate::setFadingInfoParamSet3(const sgpGasmInfoParamSet &paramSet)
{
  m_fadingParamSet3 = paramSet;
}
  
void sgpGasmOperatorMutate::setIslandLimit(uint value)
{
  m_islandLimit = value;
}

void sgpGasmOperatorMutate::setExperimentParams(const sgpGaExperimentParams *params)
{
  m_experimentParams = params;
}

// run
void sgpGasmOperatorMutate::init()
{
  if (m_metaForInfoBlock.size() == 0)
    calcMetaForInfo(m_metaForInfoBlock);
  calcInfoBlockSize(m_metaForInfoBlock, m_infoBlockSize); 
  m_changeRatio = 1.0;   
}

void sgpGasmOperatorMutate::calcInfoBlockSize(const sgpGaGenomeMetaList &metaInfo, uint &output)
{
  uint genomeSize = 0;
  
  for(sgpGaGenomeMetaList::const_iterator it =metaInfo.begin(), epos = metaInfo.end(); it != epos; it++)
  {
    genomeSize += it->genSize;
  }  
  output = genomeSize;
}  

void sgpGasmOperatorMutate::execute(sgpGaGeneration &newGeneration)
{  
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.clear();
#endif  

  m_changedEntityCount = 0;
  m_scannedCount = 0;

  if (m_islandLimit > 0)
    executeByIslands(newGeneration);
  else
    executeOnAll(newGeneration);
      
  if ((m_features & gmfControlMutRate) != 0) {
    updateMutRate(m_scannedCount, m_changedEntityCount);
  }
}

void sgpGasmOperatorMutate::executeByIslands(sgpGaGeneration &newGeneration)
{
  scDataNode islandItems;  
  intPrepareIslandMap(newGeneration, m_islandLimit, islandItems);

  scString islandName;
  for(uint i = 0, epos = m_islandLimit; i != epos; i++)
  {
    islandName = toString(i); 
    if (islandItems.hasChild(islandName))
      executeOnIsland(newGeneration, islandItems[islandName], i);
  }
}

void sgpGasmOperatorMutate::intPrepareIslandMap(const sgpGaGeneration &input, uint islandLimit, scDataNode &output)
{
  //::prepareIslandMap(input, islandLimit, output);
  m_islandTool->prepareIslandMap(input, output);
}

void sgpGasmOperatorMutate::executeOnIsland(sgpGaGeneration &newGeneration, scDataNode &islandItems, uint islandId)
{
  executeOnBlockByIds(newGeneration, islandItems);
}

void sgpGasmOperatorMutate::executeOnAll(sgpGaGeneration &newGeneration)
{
  executeOnBlock(newGeneration);
}

void sgpGasmOperatorMutate::executeOnBlock(sgpGaGeneration &newGeneration)
{
  double baseProb = m_probability;
  double codeProb;

  initStep(newGeneration);
  for(int i = newGeneration.beginPos(), epos = newGeneration.size(); i != epos; i++)
  {
    codeProb = calcCodeMutProb(checked_cast_ref<sgpEntityForGasm &>(newGeneration.at(i)), baseProb);
    m_scannedCount++;
    if (processEntity(newGeneration, i, m_infoProbability, codeProb))
      m_changedEntityCount++;      
  }    
}

void sgpGasmOperatorMutate::executeOnBlockByIds(sgpGaGeneration &newGeneration, scDataNode &itemList)
{
  double baseProb = m_probability;
  double codeProb;
  uint idx;

  initStep(newGeneration, itemList);
  for(int i = 0, epos = itemList.size(); i != epos; i++)
  {
    idx = itemList.getUInt(i);
    codeProb = calcCodeMutProb(checked_cast_ref<sgpEntityForGasm &>(newGeneration.at(idx)), baseProb);
    m_scannedCount++;
    if (processEntity(newGeneration, idx, m_infoProbability, codeProb))
      m_changedEntityCount++;      
  }    
}

double sgpGasmOperatorMutate::calcCodeMutProb(const sgpEntityForGasm &workInfo, double baseProb)
{
  double res;
  double probParam = 0.0;
  
  bool found;

  if (workInfo.hasInfoBlock())
  {
    found = workInfo.getInfoDouble(SGP_INFOBLOCK_IDX_MUT_MAIN_PARAMS_BASE + SGP_INFOBLOCK_IDX_MUT_PROB, probParam);
  } else {
    found = false;
  }  

  if (found) {  
    res = (probParam - 0.5) * 2.0;
    res += baseProb;
  } else {
    res = baseProb;
  }
  
  return res;
}

void sgpGasmOperatorMutate::initStep(sgpGaGeneration &newGeneration)
{
  updateBlockSizeLimit(newGeneration);
}

void sgpGasmOperatorMutate::initStep(sgpGaGeneration &newGeneration, const scDataNode &itemList)
{
  updateBlockSizeLimit(newGeneration, itemList);
}

void sgpGasmOperatorMutate::updateBlockSizeLimit(sgpGaGeneration &newGeneration)
{
  sgpProgramCode program;
  uint maxValue = 0;
  for(int i = newGeneration.beginPos(), epos = newGeneration.size(); i != epos; i++)
  {
    checked_cast_ref<sgpEntityForGasm &>(newGeneration.at(i)).getProgramCode(program);
    maxValue = SC_MAX(program.getMaxBlockLength(), maxValue);
  }  
  maxValue = round(static_cast<double>(maxValue) * SGP_MUT_MAX_SIZE_INC_RATE);
  if (m_blockSizeLimit > 0)  
    m_blockSizeLimitStep = SC_MIN(m_blockSizeLimit, maxValue);
  else  
    m_blockSizeLimitStep = maxValue;
}

void sgpGasmOperatorMutate::updateBlockSizeLimit(sgpGaGeneration &newGeneration, const scDataNode &itemList)
{
  sgpProgramCode program;
  uint maxValue = 0;
  uint idx;
  for(int i = 0, epos = itemList.size(); i != epos; i++)
  {
    idx = itemList.getUInt(i);
    checked_cast_ref<sgpEntityForGasm &>(newGeneration.at(idx)).getProgramCode(program);
    maxValue = SC_MAX(program.getMaxBlockLength(), maxValue);
  }  
  maxValue = round(static_cast<double>(maxValue) * SGP_MUT_MAX_SIZE_INC_RATE);
  if (m_blockSizeLimit > 0)  
    m_blockSizeLimitStep = SC_MIN(m_blockSizeLimit, maxValue);
  else  
    m_blockSizeLimitStep = maxValue;
}

void sgpGasmOperatorMutate::updateMutRate(uint totalPopSize, uint changedEntityCount)
{
  float changedRate;
  if (totalPopSize > 0) 
  {
     changedRate = static_cast<float>(changedEntityCount) / static_cast<float>(totalPopSize);
#ifdef OPT_KEEP_MUT_RATE_IN_MID     
     float midValue = m_changeRateMin + (m_changeRateMax - m_changeRateMin) / 2.0;
     if (changedRate > midValue)
       m_changeRatio = m_changeRatio * SGP_MUT_CHANGE_RATIO_FACTOR_DEC;
     else if (changedRate < midValue)
       m_changeRatio = m_changeRatio * SGP_MUT_CHANGE_RATIO_FACTOR_INC;
#endif       
     if (changedRate > m_changeRateMax)
       m_changeRatio = m_changeRatio * SGP_MUT_CHANGE_RATIO_FACTOR_DEC;
     else if (changedRate < m_changeRateMin)
       m_changeRatio = m_changeRatio * SGP_MUT_CHANGE_RATIO_FACTOR_INC;
  }   
}

bool sgpGasmOperatorMutate::processEntity(sgpGaGeneration &newGeneration, uint entityIndex, 
  double infoProb, double codeProb)
{
  sgpGaGenome genome;
  scDataNode blockMeta;

  int genomeCount;
  uint inputCount;
  bool genomeModified;
  double useProb;
  bool bInfoExists, bInfoBlock; 
  bool mutPerformed;
  bool entityMutated = false;
  
  genomeCount = newGeneration.at(entityIndex).getGenomeCount();
  sgpEntityForGasm *workInfo = checked_cast<sgpEntityForGasm *>(newGeneration.atPtr(entityIndex));
  bInfoExists = workInfo->hasInfoBlock();

#ifdef TRACE_GASM_MUTATE
  sgpGaGenome infoGenome;
  newGeneration.at(entityIndex).getGenome(SGP_GASM_INFO_BLOCK_IDX, infoGenome);
  bool infoOk = (infoGenome.size() >= 20);
#endif    

  for(int j = 0; j != genomeCount; j++) {
    newGeneration.at(entityIndex).getGenome(j, genome);
    bInfoBlock = bInfoExists && (j == SGP_GASM_INFO_BLOCK_IDX);
    if (bInfoBlock) {
      inputCount = 0;
      useProb = infoProb;
    } else {  
      getBlockMeta(newGeneration, entityIndex, j, blockMeta);
      inputCount = getInputCountFromBlockMeta(blockMeta);
      useProb = codeProb;
    }  

    genomeModified = processGenome(genome, genomeCount, useProb, bInfoBlock, inputCount, workInfo, j, entityIndex, mutPerformed);
    entityMutated = entityMutated || mutPerformed;
    
#ifdef TRACE_GASM_MUTATE
    if (genomeModified && !bInfoBlock)
      traceEntityChanged(newGeneration, entityIndex);
    if (genomeModified && (infoGenome.size() < 20)) {
      if (infoOk)
        throw scError(
          scString("Genome lost info part (in mutation), genome count: ")+
          toString(workInfo->getGenomeCount())+
          scString(", current size: ")+toString(infoGenome.size())+
          scString(", current genome: ")+toString(j)
        );  
      else   
        throw scError(
          scString("Genome lost info part (somewhere), current size: ")+
          toString(workInfo->getGenomeCount())+
          scString(", current size: ")+toString(infoGenome.size())+
          scString(", current genome: ")+toString(j)          
        );  
    }  
#endif      
    if (!genome.empty())
      newGeneration.at(entityIndex).setGenome(j, genome);
  }  

  bool res = entityMutated;

#ifdef TRACE_ENTITY_BIO
  if (entityMutated)
    sgpEntityTracer::handleEntityChanged(entityIndex, checked_cast<sgpEntityForGasm &>(newGeneration.at(entityIndex)));
#endif

#ifdef USE_GASM_OPERATOR_STATS
  if (entityMutated)
    m_counters.setUIntDef("gx-mut-code-f-chg-entities", m_counters.getUInt("gx-mut-code-f-chg-entities", 0)+1);
#endif    
    
  if ((getFeatures() & gmfFadeMutTypeRatios) != 0)
    performFadeMutTypeProbs(newGeneration, entityIndex);
    
  invokeNextEntity();
  return res;    
}

// decrement mutation type probabilities
void sgpGasmOperatorMutate::performFadeMutTypeProbs(sgpGaGeneration &newGeneration,
  uint entityIndex)
{
  sgpEntityForGasm *workInfo = checked_cast<sgpEntityForGasm *>(newGeneration.atPtr(entityIndex));

  double baseFadeRatio;
  bool useFade;
    
  readFadeParams(workInfo, SGP_INFOPARAM_IDX_OTHER_PARAM_INFO_FADE_RATIO, baseFadeRatio, useFade);
  if (useFade) fadeInfoParams(workInfo, m_fadingParamSet, baseFadeRatio); 

  readFadeParams(workInfo, SGP_INFOPARAM_IDX_OTHER_PARAM_INFO_FADE_RATIO2, baseFadeRatio, useFade);
  if (useFade) fadeInfoParams(workInfo, m_fadingParamSet2, baseFadeRatio); 

  readFadeParams(workInfo, SGP_INFOPARAM_IDX_OTHER_PARAM_INFO_FADE_RATIO3, baseFadeRatio, useFade);
  if (useFade) fadeInfoParams(workInfo, m_fadingParamSet3, baseFadeRatio); 
}

void sgpGasmOperatorMutate::readFadeParams(sgpEntityForGasm *workInfo, uint paramIdx, double &baseFadeRatio, bool &useFade)
{  
  if (!workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_OTHER_PARAM_BASE + paramIdx, baseFadeRatio)) {
    baseFadeRatio = SGP_MUT_FADE_TYPE_PROB_RATIO;
    useFade = (baseFadeRatio <= SGP_MUT_FADE_MAX_USED_VALUE);
  } else {
  // value is 0..1
  // valid output range is 0,1..0,9  
  // if value is > max then no fading 
    useFade = (baseFadeRatio <= SGP_MUT_FADE_MAX_USED_VALUE);
    // rescale from 0,0..0,9 to 0,0..1,0 
    baseFadeRatio = baseFadeRatio / SGP_MUT_FADE_MAX_USED_VALUE;
    // rescale from 0,0..1,0 to 0,1..0,9
    baseFadeRatio = 0.1 + baseFadeRatio * 0.8;
  }
}  

void sgpGasmOperatorMutate::fadeInfoParams(sgpEntityForGasm *workInfo, const sgpGasmInfoParamSet &paramSet, 
  double baseFadeRatio)
{  
  uint infoIndex;
  double minValue, useProb;
  bool found;

  for(sgpGasmInfoParamSet::const_iterator it = paramSet.begin(), epos = paramSet.end(); it != epos; ++it)
  {
    infoIndex = *it;
    found  = workInfo->getInfoDouble(infoIndex, useProb);
    if (found) {
      useProb = useProb * baseFadeRatio;
      workInfo->getInfoDoubleMinNonZero(infoIndex, minValue);
      workInfo->setInfoDouble(infoIndex, std::max<double>(useProb, minValue));
    }  
  } 
}
  
void sgpGasmOperatorMutate::traceEntityChanged(const sgpGaGeneration &newGeneration, 
  uint itemIndex) const
{
  if (m_genomeChangedTracer)
    m_genomeChangedTracer->execute(newGeneration, itemIndex, "mutate");
}
  
void sgpGasmOperatorMutate::getBlockMeta(sgpGaGeneration &newGeneration, 
  uint entityIndex, uint genomeIndex, scDataNode &output)
{
  checked_cast_ref<sgpEntityForGasm &>(newGeneration.at(entityIndex)).getGenomeArgMeta(genomeIndex, output);  
}

uint sgpGasmOperatorMutate::getInputCountFromBlockMeta(const scDataNode &blockMeta)
{
  uint res;
  if (blockMeta.size()) {
    scDataNode element;
    blockMeta.getElement(0, element);
    res = element.size();
  } else {
    res = 0;
  }  
  return res;    
}

bool sgpGasmOperatorMutate::processGenome(sgpGaGenome &genome, int genomeCount, 
  double aProb, bool infoBlock, uint inputCount, sgpEntityForGasm *workInfo, uint genomeIndex, uint entityIndex, bool &mutPerformed)
{
  const double MACRO_CENTER_FILTER_MARGIN = 0.1;
  bool res = false;
  
  uint genomeSize;
  sgpGaGenomeMetaList *metaInfoPtr;
  std::auto_ptr<sgpGaGenomeMetaList> metaInfoGuard;
  bool metaReady = false;
  sgpGasmProbList genPosProbs;

  if (infoBlock) {
    genomeSize = m_infoBlockSize;      
    metaInfoPtr = &m_metaForInfoBlock;
  } else {  
    metaInfoGuard.reset(new sgpGaGenomeMetaList);
    calcMetaForCode(genome, workInfo, *metaInfoGuard, genomeSize);  
    metaInfoPtr = metaInfoGuard.get();
    prepareMutGenProbs(genPosProbs, workInfo, genomeSize);
    metaReady = true;
  }  
  
  // normalized probability 
  // - probability of change in whole organism should not be affected by number of genomes
  // - it depends on length of genome

  assert(infoBlock || (metaInfoPtr->size() == genome.size()));
  
  double normProb;
  uint multiProb;
  double partProb;
  
  if (!infoBlock) {
// in code probability depends only on number of instructions    
// it shouldn't depend on area factors

    double sizeFactor = SC_MAX(1.0, sqrt(static_cast<double>(genome.size())));
    
    if ((m_features & gmfMacrosEnabled) != 0) 
    {
      uint blockNo = genomeIndex - getCodeBlockOffset(workInfo);
      
      normProb = aProb * sizeFactor; 
      
      if (blockNo > 0) // if macro
      {
        double macroFilter;
        if (workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_OTHER_PARAM_BASE + SGP_MUT_OTHER_PARAM_IDX_MACRO_CHG_FILTER, macroFilter)) {
          if (macroFilter >= 0.5)
          {
            macroFilter = (macroFilter - 0.5) / 0.5 + 0.0001; 
            normProb *= macroFilter;
          }  
        }
      }      
      
      if ((m_features & gmfMaxTailMutRate) != 0)
      {
        uint blockCount = genomeCount-getCodeBlockOffset(workInfo);

        if (blockCount > 1) {
          double changeCenter;
          if (!workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_OTHER_PARAM_BASE + SGP_MUT_OTHER_PARAM_IDX_MACRO_CHG_CENTER, changeCenter)) {
            changeCenter = 1.0; // last macro
          }  
          double changeShape; 
          if (!workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_OTHER_PARAM_BASE + SGP_MUT_OTHER_PARAM_IDX_MACRO_CHG_DIST_SHAPE, changeShape)) {
            changeShape = 0.0; // flat
          }  
        
        // maximize prob near change center
          double tailFilter, x;
          // x: relative pos in macro list, 1.0 - last macro/block, 0.0 - first           
          x = static_cast<double>(blockNo) / static_cast<double>(blockCount - 1);
          // x: relative distance to changeCenter, 0 - max distance, 1 - zero distance
          x = 1.0 - abs(x - changeCenter);          
          // changeShape is fading, normal value is 0.0 - should be flat in this position
          tailFilter = ::centerDistribFormula(x, 1.0 - changeShape);
          // rescale => min = margin, max = 1.0
          tailFilter = MACRO_CENTER_FILTER_MARGIN + tailFilter * (1.0 - MACRO_CENTER_FILTER_MARGIN); 
          normProb *= tailFilter;
        }
      } 
      
    } else {
    // ADSs or other blocks of code
      normProb = (aProb / static_cast<double>(genomeCount)) * sizeFactor; 
    }  
  
    // handle multi-point mutation, if prob > 1.0 then it is multi-point mutation (2.4 => 3 points with prob 0.8)
    if (normProb <= 0.0)
       multiProb = 0;
    else
       multiProb = uint(normProb) + 1;
    
    if (multiProb > genomeSize)
      multiProb = genomeSize;  

    partProb = normProb / static_cast<double>(multiProb);

    if (partProb < SGP_MIN_MUT_PROB_VALUE)
      partProb = SGP_MIN_MUT_PROB_VALUE;

    // do not affect probability of mutation, only number of mutations
    multiProb *= getMultiPointRatio(workInfo);

    if ((m_features & gmfAdfsEnabled) != 0) {
      // ADF level protection - higher level - less mutations
      double blockDegrad;
      if (workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_OTHER_PARAM_BASE + SGP_MUT_OTHER_PARAM_IDX_BLOCK_DEGRAD, blockDegrad)) {
        partProb *= ::calcBlockDegradFactor(genomeIndex, blockDegrad);
      }        
    }
  } else { 
  // info block  
    partProb = aProb;
    multiProb = 1;
  }  
    
  uint mutPoint;
  uint mutVarPoint;
  uint curOffset;
  scDataNode element;
  sgpGasmProbList mutTypeProbs;
  bool infoBlockExists = workInfo->hasInfoBlock();
  
  mutPerformed = false;

  double totalCost = 0.0;   
  double mutCost; // how many instructions have been affected by mutation
  double mutCostTarget = static_cast<double>(multiProb); // how many should be affected
  double filterProb = 1.0;
  
  uint islandId;
  
  if ((m_experimentParams != SC_NULL) && m_islandTool->getIslandId(*workInfo, islandId))
  {  
    if (m_experimentParams->getDouble(islandId, SGP_EXP_PAR_BLOCK_IDX_MUT + SGP_MUT_EP_BASE_CHG_PROB, filterProb))
    {
      filterProb = filterProb * 2.0; // can increase or descrease ratio
    } 
  }

  if ((m_features & gmfControlMutRate) != 0) {
    filterProb = filterProb * m_changeRatio;
  }
   
  partProb *= filterProb;
  
  while (totalCost < mutCostTarget) {
    if (workInfo->getGenomeCount() <= genomeIndex)
      break;

    mutCost = 1.0;  
    
    if (randomFlip(partProb)) {    
      if (!metaReady && !infoBlock) {
        calcMetaForCode(genome, workInfo, *metaInfoGuard, genomeSize);  
        prepareMutGenProbs(genPosProbs, workInfo, genomeSize);
        metaReady = true;
      }  
    
      if (infoBlock)
        mutPoint = randomInt(0, genomeSize - 1);
      else
        mutPoint = ::selectProbItem(genPosProbs);
          
      curOffset = 0;
      res = true;
            
  #ifdef USE_GASM_OPERATOR_STATS
    m_counters.setUIntDef("gx-mut-a-tries", m_counters.getUInt("gx-mut-a-tries", 0)+1);
  #endif    
      
      mutVarPoint = 0;
      for(sgpGaGenomeMetaList::const_iterator it = metaInfoPtr->begin(), epos = metaInfoPtr->end(); it != epos; ++it)
      {
        if ((mutPoint < curOffset + it->genSize) && (it->genSize > 0))
        {
          if (infoBlock) {
            mutateGenomeInfo(genome, workInfo, mutVarPoint, mutPoint - curOffset);            
            mutPerformed = true;
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleEntityChangedInPoint(entityIndex, "info", genomeIndex, mutVarPoint);
#endif            
          } else { 
            if (infoBlockExists)
              readMutTypeProbs(workInfo, mutTypeProbs);
            if (mutateGenomeCode(*metaInfoPtr, genome, mutVarPoint, mutPoint - curOffset, inputCount, mutTypeProbs, 
              workInfo, genomeIndex, entityIndex, mutCost)) {                            
              mutPerformed = true;  
            }  
            metaReady = false;
          }  
          break;
        } else {
          curOffset += (it->genSize);
          mutVarPoint++;
        } // if not inside offset 
      } // for it 
    } // if p < normProb

    totalCost += mutCost;
  } // while totalCost
  
#ifdef USE_GASM_OPERATOR_STATS
  if (mutPerformed)
    m_counters.setUIntDef("gx-mut-code-e-chg-genomes", m_counters.getUInt("gx-mut-code-e-chg-genomes", 0)+1);
#endif    
  
  return res; 
} // function

// returns vector of probabilities of mutation positions, includes only position of existing bytes
void sgpGasmOperatorMutate::prepareMutGenProbs(sgpGasmProbList &output, const sgpEntityForGasm *workInfo, uint genomeSize)
{
  const double GEN_MUT_POS_PROB_MARGIN = 0.1;
  
  output.resize(genomeSize);
  
  if (genomeSize < 2)
  {
    output.resize(1);
    output[0] = 1.0;
    return;
  }  
  
  double changeCenter;
  if (!workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_OTHER_PARAM_BASE + SGP_MUT_OTHER_PARAM_IDX_GEN_CHG_CENTER, changeCenter)) {
    changeCenter = 1.0; // end of genome
  }  
  double changeShape; 
  if (!workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_OTHER_PARAM_BASE + SGP_MUT_OTHER_PARAM_IDX_GEN_CHG_DIST_SHAPE, changeShape)) {
    changeShape = 0.0; // flat
  }  
  
  // since shape is flat (standarized) when = 0.0, we need to rescale it
  changeShape = 1.0 - changeShape;
  
  double x, prob;
  double maxPos = static_cast<double>(genomeSize) - 1.0;
  for(uint i = 0, epos = output.size(); i != epos; i++)
  {
    // x: relative pos in genome, 0.0 - first pos, 1.0 - after last cell
    x = static_cast<double>(i) / maxPos;
    // x: relative distance to changeCenter, 0 - max distance, 1 - zero distance
    x = 1.0 - abs(x - changeCenter);          
    prob = ::centerDistribFormula(x, changeShape);
    // rescale => min = margin, max = 1.0
    prob = GEN_MUT_POS_PROB_MARGIN + prob * (1.0 - GEN_MUT_POS_PROB_MARGIN);         
    output[i] = prob;
  }
}

void sgpGasmOperatorMutate::readMutTypeProbs(const sgpEntityForGasm *workInfo, sgpGasmProbList &mutTypeProbs)
{
  uint idx = 0;
  bool found;
  double useProb;
  uint typeCount = getTypeProbsSize();
  
  mutTypeProbs.clear();
  mutTypeProbs.reserve(typeCount);
  
  do {
    found  = workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_TYPE_PROB_BASE+idx, useProb);
    if (found)
      mutTypeProbs.push_back(useProb);
    idx++;  
  } while ((idx < typeCount) && found);     
}

bool sgpGasmOperatorMutate::readMutInstrCodeProbs(const sgpEntityForGasm *workInfo, sgpGasmProbList &outProbs)
{
  uint idx = 0;
  bool found;
  double useProb;
  uint maxCnt = m_functions.size();

  outProbs.clear();
  outProbs.reserve(maxCnt);
  
  do {
    found  = workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_INSTR_PROBS_BASE + idx, useProb);
    if (found)
      outProbs.push_back(useProb);
    idx++;  
  } while ((idx < maxCnt) && found);     

  return (!outProbs.empty());
}

void sgpGasmOperatorMutate::calcMetaForInfo(sgpGaGenomeMetaList &info)
{
  sgpEntityForGasm::buildMetaForInfoBlock(info);
}  

uint sgpGasmOperatorMutate::countGenomeTypes(const sgpGaGenomeMetaList &info, uint userTypeMask, uint instrOffset) const
{
  uint res = 0;
  
  for(uint i = instrOffset, epos = info.size(); i != epos; i++)
  {
    if ((info[i].userType & userTypeMask) != 0)
      res++;
  }  
  return res;
}

void sgpGasmOperatorMutate::calcMetaForCode(const sgpGaGenome &genome, 
  const sgpEntityForGasm *workInfo,
  sgpGaGenomeMetaList &info, uint &genomeSize)
{
  sgpEntityForGasm::buildMetaForCode(genome, info);
  if (workInfo->hasInfoBlock()) {
    // recalculate gen size with area factors
    sgpGasmFactorList factors;
    calcGenSizeFactors(genome, workInfo, info, factors);
    if (!factors.empty()) {
      for(uint i=0,epos=info.size(); i != epos; ++i) {
        info[i].genSize = 1.0+(double(info[i].genSize) * factors[i]);
      }
    }
  }  
  genomeSize = 0;
  for(sgpGaGenomeMetaList::const_iterator it=info.begin(),epos= info.end(); it != epos; ++it)
    genomeSize += it->genSize;
}  

void sgpGasmOperatorMutate::calcMetaForCodeScan(const sgpGaGenome &genome, sgpGaGenomeMetaList &info)
{
  sgpEntityForGasm::buildMetaForCode(genome, info);
}  

// recalculate gen size with area factors
void sgpGasmOperatorMutate::calcGenSizeFactors(const sgpGaGenome &genome, 
  const sgpEntityForGasm *workInfo,
  const sgpGaGenomeMetaList &info,
  sgpGasmFactorList &factors)
{  
  double areaFactorInstr = 1.0;
  double areaFactorInput = 1.0;
  double areaFactorInout = 1.0;
  double areaFactorInConst = 1.0;
  double areaFactorInReg   = 1.0;
  double value;
  if (workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_CODE_AREA_BASE + SGP_MUT_AREA_IDX_INSTR, value))
    areaFactorInstr = value;    
  if (workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_CODE_AREA_BASE + SGP_MUT_AREA_IDX_INPUT, value))
    areaFactorInput = value;    
  if (workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_CODE_AREA_BASE + SGP_MUT_AREA_IDX_INOUT, value))
    areaFactorInout = value;    
  if (workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_CODE_AREA_BASE + SGP_MUT_AREA_IDX_INCONST, value))
    areaFactorInConst = value;    
  if (workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_CODE_AREA_BASE + SGP_MUT_AREA_IDX_INREG, value))
    areaFactorInReg = value;    

  double factorSum = areaFactorInstr + areaFactorInput + areaFactorInout;
  if (equDouble(factorSum, 0.0))
    factorSum = 1.0;

  areaFactorInstr = areaFactorInstr / factorSum * 100.0;
  areaFactorInput = areaFactorInput / factorSum * 100.0;
  areaFactorInout = areaFactorInout / factorSum * 100.0;
  
  factorSum = areaFactorInConst + areaFactorInReg;
  if (equDouble(factorSum, 0.0))
    factorSum = 1.0;
  
  areaFactorInConst = areaFactorInConst / factorSum; 
  areaFactorInReg = areaFactorInReg / factorSum; 
     
  uint instrOffset = 0;
  
  uint instrCode, instrCodeRaw, argCount, argCountInCode;
  scDataNode args;
  const scDataNode *argMetaPtr;
  uint ioMode;
  sgpFunction *functor; 
  uint argEndPos;
  uint endPos = info.size();
  double cellFactor;
  uint instrSize, inputSize, outputSize;
  double varFactor;
  
#ifdef OPER_DEBUG  
if (genome.size())
  ::debugTestInstruction(genome[0].getAsUInt());
#endif
  
  while(instrOffset < endPos) {
    if (info[instrOffset].userType == ggtInstrCode) {
      instrCode = genome[instrOffset].getAsUInt();
      sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCountInCode);
      functor = ::getFunctorForInstrCode(m_functions, instrCodeRaw);

      instrSize = argCountInCode + 1;
      cellFactor = double(instrSize); 
      factors.push_back(areaFactorInstr * cellFactor);      
      if (functor != SC_NULL)
        argMetaPtr = functor->getArgMeta();
      else  
        argMetaPtr = SC_NULL;
      
      if ((functor != SC_NULL) && (argMetaPtr != SC_NULL))
      {
        sgpGasmCodeProcessor::getArguments(genome, instrOffset+1, argCountInCode, args);
        
        argCount = SC_MIN(argCountInCode, args.size());
        argEndPos = SC_MIN(argCount, argMetaPtr->size());
        inputSize = outputSize = 0;

        for(uint argNo=0; argNo != argCountInCode; argNo++)
        {
          if (argNo < argEndPos)
            ioMode = sgpVMachine::getArgMetaParamUInt(*argMetaPtr, argNo, GASM_ARG_META_IO_MODE);    
          else 
            ioMode = 0; // incorrect arg
              
          if ((ioMode & gatfOutput) != 0) {
            outputSize++;
          } else {
            inputSize++;
          }  
        } // for
        
        for(uint argNo=0; argNo != argCountInCode; argNo++)
        {
          if (info[instrOffset+1+argNo].userType == ggtRegNo) {
            varFactor = areaFactorInReg;
          } else {
            varFactor = areaFactorInConst;
          }  

          if (argNo < argEndPos)
            ioMode = sgpVMachine::getArgMetaParamUInt(*argMetaPtr, argNo, GASM_ARG_META_IO_MODE);    
          else 
            ioMode = 0; // incorrect arg
              
          if ((ioMode & gatfOutput) != 0) {
            factors.push_back(areaFactorInout * cellFactor * 1.0 / double(outputSize));      
          } else {
          // 2.0 - do not make input-only cells weaker then other cells, make it in 50% of items stronger
            factors.push_back(
              ((varFactor + areaFactorInput) / 2.0) * cellFactor  / double(inputSize));      
          }  
        } // for
      } // if functor OK
      else {
        throw scError(scString("Unknown instruction: ")+toString(instrCodeRaw));
      }  
      instrOffset+=instrSize;
    } else {
    // not a instruction
      factors.push_back(1.0);      
      instrOffset++;
    }  
  } // while
#ifdef OPER_DEBUG  
  if (info.size() != factors.size())
#endif  
    assert(info.size() == factors.size());
}

void sgpGasmOperatorMutate::getArgMetaInfo(uint instrCode, uint argNo, uint &dataType, uint &ioMode)
{
  sgpFunction *functor;             
  scDataNode args;
  const scDataNode *argMeta;
       
  functor = ::getFunctorForInstrCode(m_functions, instrCode);

  // in case of errors - zero
  ioMode = 0;   
  dataType = 0;
   
  if ((functor != SC_NULL)) {   
    argMeta = functor->getArgMeta();
    if ((argMeta != SC_NULL) && (argNo < argMeta->size())) 
    {
      ioMode = sgpVMachine::getArgMetaParamUInt(*argMeta, argNo, GASM_ARG_META_IO_MODE);    
      dataType = sgpVMachine::getArgMetaParamUInt(*argMeta, argNo, GASM_ARG_META_DATA_TYPE);    
    }      
  }      
}

void sgpGasmOperatorMutate::mutateGenomeInfo(sgpGaGenome &genome, 
  const sgpEntityForGasm *workInfo,  
  int varIndex, uint offset)
{
  scDataNode val;
  int finalPos;
  
  finalPos = SGP_INFO_DATA_OFFSET+varIndex;
  val = genome[finalPos];  
  
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-c-chg-info", m_counters.getUInt("gx-mut-code-c-chg-info", 0)+1);
  int islandIdx = workInfo->getInfoValuePosInGenome(SGP_INFOBLOCK_IDX_OTHER_PARAM_BASE + SGP_INFOPARAM_IDX_OTHER_PARAM_ISLAND_ID);
  uint oldVal, newVal;
  oldVal = 0;
  if ((m_islandLimit > 0) && (finalPos == islandIdx)) 
    workInfo->getInfoUInt(SGP_INFOBLOCK_IDX_OTHER_PARAM_BASE + SGP_INFOPARAM_IDX_OTHER_PARAM_ISLAND_ID, oldVal);
#endif 
  
  mutateVar(
    m_metaForInfoBlock[varIndex],
    offset,
    val
  );  
  
#ifdef USE_GASM_OPERATOR_STATS
  if ((m_islandLimit > 0) && (finalPos == islandIdx)) 
  {
    workInfo->castValueAsUInt(val, newVal);
    oldVal = ::calcIslandId(oldVal, m_islandLimit);
    newVal = ::calcIslandId(newVal, m_islandLimit);
    if (oldVal != newVal)
      m_counters.setUIntDef("gp-island-mig-cnt-step", m_counters.getUInt("gp-island-mig-cnt-step", 0)+1);
  }    
#endif 
  
  genome[finalPos] = val;
}

inline void sgpGasmOperatorMutate::mutateVar(const sgpGaGenomeMetaInfo &metaInfo, uint offset, scDataNode &var)
{
  switch (metaInfo.genType) {
    case gagtConst:
      break;
    case gagtRanged: {
      sgpGaOperatorInit::getRandomRanged(var, metaInfo.minValue, metaInfo.maxValue, var);
      break;
    }  
    case gagtAlphaString: {
      scString value = var.getAsString();
      scString newChar;
      if (offset > value.length())
        throw scError("Invalid string offset: "+toString(offset));
      randomString(metaInfo.minValue.getAsString(), 1, newChar);  
      value[offset] = newChar[0];      
      var.setAsString(value);
      break;
    }  
    default:
      throw scError("Unknown var type for mutate: "+toString(metaInfo.genType));
  }
}

sgpGasmMutType sgpGasmOperatorMutate::selectRandomMutateMethodUniform(
  uint varType, uint ioMode, uint instrOffset) 
{
  sgpGasmMutType mutType;
  uint mutMask;
  uint bitCount;
  int mutSelectedNo;
  uint mutCode;
  double mutMethodProb = randomDouble(0.0, 1.0); 
  
  if (mutMethodProb < SGP_GASM_MUT_PROB_INS_DEL) {
    
    mutMask = 
      gmtDelete+
      gmtInsert+
      gmtReplace;      
    bitCount = 3;      
  } else {
    switch (varType) {
      case ggtValue:
        // simple value mutation is most important mutation - give it higher prob 
        if (randomFlip(SGP_GASM_MUT_PROB_VALUE_VALUE)) {
          mutMask = 
            gmtValue;
          bitCount = 1;
        } else {
          mutMask = 
//            gmtValue+
            gmtValueReg+
            gmtValueTypeUp+
            gmtValueTypeDown+
            gmtSwapArgs+
            gmtValueNeg;
//          bitCount = 6;  
          bitCount = 5;  
        }
        break;  
      case ggtInstrCode:
        mutMask = 
          gmtInstrCode
          +gmtJoinInstr
          +gmtSplitInstr
          ;
        bitCount = 3;
        break;
      case ggtRegNo:
        if ((ioMode & gatfOutput) != 0) {
        // output
          mutMask = 
            gmtRegNo+
            gmtRegNoZero;
          bitCount = 2;  
        } else {
        // input  
          mutMask = 
            gmtRegValue+
            gmtRegNo+
            gmtSwapArgs;
          bitCount = 3;  
        }  
        break;
      default:
        throw scError("Unknown genome type: "+toString(varType));
    } // switch   
  } // if
          
  // find nth mutation code where n = mutSelectedNo 
  mutSelectedNo = randomInt(1, bitCount);
  mutCode = 1;
  while (mutSelectedNo > 0) {
    if ((mutMask & mutCode) != 0) {
      mutSelectedNo--;
      if (mutSelectedNo == 0)
        break;
    }  
    mutCode = mutCode << 1;
  }
  mutType = sgpGasmMutType(mutCode);
  
  return mutType;
}

sgpGasmMutType sgpGasmOperatorMutate::selectRandomMutateMethodWithProbs(uint varType, uint ioMode, uint instrOffset)
{
  sgpGasmMutType mutType;
  sgpGasmProbList workProbs;
  uint idx;
  
  workProbs = m_typeProbSectionGlobal;
  idx = ::selectProbItem(workProbs);
  if (m_typeListGlobal[idx] != gmtUndef) {
    mutType = m_typeListGlobal[idx];
    if (mutType == gmtBlock) {
      if ((m_features & gmfMacrosEnabled) != 0) {
        workProbs = m_typeProbSectionMacro;
#ifndef OPT_MACRO_DEL_ENABLED 
        workProbs[SGP_GASM_MUT_PROB_IDX_MACRO_DEL] = 0.0;
#endif          
        idx = ::selectProbItem(workProbs);
        mutType = m_typeListMacro[idx];
      }  
    }
  }  
  else 
    {
    switch (varType) {
      case ggtValue: {
        idx = ::selectProbItem(m_typeProbSectionValue);
        mutType = m_typeListValue[idx];
        break;
      }
      case ggtInstrCode: {
        workProbs = m_typeProbSectionInstr;
        if (instrOffset == 0) 
          workProbs[SGP_GASM_MUT_PROB_IDX_INSTR_SPLIT_INSTR] = 0.0;
        idx = ::selectProbItem(workProbs);  
        mutType = m_typeListInstr[idx];
        break;
      }  
      case ggtRegNo: {
        workProbs = m_typeProbSectionRegNo;      
        if ((ioMode & gatfOutput) != 0) {
          workProbs[SGP_GASM_MUT_PROB_IDX_REGNO_REG_VALUE] = 0.0;
          workProbs[SGP_GASM_MUT_PROB_IDX_REGNO_SWAP_ARGS] = 0.0;
        } else {
          workProbs[SGP_GASM_MUT_PROB_IDX_REGNO_REGNO_ZERO] = 0.0;
        }
        idx = ::selectProbItem(workProbs);  
        mutType = m_typeListRegNo[idx];
        break;
      }  
      default:
        throw scError("Unknown genome type: "+toString(varType));
      } // switch varType  
    } // undef
  
  assert(mutType != gmtUndef);
  return mutType;
}

void sgpGasmOperatorMutate::copyProbs(sgpGasmProbList &output, const sgpGasmProbList &input, uint startOffset, uint blockSize)
{
  output.clear();
  output.reserve(blockSize);
  for(uint i=startOffset,epos=startOffset+blockSize; i != epos; i++)
    output.push_back(input[i]);
}

sgpGasmMutType sgpGasmOperatorMutate::selectRandomMutateMethodWithProbsFromInfo(
  const sgpGasmProbList &probList, uint varType, uint ioMode, uint instrOffset, uint blockCount, 
  const sgpEntityForGasm *workInfo,
  bool mainBlock, uint blockSize)
{
  sgpGasmMutType mutType;
  sgpGasmProbList workProbs;
  uint idx;

  bool growDisabled = ((m_blockSizeLimitStep > 0) && (blockSize >= m_blockSizeLimitStep));
  // use step limit only in some random cases, global limit - always 
  if (growDisabled) {
    growDisabled = growDisabled && randomFlip(SGP_MUT_SIZE_LIMIT_APPLY_PROB);
    if (!growDisabled)
    // always limit by global max   
      growDisabled = ((m_blockSizeLimit > 0) && (blockSize >= m_blockSizeLimit));      
  }  
  
  copyProbs(workProbs, probList, SGP_GASM_MUT_PROB_SECTION_GLOBAL_INFO_OFFSET, SGP_GASM_MUT_PROB_SECTION_GLOBAL_SIZE);
  
  if (
       growDisabled
     )
    workProbs[SGP_GASM_MUT_PROB_IDX_GLOBAL_INSERT] = 0.0;

  if (
       ((m_features & gmfMacrosEnabled) == 0) 
       &&
       (m_maxBlockCount > 0) 
       && 
       (blockCount >= m_maxBlockCount)
     )  
  {   
    workProbs[SGP_GASM_MUT_PROB_IDX_GLOBAL_BLOCK] = 0.0;
  }
  
  idx = ::selectProbItem(workProbs);

  double globalMutRatio;
  if (!workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_OTHER_PARAM_BASE + SGP_MUT_OTHER_PARAM_IDX_GLOBAL_RATIO, globalMutRatio))
    globalMutRatio = m_globalMutRatio;    
  else { 
    double mutTypeFactor; 

    if ((m_features & gmfMacrosEnabled) != 0) 
      mutTypeFactor = static_cast<double>(SGP_GASM_MUT_PROB_SECTION_GLOBAL_SIZE + SGP_GASM_MUT_PROB_SECTION_MACRO_SIZE);
    else
      mutTypeFactor = static_cast<double>(SGP_GASM_MUT_PROB_SECTION_GLOBAL_SIZE);
      
    mutTypeFactor = mutTypeFactor / static_cast<double>(SGP_GASM_MUT_PROB_ALL_SECTION_SIZE);
    double outFactor = 0.5 + (1.5 * globalMutRatio);  
    globalMutRatio = outFactor * mutTypeFactor;
  }  
  
  double noMutRatio;
  if (!workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_OTHER_PARAM_BASE + SGP_MUT_OTHER_PARAM_IDX_NOMUT_RATIO, noMutRatio))
    noMutRatio = 0.0;    
  else  
    noMutRatio *= SGP_MUT_NOMUT_RATIO_INFO_FILTER;
  
  if (randomFlip(noMutRatio)) {
#ifdef USE_GASM_OPERATOR_STATS
    m_counters.setUIntDef("gx-mut-place-none", m_counters.getUInt("gx-mut-place-none", 0)+1);
#endif   
    mutType = gmtNoChange; 
  } else if (randomFlip(globalMutRatio) && (m_typeListGlobal[idx] != gmtUndef)) {
    mutType = m_typeListGlobal[idx];

    if (mutType == gmtBlock) {
      if ((m_features & gmfMacrosEnabled) != 0) {
        copyProbs(workProbs, probList, SGP_GASM_MUT_PROB_SECTION_MACRO_INFO_OFFSET, SGP_GASM_MUT_PROB_SECTION_MACRO_SIZE);

        if ((m_features & gmfMacroDelEnabled) == 0) {
          workProbs[SGP_GASM_MUT_PROB_IDX_MACRO_DEL] = 0.0;
        }
        
        if (mainBlock) {        
          workProbs[SGP_GASM_MUT_PROB_IDX_MACRO_DEL] = 0.0;
          workProbs[SGP_GASM_MUT_PROB_IDX_MACRO_ARG_ADD] = 0.0;
          workProbs[SGP_GASM_MUT_PROB_IDX_MACRO_ARG_DEL] = 0.0;
        } 
        
        if ((blockCount < 2) || growDisabled) {
          workProbs[SGP_GASM_MUT_PROB_IDX_MACRO_INS] = 0.0;
        } 
        
        // lower probability of macro generation for genomes close to limit
        if (m_maxBlockCount > 0) 
        {
          double filter = 
            static_cast<double>(std::min<uint>(blockCount, m_maxBlockCount)) /
            static_cast<double>(m_maxBlockCount);
          filter = 1.0 - filter;   
          workProbs[SGP_GASM_MUT_PROB_IDX_MACRO_GEN] *= filter;
        }
        
        idx = ::selectProbItem(workProbs);
        mutType = m_typeListMacro[idx];
      }  
    }
    
#ifdef USE_GASM_OPERATOR_STATS
    m_counters.setUIntDef("gx-mut-place-global", m_counters.getUInt("gx-mut-place-global", 0)+1);
#endif    
  }  
  else 
    {
    switch (varType) {
      case ggtValue: {
#ifdef USE_GASM_OPERATOR_STATS
    m_counters.setUIntDef("gx-mut-place-value", m_counters.getUInt("gx-mut-place-value", 0)+1);
#endif    
        copyProbs(workProbs, probList, SGP_GASM_MUT_PROB_SECTION_VALUE_INFO_OFFSET, SGP_GASM_MUT_PROB_SECTION_VALUE_SIZE);
        idx = ::selectProbItem(workProbs);
        mutType = m_typeListValue[idx];
        break;
      }
      case ggtInstrCode: {
#ifdef USE_GASM_OPERATOR_STATS
    m_counters.setUIntDef("gx-mut-place-instr", m_counters.getUInt("gx-mut-place-instr", 0)+1);
#endif    
        copyProbs(workProbs, probList, SGP_GASM_MUT_PROB_SECTION_INSTR_INFO_OFFSET, SGP_GASM_MUT_PROB_SECTION_INSTR_SIZE);
        if ((instrOffset == 0) || growDisabled)
          workProbs[SGP_GASM_MUT_PROB_IDX_INSTR_SPLIT_INSTR] = 0.0;
        idx = ::selectProbItem(workProbs);  
        mutType = m_typeListInstr[idx];
        break;
      }  
      case ggtRegNo: {
#ifdef USE_GASM_OPERATOR_STATS
    m_counters.setUIntDef("gx-mut-place-reg", m_counters.getUInt("gx-mut-place-reg", 0)+1);
#endif    
        copyProbs(workProbs, probList, SGP_GASM_MUT_PROB_SECTION_REGNO_INFO_OFFSET, SGP_GASM_MUT_PROB_SECTION_REGNO_SIZE);
        if ((ioMode & gatfOutput) != 0) {
          workProbs[SGP_GASM_MUT_PROB_IDX_REGNO_REG_VALUE] = 0.0;
          workProbs[SGP_GASM_MUT_PROB_IDX_REGNO_SWAP_ARGS] = 0.0;
        } else {
          workProbs[SGP_GASM_MUT_PROB_IDX_REGNO_REGNO_ZERO] = 0.0;
        }
        idx = ::selectProbItem(workProbs);  
        mutType = m_typeListRegNo[idx];
        break;
      }  
      default:
        throw scError("Unknown genome type: "+toString(varType));
      } // switch varType  
    } // undef
  
  assert(mutType != gmtUndef);
  return mutType;
}

  
bool sgpGasmOperatorMutate::mutateGenomeCode(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  int varIndex, uint offset, uint inputCount, const sgpGasmProbList &mutTypeProbs, sgpEntityForGasm *workInfo, 
  uint genomeIndex, uint entityIndex,   
  double &mutCost)
{
  uint instrOffset = findInstrBeginBackward(info, varIndex);
     
  uint instrCode, instrCodeRaw, argCount;

#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-a-chgs", m_counters.getUInt("gx-mut-code-a-chgs", 0)+1);
#endif    
  mutCost = 0.1;
  instrCode = genome[instrOffset].getAsUInt();
  sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
  sgpFunction *functor =    
    ::getFunctorForInstrCode(m_functions, instrCodeRaw);
  
  if (functor == SC_NULL)
    return false;
      
  scDataNode argMeta;
  uint ioMode;
    
  if (!functor->getArgMeta(argMeta)) 
    return false;      
  
  uint argNo;
  if (uint(varIndex) > instrOffset) { 
     argNo = varIndex - instrOffset - 1; 
    if (argNo >= argMeta.size()) 
      ioMode = 0;
    else  
      ioMode = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_IO_MODE);
  } else {
    argNo = 0;
    ioMode = 0;
  }    
    
  sgpGasmMutType mutType;
  if (m_typeProbs.size() != 0) 
    mutType = selectRandomMutateMethodWithProbs(info[varIndex].userType, ioMode, instrOffset);
  else {
    if (mutTypeProbs.size() > 0) {
      uint codeBlockIndex = genomeIndex - getCodeBlockOffset(workInfo);
      uint codeBlockCount = workInfo->getGenomeCount() - getCodeBlockOffset(workInfo);
      mutType = selectRandomMutateMethodWithProbsFromInfo(mutTypeProbs, info[varIndex].userType, ioMode, instrOffset, codeBlockCount, workInfo, 
        (codeBlockIndex == 0), genome.size()
      );
    } else { 
      mutType = selectRandomMutateMethodUniform(info[varIndex].userType, ioMode, instrOffset);  
    }  
  }    
  return doMutateGenomeCode(info, genome, varIndex, offset, inputCount, mutType, instrOffset, ioMode, argMeta, argNo, workInfo, 
    genomeIndex, entityIndex,
    mutCost);
}

bool sgpGasmOperatorMutate::doMutateGenomeCode(const sgpGaGenomeMetaList &info, 
  sgpGaGenome &genome, int varIndex, uint offset, uint inputCount, 
  sgpGasmMutType mutType, uint instrOffset, uint ioMode, scDataNode &argMeta, uint argNo, sgpEntityForGasm *workInfo, 
  uint genomeIndex, uint entityIndex,
  double &mutCost)
{
  bool changed = false;
#ifdef KEEP_OLD_GENOMES
  sgpGaGenome genomeBefore = genome;
#endif

#ifdef TRACE_ENTITY_BIO
  scString mutName("???");
#endif  

#ifdef OPT_VALIDATE_MAIN_SIZE
sgpGasmCodeProcessor::checkMainNotEmpty(workInfo);
#endif    

#ifdef USE_GASM_OPERATOR_STATS
  if (mutType != gmtNoChange)
    m_counters.setUIntDef("gx-mut-code-b-chgs", m_counters.getUInt("gx-mut-code-b-chgs", 0)+1);
#endif    
  mutCost = 1.0;
  switch (mutType) {
    case gmtValue:
      changed = mutateGenomeCodeValue(info, genome, varIndex, offset, workInfo);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-value";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
    m_counters.setUIntDef("gx-mut-code-value", m_counters.getUInt("gx-mut-code-value", 0)+1);
    m_counters.setUIntDef("gx-mut-code-c-arg", m_counters.getUInt("gx-mut-code-c-arg", 0)+1);
#ifdef VALIDATE_DUPS  
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-value");
#endif      
#endif    
      break;
    case gmtValueReg: 
      changed = mutateGenomeCodeValueReg(info, genome, varIndex, offset, instrOffset, inputCount, ioMode, argMeta);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-value-reg";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
    m_counters.setUIntDef("gx-mut-code-value-reg", m_counters.getUInt("gx-mut-code-value-reg", 0)+1);
    m_counters.setUIntDef("gx-mut-code-c-arg", m_counters.getUInt("gx-mut-code-c-arg", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-value-reg");
#endif      
#endif    
      break;
    case gmtValueTypeUp:
      changed = mutateGenomeCodeTypeUp(info, genome, instrOffset, varIndex, offset);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-type-up";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-type-up", m_counters.getUInt("gx-mut-code-type-up", 0)+1);
  m_counters.setUIntDef("gx-mut-code-c-arg", m_counters.getUInt("gx-mut-code-c-arg", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-type-up");
#endif
#endif    
      break;
    case gmtValueTypeDown:
      changed = mutateGenomeCodeTypeDown(info, genome, instrOffset, varIndex, offset);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-type-down";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-type-down", m_counters.getUInt("gx-mut-code-type-down", 0)+1);
  m_counters.setUIntDef("gx-mut-code-c-arg", m_counters.getUInt("gx-mut-code-c-arg", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-type-down");
#endif      
#endif    
      break;
    case gmtValueNeg:
      changed = mutateGenomeCodeValueNeg(info, genome, varIndex, offset);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-value-neg";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-value-neg", m_counters.getUInt("gx-mut-code-value-neg", 0)+1);
  m_counters.setUIntDef("gx-mut-code-c-arg", m_counters.getUInt("gx-mut-code-c-arg", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-value-neg");
#endif      
#endif    
      break;
    case gmtSwapArgs:  
      changed = mutateGenomeCodeSwapArgs(info, genome, varIndex, argMeta, argNo, ioMode);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-swap-args";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-swap-args", m_counters.getUInt("gx-mut-code-swap-args", 0)+1);
  m_counters.setUIntDef("gx-mut-code-c-arg", m_counters.getUInt("gx-mut-code-c-arg", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-swap-args");
#endif      
#endif    
      break;
    case gmtRegValue:     
      changed = mutateGenomeCodeRegValue(info, genome, instrOffset, varIndex, offset, argMeta);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-reg-value";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-reg-value", m_counters.getUInt("gx-mut-code-reg-value", 0)+1);
  m_counters.setUIntDef("gx-mut-code-c-arg", m_counters.getUInt("gx-mut-code-c-arg", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-reg-value");
#endif      
#endif    
      break;
    case gmtRegNo: 
      changed = mutateGenomeCodeRegNo(info, genome, varIndex, offset, instrOffset, inputCount, ioMode);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-reg-no";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-reg-no", m_counters.getUInt("gx-mut-code-reg-no", 0)+1);
  m_counters.setUIntDef("gx-mut-code-c-arg", m_counters.getUInt("gx-mut-code-c-arg", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-reg-no");
#endif      
#endif    
      break;    
    case gmtRegNoZero: 
      changed = mutateGenomeCodeRegNoZero(info, genome, varIndex, offset, instrOffset, inputCount, ioMode);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-reg-no-zero";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-reg-no-zero", m_counters.getUInt("gx-mut-code-reg-no-zero", 0)+1);
  m_counters.setUIntDef("gx-mut-code-c-arg", m_counters.getUInt("gx-mut-code-c-arg", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-reg-no-zero");
#endif      
#endif    
      break;    
    case gmtInstrCode: 
      changed = mutateGenomeCodeInstrCode(info, genome, varIndex, offset, workInfo);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-instr-code";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-instr-code", m_counters.getUInt("gx-mut-code-instr-code", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-instr-code");
#endif      
#endif    
      break;
    case gmtInsert:
      changed = mutateGenomeCodeInsert(info, genome, instrOffset, varIndex, offset, inputCount, workInfo, genomeIndex);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-insert";      
#endif
      if (changed) { 
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-instr-insert-done", m_counters.getUInt("gx-mut-code-instr-insert-done", 0)+1);
#endif      
      }
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-instr-insert", m_counters.getUInt("gx-mut-code-instr-insert", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-instr-insert");
#endif      
#endif    
      break;
    case gmtDelete:
      changed = mutateGenomeCodeDelete(info, genome, instrOffset, varIndex, offset);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-delete";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-instr-delete", m_counters.getUInt("gx-mut-code-instr-delete", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-instr-delete");
#endif      
#endif    
      break;
    case gmtReplace:
      changed = mutateGenomeCodeReplace(info, genome, instrOffset, varIndex, offset, inputCount, workInfo, genomeIndex);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-replace";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-instr-replace", m_counters.getUInt("gx-mut-code-instr-replace", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-instr-replace");
#endif      
#endif    
      break;
    case gmtSwapInstr: 
      changed = mutateGenomeCodeSwapInstr(info, genome, varIndex, offset, instrOffset);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-swap-instr";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-instr-swap", m_counters.getUInt("gx-mut-code-instr-swap", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-instr-swap");
#endif      
#endif    
      break;
    case gmtJoinInstr: 
      changed = mutateGenomeCodeJoinInstr(info, genome, varIndex, offset, instrOffset);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-instr-join";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-instr-join", m_counters.getUInt("gx-mut-code-instr-join", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-instr-join");
#endif      
#endif    
      break;
    case gmtSplitInstr: 
      changed = mutateGenomeCodeSplitInstr(info, genome, varIndex, offset, instrOffset, inputCount, argMeta, workInfo, genomeIndex);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-instr-split";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-instr-split", m_counters.getUInt("gx-mut-code-instr-split", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-instr-split");
#endif      
#endif    
      break;
    case gmtLinkInstr: 
      changed = mutateGenomeCodeLinkInstr(info, genome, varIndex, instrOffset, argMeta, workInfo);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-instr-link";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-instr-link", m_counters.getUInt("gx-mut-code-instr-link", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-instr-link");
#endif      
#endif    
      break;
    case gmtBlock:
      changed = true;
      mutateGenomeCodeGenBlock(info, genome, varIndex, offset, inputCount, workInfo, genomeIndex, mutCost);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-block-gen";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-instr-gen-block", m_counters.getUInt("gx-mut-code-instr-gen-block", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-instr-gen-block");
#endif      
#endif    
      break;
    case gmtMacroIns:
      changed = mutateGenomeCodeMacroInsert(info, genome, instrOffset, inputCount, workInfo, genomeIndex, mutCost);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-mac-ins";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-mac-ins", m_counters.getUInt("gx-mut-code-mac-ins", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-mac-ins");
#endif      
#endif    
      break;
    case gmtMacroDel:
      changed = mutateGenomeCodeMacroDelete(info, genome, workInfo, genomeIndex, mutCost);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-mac-del";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-mac-del", m_counters.getUInt("gx-mut-code-mac-del", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-mac-del");
#endif      
#endif    
      break;
    case gmtMacroGen:
      changed = mutateGenomeCodeMacroGenerate(info, genome, instrOffset, inputCount, genomeIndex, workInfo, mutCost);
#ifdef TRACE_ENTITY_BIO
      mutName = "mut-code-mac-gen";      
#endif
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-mac-gen", m_counters.getUInt("gx-mut-code-mac-gen", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-mac-gen");
#endif      
#endif    
      break;
    case gmtMacroArgAdd:
    case gmtMacroArgDel:
    case gmtNoChange:
      // do nothing
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-a-no-change", m_counters.getUInt("gx-mut-code-a-no-change", 0)+1);
#ifdef VALIDATE_DUPS    
      monitorDupsFromOperator(genomeBefore, genome, "mut-code-no-chg");
#endif      
#endif    
      break;
    default:
      throw scError("Unknown mutation code: "+toString(mutType));   
  }
#ifdef USE_GASM_OPERATOR_STATS
  if (changed && (mutType != gmtNoChange))
    m_counters.setUIntDef("gx-mut-code-c-chg-code", m_counters.getUInt("gx-mut-code-c-chg-code", 0)+1);
#endif    
#ifdef TRACE_ENTITY_BIO
  if (changed) {
    sgpEntityTracer::handleEntityChangedInPoint(entityIndex, mutName, genomeIndex, varIndex);
  }  
#endif        
#ifdef OPT_VALIDATE_MAIN_SIZE
sgpGasmCodeProcessor::checkMainNotEmpty(workInfo);
#endif    
  return changed;
}

bool sgpGasmOperatorMutate::mutateGenomeCodeInstrCode(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  int varIndex, uint offset, const sgpEntityForGasm *workInfo)
{      
  bool res = false;
  uint instrCode = 
    genome[varIndex].getAsUInt();
  uint instrCodeRaw, argCount, instrCodeLimit;  
  long64 calcCode;
  sgpGasmProbList instrCodeProbs;  
    
  sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);

  sgpFunction *func = getFunctorForInstrCode(m_functions, instrCodeRaw);
  
  // check if old is "protected" instruction - if yes, do not change instruction code
  if (func != SC_NULL) 
  {
    if (func->hasDynamicArgs())
      return false;
  }      
  
  instrCodeLimit = m_functions.size();    
  calcCode = instrCodeRaw;

  if (readMutInstrCodeProbs(workInfo, instrCodeProbs)) {
    instrCodeRaw = ::selectProbItem(instrCodeProbs);
  } else {
    calcCode += 
        (long64(instrCodeLimit) + randomInt(m_instrCodeChangeMin, m_instrCodeChangeMax));
    instrCodeRaw = uint(calcCode);
  }
      
  //instrCodeRaw += instrCodeLimit;
  instrCodeRaw = instrCodeRaw % instrCodeLimit;
  
  func = getFunctorForInstrCode(m_functions, instrCodeRaw);
  
  // check if new is "protected" instruction - if yes, do not change instruction code
  if (func != SC_NULL) 
  {
    if (func->hasDynamicArgs())
      return false;
  }      
  
  if (func != SC_NULL) {      
    scDataNode args;
    
    sgpGasmCodeProcessor::getArguments(genome, varIndex+1, argCount, args);
    if (m_machine->verifyArgs(args, func)) {
      instrCode = sgpVMachine::encodeInstr(instrCodeRaw, argCount);  
      genome[varIndex].setAsUInt(instrCode);
      res = true;
    }  
  }        
  return res;
}

// returns <true> if code modified
bool sgpGasmOperatorMutate::mutateGenomeCodeValueReg(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  int varIndex, uint offset, uint instrOffset, uint blockInputCount, uint ioMode, scDataNode &argMeta)
{
  bool res = false;

  uint argNo = varIndex - instrOffset - 1;   
  uint argTypes;
  
  if (argNo < argMeta.size())
    argTypes = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_ARG_TYPE);    
  else  
    argTypes = 0;
  
  if ((argTypes & gatfRegister) == 0)
    return res;
  
  uint instrCode = 
    genome[instrOffset].getAsUInt();
  uint instrCodeRaw, argCount, dataTypes, ioModeForArg;  
    
  sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
  getArgMetaInfo(instrCodeRaw, argNo, dataTypes, ioModeForArg);
  sgpGasmRegSet regSet;

  sgpGasmCodeProcessor::prepareRegisterSet(regSet, m_supportedDataTypes & dataTypes, ioModeForArg, m_machine);
  
  if ((ioMode & gatfOutput) != 0) {
  // output  
    genome[varIndex].copyFrom(sgpGasmCodeProcessor::randomRegNoAsValue(regSet));
    res = true;
  } else {
  // input
    sgpGasmRegSet writtenRegs, workRegs;
    findWrittenRegs(info, genome, instrOffset, blockInputCount, writtenRegs);

    // join writtenRegs with regSet
    for(sgpGasmRegSet::const_iterator it = writtenRegs.begin(), epos = writtenRegs.end(); it != epos; ++it)
    {
      if (regSet.find(*it) != regSet.end())
        workRegs.insert(*it);
    }
    
    if (!workRegs.empty()) {
      genome[varIndex].copyFrom(sgpGasmCodeProcessor::randomRegNoAsValue(workRegs));
      res = true;
    }  
  }
  return res;
}

bool sgpGasmOperatorMutate::mutateGenomeCodeRegNo(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  int varIndex, uint offset, uint instrOffset, uint blockInputCount, uint ioMode)
{
  bool res = false;
  uint instrCode = 
    genome[instrOffset].getAsUInt();
  uint instrCodeRaw, argCount, dataTypes, ioModeForArg;  
    
  sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
  getArgMetaInfo(instrCodeRaw, varIndex - instrOffset - 1, dataTypes, ioModeForArg);
  sgpGasmRegSet regSet;

  sgpGasmCodeProcessor::prepareRegisterSet(regSet, m_supportedDataTypes & dataTypes, ioModeForArg, m_machine);

  //uint regNo = genome[varIndex].getAsUInt();
  
  if ((ioMode & gatfOutput) != 0) {
  // output
    uint regNoRaw = sgpVMachine::getRegisterNo(genome[varIndex]);
   
    bool changeAllowed = true;

    // protect last write to block output
    if ((gmfProtectBlockOutput & m_features) != 0) {
      if (regNoRaw == SGP_REGNO_OUTPUT) {
        changeAllowed = (getRegChangeCount(info, genome, regNoRaw) > 1);
      }
    }

    if (changeAllowed) {
      genome[varIndex].copyFrom(sgpGasmCodeProcessor::randomRegNoAsValue(regSet));
      res = true;
    }
  } else {
  // input 
    sgpGasmRegSet writtenRegs, workRegs;
    findWrittenRegs(info, genome, instrOffset, blockInputCount, writtenRegs);
    
    // join writtenRegs with regSet
    for(sgpGasmRegSet::const_iterator it = writtenRegs.begin(), epos = writtenRegs.end(); it != epos; ++it)
    {
      if (regSet.find(*it) != regSet.end())
        workRegs.insert(*it);
    }
    
    if (!workRegs.empty()) {
      genome[varIndex].copyFrom(sgpGasmCodeProcessor::randomRegNoAsValue(workRegs));
      res = true;
    }  
  }
  return res;
}

uint sgpGasmOperatorMutate::getRegChangeCount(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, uint regNo)
{
  sgpGasmOutRegCellMap outMap;
  findOutRegCellMap(info, genome, genome.size(), outMap);
  return outMap.count(regNo); 
}

bool sgpGasmOperatorMutate::mutateGenomeCodeRegNoZero(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  int varIndex, uint offset, uint instrOffset, uint blockInputCount, uint ioMode)
{
  bool res = false;
  
  if ((ioMode & gatfOutput) != 0) {
  // output
    scDataNode regNoNode;
    sgpVMachine::buildRegisterArg(regNoNode, SGP_REGB_OUTPUT);
    
    genome[varIndex].copyFrom(regNoNode);    
    res = true;
  } 
  return res;
}

void sgpGasmOperatorMutate::findWrittenRegs(const sgpGaGenomeMetaList &info, 
  sgpGaGenome &genome, uint endPos, uint blockInputCount, sgpGasmRegSet &writtenRegs)
{
  uint instrOffset = 0;
  
  writtenRegs.clear(); 
  if (blockInputCount > 0)
    for(uint i=SGP_REGB_INPUT,epos=SGP_REGB_INPUT+blockInputCount;i!=epos;i++)
      writtenRegs.insert(i);

  uint instrCode, instrCodeRaw, argCount, skipSize;
  scDataNode argMeta, args;
  uint ioMode, regNo;
  sgpFunction *functor; 
  
  while(instrOffset < endPos) {
    if (info[instrOffset].userType == ggtInstrCode) {
      instrCode = genome[instrOffset].getAsUInt();
      sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
      functor = ::getFunctorForInstrCode(m_functions, instrCodeRaw);
      sgpGasmCodeProcessor::getArguments(genome, instrOffset+1, argCount, args);
      argCount = SC_MIN(argCount, args.size());
      skipSize = argCount + 1;
      
      if ((functor != SC_NULL) && functor->getArgMeta(argMeta)) {
        for(uint argNo=0,epos=SC_MIN(argCount, argMeta.size()); argNo != epos; argNo++)
        {
          ioMode = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_IO_MODE);    
          if ((ioMode & gatfOutput) != 0) {
            if (info[instrOffset+1+argNo].userType == ggtRegNo) {
              regNo = sgpVMachine::getRegisterNo(args[argNo]);
              writtenRegs.insert(regNo);
            } // if reg#
          } // if output 
        } // for
      } // if functor OK
    } else {
      skipSize = 1;
    }
    instrOffset += skipSize;
  } // while
}  

uint sgpGasmOperatorMutate::getInstrOutputDataTypeSet(const scDataNode &argMeta)
{
  uint res = 0;
  uint ioMode, dataType;

  for(uint argNo = 0, epos = argMeta.size(); argNo != epos; argNo++)
  {
    ioMode = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_IO_MODE);    
    if ((ioMode & gatfOutput) != 0) {
      dataType = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_DATA_TYPE);    
      res = res | dataType;
    } // if output 
  } // for

  return res;
}

// list output regs in form [offset -> output reg]
void sgpGasmOperatorMutate::findOutRegCellMap(const sgpGaGenomeMetaList &info, 
  sgpGaGenome &genome, uint endPos, sgpGasmOutRegCellMap &outputMap)
{
  uint instrOffset = 0;  
  uint instrCode, instrCodeRaw, argCount;
  scDataNode argMeta, args;
  uint ioMode, regNo;
  sgpFunction *functor; 
  
  while(instrOffset < endPos) {
    if (info[instrOffset].userType == ggtInstrCode) {
      instrCode = genome[instrOffset].getAsUInt();
      sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
      functor = ::getFunctorForInstrCode(m_functions, instrCodeRaw);
      sgpGasmCodeProcessor::getArguments(genome, instrOffset+1, argCount, args);
      argCount = SC_MIN(argCount, args.size());
      
      if ((functor != SC_NULL) && functor->getArgMeta(argMeta)) {
        for(uint argNo=0,epos=SC_MIN(argCount, argMeta.size()); argNo != epos; argNo++)
        {
          ioMode = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_IO_MODE);    
          if ((ioMode & gatfOutput) != 0) {
            if (info[instrOffset+1+argNo].userType == ggtRegNo) {
              regNo = sgpVMachine::getRegisterNo(args[argNo]);
              outputMap.insert(std::make_pair<uint, cell_size_t>(regNo, instrOffset));
            } // if reg#
          } // if output 
        } // for
      } // if functor OK
    }
    instrOffset++;
  } // while
}  
    
uint sgpGasmOperatorMutate::findInstrBeginBackward(const sgpGaGenomeMetaList &info, uint startOffset)
{
  uint res = startOffset;
  while(res > 0) {
    if (info[res].userType == ggtInstrCode) 
      break;
    res--;
  }    
  return res;
}
  
/// - constant value change:
///   - bool: false <-> true
///   - string: alphastring mutation
///   - xint: bit mutation
///   - real: 
///     abs(val) > 1.0 -> val +/- random(0.1*abs(val)) +/- 10*random(1.0) +/- 10^random(10.0)
///     abs(val) <= 1.0 -> val = val +/- random(abs(0.1*val))
bool sgpGasmOperatorMutate::mutateGenomeCodeValue(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  int varIndex, uint offset, sgpEntityForGasm *workInfo)
{
  bool res = true;
  scDataNodeValueType vtype = genome[varIndex].getValueType();
  uint varSize;
  bool signedVal;
  uint bitNo;
  
  switch (vtype) {
    case vt_bool:
      genome[varIndex].setAsBool(!genome[varIndex].getAsBool());
      break;
    case vt_string: {
      scString val = genome[varIndex].getAsString();
      if (val.empty()) {
        val = char(randomInt(32, 255));
      } else {  
        if (offset > val.length())
          throw scError("Invalid string offset: "+toString(offset));
        val[offset] = char(randomInt(32, 255));
      }  
      genome[varIndex].setAsString(val);
      break;
    }  
    case vt_byte:
    case vt_int:
    case vt_uint:
    case vt_int64:
    case vt_uint64:
      signedVal = false;     
      switch (vtype) {
        case vt_byte:
          varSize = sizeof(byte)*8;
          break;
        case vt_int:
          varSize = sizeof(int)*8;
          signedVal = true;     
          break;          
        case vt_uint:
          varSize = sizeof(int)*8;
          break;
        case vt_int64:
          varSize = sizeof(ulong64)*8;
          signedVal = true;     
          break;          
        case vt_uint64:
          varSize = sizeof(ulong64)*8;
          break;
        default:
            throw scError("Wrong xint type: "+toString(vtype));
      } // switch for xint size detection
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-value-xint", m_counters.getUInt("gx-mut-code-value-xint", 0)+1);
#endif          
      bitNo = randomInt(0, varSize - 1);
      if ((bitNo == varSize - 1) && signedVal) {
      // change sign  
        switch (vtype) {
          case vt_int:
            genome[varIndex].setAsInt(-genome[varIndex].getAsInt());
            break;          
          case vt_int64:
            genome[varIndex].setAsInt64(-genome[varIndex].getAsInt64());
            break;          
          default:
            throw scError("Wrong xint type: "+toString(vtype));
        } // switch for xint
      } else {
      // change value bit
        ulong64 bit = 1;
        if (bitNo > 0)
          bit = bit << bitNo;

        byte operCode = byte(randomInt(0, SG_MUT_VAL_XINT_OPER_CODE_MAX));
        double stepSizeRatio = getStepSizeRatio(workInfo, SGP_MUT_XINT_SPREAD);
        double valChange = static_cast<double>(bit) * stepSizeRatio;
        byte signBits = byte(randomInt(0, 3));
        if ((signBits & 1) != 0)
          valChange *= -1.0;
        
        switch (vtype) {
          case vt_byte: {
            byte oldValue = genome[varIndex].getAsByte();
            byte byteChange = 1+static_cast<byte>(valChange);
            byte newValue = oldValue;
            if ((operCode == SG_MUT_VAL_XINT_OPER_CODE_DIV) && (byteChange == 0))
              operCode = SG_MUT_VAL_XINT_OPER_CODE_PLUS;
          
            switch (operCode) {
              case SG_MUT_VAL_XINT_OPER_CODE_PLUS:
                newValue = oldValue + byteChange;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_MINUS:
                newValue = oldValue - byteChange;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_MULT:
                newValue = oldValue * byteChange;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_DIV:
                newValue = oldValue / byteChange;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_XOR:
                newValue = oldValue ^ byteChange;
                break;
              default:
                throw scError(scString("Unknown mut-xint-byte operation code: ")+toString(operCode));  
            } 
            genome[varIndex].setAsByte(newValue);
            break;
          } // vt_byte 
          case vt_int: {
            int oldValue = genome[varIndex].getAsInt();
            int intChange = 1+static_cast<int>(valChange);
            int newValue = oldValue;
            if ((operCode == SG_MUT_VAL_XINT_OPER_CODE_DIV) && (intChange == 0))
              operCode = SG_MUT_VAL_XINT_OPER_CODE_PLUS;

            switch (operCode) {
              case SG_MUT_VAL_XINT_OPER_CODE_PLUS:
                newValue = oldValue + intChange;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_MINUS:
                newValue = oldValue - intChange;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_MULT:
                newValue = oldValue * intChange;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_DIV:
                newValue = oldValue / intChange;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_XOR:
                newValue = oldValue ^ intChange;
                break;
              default:
                throw scError(scString("Unknown mut-xint-int operation code: ")+toString(operCode));  
            } 
            genome[varIndex].setAsInt(newValue);  
            break;                
          }      
          case vt_uint: {
            uint uintChange = 1+static_cast<uint>(valChange);
            uint oldValue = genome[varIndex].getAsUInt();
            uint newValue = oldValue;
            if ((operCode == SG_MUT_VAL_XINT_OPER_CODE_DIV) && (uintChange == 0))
              operCode = SG_MUT_VAL_XINT_OPER_CODE_PLUS;

            switch (operCode) {
              case SG_MUT_VAL_XINT_OPER_CODE_PLUS:
                newValue = oldValue + uintChange;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_MINUS:
                newValue = oldValue - uintChange;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_MULT:
                newValue = oldValue * uintChange;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_DIV:
                newValue = oldValue / uintChange;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_XOR:
                newValue = oldValue ^ uintChange;
                break;
              default:
                throw scError(scString("Unknown mut-xint-uint operation code: ")+toString(operCode));  
            } 

            if (!m_machine->isRegisterNo(newValue)) 
              genome[varIndex].setAsUInt(newValue);
            else
              res = false;  
            
            break;
          } // uint  
          case vt_int64: {
            long64 oldValue = genome[varIndex].getAsInt64();
            long64 newValue = oldValue;
            long64 long64Change = 1+static_cast<long64>(valChange);
            if ((operCode == SG_MUT_VAL_XINT_OPER_CODE_DIV) && (long64Change == 0))
              operCode = SG_MUT_VAL_XINT_OPER_CODE_PLUS;

            switch (operCode) {
              case SG_MUT_VAL_XINT_OPER_CODE_PLUS:
                newValue = oldValue + long64Change;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_MINUS:
                newValue = oldValue - long64Change;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_MULT:
                newValue = oldValue * long64Change;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_DIV:
                newValue = oldValue / long64Change;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_XOR:
                newValue = oldValue ^ long64Change;
                break;
              default:
                throw scError(scString("Unknown mut-xint-int64 operation code: ")+toString(operCode));  
            } 
            genome[varIndex].setAsInt64(newValue);
            break;        
          } // int64   
          case vt_uint64: {
            ulong64 oldValue = genome[varIndex].getAsUInt64();
            ulong64 newValue = oldValue;
            ulong64 ulong64Change = 1+static_cast<ulong64>(valChange);
            if ((operCode == SG_MUT_VAL_XINT_OPER_CODE_DIV) && (ulong64Change == 0))
              operCode = SG_MUT_VAL_XINT_OPER_CODE_PLUS;

            switch (operCode) {
              case SG_MUT_VAL_XINT_OPER_CODE_PLUS:
                newValue = oldValue + ulong64Change;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_MINUS:
                newValue = oldValue - ulong64Change;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_MULT:
                newValue = oldValue * ulong64Change;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_DIV:
                newValue = oldValue / ulong64Change;
                break;
              case SG_MUT_VAL_XINT_OPER_CODE_XOR:
                newValue = oldValue ^ ulong64Change;
                break;
              default:
                throw scError(scString("Unknown mut-xint-uint64 operation code: ")+toString(operCode));  
            } 
            genome[varIndex].setAsUInt64(newValue); 
            break;
          }  
          default:
              throw scError("Wrong xint type: "+toString(vtype));
        } // switch for xint
      }
      break;  // end of case for all xint    
    case vt_float:
    case vt_double:
    case vt_xdouble:
    {
        byte signBits = byte(randomInt(0, 3));
        byte operCode = byte(randomInt(0, 3));
        uint bitNo = static_cast<uint>(randomInt(0,10));
        uint bitChange;
        float signFrac = ((signBits & 1) != 0)?-1.0:1.0;
        double valChange;
        double stepSizeRatio;

    stepSizeRatio = getStepSizeRatio(workInfo, SGP_MUT_REAL_SPREAD);
    
if (std::fabs(genome[varIndex].getAsFloat()) < 1.0) {
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-value-fsmall", m_counters.getUInt("gx-mut-code-value-fsmall", 0)+1);
#endif    
        valChange = 
          (signFrac * randomDouble(0.0, m_smallFloatChangeRatio * stepSizeRatio));
} else {        
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-value-flarge", m_counters.getUInt("gx-mut-code-value-flarge", 0)+1);
#endif    
        valChange = 
          (signFrac * randomDouble(0.0, m_largeFloatChangeRatio * stepSizeRatio));
}
          
        if (bitNo > 0) { 
          bitChange = 1 << bitNo;
          valChange = valChange / double(bitChange);
        }
        
        // if divide by 0 - change to multiply
        if ((operCode == 3) && equFloat(valChange, 0.0))
          operCode = 2;
          
        switch (vtype) {
          case vt_float: {
            float fValue = genome[varIndex].getAsFloat();
            switch (operCode) {
              case 1:
                fValue = fValue - valChange;
                break;
              case 2:
                fValue = fValue * valChange;
                break;                 
              case 3:
                fValue = fValue / valChange;
                break;
              default: // case 0
                fValue = fValue + valChange;
                break;
            }                                               
            genome[varIndex].setAsFloat(fValue); 
            break;
          }  
          case vt_double: {
            double fValue = genome[varIndex].getAsDouble();
            switch (operCode) {
              case 1:
                fValue = fValue - valChange;
                break;
              case 2:
                fValue = fValue * valChange;
                break;                 
              case 3:
                fValue = fValue / valChange;
                break;
              default: // case 0
                fValue = fValue + valChange;
                break;
            }                                               
            genome[varIndex].setAsDouble(fValue); 
            break;
          }  
          case vt_xdouble: {
            xdouble fValue = genome[varIndex].getAsXDouble();
            switch (operCode) {
              case 1:
                fValue = fValue - valChange;
                break;
              case 2:
                fValue = fValue * valChange;
                break;                 
              case 3:
                fValue = fValue / valChange;
                break;
              default: // case 0
                fValue = fValue + valChange;
                break;
            }                                               
            genome[varIndex].setAsXDouble(fValue); 
            break;
          }  
          default:
              throw scError("Wrong type: "+toString(vtype));
        } // switch for abs(x)>=1.0 
      }
      break;
    default: {
  //vt_parent, vt_array, vt_null, 
  //vt_vptr,
  //vt_date, vt_time, vt_datetime
      break; // do nothing
    }  
//    default:
//      /throw scError("Unknown var type for mutate: "+toString(metaInfo.genType));
  }
  return res;
}

double sgpGasmOperatorMutate::getStepSizeRatio(const sgpEntityForGasm *workInfo, double spread)
{
  double stepSizeRatio;
  if (!workInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_OTHER_PARAM_BASE + SGP_MUT_OTHER_PARAM_IDX_VAL_CHG_STEP, stepSizeRatio)) {                
    stepSizeRatio = 1.0;
  } else {
    // ratio can be 
    //   0.5 => 1.0
    //   0.0 => 1/(2^(spread/2))
    //   1.0 => (2^(spread/2))
    stepSizeRatio = 1.0 / pow(2.0, (0.5 - stepSizeRatio) * spread);      
  }  
  return stepSizeRatio;
}

uint sgpGasmOperatorMutate::getMultiPointRatio(const sgpEntityForGasm *workInfo)
{
  const uint MIN_BIT_COUNT = 4;
  uint value;
  uint res = 1;
  uint bitCount;
  
  if (workInfo->getInfoUInt(SGP_INFOBLOCK_IDX_MUT_OTHER_PARAM_BASE + SGP_MUT_OTHER_PARAM_IDX_MULTI_POINT_RATIO, value)) 
  {
    // calc bits in value, if < min then result = 1
    bitCount = countBitOnes(value);
    if (bitCount >= MIN_BIT_COUNT) {
      res = bitCount - MIN_BIT_COUNT + 2;
    }  
  }  
  return res;
}

bool sgpGasmOperatorMutate::mutateGenomeCodeRegValue(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  uint instrOffset, int varIndex, uint offset, scDataNode &argMeta)
{  
  bool res = true;
  uint argNo = varIndex - instrOffset - 1; 
  uint argTypes;
  
  if (argNo < argMeta.size())
    argTypes = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_ARG_TYPE);    
  else
    argTypes = 0;  
  
  if ((argTypes & gatfConst) == 0)
  {
    res = false;
    return res;
  }  

  genome[varIndex].setAsUInt(
    sgpVMachine::getRegisterNo(genome[varIndex]));

  if (m_supportedDataNodeTypes.find(genome[varIndex].getValueType()) == m_supportedDataNodeTypes.end())
  {
    mutateGenomeCodeTypeUp(info, genome, instrOffset, varIndex, offset);
    if (m_supportedDataNodeTypes.find(genome[varIndex].getValueType()) == m_supportedDataNodeTypes.end())
      mutateGenomeCodeTypeDown(info, genome, instrOffset, varIndex, offset);
  }  

  return res;
}      

// swap arguments if > 1, do not swap output-only arguments
bool sgpGasmOperatorMutate::mutateGenomeCodeSwapArgs(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  int varIndex, scDataNode &argMeta, uint argNo, uint ioMode)
{
  bool res = false;
  
  if ((ioMode & gatfInput) == 0)
    return false;

  uint argCount = argMeta.size();
  if (argCount > 1) {
    uint argNo2 = (argNo + 1) % argCount;
    uint ioMode2 = sgpVMachine::getArgMetaParamUInt(argMeta, argNo2, GASM_ARG_META_IO_MODE);
    if ((ioMode2 & gatfInput) == 0) {
      // output cannot be used - check prior argument
      if (argNo > 0)
        argNo2 = argNo - 1;
      else
        argNo2 = argCount - 1;  
      ioMode2 = sgpVMachine::getArgMetaParamUInt(argMeta, argNo2, GASM_ARG_META_IO_MODE);  
    }
    
    if ((argNo != argNo2) && ((ioMode2 & gatfInput) != 0) && (ioMode == ioMode2)) {
      int varIndex2 = argNo2;
      varIndex2 = varIndex + varIndex2 - int(argNo);
      if (varIndex2 < int(genome.size())) {
        scDataNodeValue tmp = genome[varIndex];   
        genome[varIndex] = genome[varIndex2];
        genome[varIndex2] = tmp;
        res = true;
      }
    }
  }
  return res;
}  

// f(x) = -x
// returns <true> if code modified
bool sgpGasmOperatorMutate::mutateGenomeCodeValueNeg(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  int varIndex, uint offset)
{
  bool res = true;
  
  if (info[varIndex].userType != ggtValue) {
    res = false;
    return res;
  }  
  
  scDataNodeValue value;
  value.copyFrom(genome[varIndex]);

  scDataNodeValueType vtype = value.getValueType();

  switch (vtype) {
    case vt_bool:
      value.setAsBool(!value.getAsBool());
      break;
    case vt_byte:
      if (value.getAsByte() > 0)
        value.setAsByte(0);
      else  
        value.setAsByte(static_cast<byte>(randomInt(1, 255)));
      break;
    case vt_int:
      value.setAsInt(-value.getAsInt());
      break;
    case vt_uint:
      if (value.getAsUInt() > 0)
        value.setAsUInt(0);
      else  
        value.setAsUInt(randomUInt(1, UINT_MAX));
      break;
    case vt_int64:
      value.setAsInt64(-value.getAsInt64());
      break;
    case vt_uint64:
      if (value.getAsUInt64() > 0)
        value.setAsUInt64(0);
      else  
        value.setAsUInt64(randomUInt(1, UINT_MAX));
      break;
    case vt_float:
      value.setAsFloat(-value.getAsFloat());
      break;
    case vt_double:
      value.setAsDouble(-value.getAsDouble());
      break;
    case vt_xdouble:
      value.setAsXDouble(-value.getAsXDouble());
      break;
    default:     
  //vt_parent, vt_array, vt_null, 
  //vt_vptr,
  //vt_date, vt_time, vt_datetime
  //vt_string: 
      res = false;
      break; // do nothing
  }
  if (res) 
    genome[varIndex].copyFrom(value);
  return res;
}  

void sgpGasmOperatorMutate::getArgDataTypes(sgpGaGenome &genome, uint instrOffset, int varIndex, uint &dataTypes)
{
  uint instrCode = 
    genome[instrOffset].getAsUInt();
  uint instrCodeRaw, argCount, ioModeForArg;  
      
  sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
  getArgMetaInfo(instrCodeRaw, varIndex - instrOffset - 1, dataTypes, ioModeForArg);
}

void sgpGasmOperatorMutate::getArgTypesAsSetFinal(sgpGaGenome &genome, uint instrOffset, int varIndex, sgpGasmDataNodeTypeSet &outset)
{
  uint dataTypes;
  sgpGasmDataNodeTypeSet nodeDataTypes;  
  getArgDataTypes(genome, instrOffset, varIndex, dataTypes);
  sgpGasmCodeProcessor::castTypeSet(dataTypes, nodeDataTypes);

  outset.clear();
  for(sgpGasmDataNodeTypeSet::const_iterator it = nodeDataTypes.begin(), epos = nodeDataTypes.end(); it != epos; ++it)
    if (m_supportedDataNodeTypes.find(*it) != m_supportedDataNodeTypes.end())
      outset.insert(*it);
}

bool sgpGasmOperatorMutate::mutateGenomeCodeTypeUp(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  uint instrOffset, int varIndex, uint offset)
{
  bool res;
  scDataNodeValue value;
  sgpGasmDataNodeTypeSet workTypes;  
  getArgTypesAsSetFinal(genome, instrOffset, varIndex, workTypes);
  
  sgpFunction *functor = getFunctor(genome, instrOffset);
  if (functor != SC_NULL)
    if (functor->hasDynamicArgs())
      return false;
  
  value.copyFrom(genome[varIndex]);
  
  scDataNodeValueType oldType, newType;
  do {
    oldType = value.getValueType();
    mutateGenomeCodeTypeUp(value);
    newType = value.getValueType();
    if (workTypes.find(newType) != workTypes.end())
      break;
  } while (oldType != newType);  

  if (oldType != newType) {
    genome[varIndex].copyFrom(value);
    res = true;
  } else {
    res = false;
  }
  
  return res;  
}

void sgpGasmOperatorMutate::mutateGenomeCodeTypeUp(scDataNodeValue &value)
{
  scDataNodeValueType vtype = value.getValueType();

  switch (vtype) {
    case vt_null:
      value.setAsBool(randomBool());
      break;
    case vt_bool:
      value.setAsByte(value.getAsBool());
      break;
    case vt_byte:
      value.setAsInt(value.getAsByte());
      break;
    case vt_int:
      value.setAsInt64(value.getAsInt());
      break;
    case vt_uint:
      value.setAsUInt64(value.getAsUInt());
      break;
    case vt_int64:
      value.setAsFloat(value.getAsInt64());
      break;
    case vt_uint64:
      value.setAsFloat(value.getAsUInt64());
      break;
    case vt_float:
      value.setAsDouble(value.getAsFloat());
      break;
    case vt_double:
      value.setAsXDouble(value.getAsDouble());
      break;
    case vt_xdouble:
      value.setAsString(value.getAsString());
      break;
    case vt_string: 
      break; // highest type
    default: {
  //vt_parent, vt_array, vt_null, 
  //vt_vptr,
  //vt_date, vt_time, vt_datetime
      break; // do nothing
    }  
  }
}


bool sgpGasmOperatorMutate::mutateGenomeCodeTypeDown(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  uint instrOffset, int varIndex, uint offset)
{
  bool res;
  scDataNodeValue value;
  sgpGasmDataNodeTypeSet workTypes;  
  getArgTypesAsSetFinal(genome, instrOffset, varIndex, workTypes);

  sgpFunction *functor = getFunctor(genome, instrOffset);
  if (functor != SC_NULL)
    if (functor->hasDynamicArgs())
      return false;
  
  value.copyFrom(genome[varIndex]);
  scDataNodeValueType oldType, newType;  
  do {
    oldType = value.getValueType();
    mutateGenomeCodeTypeDown(value);
    newType = value.getValueType();
    if (workTypes.find(newType) != workTypes.end())
      break;
  } while (oldType != newType);  
  if (oldType != newType) {
    genome[varIndex].copyFrom(value);
    res = true;
  } else {
    res = false;
  } 
  return res;   
}

void sgpGasmOperatorMutate::mutateGenomeCodeTypeDown(scDataNodeValue &value)
{
  scDataNodeValueType vtype = value.getValueType();
  switch (vtype) {
    case vt_null:
      break; // lowest type - do nothing
    case vt_bool:
      value.setAsNull();
      break; 
    case vt_byte:
      value.setAsBool(value.getAsByte() > 0);
      break;
    case vt_int:
      value.setAsByte(static_cast<byte>(value.getAsInt()));
      break;
    case vt_uint:
      value.setAsByte(static_cast<byte>(value.getAsUInt()));
      break;
    case vt_int64:
      value.setAsInt(static_cast<int64>(value.getAsInt64()));
      break;
    case vt_uint64:
      value.setAsUInt(static_cast<uint>(value.getAsUInt64()));
      break;
    case vt_float:
      value.setAsInt64(static_cast<int64>(value.getAsFloat()));
      break;
    case vt_double:
      value.setAsFloat(static_cast<float>(value.getAsDouble()));
      break;
    case vt_xdouble:
      value.setAsDouble(static_cast<double>(value.getAsXDouble()));
      break;
    case vt_string: 
      try {
        value.setAsXDouble(static_cast<xdouble>(value.getAsXDouble()));
      }
      catch(...) {
        try {
          // try with int conversion
          value.setAsInt64(static_cast<int64>(value.getAsInt64()));
        } 
        catch(...) {
          // not a int -> change string to (int = length of string)
          value.setAsInt64(value.getAsString().length());
        }  
      }    
      break;
    default: {
  //vt_parent, vt_array, vt_null, 
  //vt_vptr,
  //vt_date, vt_time, vt_datetime
      break; // do nothing
    }  
  }
}

uint sgpGasmOperatorMutate::genomeToBlockIndex(const sgpEntityForGasm *workInfo, uint value)
{
  return (value - getCodeBlockOffset(workInfo));
}

bool sgpGasmOperatorMutate::canAddCodeToGenome(const sgpEntityForGasm *workInfo, const sgpProgramCode &code, const sgpGaGenome &genome, uint genomeIndex)
{
  bool res = true;
  if ((m_features & gmfMacrosEnabled) != 0) {
    uint targetBlockIndex = genomeToBlockIndex(workInfo, genomeIndex);
    if (targetBlockIndex != SG_MUT_MAIN_BLOCK_INDEX) {
      scDataNode mainCode;
      code.getBlockCode(SG_MUT_MAIN_BLOCK_INDEX, mainCode);
      assert(mainCode.size() > 0);  
      if (mainCode.size() < genome.size()*2)
        res = false;
    }  
  }
  return res;
}

// Insert new instruction BEFORE the current one
// --------------------------------------------
// Steps:
// - find offset after the current instruction
// - build new random instruction
// - if this is not last instruction chain it with the next one
//   - set random input argument of next instruction as = output from the new instruction
// - otherwise (last instruction) - force output of new instruction to be = output of block
bool sgpGasmOperatorMutate::mutateGenomeCodeInsert(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  uint initInstrOffset,
  int varIndex, uint offset, uint blockInputCount, const sgpEntityForGasm *workInfo, uint genomeIndex)
{
  const uint MAX_INSTR_INS_TRY_CNT = 5;
  uint targetBlockIndex = genomeToBlockIndex(workInfo, genomeIndex);
  sgpProgramCode code;  
  workInfo->getProgramCode(code);

#ifdef OPT_LIMIT_MACRO_SIZE_BY_MAIN  
  if (!canAddCodeToGenome(workInfo, code, genome, genomeIndex))
    return false;
#endif

  uint instrOffset;
  
  sgpGaGenome newCode;
  sgpGasmRegSet writtenRegs;
  bool chainWithNext = (instrOffset < genome.size());
  bool res = false;
  uint tryCount = MAX_INSTR_INS_TRY_CNT;
  
  sgpGasmProbList instrCodeProbs;  
  readMutInstrCodeProbs(workInfo, instrCodeProbs);  
  
  findWrittenRegs(info, genome, instrOffset, blockInputCount, writtenRegs);
  
  while((tryCount > 0) && !res) {
  
    if (sgpGasmCodeProcessor::buildRandomInstr(m_functions, writtenRegs, m_supportedDataTypes, m_supportedDataNodeTypes, instrCodeProbs, m_machine, workInfo, &targetBlockIndex, newCode))
    { 
      uint instrCode = newCode[0].getAsUInt();
      uint instrCodeRaw, argCount;
      
      sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
      sgpFunction *func = getFunctorForInstrCode(m_functions, instrCodeRaw);
      if (func != SC_NULL) {      
        scDataNode args;
        
        sgpGasmCodeProcessor::getArguments(newCode, 1, argCount, args);      
        if (m_machine->verifyArgs(args, func)) 
        {   
          scDataNode argMetaForNew;
          if (!getArgMetaForInstr(newCode, 0, argMetaForNew))
            argMetaForNew.clear();

          // if not last instruction - modify input of next instruction to be = to output of new instr.          
          if (chainWithNext) {
            uint outputForNew = 0;
            scDataNode argMetaForNext;
            chainWithNext = !argMetaForNew.isNull();
            if (chainWithNext)
              chainWithNext = getOutputRegNo(newCode, 0, argMetaForNew, outputForNew);
            if (chainWithNext) 
              chainWithNext = getArgMetaForInstr(genome, instrOffset, argMetaForNext);
            if (chainWithNext) 
              setRandomInputArgAsRegNo(genome, instrOffset, argMetaForNext, outputForNew);
          }

          // on last instruction set output of new code to #0 (block output)
          if ((!argMetaForNew.isNull()) && (instrOffset >= genome.size()))
            setOutputRegNo(newCode, 0, argMetaForNew, SGP_REGNO_OUTPUT);
          
          // insert newCode
          genome.insert(genome.begin() + instrOffset,
            newCode.begin(),
            newCode.end());
            
          res = true;          
        } 
        
        // chain with next instruction     
      }      
    }  
    
    tryCount--;
  }
  
  return res;
}

void sgpGasmOperatorMutate::buildRandomBlock(uint blockInputCount, uint instrLimit, 
  const sgpEntityForGasm *workInfo, uint targetBlockIndex,
  scDataNode &output)
{
  sgpGaGenome newCode;
  sgpGasmRegSet writtenRegs;

  if (blockInputCount > 0)
    for(uint i=SGP_REGB_INPUT,epos=SGP_REGB_INPUT+blockInputCount;i!=epos;i++)
      writtenRegs.insert(i);

  sgpGasmProbList instrCodeProbs;  
  readMutInstrCodeProbs(workInfo, instrCodeProbs);  
    
  uint tryCnt = 10*instrLimit;
  uint instrCnt = instrLimit;
  scDataNode newCell;
  
  do {  
    newCode.clear();
    if (sgpGasmCodeProcessor::buildRandomInstr(m_functions, writtenRegs, m_supportedDataTypes, m_supportedDataNodeTypes, instrCodeProbs, 
      m_machine, SC_NULL, &targetBlockIndex, newCode))
    { 
      uint instrCode = newCode[0].getAsUInt();
      uint instrCodeRaw, argCount;

      instrCnt--;
      
      sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
      sgpFunction *func = getFunctorForInstrCode(m_functions, instrCodeRaw);
      if (func != SC_NULL) {      
        scDataNode args;
        
        sgpGasmCodeProcessor::getArguments(newCode, 1, argCount, args);
        if (m_machine->verifyArgs(args, func)) 
        {    
          // insert newCode
          for(uint j=0, eposj = newCode.size(); j != eposj; j++) {
            newCell = newCode[j];
            output.addItem(newCell);
          }
          updateWrittenRegs(newCode, 0, writtenRegs);  
          newCode.clear();        
        } 
      }    
    }  
    tryCnt--;
  } while ((tryCnt > 0) && (instrCnt > 0));
}

sgpFunction *sgpGasmOperatorMutate::getFunctor(const sgpGaGenome &genome, uint instrOffset) const
{
  uint instrCode = genome[instrOffset].getAsUInt();
  uint instrCodeRaw, argCount;
  
  sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
  sgpFunction *func = 
    ::getFunctorForInstrCode(m_functions, instrCodeRaw);

  return func;
}

void sgpGasmOperatorMutate::updateWrittenRegs(sgpGaGenome &newCode, uint instrOffset, sgpGasmRegSet &writtenRegs) 
{
  uint instrCode = newCode[instrOffset].getAsUInt();
  uint instrCodeRaw, argCount;
  
  sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
  sgpFunction *func = 
    ::getFunctorForInstrCode(m_functions, instrCodeRaw);
  if (func != SC_NULL) {      
    scDataNode args;
    
    sgpGasmCodeProcessor::getArguments(newCode, instrOffset+1, argCount, args);

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

bool sgpGasmOperatorMutate::mutateGenomeCodeDelete(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  uint instrOffset,
  int varIndex, uint offset)
{
  bool res = false;
  uint useVarIndex = instrOffset; 
  if (useVarIndex >= genome.size())
    return false;  
  uint instrCode = genome[useVarIndex].getAsUInt();
  uint instrCodeRaw, argCnt;
    
  sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCnt);
  uint delSize = 1 + argCnt;    
  if (genome.size() > delSize) {
    genome.erase(genome.begin() + useVarIndex, genome.begin() + useVarIndex + delSize);
    res = true;
  }  
  assert(!genome.empty());
  return res;
}

bool sgpGasmOperatorMutate::mutateGenomeCodeReplace(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  uint instrOffset,
  int varIndex, uint offset, uint blockInputCount, const sgpEntityForGasm *workInfo, uint genomeIndex)
{
  bool res;
  sgpGaGenomeMetaList newInfo;
  
  res = mutateGenomeCodeDelete(info, genome, instrOffset, varIndex, offset);
  calcMetaForCodeScan(genome, newInfo);  
  if (mutateGenomeCodeInsert(newInfo, genome, instrOffset, varIndex, offset, blockInputCount, workInfo, genomeIndex))
    res = true;
  assert(!genome.empty());  
  return res;
}

bool sgpGasmOperatorMutate::isFunctionSupported(const scString &aName)
{
  return (m_functionNameMap.hasChild(aName));   
}

uint sgpGasmOperatorMutate::getInstrCode(const scString &aName)
{
  uint res = m_functionNameMap.getUInt(aName, 0);
  return res;
}

// a) Copy block to new position
// b) Fill block at old position with random code
// c) Insert in block at old position instructions (in the middle):
//   call #random, new-block-id [, block-input-regs]
//   move #0, #random
//   ; #random - random register with supported data type
void sgpGasmOperatorMutate::mutateGenomeCodeGenBlock(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  int varIndex, uint offset, uint blockInputCount, sgpEntityForGasm *workInfo, uint genomeIndex, double &mutCost)
{
  mutCost = 0.1;
  
  if (!isFunctionSupported("call"))
    return;

  scDataNode blockCode, newBlockCode;
  sgpProgramCode code;
  scDataNode inputArgs, outputArgs;
  uint oldBlockPos = genomeIndex - getCodeBlockOffset(workInfo);

  workInfo->getProgramCode(code);

  uint newBlockPos = code.getBlockCount();

  uint maxInstrCount = 2+countGenomeTypes(info, ggtInstrCode);
  uint targetBlockIndex = code.getBlockCount();
  buildRandomBlock(blockInputCount, randomUInt(maxInstrCount/2, maxInstrCount), workInfo, targetBlockIndex, newBlockCode);
  if (newBlockCode.size() == 0)
    return; // generation of new block failed
      
  code.getBlockMetaInfo(oldBlockPos, inputArgs, outputArgs);
  code.getBlockCode(oldBlockPos, blockCode);
  code.addBlock(inputArgs, outputArgs, blockCode);

  code.setBlock(oldBlockPos, inputArgs, outputArgs, newBlockCode);
  newBlockCode.copyTo(genome);

  // insert call #0, block-no, <input>
  std::auto_ptr<sgpGaGenomeMetaList> metaInfoGuard;

  metaInfoGuard.reset(new sgpGaGenomeMetaList);
  calcMetaForCodeScan(genome, *metaInfoGuard);  

  uint instrOffset = findInstrBeginBackward(*metaInfoGuard, randomUInt(0, genome.size() / 2));  

  sgpGaGenome newCode;
  scDataNodeValue newCell; 
  scDataNode newCellNode;

  // add instr code
  uint instrCodeRaw = getInstrCode("call");
  uint instrCode = sgpVMachine::encodeInstr(instrCodeRaw, 2 + blockInputCount);
  newCell.setAsUInt(instrCode);  
  newCode.push_back(newCell);

  // add block-no = genomeIndex  
  newCell.setAsUInt(newBlockPos);
  newCode.push_back(newCell);

  // add output = #0  
  newCellNode = sgpVMachine::buildRegisterArg(SGP_REGB_OUTPUT);
  newCell = newCellNode;
  newCode.push_back(newCell);

  // add input argument(s)
  for(uint i=0,epos = blockInputCount; i!=epos; i++) {
    newCellNode = sgpVMachine::buildRegisterArg(SGP_REGB_INPUT+i);
    newCell.copyFrom(newCellNode);
    newCode.push_back(newCell);
  }
  
  // insert instruction
  genome.insert(genome.begin() + instrOffset,
    newCode.begin(),
    newCode.end());

  mutCost = 1.0;

  blockCode.copyValueFrom(genome);
  code.setBlock(oldBlockPos, inputArgs, outputArgs, blockCode);
    
  workInfo->setProgramCode(code.getFullCode());
  
#ifdef OPT_SPLIT_ON_GEN_BLOCK  
  // split call - increase probability of development
  scDataNode argMeta;  
  metaInfoGuard.reset(new sgpGaGenomeMetaList);
  calcMetaForCode(genome, workInfo, *metaInfoGuard, genomeSize);

  sgpFunction *functor = ::getFunctorForInstrCode(m_functions, instrCodeRaw);
  if ((functor != SC_NULL) && functor->getArgMeta(argMeta)) {   
    mutateGenomeCodeSplitInstr(*metaInfoGuard, genome, instrOffset, 0, instrOffset, blockInputCount, argMeta);
  }  
#endif

#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-gen-block-done", m_counters.getUInt("gx-mut-code-gen-block-done", 0)+1);
#endif    

// Generate new block/macro basing on part of code from specified block.
// ---------------------------------------------------------------------
// select part of code
// generate as a new block
// expand macros inside the new code
// prepare input:
// - find list of non-written-before registers in block
// - find list of used input registers
// - replace all with new input registers - but only 
//   till place where the register is written in code
// - build list of block input args basing on replaced register types
// prepare output:
// - modify output of last instruction - let it be #0: block output
// - define output arg meta - according to replaced reg type
bool sgpGasmOperatorMutate::mutateGenomeCodeMacroGenerate(const sgpGaGenomeMetaList &initInfo, sgpGaGenome &initGenome, 
  uint initInstrOffset, uint blockInputCount, uint genomeIndex, sgpEntityForGasm *workInfo, double &mutCost)
{  
  scDataNode blockCode, newBlockCode;
  scDataNode fullCode;
  sgpProgramCode code;

#ifdef OPT_VALIDATE_MAIN_SIZE
  sgpGasmCodeProcessor::checkMainNotEmpty(workInfo);
#endif  

  mutCost = 0.1;
  workInfo->getProgramCode(fullCode);
  code.setFullCode(fullCode);
  
  // --- expand & update info
  sgpGaGenome genome;
  sgpGaGenomeMetaList info;
  sgpProgramCode expandedCode;
  uint targetBlockIndex = genomeToBlockIndex(workInfo, genomeIndex);
  
  expandedCode.setFullCode(fullCode);  
  m_machine->expandCode(expandedCode);  
  scDataNode expandedBlockCode;
  expandedCode.getBlockCode(targetBlockIndex, expandedBlockCode);
  expandedBlockCode.copyTo(genome);

  calcMetaForCodeScan(genome, info); 
  uint instrOffset = initInstrOffset;
  
  if (instrOffset >= info.size())   
  {
    if (!info.empty())
      instrOffset = info.size() - 1;
    else
      return false;
  }  
  
  instrOffset = findInstrBeginBackward(info, instrOffset);  
  // ---
  
//--- select number of instructions in macro - using fading probability
  uint tailInstrCount = std::max<uint>(1, countGenomeTypes(info, ggtInstrCode, instrOffset));
  tailInstrCount = std::min<uint>(tailInstrCount, SGP_MUT_MAX_MACRO_INSTR_CNT_ON_GEN);
  sgpGasmProbList macroLenProbs(tailInstrCount);
  for(uint i=0, epos = tailInstrCount; i != epos; i++)
    macroLenProbs[i] = 1.0 / (1.0 + pow(SGP_MUT_MACRO_LEN_DEC_FACTOR, static_cast<double>(i+1)));
    
  uint macroInstrCount = std::min<uint>(tailInstrCount, 1 + ::selectProbItem(macroLenProbs));
//---  
  uint endOffset = findNthInstrForward(info, instrOffset + 1, macroInstrCount);  

  sgpGaGenome newCode;

  std::copy(genome.begin() + instrOffset, 
            genome.begin() + endOffset, 
            std::back_inserter(newCode));
            
  if (newCode.empty()) 
    return false;          

  mutCost = 1.0;

  // handle input args
  std::set<uint> writtenRegs;
  std::map<uint, uint> inputRegsForChange; // old-reg-no -> new-reg-no
  std::map<uint, uint>::const_iterator inputArgIt;
  
  uint instrCode, instrCodeRaw, argCount;  
  scDataNode argMeta, args, newArg;
  uint ioMode, argDataTypes;
  uint regNo, nextRegNo, newRegNo;
  sgpFunction *functor; 
  uint instrOffsetForScan, lastInstrOffset;
  uint newArgDataType;
  scDataNode inputArgMeta, outputArgMeta;
  scDataNode inputArgs;
  inputArgs.setAsParent();
  
  nextRegNo = SGP_REGB_INPUT;
  instrOffsetForScan = instrOffset;
  
  while(instrOffsetForScan < endOffset) {
    if (info[instrOffsetForScan].userType == ggtInstrCode) {
      instrCode = genome[instrOffsetForScan].getAsUInt();
      sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
      functor = ::getFunctorForInstrCode(m_functions, instrCodeRaw);
      sgpGasmCodeProcessor::getArguments(genome, instrOffsetForScan+1, argCount, args);
      argCount = SC_MIN(argCount, args.size());
      
      if ((functor != SC_NULL) && functor->getArgMeta(argMeta)) {
        for(uint argNo=0,epos=SC_MIN(argCount, argMeta.size()); argNo != epos; argNo++)
        {
          ioMode = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_IO_MODE);    
          argDataTypes = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_DATA_TYPE);
          argDataTypes &= m_supportedDataTypes;    
          
          if (info[instrOffsetForScan+1+argNo].userType == ggtRegNo) {
            regNo = sgpVMachine::getRegisterNo(args[argNo]);
            if ((ioMode & gatfOutput) != 0) {
            // output reg
              writtenRegs.insert(regNo);
            } else {
            // input reg
              if (writtenRegs.find(regNo) == writtenRegs.end()) {
              // we replace input reg if not modified already
                inputArgIt = inputRegsForChange.find(regNo);
                if (inputArgIt != inputRegsForChange.end()) {
                // replacement map exists
                  newRegNo = inputArgIt->second;
                } else {
                // new replacement map
                  newRegNo = nextRegNo;
                // define meta for input
                  if (inputArgMeta.size() < (SGP_REGB_INPUT_MAX - SGP_REGB_INPUT + 1)) {
                    newArgDataType = ::getRandomBitSetItem(argDataTypes);
                    newArg.clear();
                    sgpVMachine::initDataNodeAs(
                      static_cast<sgpGvmDataTypeFlag>(newArgDataType), newArg);
                    inputArgMeta.addChild(new scDataNode(newArg));
                    inputRegsForChange.insert(std::make_pair<uint, uint>(regNo, newRegNo));
                    inputArgs.addChild(new scDataNode(regNo));
                  }  
                  if (nextRegNo < SGP_REGB_INPUT_MAX)
                    nextRegNo++;
                }
                newCode[instrOffsetForScan - instrOffset + 1 + argNo] =
                  sgpVMachine::buildRegisterArg(newRegNo);
              }
            }
          }
        } // for
      } // if functor OK
    }
    instrOffsetForScan++;
  } // while
   
  uint outputRegNo = SGP_REGB_OUTPUT;
  
  // find last instr & replace it's output with #0 (if output exists)
  lastInstrOffset = 0;
  if (endOffset > 0) {
    instrOffsetForScan = endOffset - 1;
    while((instrOffsetForScan >= instrOffset) && (lastInstrOffset == 0)) {
      if (info[instrOffsetForScan].userType == ggtInstrCode) {
        instrCode = genome[instrOffsetForScan].getAsUInt();
        sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
        functor = ::getFunctorForInstrCode(m_functions, instrCodeRaw);
        sgpGasmCodeProcessor::getArguments(genome, instrOffsetForScan+1, argCount, args);
        argCount = SC_MIN(argCount, args.size());
        
        if ((functor != SC_NULL) && functor->getArgMeta(argMeta)) {
          for(uint argNo=0,epos=SC_MIN(argCount, argMeta.size()); argNo != epos; argNo++)
          {
            ioMode = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_IO_MODE);    
            argDataTypes = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_DATA_TYPE);    
            argDataTypes &= m_supportedDataTypes;    
            
            if (info[instrOffsetForScan+1+argNo].userType == ggtRegNo) {
              outputRegNo = sgpVMachine::getRegisterNo(args[argNo]);
              if ((ioMode & gatfOutput) != 0) {
              // output reg
                newCode[instrOffsetForScan - instrOffset + 1 + argNo] =
                  sgpVMachine::buildRegisterArg(SGP_REGB_OUTPUT);
                // break loop  
                lastInstrOffset = endOffset;
                newArgDataType = ::getRandomBitSetItem(argDataTypes);
                newArg.clear();
                sgpVMachine::initDataNodeAs(
                  static_cast<sgpGvmDataTypeFlag>(newArgDataType), newArg);
                outputArgMeta.addChild(new scDataNode(newArg));
                break;  
              } // if output
            } // if register
          } // for
        } // if functor OK
      } // if instruction 
      if (instrOffsetForScan > 0)
        instrOffsetForScan--;
      else
        break;  
    } // while
  } // if endoffset ok
  
  assert(newCode.size() > 0);
  newBlockCode = newCode;  
  
#ifdef OPT_VALIDATE_MAIN_SIZE
  sgpGasmCodeProcessor::checkMainNotEmpty(workInfo);
  fullCode = code.getFullCode();
  workInfo->setProgramCode(fullCode);
  sgpGasmCodeProcessor::checkMainNotEmpty(workInfo);
#endif  
          
  code.addBlock(inputArgMeta, outputArgMeta, newBlockCode);

  // regNo = output-reg-no
  // inputRegsForChange => input registers => input args mapping   
  // instrOffset => offset of first instruction
  // endOffset => points one byte after last instruction byte
  if (isFunctionSupported("macro"))
  {
    sgpGaGenome newCode;    
    buildMacroInstr(newCode, code.getBlockCount() - 1, outputRegNo, inputArgs);

    genome.erase(genome.begin() + instrOffset, 
                 genome.begin() + endOffset);

    genome.insert(genome.begin() + instrOffset,
      newCode.begin(),
      newCode.end());
  }  
      
  fullCode = code.getFullCode();
#ifdef OPT_VALIDATE_MAIN_SIZE
sgpGasmCodeProcessor::checkMainNotEmpty(workInfo);
#endif  
  workInfo->setProgramCode(fullCode);
#ifdef OPT_VALIDATE_MAIN_SIZE
sgpGasmCodeProcessor::checkMainNotEmpty(workInfo);
#endif

#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-mac-gen-done", m_counters.getUInt("gx-mut-code-mac-gen-done", 0)+1);
#endif    

  return true;
}

// Insert into specified block randomly selected block/macro.
// ---------------------------------------------------------------------
// - select macro ID <> current one and <> main block
// - input: replace input args with randomly selected written regs for place of insert (use type info)
//          use instruction distance for probability calc, 
//          ? use unique regs
// - output: replace #0 with input register no from next instruction(s) - if type matched 
//           if no possible #reg-no found: leave as #0
bool sgpGasmOperatorMutate::mutateGenomeCodeMacroInsert(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  uint instrOffset, uint blockInputCount, sgpEntityForGasm *workInfo, uint genomeIndex, double &mutCost)
{  
  scDataNode blockCode, newBlockCode;
  sgpProgramCode code;
  //scDataNode fullCode;
  uint targetBlockIndex = genomeIndex - getCodeBlockOffset(workInfo);
  mutCost = 0.1;

  //workInfo->getProgramCode(fullCode);
  //code.setFullCode(fullCode);
  workInfo->getProgramCode(code);
  
  if (
       (code.getBlockCount() < 2) 
       ||
       (
         (code.getBlockCount() == 2)
         &&
         (targetBlockIndex != SG_MUT_MAIN_BLOCK_INDEX)
       )
     )    
  {   
    return false;
  }
    
  // find ID of block to be inserted  
  uint macroBlockIndex;  
  do {
    macroBlockIndex = randomUInt(1, code.getBlockCount() - 1);
  } while(((macroBlockIndex == targetBlockIndex) || (macroBlockIndex == SG_MUT_MAIN_BLOCK_INDEX)));

  // prepare input 
  // 1. find written regs
  scDataNode writtenRegsWithDist;
  findWrittenRegsWithDist(info, genome, instrOffset, writtenRegsWithDist);      
  processRegDataTypes(writtenRegsWithDist);
  // 2. add info about input of the current block
  appendInputArgsWithDist(code, targetBlockIndex, genome.size(), writtenRegsWithDist);  
  // 3. for each input reg from macro:
  // - find matching list of possible regs
  // - select with usage of probability reg
  // - replace input reg in macro with the new one
  scDataNode macroInputMeta;
  code.getBlockMetaInfoForInput(macroBlockIndex, macroInputMeta);
  code.getBlockCode(macroBlockIndex, newBlockCode);

  // do not allow to insert macro if main block is smaller then the target block    
#ifdef OPT_LIMIT_MACRO_SIZE_BY_MAIN  
  if (!canAddCodeToGenome(workInfo, code, genome, genomeIndex))
    return false;
#endif
  
  sgpGaGenome macroGenomeCode;
  newBlockCode.copyTo(macroGenomeCode);
  
  if (macroGenomeCode.empty())
    return false;

  sgpGaGenomeMetaList macroInfo;
  sgpEntityForGasm::buildMetaForCode(macroGenomeCode, macroInfo);

  mutCost = static_cast<double>(countGenomeTypes(macroInfo, ggtInstrCode));

  // replace macro's input arguments with already written regs
  macroReplaceInputRegs(macroInputMeta, writtenRegsWithDist, macroInfo, macroGenomeCode);

  // prepare output
  scDataNode macroOutputMeta;
  code.getBlockMetaInfoForInput(macroBlockIndex, macroOutputMeta);
  if (!macroOutputMeta.empty()) {
    uint outputRegNo;
    uint outputDataType = sgpVMachine::castDataNodeToGasmType(macroOutputMeta[0].getValueType());
    // 1. find register that can be used as output (use data type matching)
    outputRegNo = macroFindMatchingInputArgReg(info, genome, instrOffset, outputDataType, SGP_REGB_OUTPUT);
    // 2. replace old macro output #0 with found register no
    macroReplaceOutRegInCode(SGP_REGB_OUTPUT, outputRegNo, 0, macroInfo, macroGenomeCode);
  }
  
  // insert macro code
  genome.insert(genome.begin() + instrOffset,
      macroGenomeCode.begin(),
      macroGenomeCode.end());

#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-mut-code-mac-ins-done", m_counters.getUInt("gx-mut-code-mac-ins-done", 0)+1);
#endif    

  return true;
}

void sgpGasmOperatorMutate::buildMacroInstr(sgpGaGenome &newCode, uint macroNo, uint outputRegNo, const scDataNode &inputArgs)
{
  scDataNodeValue newCell; 
  scDataNode newCellNode;

  newCode.clear();
  // add instr code
  uint instrCodeRaw = getInstrCode("macro");
  uint instrCode = sgpVMachine::encodeInstr(instrCodeRaw, 2 + inputArgs.size());
  newCell.setAsUInt(instrCode);  
  newCode.push_back(newCell);

  // add block-no = genomeIndex  
  newCell.setAsUInt(macroNo);
  newCode.push_back(newCell);

  // add output = #0  
  newCellNode = sgpVMachine::buildRegisterArg(outputRegNo);
  newCell.copyFrom(newCellNode);
  newCode.push_back(newCell);

  // add input argument(s)
  for(uint i=0,epos = inputArgs.size(); i!=epos; i++) {
    newCellNode = sgpVMachine::buildRegisterArg(inputArgs.getUInt(i));
    newCell.copyFrom(newCellNode);
    newCode.push_back(newCell);
  }
}
      
// find first input register that can be used as output (use data type matching)
uint sgpGasmOperatorMutate::macroFindMatchingInputArgReg(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  cell_size_t instrOffset, uint dataTypes, uint defValue)
{
  uint res = defValue;
  
  uint instrCode, instrCodeRaw, argCount;  
  scDataNode argMeta, args, newArg;
  uint ioMode, argDataTypes;
  uint regNo;
  sgpFunction *functor; 
  cell_size_t instrOffsetForScan;
  cell_size_t endOffset;
  
  std::set<uint> writtenRegs;

  instrOffsetForScan = instrOffset;
  endOffset = genome.size();
  
  while(instrOffsetForScan < endOffset) {
    if (info[instrOffsetForScan].userType == ggtInstrCode) {
      instrCode = genome[instrOffsetForScan].getAsUInt();
      sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
      functor = ::getFunctorForInstrCode(m_functions, instrCodeRaw);
      sgpGasmCodeProcessor::getArguments(genome, instrOffsetForScan+1, argCount, args);
      argCount = SC_MIN(argCount, args.size());
      
      if ((functor != SC_NULL) && functor->getArgMeta(argMeta)) {
        for(uint argNo=0,epos=SC_MIN(argCount, argMeta.size()); argNo != epos; argNo++)
        {
          ioMode = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_IO_MODE);    
          argDataTypes = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_DATA_TYPE);    
          
          if (info[instrOffsetForScan+1+argNo].userType == ggtRegNo) {
            regNo = sgpVMachine::getRegisterNo(args[argNo]);
            if ((ioMode & gatfOutput) != 0) {
              writtenRegs.insert(regNo);
            } else {
              // input reg             
              if (((argDataTypes & dataTypes) != 0) && (writtenRegs.find(regNo) == writtenRegs.end())) {
              // unwritten match found
                res = regNo;
                instrOffsetForScan = endOffset;
                break; // for argNo
              }  
            }
          }
        } // for
      } // if functor OK
    }
    instrOffsetForScan++;
  } // while
  return res;
}

// replace old reg-no with new reg-no (only on output)
void sgpGasmOperatorMutate::macroReplaceOutRegInCode(uint oldRegNo, uint newRegNo, cell_size_t instrOffset, 
  const sgpGaGenomeMetaList &info, sgpGaGenome &genome)
{
  uint instrCode, instrCodeRaw, argCount;  
  scDataNode argMeta, args, newArg;
  uint ioMode;
  uint regNo;
  sgpFunction *functor; 
  cell_size_t instrOffsetForScan;
  cell_size_t endOffset;

  instrOffsetForScan = instrOffset;
  endOffset = genome.size();
  
  while(instrOffsetForScan < endOffset) {
    if (info[instrOffsetForScan].userType == ggtInstrCode) {
      instrCode = genome[instrOffsetForScan].getAsUInt();
      sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
      functor = ::getFunctorForInstrCode(m_functions, instrCodeRaw);
      sgpGasmCodeProcessor::getArguments(genome, instrOffsetForScan+1, argCount, args);
      argCount = SC_MIN(argCount, args.size());
      
      if ((functor != SC_NULL) && functor->getArgMeta(argMeta)) {
        for(uint argNo=0,epos=SC_MIN(argCount, argMeta.size()); argNo != epos; argNo++)
        {
          ioMode = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_IO_MODE);    
          
          if (info[instrOffsetForScan+1+argNo].userType == ggtRegNo) {
            regNo = sgpVMachine::getRegisterNo(args[argNo]);
            if ((ioMode & gatfOutput) != 0) {
            // output reg
              if (regNo == oldRegNo) {
              // replace arg  
                genome[instrOffsetForScan + 1 + argNo] =
                  sgpVMachine::buildRegisterArg(newRegNo);
              }
            }
          }
        } // for
      } // if functor OK
    }
    instrOffsetForScan++;
  } // while
}

// Find written regs & store distance (in instr. count)
// Start from instrOffset and go backward
// Input: genome, info, instrOffset
// Output: writtenRegWithDistance (list of tuple: reg-no, distance-in-instr, data-type)
void sgpGasmOperatorMutate::findWrittenRegsWithDist(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  uint instrOffset, scDataNode &writtenRegWithDistance)
{
  cell_size_t instrOffsetForScan, endOffset;
  uint instrDist = 1;  
  std::auto_ptr<scDataNode> newTuple;
  uint instrCode, instrCodeRaw, argCount;  
  scDataNode argMeta, args, newArg;
  uint ioMode, argDataTypes;
  uint regNo;
  sgpFunction *functor; 
  
  endOffset = instrOffset;
  
  if (endOffset > 0) {
    instrOffsetForScan = endOffset - 1;
    while((instrOffsetForScan >= instrOffset)) {
      if (info[instrOffsetForScan].userType == ggtInstrCode) {
        instrCode = genome[instrOffsetForScan].getAsUInt();
        sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
        functor = ::getFunctorForInstrCode(m_functions, instrCodeRaw);
        sgpGasmCodeProcessor::getArguments(genome, instrOffsetForScan+1, argCount, args);
        argCount = SC_MIN(argCount, args.size());
        
        if ((functor != SC_NULL) && functor->getArgMeta(argMeta)) {
          for(uint argNo=0,epos=SC_MIN(argCount, argMeta.size()); argNo != epos; argNo++)
          {
            ioMode = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_IO_MODE);    
            argDataTypes = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_DATA_TYPE);    
            argDataTypes &= m_supportedDataTypes;    
            
            if (info[instrOffsetForScan+1+argNo].userType == ggtRegNo) {
              regNo = sgpVMachine::getRegisterNo(args[argNo]);
              if (((ioMode & gatfOutput) != 0) && (!writtenRegWithDistance.hasChild(toString(regNo)))) {
                newTuple.reset(new scDataNode(toString(regNo)));
                newTuple->addChild(new scDataNode(regNo));
                newTuple->addChild(new scDataNode(instrDist));
                newTuple->addChild(new scDataNode(argDataTypes));
                writtenRegWithDistance.addChild(newTuple.release());                
              } // if output
            } // if register
          } // for
        } // if functor OK

        instrDist++;
      } // if instruction 
      if (instrOffsetForScan > 0) 
        instrOffsetForScan--;
      else 
        break;  
    } // while
  } // if endoffset ok
}  

void sgpGasmOperatorMutate::appendInputArgsWithDist(const sgpProgramCode& code, uint codeBlockIndex, uint distance,
  scDataNode &writtenRegWithDistance)
{  
  scDataNode inputArgs;
  std::auto_ptr<scDataNode> newTuple;
  
  code.getBlockMetaInfoForInput(codeBlockIndex, inputArgs);
  for(uint i=0, epos = inputArgs.size(); i != epos; i++) {
    newTuple.reset(new scDataNode());
    newTuple->addChild(new scDataNode(SGP_REGB_INPUT + i));
    newTuple->addChild(new scDataNode(distance));
    newTuple->addChild(new scDataNode(static_cast<uint>(sgpVMachine::castDataNodeToGasmType(inputArgs[i].getValueType()))));
    writtenRegWithDistance.addChild(newTuple.release());                
  }  
}

// compare & filter stored register data types with data types provided by vmachine
void sgpGasmOperatorMutate::processRegDataTypes(scDataNode &regsWithDistance)
{
  uint dataTypes, regDefType;
  
  for(uint i=0, epos = regsWithDistance.size(); i != epos; i++) {
    dataTypes = regsWithDistance[i].getUInt(WRITTEN_REGS_DATA_TYPE);
    regDefType = m_machine->getRegisterDefaultDataType(regsWithDistance[i].getUInt(WRITTEN_REGS_REGNO));;
    if (regDefType != gdtfVariant)
      dataTypes &= regDefType;
    regsWithDistance[i].setUInt(WRITTEN_REGS_DATA_TYPE, dataTypes);    
  }
}

// For each macro's input argument:
// - find matching list of possible regs using data type
// - select with usage of probability reg
// - replace input reg in macro with the new one
void sgpGasmOperatorMutate::macroReplaceInputRegs(const scDataNode& macroInputMeta, const scDataNode& writtenRegsWithDist, 
  const sgpGaGenomeMetaList &macroInfo, sgpGaGenome& blockCode)
{
  const uint WRITTEN_REGS_PROP_REGNO = 0;
  const uint WRITTEN_REGS_PROP_DIST = 1;
  const uint WRITTEN_REGS_PROP_DATATYPE = 2;

  uint argDataType;
  uint regDataTypes;
  std::vector<uint> allowedItems;
  std::set<uint> usedItems;
  sgpGasmProbList regProbs;
  uint newRegNo, newRegNoIdx;
  
  for(uint i=0, epos = macroInputMeta.size(); i != epos; i++)
  {      
    if (usedItems.size() == writtenRegsWithDist.size())
      usedItems.clear(); // restart buffer

    argDataType = sgpVMachine::castDataNodeToGasmType(macroInputMeta[i].getValueType());
    // find list of possible written regs
    allowedItems.clear();
    regProbs.clear();
    for(uint j=0, eposj = writtenRegsWithDist.size(); j != eposj; j++) {
      regDataTypes = writtenRegsWithDist[j].getUInt(WRITTEN_REGS_PROP_DATATYPE);
      if (
           ((regDataTypes & argDataType) != 0)
           && 
           (usedItems.find(j) == usedItems.end())
         )  
      {
        allowedItems.push_back(j);
        // distance is 1..k
        // prob(dist) = 1 / (2^dist)
        regProbs.push_back(1.0 / pow(2.0, static_cast<double>(writtenRegsWithDist[j].getUInt(WRITTEN_REGS_PROP_DIST))));
      }  
    }
    // select register
    if (!regProbs.empty()) {
      newRegNoIdx = ::selectProbItem(regProbs);
      newRegNoIdx = allowedItems[newRegNoIdx];
      newRegNo = writtenRegsWithDist[newRegNoIdx].getUInt(WRITTEN_REGS_PROP_REGNO);
      usedItems.insert(newRegNoIdx);
    } else {
      newRegNo = SGP_REGB_INPUT + i;
    }    
    // replace in code
    macroReplaceArgRegReadInCode(SGP_REGB_INPUT + i, newRegNo, 0, macroInfo, blockCode);
  }
}

// Replace all usage of a given old reg-no by a new one (only on input side).
// Do it until first write to old reg-no has been found.
void sgpGasmOperatorMutate::macroReplaceArgRegReadInCode(uint oldRegNo, uint newRegNo, cell_size_t instrOffset, 
  const sgpGaGenomeMetaList& info, sgpGaGenome& genome)
{
  uint instrCode, instrCodeRaw, argCount;  
  scDataNode argMeta, args, newArg;
  uint ioMode;
//  uint argDataTypes;
  uint regNo;
  sgpFunction *functor; 
  cell_size_t instrOffsetForScan;
  cell_size_t endOffset;

  instrOffsetForScan = instrOffset;
  endOffset = genome.size();
  
  while(instrOffsetForScan < endOffset) {
    if (info[instrOffsetForScan].userType == ggtInstrCode) {
      instrCode = genome[instrOffsetForScan].getAsUInt();
      sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
      functor = ::getFunctorForInstrCode(m_functions, instrCodeRaw);
      sgpGasmCodeProcessor::getArguments(genome, instrOffsetForScan+1, argCount, args);
      argCount = SC_MIN(argCount, args.size());
      
      if ((functor != SC_NULL) && functor->getArgMeta(argMeta)) {
        for(uint argNo=0,epos=SC_MIN(argCount, argMeta.size()); argNo != epos; argNo++)
        {
          ioMode = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_IO_MODE);    
          
          if (info[instrOffsetForScan+1+argNo].userType == ggtRegNo) {
            regNo = sgpVMachine::getRegisterNo(args[argNo]);
            if ((ioMode & gatfOutput) != 0) {
            // output reg
              if (regNo == oldRegNo) {
              // stop processing
                instrOffsetForScan = endOffset;
                break;
              }
            } else {
            // input reg
              if (regNo == oldRegNo) {
              // replace arg  
                genome[instrOffsetForScan + 1 + argNo] =
                  sgpVMachine::buildRegisterArg(newRegNo);
              }
            }
          }
        } // for
      } // if functor OK
    }
    instrOffsetForScan++;
  } // while
}

// delete specified block/macro
bool sgpGasmOperatorMutate::mutateGenomeCodeMacroDelete(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  sgpEntityForGasm *workInfo, uint genomeIndex, double &mutCost)
{  
  scDataNode blockCode, newBlockCode;
  sgpProgramCode code;
  uint codeBlockIndex = genomeIndex - getCodeBlockOffset(workInfo);  
  
#ifndef OPT_MACRO_DEL_ENABLED 
  return false;
#endif
  
#ifdef OPT_LOW_MACRO_DEL_PROB  
  if (workInfo->getGenomeCount() <= 1) 
    return false;
  double changeProb = 1.0 / static_cast<double>(workInfo->getGenomeCount() - 1);
  if (!randomFlip(changeProb))
    return false;
#endif
    
  mutCost = static_cast<double>(countGenomeTypes(info, ggtInstrCode));

  workInfo->getProgramCode(code);
  assert(codeBlockIndex != 0);
  code.eraseBlock(codeBlockIndex);
  workInfo->setProgramCode(code.getFullCode());
  
  genome.clear();
  return true;
}

// swap two instructions, prior or next to the selected one
bool sgpGasmOperatorMutate::mutateGenomeCodeSwapInstr(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  int varIndex, uint offset, uint instrOffset)
{
  bool res = false;
  uint instrOffset2 = instrOffset;

  if (instrOffset > 0) {
    instrOffset2 = findInstrBeginBackward(info, instrOffset - 1);
  }  
  if (instrOffset2 == instrOffset) {
    instrOffset2 = findInstrBeginForward(info, instrOffset+1); 
  }
  
  if (
       (instrOffset2 != instrOffset)
       &&
       (instrOffset2 < genome.size())
       &&
       (instrOffset < genome.size())
     )  
  {
    uint instrCode1, instrCode2;
    instrCode1 = genome[instrOffset].getAsUInt();
    instrCode2 = genome[instrOffset2].getAsUInt();
    uint instrCodeRaw, argCnt1, argCnt2;
    
    sgpVMachine::decodeInstr(instrCode1, instrCodeRaw, argCnt1);    
    sgpVMachine::decodeInstr(instrCode2, instrCodeRaw, argCnt2);    
    swapGenomeBlocks(genome, instrOffset, argCnt1+1, instrOffset2, argCnt2+1); 
    res = true;
  }
  return res;
}  

// Split one instruction into two instructions. 
// New instruction is placed AFTER the old one.
// ---------------------------------------------------------------------------
// Summary
// In general, there are the following ways of splitting instruction:
// a) new instruction is before the old one: sin(2*x) => sin((1+x)*x)
// b) new instruction is after the old one: sin(2*x) => sin(2*x)+2
//------------------------
// Version (a) = INSERT
// - insert new random instruction before the current one
// - use output of instruction as argument in the old one
// => mut-insert
//------------------------
// Version (b) = SPLIT
// - insert new random instruction after the current one
// - if old instruction output can be read:
//   - set random input arg of new instruction to reg-no from output of old one
//   - set output of new instruction to the same as it was in old instruction
// - otherwise (output cannot be read, e.g. #0)
//   - select random reg-no for output of old instruction
//   - copy output reg no of old instruction to new instruction
//   - modify output of old instruction to selected random reg-no
//   - modify random input arg of new instruction to selected random reg-no
bool sgpGasmOperatorMutate::mutateGenomeCodeSplitInstr(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  int varIndex, uint offset, uint instrOffset, uint blockInputCount, const scDataNode &argMeta, 
  const sgpEntityForGasm *workInfo, uint genomeIndex)
{
  uint targetBlockIndex = genomeToBlockIndex(workInfo, genomeIndex);
  sgpGaGenome newCode;

  sgpGasmRegSet writtenRegs;  
  findWrittenRegs(info, genome, instrOffset, blockInputCount, writtenRegs);

  sgpGasmProbList instrCodeProbs;  
  readMutInstrCodeProbs(workInfo, instrCodeProbs);  
  
  if (sgpGasmCodeProcessor::buildRandomInstr(m_functions, writtenRegs, m_supportedDataTypes, m_supportedDataNodeTypes, instrCodeProbs, 
    m_machine, workInfo, &targetBlockIndex, newCode)) 
  {
    uint outputOld;
    if (!getOutputRegNo(genome, instrOffset, argMeta, outputOld))
      return false;
      
    uint outputNew;
    scDataNode argMetaNew;
    if (!getArgMetaForInstr(newCode, 0, argMetaNew))
      return false;
      
    if (m_machine->canReadRegister(outputOld)) {
      setRandomInputArgAsRegNo(newCode, 0, argMetaNew, outputOld);
      setOutputRegNo(newCode, 0, argMetaNew, outputOld);
    } else {      
// find set of possible registers for output of new instr
      if (!getOutputRegNo(newCode, 0, argMetaNew, outputNew))
        return false;

// -- find new output reg for old instruction
      uint oldOutDataType = m_machine->getRegisterDefaultDataType(outputOld);      
      uint searchDataType, outSetOld, outSetNew;

      if (oldOutDataType == gdtfVariant)
        searchDataType = m_supportedDataTypes;
      else
        searchDataType = m_supportedDataTypes & oldOutDataType;
          
      outSetOld = getInstrOutputDataTypeSet(argMeta);
      outSetNew = getInstrOutputDataTypeSet(argMetaNew);
      
      if ((outSetOld & outSetNew) == 0)
        return false;
          
      searchDataType &= outSetOld;
          
      sgpGasmRegSet regSet;
      sgpGasmCodeProcessor::prepareRegisterSet(regSet, searchDataType, gatfOutput, m_machine);
      if (regSet.size() > 1) {
         uint outputNewRaw;
         do {
           outputNewRaw = sgpGasmCodeProcessor::randomRegNo(regSet);
           regSet.erase(regSet.find(outputNewRaw));
         } while ((outputNewRaw == outputOld) && (!m_machine->canReadRegister(outputNewRaw)) && (!regSet.empty()));              
         outputNew = outputNewRaw;         
      } else {
        //outputNew = sgpGasmCodeProcessor::randomRegNo(regSet).getAsUInt();
        outputNew = sgpGasmCodeProcessor::randomRegNo(regSet);
      }

      setOutputRegNo(newCode, 0, argMetaNew, outputOld);
      setOutputRegNo(genome, instrOffset, argMeta, outputNew);
      setRandomInputArgAsRegNo(newCode, 0, argMetaNew, outputNew);
    }
    
    uint instrCode1;
    instrCode1 = genome[instrOffset].getAsUInt();
    uint instrCodeRaw, argCnt1;
    
    sgpVMachine::decodeInstr(instrCode1, instrCodeRaw, argCnt1);    

    genome.insert(genome.begin() + instrOffset + 1 + argCnt1,
      newCode.begin(),
      newCode.end());
    return true;   
  }  
  return false;  
}  

// Link current instruction with next one
// Steps:
// - find next instruction start
// - if exists:
//   - read output reg no from the current instruction
//   - copy this output to next instruction as input argument
bool sgpGasmOperatorMutate::mutateGenomeCodeLinkInstr(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  int varIndex, uint instrOffset, const scDataNode &argMeta, 
  const sgpEntityForGasm *workInfo)
{
  uint outputForNext = 0;
  scDataNode argMetaForNext;
  bool chain;
  uint nextInstrIndex = findInstrBeginForward(info, instrOffset+1);
  
  chain = (nextInstrIndex < genome.size());
  if (chain)
    chain = getOutputRegNo(genome, instrOffset, argMeta, outputForNext);
  if (chain) 
    chain = getArgMetaForInstr(genome, nextInstrIndex, argMetaForNext);
  if (chain) { 
    return setRandomInputArgAsRegNo(genome, nextInstrIndex, argMetaForNext, outputForNext);
  } else {
    return false;
  }    
}  

// join two instructions into one
// - select two instructions
// - choose one of them as a result 
// - copy random args from the second instr
bool sgpGasmOperatorMutate::mutateGenomeCodeJoinInstr(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
  int varIndex, uint offset, uint instrOffset)
{
  bool res = false;
  uint instrOffset2 = instrOffset;
  if (instrOffset > 0) {
    instrOffset2 = findInstrBeginBackward(info, instrOffset - 1);
  }  
  if (instrOffset2 == instrOffset) {
    instrOffset2 = findInstrBeginForward(info, instrOffset+1); 
  }
  if (
       (instrOffset2 != instrOffset)
       &&
       (instrOffset2 < genome.size())
       &&
       (instrOffset < genome.size())
     )  
  {
    uint instrCode1, instrCode2;
    instrCode1 = genome[instrOffset].getAsUInt();
    instrCode2 = genome[instrOffset2].getAsUInt();
    uint instrCodeRaw, argCnt1, argCnt2;
    
    sgpVMachine::decodeInstr(instrCode1, instrCodeRaw, argCnt1);    
    sgpVMachine::decodeInstr(instrCode2, instrCodeRaw, argCnt2);  
    uint minCnt = SC_MIN(argCnt1, argCnt2);  
    bool firstInstr = randomBool();
    if (firstInstr) {
      for(uint i=0,epos=minCnt; i!=epos; i++)
      {
        if (randomBool()) 
          genome[instrOffset+1+i].copyFrom(genome[instrOffset2+1+i]); 
      }
      genome.erase(genome.begin() + instrOffset2, 
                   genome.begin() + instrOffset2 + argCnt2 + 1);
      res = true;             
    } else {
      for(uint i=0,epos=minCnt; i!=epos; i++)
      {
        if (randomBool()) 
          genome[instrOffset2+1+i].copyFrom(genome[instrOffset+1+i]); 
      }
      genome.erase(genome.begin() + instrOffset, 
                   genome.begin() + instrOffset + argCnt1 + 1);
      res = true;             
    }    
  }
  assert(!genome.empty());
  return res;
}  

bool sgpGasmOperatorMutate::getOutputRegNo(const sgpGaGenome &genome, uint instrOffset, const scDataNode &argMeta, uint &regNo)
{
  bool res = false;
  uint ioMode;
  uint argNo = 0;
  while(argNo < argMeta.size()) {
    ioMode = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_IO_MODE);
    if ((ioMode & gatfOutput) != 0) {
      if (instrOffset+argNo+1 < genome.size()) {
        if (genome[instrOffset+argNo+1].getValueType() == vt_uint)
        {
          regNo = sgpVMachine::getRegisterNo(genome[instrOffset+argNo+1]); 
          res = true;
        }  
      }  
      break;
    }
    argNo++;
  }
  return res;
}

// monitor duplicates generated by operators
// add to statistics
void sgpGasmOperatorMutate::monitorDupsFromOperator(const sgpGaGenome &genomeBefore, const sgpGaGenome &genomeAfter, 
  const scString &operTypeName) 
{
  sgpGaGenomeMetaList infoBefore;
  sgpGaGenomeMetaList infoAfter;
  double ratioBefore, ratioAfter;
  sgpEntityForGasm::buildMetaForCode(genomeBefore, infoBefore);
  sgpEntityForGasm::buildMetaForCode(genomeAfter, infoAfter);
  ratioBefore = sgpGasmCodeProcessor::calcDupsRatio(infoBefore, genomeBefore);
  ratioAfter = sgpGasmCodeProcessor::calcDupsRatio(infoAfter, genomeAfter);
  if (ratioBefore < ratioAfter) {
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-dups-"+operTypeName, m_counters.getUInt("gx-dups-"+operTypeName, 0)+1);
#endif    
  }
}
 
bool sgpGasmOperatorMutate::getArgMetaForInstr(const sgpGaGenome &genome, uint instrOffset, scDataNode &argMeta)
{
  uint instrCode = genome[instrOffset].getAsUInt();
  uint instrCodeRaw, argCount;
  sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
  sgpFunction *functor =    
    ::getFunctorForInstrCode(m_functions, instrCodeRaw);
  
  if (functor == SC_NULL)
    return false;
  else        
    return functor->getArgMeta(argMeta);
}

void sgpGasmOperatorMutate::setOutputRegNo(sgpGaGenome &genome, uint instrOffset, const scDataNode &argMeta, uint regNo)
{
  uint ioMode, argType;
  uint argNo = 0;
  while(argNo < argMeta.size()) {
    ioMode = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_IO_MODE);
    argType = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_ARG_TYPE);    
    if (((ioMode & gatfOutput) != 0) && ((argType & gatfRegister) != 0)) {
      scDataNode regNode;
      sgpVMachine::buildRegisterArg(regNode, regNo);
      if (instrOffset+argNo+1 < genome.size()) {
        genome[instrOffset+argNo+1].copyFrom(regNode);
      }  
      break;
    }
    argNo++;
  }      
}

bool sgpGasmOperatorMutate::setRandomInputArgAsRegNo(sgpGaGenome &genome, uint instrOffset, const scDataNode &argMeta, uint regNo)
{
  bool res;
  uint ioMode;
  uint argNo = 0;
  uint inputCnt = 0;
  uint inputNo;
  uint argType;

  for(uint i = 0, epos = argMeta.size(); i != epos; i++) {
    ioMode = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_IO_MODE);
    argType = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_ARG_TYPE);
    if (((ioMode & gatfInput) != 0) && ((argType & gatfRegister) != 0))
      inputCnt++;
  }
  if (inputCnt == 0) {
    res = false;
  } else {
    res = true;   
    inputNo = randomInt(0, inputCnt - 1);

    while(argNo < argMeta.size()) {
      ioMode = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_IO_MODE);
      argType = sgpVMachine::getArgMetaParamUInt(argMeta, argNo, GASM_ARG_META_ARG_TYPE);
      if (((ioMode & gatfInput) != 0) && ((argType & gatfRegister) != 0)) {
        if (inputNo == 0) {
          scDataNode regNode;
          sgpVMachine::buildRegisterArg(regNode, regNo);
          if (instrOffset+argNo+1 < genome.size()) {
            genome[instrOffset+argNo+1].copyFrom(regNode);
          }  
          break;
        } else {
          inputNo--;
        }  
      }
      argNo++;
    }
  }
  return res;  
}

void sgpGasmOperatorMutate::swapGenomeBlocks(sgpGaGenome &genome, uint offset1, uint size1, uint offset2, uint size2)
{
  uint useOffset1, useOffset2;
  uint useSize1, useSize2;
  
  sgpGaGenome tmpVal1, tmpVal2;
  
  if (offset1 <= offset2) {
    useOffset1 = offset1;
    useSize1 = size1;
    useOffset2 = offset2;
    useSize2 = size2;
  } else {
    useOffset1 = offset2;
    useSize1 = size2;
    useOffset2 = offset1;
    useSize2 = size1;
  }

  std::copy (genome.begin() + useOffset1, 
             genome.begin() + useOffset1 + useSize1, 
             std::back_inserter(tmpVal1));

  std::copy (genome.begin() + useOffset2, 
             genome.begin() + useOffset2 + useSize2, 
             std::back_inserter(tmpVal2));

  uint noffset;

  genome.erase(genome.begin() + useOffset1, 
               genome.begin() + useOffset1 + useSize1);

  genome.insert(genome.begin() + useOffset1,
    tmpVal2.begin(),
    tmpVal2.end());

  noffset = uint(int(useOffset2) + (int(useSize2) - int(useSize1)));
   
  genome.erase(genome.begin() + noffset, 
               genome.begin() + noffset + useSize2);

  genome.insert(genome.begin() + noffset,
    tmpVal1.begin(),
    tmpVal1.end());      
  assert(!genome.empty());  
}

// find start of instruction 
uint sgpGasmOperatorMutate::findInstrBeginForward(const sgpGaGenomeMetaList &info, uint aStart)
{
  uint res = aStart;
  uint epos = info.size();
  while ((res != epos) && (info[res].userType != ggtInstrCode))
    res++;
  return res;  
}

// find start of Nth instruction (n >= 1)
uint sgpGasmOperatorMutate::findNthInstrForward(const sgpGaGenomeMetaList &info, uint aStart, uint n)
{
  uint res = aStart;
  uint epos = info.size();
  uint instrCount = 0;
  assert(n >= 1);
  while (res < epos) {
    if (info[res].userType == ggtInstrCode) {
      instrCount++;
      if (instrCount >= n)
        break;
    }
    res++;
  }  
  return res;  
}

uint sgpGasmOperatorMutate::getCodeBlockOffset(const sgpEntityForGasm *workInfo)
{
  if (workInfo->hasInfoBlock())
    return 1;
  else
    return 0;  
}

void sgpGasmOperatorMutate::invokeNextEntity()
{
  if (m_yieldSignal != SC_NULL)
    m_yieldSignal->execute();
}
