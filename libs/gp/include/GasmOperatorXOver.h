/////////////////////////////////////////////////////////////////////////////
// Name:        GasmOperatorXOver.h
// Project:     scLib
// Purpose:     XOver operator for GP evolution.
// Author:      Piotr Likus
// Modified by:
// Created:     29/12/2009
/////////////////////////////////////////////////////////////////////////////


#ifndef _GASMOPERATORXOVER_H__
#define _GASMOPERATORXOVER_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file core.h
///
/// File description

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sc/dtypes.h"
#include "sc/events/Events.h"

#include "sgp/GasmEvolver.h"
#include "sgp/GaOperatorBasic.h"
#include "sgp/GasmOperator.h"
#include "sgp/OperatorXOverIslands.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
const double SGP_GASM_DEF_OPER_PROB_XOVER = 0.6;
const double SGP_GASM_DEF_XOVER_DISTANCE_FACTOR = 0.33;
const double SGP_GASM_DEF_SPECIES_THRESHOLD = 0.5;

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// sgpGasmOperatorXOver
// ----------------------------------------------------------------------------
class sgpGasmOperatorXOver: public sgpOperatorXOverIslands {
  typedef sgpOperatorXOverIslands inherited;
public:
  // construction
  sgpGasmOperatorXOver();
  virtual ~sgpGasmOperatorXOver();
  // properties
  void setMatchEnabled(bool value);
  double getMatchThreshold();
  void setMatchThreshold(double aValue);
  virtual void getCounters(scDataNode &output);  
  void setCompareTool(sgpGaGenomeCompareTool *tool);
  void setCompareToolEx(sgpGaGenomeCompareTool *tool);
  void setMetaForInfoBlock(const sgpGaGenomeMetaList &value);
  void setDistanceFactor(double value);
  void setDistanceFunction(sgpDistanceFunction *value);
  void setFixedGenProb(bool value);
  // run
  virtual void init();
protected:
  virtual void beforeProcess();
  bool canExecute();
  bool crossGenomesNChildren(sgpGaGeneration &newGeneration, uint first, uint second);
  bool crossGenomes(sgpGaGeneration &newGeneration, uint first, uint second,
    uint newChildCount, uint replaceParentCount);
  bool matchedGenomes(const sgpGaGeneration &newGeneration, uint first, uint second, uint genNo, double aThreshold);
  double calcGenomeDiff(const sgpGaGeneration &newGeneration, uint first, uint second);
  bool crossGenomesSamePos(uint genNo, sgpGaGeneration &newGeneration, uint first, uint second);
  uint findInstrBeginForward(const sgpGaGenomeMetaList &info, uint aStart);
  void getGenomeXCrossInfo(const sgpEntityForGasm &info, 
    double &aProb, double &aMatchThreshold);
  uint findInstrBeginBackward(const sgpGaGenomeMetaList &info, uint startOffset);
  void monitorDupsFromOperator(const sgpGaGenome &genomeBefore, const sgpGaGenome &genomeAfter, 
    const scString &operTypeName);
  virtual double calcGenomeDiff(const sgpGaGeneration &newGeneration, uint first, uint second, uint genNo);
  virtual bool doCrossGen(uint genNo, sgpEntityBase &firstEntity, sgpEntityBase &secondEntity);
  void traceGenomesCrossed(sgpGaGeneration &newGeneration, uint first, uint second,uint newIdx1, uint newIdx2, uint replacedIdx1, uint replacedIdx2);
protected:
  double m_matchThreshold;
  double m_distanceFactor;
  bool m_matchEnabled;
  bool m_fixedGenProb;
  sgpGaGenomeMetaList m_metaForInfoBlock;
  scDataNode m_counters;
  sgpGaGenomeCompareTool *m_compareTool;
  sgpDistanceFunction *m_distanceFunction;
};


#endif // _GASMOPERATORXOVER_H__
