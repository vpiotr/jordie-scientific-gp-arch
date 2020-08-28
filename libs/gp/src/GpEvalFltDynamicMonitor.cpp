/////////////////////////////////////////////////////////////////////////////
// Name:        GpEvalFltDynamicMonitor.cpp
// Project:     sgpLib
// Purpose:     Monitors progress of fitness, if progress is lower than 
//              expected - stops evolution process.
// Author:      Piotr Likus
// Modified by:
// Created:     11/06/2011
/////////////////////////////////////////////////////////////////////////////

#include "dtp/algorithm.h"

#include "sc/utils.h"

#include "sgp/GpEvalFltDynamicMonitor.h"

using namespace dtp;

const uint RESTART_BUFFER_SIZE = 25;
const double RESTART_MIN_BEST_STD_DEV_RATIO = 0.01;   
const uint RESTART_BUFFER_MARGIN = 5;
const double RESTART_MIN_PROGRESS_RATIO = 0.01;

// ----------------------------------------------------------------------------
// sgpGpEvalFltDynamicMonitor
// ----------------------------------------------------------------------------
sgpGpEvalFltDynamicMonitor::sgpGpEvalFltDynamicMonitor(sgpGaOperatorEvaluate *prior): 
    sgpGaOperatorEvaluate(), 
    m_prior(prior), 
    m_objectiveNo(-1), 
    m_bestObjValue(0.0),
    m_bufferSize(RESTART_BUFFER_SIZE)
{}

sgpGpEvalFltDynamicMonitor::~sgpGpEvalFltDynamicMonitor()
{
}

int sgpGpEvalFltDynamicMonitor::getObjectiveNo() const
{
  return m_objectiveNo;
}

void sgpGpEvalFltDynamicMonitor::setObjectiveNo(int value)
{
  m_objectiveNo = value;
}

bool sgpGpEvalFltDynamicMonitor::execute(uint stepNo, bool isNewGen, sgpGaGeneration &generation)
{
  bool res = m_prior->execute(stepNo, isNewGen, generation);
  if (res) {
    updateBest(generation);
    if (!checkProgressDynamic())
      res = false;
  }
  return res;
}

void sgpGpEvalFltDynamicMonitor::updateBest(sgpGaGeneration &generation)
{
  double bestObjValue = std::fabs(findBestValue(generation));
  if (bestObjValue > m_bestObjValue)
  {
    m_bestObjValue = bestObjValue;
    addBestValue(m_bestObjValue);
  }
}

double sgpGpEvalFltDynamicMonitor::findBestValue(sgpGaGeneration &generation)
{
  if (generation.empty())
    return 0.0;

  double res = std::fabs(generation.at(0).getFitness(m_objectiveNo));

  double val;

  for(uint i=0, epos = generation.size(); i != epos; i++)
  {
    val = std::fabs(generation.at(i).getFitness(m_objectiveNo));
    if (val > res)
      res = val;
  }

  return res;
}

void sgpGpEvalFltDynamicMonitor::addBestValue(double value)
{
  m_bestBuffer.push_front(value);
  if (m_bestBuffer.size() > m_bufferSize)
    m_bestBuffer.pop_back();
}

bool sgpGpEvalFltDynamicMonitor::isBestBufferReady() const {
  return (m_bestBuffer.size() >= m_bufferSize);
}

bool sgpGpEvalFltDynamicMonitor::checkProgressDynamic() {
  if (!isBestBufferReady())
    return true;
  bool res = true;
  double dynamic;
  
  std::vector<double> allBest;
  for(sgpMovingBuffer::const_iterator it=m_bestBuffer.begin(),epos=m_bestBuffer.end(); it != epos; ++it)
    allBest.push_back(*it);

  //dynamic = calcStdDev(allBest);
  dynamic = std_dev(allBest.begin(), allBest.end(), 0.0);

  double avgVal = avg(allBest.begin(), allBest.end(), 0.0);

  // method 1 - verify progress using std-dev compared to fraction of avg
  if (dynamic < std::abs(avgVal)*RESTART_MIN_BEST_STD_DEV_RATIO)
  {
    res = false;
    signalStop();
  }   
  
  // method 2 - compare avg(head-of-buffer) to avg(tail-of-buffer)
  if (res) {
    double firstValueMed = 0.0;
    double lastValueMed = 0.0;
    uint allSize = allBest.size();
    
    // find AVG of first & last k elements in buffer
    for(uint i=0,epos=RESTART_BUFFER_MARGIN; i != epos; i++) {
      firstValueMed += allBest[i];
      lastValueMed += allBest[allSize - 1 - i];
    }  
      
    firstValueMed = firstValueMed / double(RESTART_BUFFER_MARGIN);
    lastValueMed = lastValueMed / double(RESTART_BUFFER_MARGIN);
      
    // when start of buffer is non-empty
    if (res && !equDouble(firstValueMed, 0.0)) {
      // calculate progress as difference between head & tail avg / head avg
      double progress = std::abs(lastValueMed - firstValueMed) / std::abs(firstValueMed);    
      // if progress is minimal - stop
      if (progress < RESTART_MIN_PROGRESS_RATIO) {
        res = false;
        signalStop();
      }
    }
  }
  return res;
}

// ----------------------------------------------------------------------------
// sgpGpEvalFltDynamicMonitorByFitFunc
// ----------------------------------------------------------------------------
sgpGpEvalFltDynamicMonitorByFitFunc::sgpGpEvalFltDynamicMonitorByFitFunc(sgpGaOperatorEvaluate *prior): 
  sgpGpEvalFltDynamicMonitor(prior) 
{
}

sgpGpEvalFltDynamicMonitorByFitFunc::~sgpGpEvalFltDynamicMonitorByFitFunc()
{
}

void sgpGpEvalFltDynamicMonitorByFitFunc::setFitnessFunc(sgpFitnessFunction *func)
{
  m_fitnessFunc = func;
}

void sgpGpEvalFltDynamicMonitorByFitFunc::signalStop() {
  m_fitnessFunc->setStopStatus(gssRestartOnDynamic);  
}
