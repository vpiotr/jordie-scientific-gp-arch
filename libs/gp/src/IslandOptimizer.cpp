/////////////////////////////////////////////////////////////////////////////
// Name:        IslandOptimizer.h
// Project:     sgpLib
// Purpose:     Optimizer for islands
// Author:      Piotr Likus
// Modified by:
// Created:     21/01/2010
/////////////////////////////////////////////////////////////////////////////
#include "sgp/IslandOptimizer.h"
#include "sgp/GaEvolver.h"
#include "base/rand.h"
#include "sc/utils.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

using namespace dtp;

void sgpIslandOptimizer::setMutationProb(double value)
{
  m_mutationProb = value;
}

void sgpIslandOptimizer::setErrorParams(const sgpIslandParamIdSet &value)
{
  m_errorParams = value;
}

void sgpIslandOptimizer::setErrorParamOffset(uint value)
{
  m_errorParamOffset = value;
}

void sgpIslandOptimizer::setParams(sgpGaExperimentParamsStored *params)
{
  m_params = params;
}

void sgpIslandOptimizer::postProcess(scDataNode &itemValues)
{
  mutateValues(itemValues);
}

void sgpIslandOptimizer::mutateValues(scDataNode &itemValues)
{
  for(uint i=0, epos = itemValues.size(); i != epos; i++)
    for(uint j=0, eposj = itemValues[i].size(); j != eposj; j++)
      if (randomFlip(m_mutationProb))
        mutateItemValue(itemValues[i], j);
}

void sgpIslandOptimizer::mutateItemValue(scDataNode &valueList, int valueIndex)
{
  const static int REAL_VALUE_BIT_COUNT(12);
  
  if (getValueType(valueIndex) == vt_int)
  {// mutate integer - random bit
    int minValue = getValueMinInt(valueIndex);
    int maxValue = getValueMaxInt(valueIndex);
    int currValue = valueList.getInt(valueIndex);
    int workValue = currValue - minValue;
    int range = maxValue - minValue;
    int bitCount = getActiveBitSize(range);
    int bitNo = randomInt(0, bitCount - 1);
    uint bitMask;
    if (bitNo > 0)
      bitMask = 1 << bitNo;
    else
      bitMask = 1;  
    workValue = static_cast<int>(static_cast<uint>(workValue) ^ bitMask);  
    workValue = (workValue % range) + minValue;
    valueList.setInt(valueIndex, workValue);
  } else {
  // mutate as double
    double minValue = getValueMinDouble(valueIndex);
    double maxValue = getValueMaxDouble(valueIndex); 
    double currValue = valueList.getDouble(valueIndex);
    double range = maxValue - minValue;
    double workValue = (currValue - minValue) / range;
    scString sValue;
    sgp::encodeBitStrDouble(workValue, REAL_VALUE_BIT_COUNT, sValue);
    ulong64 binValue = sgp::decodeBitStrUInt(sValue);
    ulong64 grayValue = binToGray(binValue, REAL_VALUE_BIT_COUNT);
    int bitNo = randomInt(0, REAL_VALUE_BIT_COUNT - 1);
    ulong64 bitMask;
    if (bitNo > 0)
      bitMask = static_cast<ulong64>(1) << bitNo;
    else
      bitMask = 1;  
    grayValue = grayValue ^ bitMask;  
    binValue = grayToBin(grayValue, REAL_VALUE_BIT_COUNT);
    sValue = sgp::encodeBitStrUInt(binValue, REAL_VALUE_BIT_COUNT);
    workValue = sgp::decodeBitStrDouble(sValue);
    workValue = workValue * range + minValue;
    valueList.setDouble(valueIndex, workValue);
  }
}

// all defined dynamic objectives rescale so one of them is = minimum value, rest is = min..max
void sgpIslandOptimizer::rescaleObjectives(scDataNode &itemValues)
{
  if (m_errorParams.empty())
    return;
    
  double minValue = 0.0; 
  double maxValue = 0.0; 
  double realMinValue = 0.0; 
  double realMaxValue = 0.0; 
  sgpIslandParamIdSet::iterator it = m_errorParams.begin();
  uint paramIndex;
  double paramValue;

  // find min and max of meta value range  
  if (it != m_errorParams.end())  
  {
    paramIndex = getParamIndex(m_errorParamOffset + *it);
    minValue = m_paramMeta[paramIndex].getDouble(0);
    maxValue = m_paramMeta[paramIndex].getDouble(1);
    it++;
    while(it != m_errorParams.end())
    {
      paramIndex = getParamIndex(m_errorParamOffset + *it);
      minValue = SC_MIN(m_paramMeta[paramIndex].getDouble(0), minValue);
      maxValue = SC_MAX(m_paramMeta[paramIndex].getDouble(1), maxValue);
      it++;
    }
  }
  
  // find min and max of real value range  
  for(uint i=0, epos = itemValues.size(); i != epos; i++)
  {
    if (i == 0) {
      paramIndex = getParamIndex(m_errorParamOffset + *(m_errorParams.begin()));
      realMinValue = realMaxValue = itemValues[i].getDouble(paramIndex);
    }
    
    for(sgpIslandParamIdSet::iterator it = m_errorParams.begin(), epos = m_errorParams.end(); it != epos; ++it)
    {
      paramIndex = getParamIndex(m_errorParamOffset + *it);
      paramValue = itemValues[i].getDouble(paramIndex);
      realMinValue = SC_MIN(paramValue, realMinValue);
      realMaxValue = SC_MAX(paramValue, realMaxValue);
    }  
  }  

  // rescale values
  double rescaleFactor;
  if (realMaxValue != realMinValue)
    rescaleFactor = (1.0/(realMaxValue - realMinValue))*(maxValue - minValue);
  else
    rescaleFactor = 1.0;  
  
  for(uint i=0, epos = itemValues.size(); i != epos; i++)
  {
    for(sgpIslandParamIdSet::iterator it = m_errorParams.begin(), epos = m_errorParams.end(); it != epos; ++it)
    {
      paramIndex = getParamIndex(m_errorParamOffset + *it);
      paramValue = itemValues[i].getDouble(paramIndex);
      paramValue = (paramValue - realMinValue)*rescaleFactor + minValue;
      itemValues[i].setDouble(paramIndex, paramValue);
    }  
  }  
}

uint sgpIslandOptimizer::getParamIndex(uint paramId)
{
  uint res;
  assert(m_params != SC_NULL);
  if (!m_params->getParamIndex(0, paramId, res))
    throw scError(scString("Unknown param: ")+toString(paramId));
  return res;  
}
