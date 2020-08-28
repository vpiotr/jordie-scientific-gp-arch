/////////////////////////////////////////////////////////////////////////////
// Name:        GpEvalFltDynamicMonitor.h
// Project:     sgpLib
// Purpose:     Monitors progress of fitness, if progress is lower than 
//              expected - stops evolution process for restart.
// Author:      Piotr Likus
// Modified by:
// Created:     11/06/2011
/////////////////////////////////////////////////////////////////////////////

#ifndef _GPEFDYNMON_H__
#define _GPEFDYNMON_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file GpEvalFltDynamicMonitor.h
\brief Monitors progress of fitness.

Use as evaluation operator filter to detect stalled process.
*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sgp/GaEvolver.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------
typedef std::list<double> sgpMovingBuffer;

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
class sgpGpEvalFltDynamicMonitor: public sgpGaOperatorEvaluate {
public:
  // construct
  sgpGpEvalFltDynamicMonitor(sgpGaOperatorEvaluate *prior);
  virtual ~sgpGpEvalFltDynamicMonitor(); 
  // properties
  int getObjectiveNo() const;
  void setObjectiveNo(int value);
  // run
  virtual bool execute(uint stepNo, bool isNewGen, sgpGaGeneration &generation);
protected:
  virtual void signalStop() = 0;
  void updateBest(sgpGaGeneration &generation);
  double findBestValue(sgpGaGeneration &generation);
  void addBestValue(double value);
  bool isBestBufferReady() const;
  bool checkProgressDynamic();
private:
  sgpGaOperatorEvaluate *m_prior;
  int m_objectiveNo;
  double m_bestObjValue;
  uint m_bufferSize;
  sgpMovingBuffer m_bestBuffer;
};

class sgpGpEvalFltDynamicMonitorByFitFunc: public sgpGpEvalFltDynamicMonitor {
public:
  sgpGpEvalFltDynamicMonitorByFitFunc(sgpGaOperatorEvaluate *prior);
  virtual ~sgpGpEvalFltDynamicMonitorByFitFunc();
  void setFitnessFunc(sgpFitnessFunction *func);
protected:
  virtual void signalStop();
private:
  sgpFitnessFunction *m_fitnessFunc;
};


#endif // _GPEFDYNMON_H__
