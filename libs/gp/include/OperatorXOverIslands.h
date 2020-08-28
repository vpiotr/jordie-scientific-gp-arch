/////////////////////////////////////////////////////////////////////////////
// Name:        OperatorXOverIslands.h
// Project:     sgpLib
// Purpose:     Basic XOver operator with island suppport.
// Author:      Piotr Likus
// Modified by:
// Created:     23/07/2011
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPOPERXOVERISL_H__
#define _SGPOPERXOVERISL_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file OperatorXOverIslands.h
\brief Short file description

Long description
*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sc/dtypes.h"
#include "sc/events/Events.h"

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
const uint SGP_GASM_DEF_XOVER_NEW_ITEM_FACTOR = 2;
const double SGP_XOVER_THRESHOLD_PROB_FILTER = 0.5;

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
class sgpOperatorXOverIslands: public sgpOperatorXOverReporting {
  typedef sgpOperatorXOverReporting inherited;
public:
  // construction
  sgpOperatorXOverIslands();
  virtual ~sgpOperatorXOverIslands();

  double getEntitySelProb();
  void setEntitySelProb(double aValue);
  void setIslandLimit(uint value);
  void setEntityIslandTool(sgpEntityIslandToolIntf *tool);
  void setYieldSignal(scSignal *value);
  void setGenomeChangedTracer(sgpGenomeChangedTracer *tracer);
  void setNewItemPerPairFactor(uint value);
  virtual void getCounters(scDataNode &output) {}

  virtual void execute(sgpGaGeneration &newGeneration);
protected:
  virtual bool crossGenomes(sgpGaGeneration &newGeneration, uint first, uint second,
    uint newChildCount, uint replaceParentCount) = 0;
  virtual double calcGenomeDiff(const sgpGaGeneration &newGeneration, uint first, uint second, uint genNo) = 0;
  void executeOnIsland(sgpGaGeneration &newGeneration, scDataNode &islandItems, uint islandId, double aProb);
  void executeOnBlockByIds(sgpGaGeneration &newGeneration, scDataNode &itemIds, double aProb);
  virtual void beforeProcess() {} 
  virtual void intPrepareIslandMap(const sgpGaGeneration &input, uint islandLimit, scDataNode &output);
  virtual bool crossGenomes(sgpGaGeneration &newGeneration, uint first, uint second);
  virtual void signalNextEntity();
  virtual void signalEntityChanged(const sgpGaGeneration &newGeneration, uint itemIndex) const;
  virtual uint calcParentReplaceFactor();
  void updateParentReplaceFactor();
  bool matchedGenomes(const sgpGaGeneration &newGeneration, uint first, uint second, uint genNo, double aThreshold);
  uint doCrossGenomes(sgpGaGeneration &newGeneration, uint first, uint second, sgpEntityBase &firstEntity, sgpEntityBase &secondEntity, double prob, double threshold);
  virtual bool doCrossGen(uint genNo, sgpEntityBase &firstEntity, sgpEntityBase &secondEntity) = 0;
  void mergeChildren(uint replaceParentCount, sgpGaGeneration &newGeneration, uint first, uint second, sgpEntityBase *firstEntity, sgpEntityBase *secondEntity, uint &newIdx1, uint &newIdx2, uint &replacedIdx1, uint &replacedIdx2);
protected:
  uint m_islandLimit;
  uint m_newItemPerPairFactor;
  uint m_parentReplaceFactor;
  double m_entitySelProb;  
  sgpEntityIslandToolIntf *m_entityIslandTool;
  scSignal *m_yieldSignal;
  sgpGenomeChangedTracer *m_genomeChangedTracer;
};


#endif // _SGPOPERXOVERISL_H__
