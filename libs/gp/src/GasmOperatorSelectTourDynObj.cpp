/////////////////////////////////////////////////////////////////////////////
// Name:        GasmOperatorSelectTourDynObj.h
// Project:     scLib
// Purpose:     Select operator - tournament with dynamic objective set.
// Author:      Piotr Likus
// Modified by:
// Created:     29/12/2009
/////////////////////////////////////////////////////////////////////////////

//std
#include <cmath>
//other
#include "sgp/GasmEvolver.h"
#include "sgp/GasmOperator.h"
#include "sgp/GasmOperatorSelectTourDynObj.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

// ----------------------------------------------------------------------------
// sgpGasmOperatorSelectTournamentDynObj
// ----------------------------------------------------------------------------
sgpGasmOperatorSelectTournamentDynObj::sgpGasmOperatorSelectTournamentDynObj():sgpGaOperatorSelectTourProb()
{
  m_minErrorObjWeight = m_maxErrorObjWeight = 0.0;
}

sgpGasmOperatorSelectTournamentDynObj::~sgpGasmOperatorSelectTournamentDynObj()
{
}

void sgpGasmOperatorSelectTournamentDynObj::setMinErrorObjWeight(double value)
{
  m_minErrorObjWeight = value;
}

void sgpGasmOperatorSelectTournamentDynObj::setMaxErrorObjWeight(double value)
{
  m_maxErrorObjWeight = value;
}

void sgpGasmOperatorSelectTournamentDynObj::setObjectiveWeights(const sgpWeightVector &value)
{
  if (m_minErrorObjWeight == m_maxErrorObjWeight) {
    double maxWeight = 0.0;

  // skip weight for obj[0]
    for(sgpWeightVector::const_iterator it=value.begin()+1,epos = value.end(); it != epos; ++it)
    {
      if (fabs(*it) > maxWeight)
        maxWeight = fabs(*it);  
    }
    
    m_maxErrorObjWeight = maxWeight;
  }

  sgpGaOperatorSelectTourProb::setObjectiveWeights(value);
}

double sgpGasmOperatorSelectTournamentDynObj::getObjectiveWeight(const sgpGaGeneration &input, 
  uint first, uint second,
  uint objIndex, double defValue)
{
  bool found = false;
  double val1 = 0.0;
  double val2 = 0.0;
  const sgpEntityForGasm *firstWorkInfo = dynamic_cast<const sgpEntityForGasm *>(input.atPtr(first));
  
  if (firstWorkInfo->hasInfoBlock())
  {
    found = firstWorkInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_ERR_OBJ_WEIGHT_BASE + objIndex, val1);
  } else {
    val1 = 0.0;
  }
  
  if (found) {
    const sgpEntityForGasm *secondWorkInfo = dynamic_cast<const sgpEntityForGasm *>(input.atPtr(second));
    
    if (secondWorkInfo->hasInfoBlock())
    {
      found = secondWorkInfo->getInfoDouble(SGP_INFOBLOCK_IDX_MUT_ERR_OBJ_WEIGHT_BASE + objIndex, val2);
    } else {
      val2 = 0.0;
      found = false;
    }
  }
  
  if (found) {
    double res = (((val1 + val2) / 2.0)*(m_maxErrorObjWeight - m_minErrorObjWeight)) + m_minErrorObjWeight;
    return res;
  } else {
    return defValue;
  }
}  

