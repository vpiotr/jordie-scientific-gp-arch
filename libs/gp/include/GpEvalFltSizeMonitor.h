/////////////////////////////////////////////////////////////////////////////
// Name:        GpEvalFltSizeMonitor.h
// Project:     sgpLib
// Purpose:     Monitors size of entities, if avg size is too small 
//              - stops evolution process for restart.
// Author:      Piotr Likus
// Modified by:
// Created:     11/06/2011
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPGPEFSIZEMON_H__
#define _SGPGPEFSIZEMON_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file GpEvalFltSizeMonitor.h
\brief Monitors size of entities. Evaluation filter.

Monitors size of entities, if avg size is too small stops evolution for restart.
*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sgp/GaEvolver.h"

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

class sgpGpEvalFltSizeMonitor: public sgpGaOperatorEvaluate {
public:
  // construct
  sgpGpEvalFltSizeMonitor(sgpGaOperatorEvaluate *prior);
  virtual ~sgpGpEvalFltSizeMonitor(); 
  // properties
  // run
  virtual bool execute(uint stepNo, bool isNewGen, sgpGaGeneration &generation);
protected:
  virtual void signalStop() = 0;
  virtual bool isStopRequired(sgpGaGeneration &generation); 
private:
  sgpGaOperatorEvaluate *m_prior;
};

class sgpGpEvalFltSizeMonitorByFitFunc: public sgpGpEvalFltSizeMonitor {
public:
  sgpGpEvalFltSizeMonitorByFitFunc(sgpGaOperatorEvaluate *prior);
  virtual ~sgpGpEvalFltSizeMonitorByFitFunc();
  void setFitnessFunc(sgpFitnessFunction *func);
protected:
  virtual void signalStop();
private:
  sgpFitnessFunction *m_fitnessFunc;
};

#endif // _SGPGPEFSIZEMON_H__
