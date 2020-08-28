/////////////////////////////////////////////////////////////////////////////
// Name:        GasmOperatorEliteIslands.h
// Project:     scLib
// Purpose:     Elite operator using island information.
// Author:      Piotr Likus
// Modified by:
// Created:     16/02/2010
/////////////////////////////////////////////////////////////////////////////

#ifndef _GASMOPERATORELITEISLANDS_H__
#define _GASMOPERATORELITEISLANDS_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file core.h
///
/// Elite operator.

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sc/dtypes.h"
#include "sgp/GaOperatorBasic.h"
#include "sgp/EntityIslandTool.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
class sgpGaOperatorEliteIslands: public sgpGaOperatorEliteByWeights {
public:
  // construction
  sgpGaOperatorEliteIslands();
  virtual ~sgpGaOperatorEliteIslands();
  // properties
  void setIslandLimit(uint value);
  void setExperimentParams(const sgpGaExperimentParams *params);
  void setIslandTool(sgpEntityIslandToolIntf *value);
  // run
  virtual void execute(sgpGaGeneration &input, sgpGaGeneration &output, uint limit);
protected:
  void executeOnIsland(uint islandId, sgpGaGeneration &input, const scDataNode &islandItems, sgpGaGeneration &output, uint limit);
  virtual void getTopGenomesForIsland(const sgpGaGeneration &input, uint islandId, uint limit, const scDataNode &islandItems, sgpEntityIndexList &idList);
  void getTopGenomesForBlock(const sgpGaGeneration &input, uint limit, 
    const sgpWeightVector &objWeights, sgpEntityIndexList &idList);
  void getIslandObjectiveWeights(uint islandId, sgpWeightVector &islandWeights);
  double getObjectiveWeight(uint islandId, uint objIndex, double defValue);
  void intPrepareIslandMap(const sgpGaGeneration &newGeneration, uint islandLimit, scDataNode &output);
protected:
  uint m_islandLimit;
  const sgpGaExperimentParams *m_experimentParams;
  sgpEntityIslandToolIntf *m_islandTool;
};


#endif // _GASMOPERATORELITEISLANDS_H__
