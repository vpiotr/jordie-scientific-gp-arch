/////////////////////////////////////////////////////////////////////////////
// Name:        OperatorXOverIslands.cpp
// Project:     sgpLib
// Purpose:     Basic XOver operator with island suppport.
// Author:      Piotr Likus
// Modified by:
// Created:     23/07/2011
/////////////////////////////////////////////////////////////////////////////

#include "base\rand.h"

#include "sgp\OperatorXOverIslands.h"

const double SGP_GASM_DEF_OPER_PROB_XOVER = 0.6;

sgpOperatorXOverIslands::sgpOperatorXOverIslands(): inherited()
{
  m_entitySelProb = SGP_GASM_DEF_OPER_PROB_XOVER;
  m_islandLimit = 0;
  m_yieldSignal = SC_NULL;
  m_islandLimit = 0;
  m_newItemPerPairFactor = SGP_GASM_DEF_XOVER_NEW_ITEM_FACTOR;
  m_entityIslandTool = SC_NULL;
  updateParentReplaceFactor();
}

sgpOperatorXOverIslands::~sgpOperatorXOverIslands()
{
}

double sgpOperatorXOverIslands::getEntitySelProb()
{
  return m_entitySelProb;
}

void sgpOperatorXOverIslands::setEntitySelProb(double aValue)
{
  m_entitySelProb = aValue;
}

void sgpOperatorXOverIslands::setIslandLimit(uint value)
{
  m_islandLimit = value;
}  

void sgpOperatorXOverIslands::setYieldSignal(scSignal *value)
{
  m_yieldSignal = value;
}

void sgpOperatorXOverIslands::setNewItemPerPairFactor(uint value)
{
  m_newItemPerPairFactor = value;
  updateParentReplaceFactor();
}

void sgpOperatorXOverIslands::setGenomeChangedTracer(sgpGenomeChangedTracer *tracer)
{
  m_genomeChangedTracer = tracer;
}

void sgpOperatorXOverIslands::setEntityIslandTool(sgpEntityIslandToolIntf *value)
{
  m_entityIslandTool = value;
}

void sgpOperatorXOverIslands::updateParentReplaceFactor()
{
  m_parentReplaceFactor = calcParentReplaceFactor();
}

uint sgpOperatorXOverIslands::calcParentReplaceFactor()
{
  uint res;
  uint leftCnt;
  leftCnt = m_newItemPerPairFactor;
  if (leftCnt < 2)
    res = 2 - leftCnt;
  else  
    res = 0;
  return res;  
}

void sgpOperatorXOverIslands::execute(sgpGaGeneration &newGeneration)
{
  if (!canExecute()) 
    return;

  beforeProcess();

  scDataNode islandItems;  
  intPrepareIslandMap(newGeneration, m_islandLimit, islandItems);

  scString islandName;
  for(uint j = 0, eposj = m_islandLimit; j != eposj; j++)
  {
    islandName = toString(j);
    if (islandItems.hasChild(islandName))
      executeOnIsland(newGeneration, islandItems[islandName], j, m_entitySelProb);
  }
}

void sgpOperatorXOverIslands::intPrepareIslandMap(const sgpGaGeneration &input, uint islandLimit, scDataNode &output)
{
  m_entityIslandTool->prepareIslandMap(input, output);
}

void sgpOperatorXOverIslands::executeOnIsland(sgpGaGeneration &newGeneration, scDataNode &islandItems, uint islandId, double aProb)
{
  executeOnBlockByIds(newGeneration, islandItems, aProb);
}

void sgpOperatorXOverIslands::executeOnBlockByIds(sgpGaGeneration &newGeneration, scDataNode &itemIds, double aProb)
{
  uint secondPos;

  if (itemIds.size() > 1)
  for(uint i = 0, epos = itemIds.size(); i != epos; i++)
  {
    if (randomFlip(aProb)) {
      do {
        secondPos = randomInt(0, epos - 1);
      } while (i == secondPos);
      crossGenomes(newGeneration, itemIds.getUInt(i), itemIds.getUInt(secondPos));        
    }
  }
}

void sgpOperatorXOverIslands::signalNextEntity()
{
  if (m_yieldSignal != SC_NULL)
    m_yieldSignal->execute();
}

bool sgpOperatorXOverIslands::matchedGenomes(const sgpGaGeneration &newGeneration, uint first, uint second, uint genNo, double aThreshold)
{
  const double DIV_LIMIT = 1.0 + 1.0E-7;
  double diff = calcGenomeDiff(newGeneration, first, second, genNo);
  bool res;
  // for diff 0..a => return TRUE
  // for diff a..1 => return randomly TRUE with highest prob on diff = a, lowest on diff = 1
  if (diff >= aThreshold)
  {
    double selProb = SGP_XOVER_THRESHOLD_PROB_FILTER * (DIV_LIMIT - diff)/(DIV_LIMIT - aThreshold);
    res = randomFlip(selProb);
  } else {
    res = true;
  }  
  return res;
}

void sgpOperatorXOverIslands::signalEntityChanged(const sgpGaGeneration &newGeneration, 
  uint itemIndex) const
{
  if (m_genomeChangedTracer)
    m_genomeChangedTracer->execute(newGeneration, itemIndex, "xover");
}

bool sgpOperatorXOverIslands::crossGenomes(sgpGaGeneration &newGeneration, uint first, uint second)
{
  uint replaceParentCnt = m_parentReplaceFactor;
  uint leftCnt = m_newItemPerPairFactor;        
  uint newChildCnt;  
  bool res = false;
  
  do {
    newChildCnt = leftCnt % 3; // 0..2
    if (crossGenomes(newGeneration, first, second, newChildCnt, replaceParentCnt))
      res = true;
    leftCnt -= newChildCnt;
  } while ((leftCnt > 0) && (newChildCnt > 0));  

  signalNextEntity();

  return res;
}

uint sgpOperatorXOverIslands::doCrossGenomes(sgpGaGeneration &newGeneration, uint first, uint second, sgpEntityBase &firstEntity, sgpEntityBase &secondEntity, double prob, double threshold)
{
  uint cnt1 = newGeneration[first].getGenomeCount();
  uint cnt2 = newGeneration[second].getGenomeCount();
  uint minCnt = std::min<uint>(cnt1, cnt2);

  uint res = 0;
  for(uint i=0; i != minCnt; i++)
  {
    if (randomFlip(prob))
    {
      if (matchedGenomes(newGeneration, first, second, i, threshold))
      {
        if (doCrossGen(i, firstEntity, secondEntity))
        {
          res++;
        }  
      }  
    }
  } // for i
  return res;
}

void sgpOperatorXOverIslands::mergeChildren(uint replaceParentCount, sgpGaGeneration &newGeneration, uint first, uint second, sgpEntityBase *firstEntity, sgpEntityBase *secondEntity, uint &newIdx1, uint &newIdx2, uint &replacedIdx1, uint &replacedIdx2)
{
  std::auto_ptr<sgpEntityBase> firstEntityGuard(firstEntity);
  std::auto_ptr<sgpEntityBase> secondEntityGuard(secondEntity);

  switch(replaceParentCount) {
    case 0: {
      newGeneration.insert(firstEntityGuard.release());
      newGeneration.insert(secondEntityGuard.release());
      newIdx1 = newGeneration.size() - 2;
      newIdx2 = newGeneration.size() - 1;
      replacedIdx1 = replacedIdx2 = newGeneration.size();
      break;
    }
    case 1: {
      if (randomFlip(0.5)) 
      { // replace first
        newGeneration.insert(secondEntityGuard.release());        
        newGeneration.setItem(first, *firstEntityGuard);
        newIdx2 = newGeneration.size() - 1;
        newIdx1 = first;
        replacedIdx1 = first;
        replacedIdx2 = newGeneration.size();
      } else {
      // replace second
        newGeneration.insert(firstEntityGuard.release());        
        newGeneration.setItem(second, *secondEntityGuard);
        newIdx1 = newGeneration.size() - 1;
        newIdx2 = second;
        replacedIdx1 = newGeneration.size();
        replacedIdx2 = second;
      }        
      break;
    }
    default: {
      newGeneration.setItem(first, *firstEntityGuard);
      newGeneration.setItem(second, *secondEntityGuard);
      replacedIdx1 = newIdx1 = first;
      replacedIdx2 = newIdx2 = second;
    }
  }
}

