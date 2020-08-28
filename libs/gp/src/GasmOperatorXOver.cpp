/////////////////////////////////////////////////////////////////////////////
// Name:        GasmOperatorXOver.cpp
// Project:     scLib
// Purpose:     XOver operator for GP evolution.
// Author:      Piotr Likus
// Modified by:
// Created:     29/12/2009
/////////////////////////////////////////////////////////////////////////////

#define USE_GASM_OPERATOR_STATS

//sc
#include "sc/rand.h"
//other
#include "sgp/GasmOperatorXOver.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

const double DEF_XOVER_GEN_PROB = 0.5;

// Rescale global value basing on local factor
//   (0..0,5) => decrease global value (0.0 - max decrease, 0.4999 - min decrease) 
//   0,5 => no change
//   (0,5..1.0) => increase global value (0,500001 - min increase, 0.9999 - max increase)
// Input: 
//   globalFactor: value (0,0..1,0) 
//   localFactor: value (0,0..1,0) 
// Output: 
//   rescaled value
double recalcGlobalFactorIncDec(double globalFactor, double localFactor)
{
  const double MIN_HELPER = 1E-12;
  double res;
  if (localFactor < 0.5)
  {
    res = globalFactor * (MIN_HELPER + localFactor / 0.5);
  } else {
    res = globalFactor + (1.0 - globalFactor) * ((localFactor - 0.5) / 0.5);
  }
  return res;
}

// ----------------------------------------------------------------------------
// sgpGasmOperatorXOver
// ----------------------------------------------------------------------------
// construction
sgpGasmOperatorXOver::sgpGasmOperatorXOver(): inherited()
{
  m_matchThreshold = SGP_GASM_DEF_SPECIES_THRESHOLD;
  m_matchEnabled = true;
  m_distanceFunction = SC_NULL;
  m_distanceFactor = SGP_GASM_DEF_XOVER_DISTANCE_FACTOR;
  m_fixedGenProb = false;
  sgpEntityForGasm::buildMetaForInfoBlock(m_metaForInfoBlock);
  m_compareTool = SC_NULL;
}

sgpGasmOperatorXOver::~sgpGasmOperatorXOver()
{
}

// properties
void sgpGasmOperatorXOver::setMatchEnabled(bool value)
{
  m_matchEnabled = value;
}

double sgpGasmOperatorXOver::getMatchThreshold()
{  
  return m_matchThreshold;
}

void sgpGasmOperatorXOver::setMatchThreshold(double aValue)
{
  m_matchThreshold = aValue;
}

void sgpGasmOperatorXOver::getCounters(scDataNode &output)
{
  output = m_counters;
}

void sgpGasmOperatorXOver::setCompareTool(sgpGaGenomeCompareTool *tool)
{
  throw scError("Not implemented!");
}

void sgpGasmOperatorXOver::setCompareToolEx(sgpGaGenomeCompareTool *tool)
{
  m_compareTool = tool;
}

void sgpGasmOperatorXOver::setMetaForInfoBlock(const sgpGaGenomeMetaList &value)
{
  m_metaForInfoBlock = value;
}

void sgpGasmOperatorXOver::setDistanceFactor(double value)
{
  m_distanceFactor = value;
}

void sgpGasmOperatorXOver::setDistanceFunction(sgpDistanceFunction *value)
{
  m_distanceFunction = value;
}

void sgpGasmOperatorXOver::setFixedGenProb(bool value)
{
  m_fixedGenProb = value;
}  

// run
void sgpGasmOperatorXOver::init()
{
}

void sgpGasmOperatorXOver::beforeProcess()
{ 
  inherited::beforeProcess();
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.clear();
#endif
}

bool sgpGasmOperatorXOver::canExecute()
{
  return true;
}

bool sgpGasmOperatorXOver::crossGenomes(sgpGaGeneration &newGeneration, uint first, uint second,
  uint newChildCount, uint replaceParentCount)
{ 
  std::auto_ptr<sgpEntityForGasm> firstInfoCopyGuard(
    checked_cast<sgpEntityForGasm *>(newGeneration.newItem(newGeneration.at(first)))); 

  std::auto_ptr<sgpEntityForGasm> secondInfoCopyGuard(
    checked_cast<sgpEntityForGasm *>(newGeneration.newItem(newGeneration.at(second)))); 
      
  double firstProb, secondProb;
  double firstThreshold, secondThreshold;
  double genProb, useThreshold;
  
#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-xover-a-tries", m_counters.getUInt("gx-xover-a-tries", 0)+1);
#endif
  
  getGenomeXCrossInfo(
    checked_cast_ref<sgpEntityForGasm &>(newGeneration.at(first)),
    firstProb, firstThreshold);
  
  getGenomeXCrossInfo(
    checked_cast_ref<sgpEntityForGasm &>(newGeneration.at(second)),
    secondProb, secondThreshold);

  genProb = (firstProb + secondProb) / 2.0;
  useThreshold = (firstThreshold + secondThreshold) / 2.0;

  uint crossedCnt = 0;
  crossedCnt = doCrossGenomes(newGeneration, first, second, *firstInfoCopyGuard, *secondInfoCopyGuard, genProb, useThreshold);
  bool performed = (crossedCnt > 0);

#ifdef USE_GASM_OPERATOR_STATS
  m_counters.setUIntDef("gx-xover-b-crossed-gens", m_counters.getUInt("gx-xover-b-crossed-gens", 0)+crossedCnt);
#endif    
 
  uint replacedIdx1, replacedIdx2;
  uint newIdx1, newIdx2;
   
  if (performed)
    mergeChildren(replaceParentCount, newGeneration, first, second, firstInfoCopyGuard.release(), secondInfoCopyGuard.release(), newIdx1, newIdx2, replacedIdx1, replacedIdx2);
   
#ifdef TRACE_ENTITY_BIO
  if (performed) {
    traceGenomesCrossed(newGeneration, first, second, newIdx1, newIdx2, replacedIdx1, replacedIdx2);
  }  
#endif      

#ifdef USE_GASM_OPERATOR_STATS
  if (performed)
    m_counters.setUIntDef("gx-xover-b-crossed-items", m_counters.getUInt("gx-xover-b-crossed-items", 0)+1);
#endif

#ifdef TRACE_GASM_XOVER
  bool hasInfoBlock = checked_cast<sgpEntityForGasm &>(newGeneration.at(first)).hasInfoBlock();
  if (performed && ((firstStartGenNo != SGP_GASM_INFO_BLOCK_IDX) || (!hasInfoBlock))) {
    signalEntityChanged(newGeneration, first);
    signalEntityChanged(newGeneration, second);
  }  
#endif      

  return true;  
}

void sgpGasmOperatorXOver::traceGenomesCrossed(sgpGaGeneration &newGeneration, uint first, uint second,uint newIdx1, uint newIdx2, uint replacedIdx1, uint replacedIdx2)
{
#ifdef TRACE_ENTITY_BIO
    uint parentHandle1, parentHandle2;
    uint childHandle1, childHandle2;
    uint replacedLimit = newGeneration.size();
    bool replaced1 = (replacedIdx1 >= replacedLimit);
    bool replaced2 = (replacedIdx2 >= replacedLimit);
    
    parentHandle1 = sgpEntityTracer::getEntityHandle(first);
    parentHandle2 = sgpEntityTracer::getEntityHandle(second);

    const sgpEntityForGasm &firstWorkInfo = checked_cast<sgpEntityForGasm &>(newGeneration.at(newIdx1));
    const sgpEntityForGasm &secondWorkInfo = checked_cast<sgpEntityForGasm &>(newGeneration.at(newIdx2));
    
    sgpEntityTracer::handleEntityBorn(newIdx1, firstWorkInfo, "xover");
    sgpEntityTracer::handleEntityBorn(newIdx2, secondWorkInfo, "xover");
    
    childHandle1 = sgpEntityTracer::getEntityHandle(newIdx1);
    childHandle2 = sgpEntityTracer::getEntityHandle(newIdx2);    
    
    scDataNode parents(new scDataNode(parentHandle1), new scDataNode(parentHandle2));
    scDataNode childList(new scDataNode(childHandle1), new scDataNode(childHandle2));
    sgpEntityTracer::handleXOver(parents, childList);

    if (replaced1)           
      sgpEntityTracer::handleEntityDeathByHandle(parentHandle1, eecXOver, "xover");
    if (replaced2)  
      sgpEntityTracer::handleEntityDeathByHandle(parentHandle2, eecXOver, "xover");    
#endif
}


void sgpGasmOperatorXOver::getGenomeXCrossInfo(const sgpEntityForGasm &info, 
  double &aProb, double &aMatchThreshold)
{   
  bool foundProb = false;
  bool foundThreshold = false;
   
  if (info.hasInfoBlock())
  {
    if (!m_fixedGenProb)
      foundProb = info.getInfoDouble(SGP_INFOBLOCK_IDX_MUT_MAIN_PARAMS_BASE + SGP_INFOBLOCK_IDX_XCROSS_PROB, aProb);
    foundThreshold = info.getInfoDouble(SGP_INFOBLOCK_IDX_MUT_MAIN_PARAMS_BASE + SGP_INFOBLOCK_IDX_MATCH_RATE, aMatchThreshold);
  } 
  
  if (m_fixedGenProb || !foundProb)
    aProb = DEF_XOVER_GEN_PROB; 
  
  if (foundThreshold) {  
    aMatchThreshold = ::recalcGlobalFactorIncDec(m_matchThreshold, aMatchThreshold);
  } else {  
    aMatchThreshold = m_matchThreshold;
  }    
}

bool sgpGasmOperatorXOver::doCrossGen(uint genNo, 
  sgpEntityBase &firstEntity, sgpEntityBase &secondEntity)
{  
  sgpGaGenome genFirst, genSecond;
  sgpEntityForGasm &firstInfo = checked_cast_ref<sgpEntityForGasm &>(firstEntity);
  sgpEntityForGasm &secondInfo = checked_cast_ref<sgpEntityForGasm &>(secondEntity);

  firstInfo.getGenome(genNo, genFirst);
  secondInfo.getGenome(genNo, genSecond);

#ifdef KEEP_OLD_GENOMES
  sgpGaGenome genFirstBefore = genFirst;
  sgpGaGenome genSecondBefore = genSecond;
#endif  
  
  sgpGaGenomeMetaList *firstMetaInfo, *secondMetaInfo;
  std::auto_ptr<sgpGaGenomeMetaList> firstMetaInfoGuard, secondMetaInfoGuard;

  bool firstInfoBlock = (genNo == 0) && 
    firstInfo.hasInfoBlock();
  bool secondInfoBlock = (genNo == 0) && 
    secondInfo.hasInfoBlock();
  
  if (firstInfoBlock)
    firstMetaInfo = &m_metaForInfoBlock;
  else {   
    firstMetaInfoGuard.reset(new sgpGaGenomeMetaList());
    sgpEntityForGasm::buildMetaForCode(genFirst, *firstMetaInfoGuard);
    firstMetaInfo = firstMetaInfoGuard.get();
  }  

  if (secondInfoBlock)
    secondMetaInfo = &m_metaForInfoBlock;
  else {   
    secondMetaInfoGuard.reset(new sgpGaGenomeMetaList());
    sgpEntityForGasm::buildMetaForCode(genSecond, *secondMetaInfoGuard);
    secondMetaInfo = secondMetaInfoGuard.get();
  }  
  
  uint firstSize = genFirst.size();
  uint secondSize = genSecond.size();
  
  if ((firstSize == 0) || (secondSize == 0))
    return false;

  uint firstStartVarNo = randomInt(1, SC_MIN(firstSize-1, secondSize-1));
  if (!firstInfoBlock)
    firstStartVarNo = findInstrBeginForward(*firstMetaInfo, firstStartVarNo);
  if (firstStartVarNo >= firstSize) {
    firstStartVarNo = findInstrBeginBackward(*firstMetaInfo, firstSize - 1);
  }    
  uint firstEndPos = randomInt(firstStartVarNo+1, firstSize);
  if (!firstInfoBlock && (firstEndPos < firstSize)) 
    firstEndPos = findInstrBeginForward(*firstMetaInfo, firstEndPos);

  uint secondStartVarNo;
  
  if (firstEndPos - firstStartVarNo > secondSize) 
    return false;
  if (firstStartVarNo == 0) // single-instruction code
    return false;  
    
  secondStartVarNo = firstStartVarNo;
  if (!secondInfoBlock && (secondStartVarNo < secondSize)) 
    secondStartVarNo = findInstrBeginForward(*secondMetaInfo, secondStartVarNo);
    
  if (!secondInfoBlock && (secondStartVarNo >= secondSize)) {
    secondStartVarNo = findInstrBeginBackward(*secondMetaInfo, secondSize - 1);
  }  
  uint secondEndPos = SC_MIN(secondSize, secondStartVarNo+(firstEndPos - firstStartVarNo));
  if (!secondInfoBlock && (secondEndPos < secondSize)) 
    secondEndPos = findInstrBeginForward(*secondMetaInfo, secondEndPos);
  
  if (secondEndPos <= secondStartVarNo)
    return false;
    
  sgpGaGenome tmpVal;

  std::copy (genFirst.begin() + firstStartVarNo, 
           genFirst.begin() + firstEndPos, 
           std::back_inserter(tmpVal));

  genFirst.erase(genFirst.begin() + firstStartVarNo, 
                 genFirst.begin() + firstEndPos);

  genFirst.insert(genFirst.begin() + firstStartVarNo,
    genSecond.begin() + secondStartVarNo,
    genSecond.begin() + secondEndPos);

  genSecond.erase(genSecond.begin() + secondStartVarNo, 
                  genSecond.begin() + secondEndPos);

  genSecond.insert(genSecond.begin() + secondStartVarNo, 
    tmpVal.begin(),
    tmpVal.end());

#ifdef USE_GASM_OPERATOR_STATS
  if (firstInfoBlock)
    m_counters.setUIntDef("gx-xover-c-info-done", m_counters.getUInt("gx-xover-c-info-done", 0)+1);
  else  
    m_counters.setUIntDef("gx-xover-c-code-done", m_counters.getUInt("gx-xover-c-code-done", 0)+1);
#endif    

  firstInfo.setGenome(genNo, genFirst);
  secondInfo.setGenome(genNo, genSecond);
    
#ifdef USE_GASM_OPERATOR_STATS
#ifdef VALIDATE_DUPS    
  monitorDupsFromOperator(genFirstBefore, genFirst, "xover-1");
  monitorDupsFromOperator(genSecondBefore, genSecond, "xover-2");
#endif  
#endif    

  assert(!genFirst.empty());
  assert(!genSecond.empty());
  return true;
}

// find start of instruction 
uint sgpGasmOperatorXOver::findInstrBeginForward(const sgpGaGenomeMetaList &info, uint aStart)
{
  uint res = aStart;
  uint epos = info.size();
  while ((res < epos) && (info[res].userType != ggtInstrCode))
    res++;
  return res;  
}

uint sgpGasmOperatorXOver::findInstrBeginBackward(const sgpGaGenomeMetaList &info, uint startOffset)
{
  uint res = startOffset;
  while(res > 0) {
    if (info[res].userType == ggtInstrCode) 
      break;
    res--;
  }    
  return res;
}

// monitor duplicates generated by operators
// add to statistics
void sgpGasmOperatorXOver::monitorDupsFromOperator(const sgpGaGenome &genomeBefore, const sgpGaGenome &genomeAfter, 
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
  m_counters.setUIntDef("dups-"+operTypeName, m_counters.getUInt("dups-"+operTypeName, 0)+1);
#endif    
  }
}

double sgpGasmOperatorXOver::calcGenomeDiff(const sgpGaGeneration &newGeneration, uint first, uint second, uint genNo)
{
  return m_compareTool->calcGenomeDiff(newGeneration, first, second, genNo);
}


