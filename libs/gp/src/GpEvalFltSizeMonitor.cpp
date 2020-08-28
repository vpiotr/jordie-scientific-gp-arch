/////////////////////////////////////////////////////////////////////////////
// Name:        GpEvalFltSizeMonitor.h
// Project:     sgpLib
// Purpose:     Monitors size of entities, if avg size is too small 
//              - stops evolution process for restart.
// Author:      Piotr Likus
// Modified by:
// Created:     11/06/2011
/////////////////////////////////////////////////////////////////////////////

#include "sgp/GpEvalFltSizeMonitor.h"
#include "sgp/GasmEvolver.h"

const uint RESTART_MIN_AVG_GENOME_SIZE = 2*4;

// ----------------------------------------------------------------------------
// sgpGpEvalFltSizeMonitor
// ----------------------------------------------------------------------------
sgpGpEvalFltSizeMonitor::sgpGpEvalFltSizeMonitor(sgpGaOperatorEvaluate *prior):
  sgpGaOperatorEvaluate(),
  m_prior(prior)
{
}

sgpGpEvalFltSizeMonitor::~sgpGpEvalFltSizeMonitor()
{
}

bool sgpGpEvalFltSizeMonitor::execute(uint stepNo, bool isNewGen, sgpGaGeneration &generation)
{
  bool res = m_prior->execute(stepNo, isNewGen, generation);
  if (res) {
    if (isStopRequired(generation))
    {
      signalStop();
      res = false;
    }
  }
  return res;
}

bool sgpGpEvalFltSizeMonitor::isStopRequired(sgpGaGeneration &generation)
{
  if (generation.empty())
    return true;

  ulong64 sizeSum = 0;
  scDataNode code;
    
  sgpProgramCode prg;

  for(uint i = generation.beginPos(), epos = generation.endPos(); i!=epos; i++) {
    dynamic_cast<const sgpEntityForGasm *>(generation.atPtr(i))->getProgramCode(code);
    prg.setFullCode(code);
    sizeSum += prg.getCodeLength();      
  }  

  double avgSize = double(sizeSum) / double(generation.size());

  bool res = (avgSize > RESTART_MIN_AVG_GENOME_SIZE);

  return res;
}

// ----------------------------------------------------------------------------
// sgpGpEvalFltSizeMonitorByFitFunc
// ----------------------------------------------------------------------------
sgpGpEvalFltSizeMonitorByFitFunc::sgpGpEvalFltSizeMonitorByFitFunc(sgpGaOperatorEvaluate *prior):
    sgpGpEvalFltSizeMonitor(prior),
    m_fitnessFunc(SC_NULL)
{
}

sgpGpEvalFltSizeMonitorByFitFunc::~sgpGpEvalFltSizeMonitorByFitFunc()
{
}

void sgpGpEvalFltSizeMonitorByFitFunc::setFitnessFunc(sgpFitnessFunction *func)
{
  m_fitnessFunc = func;
}

void sgpGpEvalFltSizeMonitorByFitFunc::signalStop()
{
  m_fitnessFunc->setStopStatus(gssRestartOnSize);
}

