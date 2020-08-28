/////////////////////////////////////////////////////////////////////////////
// Name:        IslandOptimizer.h
// Project:     sgpLib
// Purpose:     Optimizer for islands
// Author:      Piotr Likus
// Modified by:
// Created:     21/01/2010
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPISLANDOPTIMIZER_H__
#define _SGPISLANDOPTIMIZER_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file core.h
///
/// File description

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sc/alg/PsoOptimizer.h"
#include "sgp/GaEvolver.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------
typedef std::set<uint> sgpIslandParamIdSet;

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
class sgpIslandOptimizer: public scPsoOptimizer {
public:
  sgpIslandOptimizer(): scPsoOptimizer(), m_mutationProb(0.0), m_errorParams(), m_params(SC_NULL), m_errorParamOffset(0) {}
  virtual ~sgpIslandOptimizer() {}
  void setMutationProb(double value);  
  void setErrorParams(const sgpIslandParamIdSet &value);
  void setParams(sgpGaExperimentParamsStored *params);
  void setErrorParamOffset(uint value);
protected:
  virtual void postProcess(scDataNode &itemValues);
  void mutateValues(scDataNode &itemValues);
  void rescaleObjectives(scDataNode &itemValues);
  void mutateItemValue(scDataNode &valueList, int itemIndex);
  uint getParamIndex(uint paramId);
protected:
  double m_mutationProb;  
  sgpIslandParamIdSet m_errorParams;
  sgpGaExperimentParamsStored *m_params;
  uint m_errorParamOffset;
};

#endif // _SGPISLANDOPTIMIZER_H__
