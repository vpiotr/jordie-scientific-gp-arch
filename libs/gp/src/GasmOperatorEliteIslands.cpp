/////////////////////////////////////////////////////////////////////////////
// Name:        GasmOperatorEliteIslands.cpp
// Project:     scLib
// Purpose:     Elite operator using island information.
// Author:      Piotr Likus
// Modified by:
// Created:     16/02/2010
/////////////////////////////////////////////////////////////////////////////
#include "sgp/GasmOperatorEliteIslands.h"
#include "sgp/GasmOperator.h"
#include "sgp/GasmIslandCommon.h"
#include "sgp/ExperimentConst.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

sgpGaOperatorEliteIslands::sgpGaOperatorEliteIslands()
{
  m_experimentParams = SC_NULL;
}

sgpGaOperatorEliteIslands::~sgpGaOperatorEliteIslands()
{
}

// properties
void sgpGaOperatorEliteIslands::setIslandLimit(uint value)
{
  m_islandLimit = value;
}

void sgpGaOperatorEliteIslands::setExperimentParams(const sgpGaExperimentParams *params)
{
  m_experimentParams = params;
}

void sgpGaOperatorEliteIslands::setIslandTool(sgpEntityIslandToolIntf *value)
{
  m_islandTool = value;
}

// run
void sgpGaOperatorEliteIslands::execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit)
{
  if (limit == 0)
    return;

  scDataNode islandItems;
  
  intPrepareIslandMap(input, m_islandLimit, islandItems);
  scString islandName;
  
  for(uint i = 0, epos = m_islandLimit; i != epos; i++) 
  {
    islandName = toString(i);
    if (islandItems.hasChild(islandName))
      executeOnIsland(i, input, islandItems[islandName], output, limit);
  }
}

void sgpGaOperatorEliteIslands::intPrepareIslandMap(const sgpGaGeneration &input, uint islandLimit, scDataNode &output)
{
  assert(m_islandTool != SC_NULL);
  m_islandTool->prepareIslandMap(input, output);
}

void sgpGaOperatorEliteIslands::executeOnIsland(uint islandId, sgpGaGeneration &input, const scDataNode &islandItems, sgpGaGeneration &output, uint limit)
{
  sgpEntityIndexList idList;
  uint addedCnt;
  uint entityIndex;

  getTopGenomesForIsland(input, islandId, limit, islandItems, idList);  

  if (!idList.empty()) {
    addedCnt = 0;
  
    while(addedCnt < limit) 
    {
      entityIndex = idList[addedCnt % idList.size()];
      output.insert(output.newItem(input.at(entityIndex)));
      addedCnt++;
#ifdef TRACE_ENTITY_BIO
  sgpEntityTracer::handleEntityMovedBuf(entityIndex, output.size() - 1, "elite-islands");
#endif      
    }
  }
}

void sgpGaOperatorEliteIslands::getTopGenomesForIsland(const sgpGaGeneration &input, uint islandId, uint limit, const scDataNode &islandItems, sgpEntityIndexList &idList)
{
  sgpWeightVector islandWeights;

  getIslandObjectiveWeights(islandId, islandWeights);
  
  sgpEntityIndexSet islandItemSet;
  for(uint i=0, epos = islandItems.size(); i != epos; i++)
    islandItemSet.insert(islandItems.getUInt(i));
  
  idList.clear();  
  if (!islandItemSet.empty())
  {
    sgpGaEvolver::getTopGenomesByWeights(input, limit, islandWeights, SC_NULL, idList, &islandItemSet);
  }  
}

void sgpGaOperatorEliteIslands::getTopGenomesForBlock(const sgpGaGeneration &input, uint limit, 
  const sgpWeightVector &objWeights, sgpEntityIndexList &idList)
{
  sgpGaGenomeList topList;    
  idList.clear();  
  sgpGaEvolver::getTopGenomesByWeights(input, limit, objWeights, topList, idList);
}

void sgpGaOperatorEliteIslands::getIslandObjectiveWeights(uint islandId, sgpWeightVector &islandWeights)
{
  islandWeights = m_objectiveWeights;
  for(uint i=0, epos = islandWeights.size(); i != epos; i++)
    islandWeights[i] = getObjectiveWeight(islandId, i, islandWeights[i]);    
}

double sgpGaOperatorEliteIslands::getObjectiveWeight(uint islandId, uint objIndex, double defValue)
{
  double res = defValue;
  double weight;
  
  if (m_experimentParams != SC_NULL)
    if (m_experimentParams->getDouble(islandId, SGP_EXP_PAR_BLOCK_IDX_TOUR + SGP_TOUR_PROB_EP_DYN_ERROR_OBJS + objIndex, weight))
      res = weight;        
              
  return res;
}
