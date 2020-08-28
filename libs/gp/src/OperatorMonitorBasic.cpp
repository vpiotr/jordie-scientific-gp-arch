/////////////////////////////////////////////////////////////////////////////
// Name:        OperatorMonitorBasic.cpp
// Project:     sgpLib
// Purpose:     Monitor class for GP/GA project
// Author:      Piotr Likus
// Modified by:
// Created:     02/05/2009
/////////////////////////////////////////////////////////////////////////////

#define COUT_ENABLED
#define LOG_PERFORMANCE
#define DEBUG_DUMP

//#define LOG_WHOLE_POP_FIT
#define REJECT_TYPE_CNT 4
//#define REPORT_COUNTER_STEP_STATS
//#define REPORT_TIMER_STEP_STATS

//#include "Precomp.h"

#ifdef COUT_ENABLED
#include <iostream>
using namespace std;
#endif

#include "base/date.h"

#include "perf/Timer.h"
#include "perf/Counter.h"
#include "perf/Log.h"

#include "sc/defs.h"
#include "sc/txtf/TxtWriter.h"

#include "sgp/GaStatistics.h"

#ifdef TRACE_ENTITY_BIO
#include "sgp\GpEntityTracer.h"
#endif

#include "grd\EventTrace.h"

#include "sgp\FitnessScanner.h"

#include "sgp/OperatorMonitorBasic.h"
#include "sgp/Experiment4Evolver.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

using namespace dtp;
using namespace perf;

typedef std::pair<uint, double> UIntDoublePair;

bool sort_second_pred_gt(const UIntDoublePair& left, const UIntDoublePair& right)
{
  return left.second > right.second;
}

class sgpEntityListingWriter: public sgpEntityListingWriterIntf {
public:
  sgpEntityListingWriter(sgpExperimentLogWriterIntf *writer): m_writer(writer) {}

  virtual void writeLine(const scString &line) {
    m_writer->writeLine(line);
  }

protected:
  sgpExperimentLogWriterIntf *m_writer;
};

sgpOperatorMonitorBasic::sgpOperatorMonitorBasic(): 
  m_notifier(SC_NULL),
  m_entityLister(SC_NULL)
{
  m_showAllGens = false; 
  m_lastBestFit = 0.0; 
  m_restartCount = 0;
  m_startStepNo = 0;
  m_primaryObjectiveIndex = sgpFitnessValue::SGP_OBJ_OFFSET;
  m_shapeObjectiveIndex = 0;
  m_fullDumpInterval = 0;
  m_islandLimit = m_lastBestIslandId = 0;
  m_experimentParams = SC_NULL;
  m_lastStepNoForTime = 0;
  m_features = mfAll;

  m_lastBestInfo.reset(new sgpOmBestInfo);
  m_localTimer.reset(new LocalTimer);
  m_lastCounters.setAsParent();

  initStepTime();
}

sgpOperatorMonitorBasic::~sgpOperatorMonitorBasic()
{
}

void sgpOperatorMonitorBasic::addStatsSrc(sgpMonitorStatsSrcIntf *stats)
{
  m_statsSrcList.push_back(stats);
}


void sgpOperatorMonitorBasic::addReporter(sgpMonitorReporterIntf *reporter)
{
  m_reporterList.push_back(reporter);
} 

void sgpOperatorMonitorBasic::addFitStatsObserver(sgpMonitorFitStatsObserverIntf *observer)
{
  m_fitStatsObserverList.push_back(observer);
}

void sgpOperatorMonitorBasic::addParamReporter(sgpMonitorParamReporterIntf *reporter)
{
  m_paramReporterList.push_back(reporter);
}

void sgpOperatorMonitorBasic::addListener(const scString &eventName, scListener *listener)
{
  getNotifier()->addListener(eventName, listener);
}

void sgpOperatorMonitorBasic::setEntityLister(sgpMonitorEntityListerIntf *entityLister)
{
  m_entityLister = entityLister;
}

void sgpOperatorMonitorBasic::setShowAllGenomes(bool aValue) {
  m_showAllGens = aValue;
}

void sgpOperatorMonitorBasic::setFitnessFunc(sgpFitnessFunction *value) {
  m_fitnessFunc = value;
}

void sgpOperatorMonitorBasic::setRestartCount(uint value) {
  m_restartCount = value;
}
  
void sgpOperatorMonitorBasic::setCoreLogFileName(const scString &fname)
{
  m_coreLogFileName = fname;
}

void sgpOperatorMonitorBasic::setPrimaryObjectiveIndex(uint value)
{
  m_primaryObjectiveIndex = value;
}

void sgpOperatorMonitorBasic::setShapeObjectiveIndex(uint value)
{
  m_shapeObjectiveIndex = value;
}

void sgpOperatorMonitorBasic::setFullDumpInterval(uint value)
{
  m_fullDumpInterval = value;
}

void sgpOperatorMonitorBasic::setRunParams(const scDataNode &params)
{
  m_runParams = params;
}  

void sgpOperatorMonitorBasic::setIslandLimit(uint value)
{
  m_islandLimit = m_lastBestIslandId = value;
}

void sgpOperatorMonitorBasic::setExperimentParams(sgpGaExperimentParams *params)
{
  m_experimentParams = params;
}

void sgpOperatorMonitorBasic::setExperimentLog(sgpExperimentLog *value)
{
  m_experimentLog = value;
}  

void sgpOperatorMonitorBasic::setEntityIslandTool(sgpEntityIslandToolIntf *entityIslandTool)
{
  m_entityIslandTool = entityIslandTool;
}

void sgpOperatorMonitorBasic::setStartStepNo(uint value)
{
  m_startStepNo = value;
}

uint sgpOperatorMonitorBasic::getStartStepNo()
{
  return m_startStepNo;
}

void sgpOperatorMonitorBasic::setFeatures(uint value)
{
  m_features = value;
}

uint sgpOperatorMonitorBasic::getFeatures()
{
  return m_features;
}

void sgpOperatorMonitorBasic::getLocalCounters(scDataNode &output) {
  output = m_counters;  
}

void sgpOperatorMonitorBasic::getCounters(scDataNode &output) 
{
  getLocalCounters(output);

  scDataNode operatorCounters;
  collectOperatorCounters(operatorCounters);

  for(uint i=0, epos = operatorCounters.size(); i != epos; i++)
    output.merge(operatorCounters[i]);
}


void sgpOperatorMonitorBasic::getTimers(scDataNode &output)
{
  intGetTimers(false, output);
}

void sgpOperatorMonitorBasic::getTimersDispName(scDataNode &output)
{
  intGetTimers(false, output);
}

void sgpOperatorMonitorBasic::getTimersCodedName(scDataNode &output)
{
  intGetTimers(true, output);
}

void sgpOperatorMonitorBasic::intGetTimers(bool withCodeName, scDataNode &output)
{
  if (m_localTimer.get() != SC_NULL)
    m_localTimer->getAll(output);

  scDataNode operatorTimers;
  collectOperatorTimers(operatorTimers);

  for(uint i=0, epos = operatorTimers.size(); i != epos; i++)
    output.merge(base::move<scDataNode>(operatorTimers[i]));
}

void sgpOperatorMonitorBasic::addCounter(const scString &name, scDataNode &output)
{
  ulong64 val = Counter::getTotal(name);
  output.addChild(name, new scDataNode(val));  
}

void sgpOperatorMonitorBasic::addTimer(const scString &code, const scString &name, cpu_ticks totalTime, bool withCodeName, 
  scDataNode &output)
{
  cpu_ticks subTime = Timer::getTotal(code);

  if (withCodeName) {
    // technical value
    output.addChild(code, new scDataNode(subTime));  
  } else {
    scString subValue;

    if (subTime >= 1000)
      subValue = toString(subTime/1000) + " s";

    if ((subTime % 1000 > 0) || (subTime < 1000)) 
    {
      if (!subValue.empty())
        subValue += " ";
      subValue += toString(subTime % 1000) + " ms";
    }

    if (totalTime > 0)  
      subValue += ", " + toString(subTime*100/totalTime) + "%";

    output.addChild(name, new scDataNode(subValue));  
  }   
}

void sgpOperatorMonitorBasic::setLastBestIslandId(uint islandId)
{
  m_lastBestIslandId = islandId;
}

void sgpOperatorMonitorBasic::addToStatsLog(const scDataNode &lineData) {
  m_experimentLog->addLineToCsvFile(lineData, "stats", "csv");
}

void sgpOperatorMonitorBasic::handleBeforeEvaluatePostProc(uint stepNo, const sgpGaGeneration &input) {    
#ifdef TRACE_TIME  
  Timer::start(TIMER_MONITOR);
#endif
  m_midFitValues.clear();
  m_midFitValues.setAsArray(vt_datanode);
  scDataNode fitVectorNode;
  sgpFitnessValue fitnessVector;

  uint bestIndex = sgpFitnessScanner(&input).getBestGenomeIndexByObjective(m_primaryObjectiveIndex, false);
  double bestFit;

  if (bestIndex < input.size())
    bestFit = input[bestIndex].getFitness();
  else  
    bestFit = 0.0;

  m_lastBestFit = bestFit;

  fitVectorNode.setAsArray(vt_double);

  for(uint j=0, eposj = input.size(); j != eposj; j++) 
  {
    input.at(j).getFitness(fitnessVector);
    fitVectorNode.clear();
    for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
    {
      fitVectorNode.addItemAsDouble(fitnessVector[i]);
    }
    m_midFitValues.addItem(fitVectorNode);
  }

#ifdef LOG_WHOLE_POP_FIT
  logWholePopFit(stepNo, m_midFitValues, "whole_pop_mfit");
#endif
    
#ifdef TRACE_TIME  
  Timer::stop(TIMER_MONITOR);
#endif
}

scString sgpOperatorMonitorBasic::getEvalContext(uint stepNo)
{
  return scString("Step=")+toString(m_startStepNo + stepNo);
}

void sgpOperatorMonitorBasic::init()
{
  inherited::init();
  resetLocalCounters();
}

void sgpOperatorMonitorBasic::resetLocalCounters()
{
  Counter::reset("gp-mon-fail-tour-for-best"); 
  Counter::reset("gp-mon-fail-match-for-best");
  Counter::reset("gp-mon-best-changed-mutate");
  Counter::reset("gp-mon-best-changed-xover");  
  Counter::reset("gp-mon-best-lost");
  Counter::reset("gp-mon-best-lost-unk");
}

void sgpOperatorMonitorBasic::handleBeforeEvaluate(uint stepNo, const sgpGaGeneration &input)
{
#ifdef DEBUG_OPER_EVAL
  cout << dateTimeToIsoStr(currentDateTime()) << ": " << getEvalContext(stepNo) << ", evaluation start"<<"\n";
#endif
  getNotifier()->notify(MONITOR_EVENT_BEFORE_EVAL_END);
}

void sgpOperatorMonitorBasic::handleAfterEvaluate(uint stepNo, const sgpGaGeneration &input) 
{      
#ifdef TRACE_TIME  
  Timer::start(TIMER_MONITOR);
#endif
  getNotifier()->notify(MONITOR_EVENT_AFTER_EVAL_BEGIN);

#ifdef DEBUG_OPER_EVAL
  cout << dateTimeToIsoStr(currentDateTime()) << ": " << getEvalContext(stepNo) << ", evaluation stop"<<"\n";
#endif

  sgpEntityIndexList idList;

  uint bestIndex, bestCount;
  sgpFitnessValue fitnessVector, fitnessSum;
  sgpFitnessValue mins, maxs, sums;
  scDataNode topElement;
  sgpWeightVector weights;

  m_fitnessFunc->getObjectiveWeights(weights);
  sgpFitnessScanner(&input).findBestWithWeights(bestIndex, bestCount, weights);
  
  assert(bestCount > 0);
  assert(bestIndex < input.size());
  
  input.at(bestIndex).getFitness(fitnessVector);

  handleBestEntityAfterEvaluate(input, bestIndex, bestCount, fitnessVector);
  
  fillFitnessStats(input, bestIndex, m_genLogLine);
  
  if (m_islandLimit > 0)
    logIslandStats(input);
  logDiversityStats(input);
  
  getNotifier()->notify(MONITOR_EVENT_AFTER_EVAL_END);

#ifdef TRACE_TIME  
  Timer::stop(TIMER_MONITOR);
#endif
}

void sgpOperatorMonitorBasic::handleBestEntityAfterEvaluate(const sgpGaGeneration &input, uint bestIndex, uint bestCount, const sgpFitnessValue &fitnessValue) 
{
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleEntityEvent(bestIndex, "selected-as-best");
#endif 

  if ((m_lastBestFitness.size() > m_primaryObjectiveIndex) 
   && (m_lastBestFitness[m_primaryObjectiveIndex] > fitnessValue[m_primaryObjectiveIndex])) {
    Counter::inc("gp-mon-best-lost");      
    Counter::inc("gp-mon-best-lost-step");      
  }
  
  m_lastBestFitness = fitnessValue; 
}

void sgpOperatorMonitorBasic::fillFitnessStats(const sgpGaGeneration &input, uint bestIndex, scDataNode &output)
{
  sgpGaGenomeList topList;
  sgpEntityIndexList idList;

  sgpFitnessValue fitnessVector, fitnessSum;
  sgpFitnessValue mins, maxs, sums;
  scDataNode topElement;
  double fit;
  sgpWeightVector weights;

  m_fitnessFunc->getObjectiveWeights(weights);
  
  input.at(bestIndex).getFitness(fitnessVector);
  
  sgpGaEvolver::getTopGenomesByWeights(input, MAX_TOP_SIZE, weights, idList);

  uint bestCount, popSize;
  if (m_lastBestInfo.get() != SC_NULL) {
    bestCount = m_lastBestInfo->bestCount;
    popSize = m_lastBestInfo->scannedCount;
  }
  else {
    bestCount = 0;
    popSize = 0;
  }
    
  output.clear();
  output.addChild("pop-size", new scDataNode(popSize));
  output.addChild("best-count", new scDataNode(bestCount));
  output.addChild("top-count", new scDataNode(idList.size()));

  //----- best middle value
  m_midFitValues.getElement(bestIndex, topElement);    
  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
  {
    output.addChild("best-mid-fit-"+toString(i), 
      new scDataNode( 
        topElement.getDouble(i)));
  }

  //----- recalculate middle fit-16
  for(uint i=0,epos=fitnessVector.size();i!=epos;i++) 
    fitnessVector[i] = topElement.getDouble(i);

  // GP-specific
  //TODO: move to application level
  if (fitnessVector.size() > 16)
    output.addChild("best-mid-fit-16a", new scDataNode(
      fitnessVector[16]));

  //----- recalculate middle totalPlus, totalMinus
  input.at(bestIndex).getFitness(fitnessVector);

  //----- best final value
  output.addChild("best-final-fit", new scDataNode(fitnessVector[0]));

  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
  {
    output.addChild("best-fin-fit-"+toString(i), new scDataNode(
      fitnessVector[i]));
  }

  //----- avg top middle value
  fitnessSum.resize(fitnessVector.size());
  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
    fitnessSum[i] = 0.0;
  
  scDataNode helper;
  scDataNode *nodePtr;

  for(uint j=0, eposj = idList.size(); j != eposj; j++) 
  {
    nodePtr = m_midFitValues.getNodePtr(idList[j], helper);
    
    for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
      fitnessSum[i] += nodePtr->getDouble(i);
  }    
  
  for(uint i=0,epos=fitnessSum.size();i!=epos;i++)
    fitnessSum[i] = fitnessSum[i] / double(idList.size()); 

  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
    output.addChild("avg-top-mid-fit-"+toString(i), new scDataNode(fitnessSum[i]));

  sgpFitnessValue avgTopMidFit = fitnessSum;

  //----- avg top final value
  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
    fitnessSum[i] = 0.0;
  
  for(uint j=0, eposj = idList.size(); j != eposj; j++) 
  {
    input.at(idList[j]).getFitness(fitnessVector);
    
    for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
      fitnessSum[i] += fitnessVector[i];
  }    
  
  for(uint i=0,epos=fitnessSum.size();i!=epos;i++)
    fitnessSum[i] = fitnessSum[i] / double(idList.size()); 

  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
    output.addChild("avg-top-fin-fit-"+toString(i), new scDataNode(fitnessSum[i]));

  //----- min & max of middle values of input
  mins.resize(fitnessVector.size());
  maxs.resize(fitnessVector.size());
  sums.resize(fitnessVector.size());
  
  if (m_midFitValues.size()) {
    m_midFitValues.getElement(0, topElement);
    for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
      mins[i] = topElement.getDouble(i);
    maxs = mins;
    sums = mins;
  }  
  
  for(uint j=1, eposj = m_midFitValues.size(); j != eposj; j++) 
  {
    //m_midFitValues.getElement(j, topElement);
    nodePtr = m_midFitValues.getNodePtr(j, helper);
    for(uint i=0,epos=fitnessVector.size();i!=epos;i++) {
      fit = nodePtr->getDouble(i);
      if (fit < mins[i])
        mins[i] = fit;
      else if (fit > maxs[i])  
        maxs[i] = fit;
      sums[i] += fit;  
    }     
  }  

  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
    output.addChild("min-inp-mid-fit-"+toString(i), new scDataNode(mins[i]));
  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
    output.addChild("max-inp-mid-fit-"+toString(i), new scDataNode(maxs[i]));
  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
    output.addChild("avg-inp-mid-fit-"+toString(i), new scDataNode(sums[i]/double(m_midFitValues.size())));
    
  //----- min & max of final values of input
  mins.resize(fitnessVector.size());
  maxs.resize(fitnessVector.size());
  if (input.size()) {
    input.at(0).getFitness(mins);
    maxs = mins;
  }  
  
  for(uint j=1, eposj = input.size(); j != eposj; j++) 
  {
    input.at(j).getFitness(fitnessVector);
    for(uint i=0,epos=fitnessVector.size();i!=epos;i++) {
      if (fitnessVector[i] < mins[i])
        mins[i] = fitnessVector[i];
      else if (fitnessVector[i] > maxs[i])  
        maxs[i] = fitnessVector[i];
    }    
  }  

  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
    output.addChild("min-inp-fin-fit-"+toString(i), new scDataNode(mins[i]));
  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
    output.addChild("max-inp-fin-fit-"+toString(i), new scDataNode(maxs[i]));


  //----- min & max of final values of top
  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
    mins[i] = maxs[i] = 0.0;
    
  if (idList.size()) {
    input.at(idList[0]).getFitness(mins);
    maxs = mins;
  }  
  
  for(uint j=1, eposj = idList.size(); j != eposj; j++) 
  {
    input.at(idList[j]).getFitness(fitnessVector);
    for(uint i=0,epos=fitnessVector.size();i!=epos;i++) {
      if (fitnessVector[i] < mins[i])
        mins[i] = fitnessVector[i];
      else if (fitnessVector[i] > maxs[i])  
        maxs[i] = fitnessVector[i];
    }    
  }  

  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
    output.addChild("min-top-fin-fit-"+toString(i), new scDataNode(mins[i]));
  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
    output.addChild("max-top-fin-fit-"+toString(i), new scDataNode(maxs[i]));

  //----- min & max of middle values of top
  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
    mins[i] = maxs[i] = 0.0;
    
  if (idList.size()) {
    m_midFitValues.getElement(idList[0], topElement);
    for(uint i=0,epos=fitnessVector.size();i!=epos;i++) 
      mins[i] = topElement.getDouble(i);      
    maxs = mins;
  }  
  
  for(uint j=1, eposj = idList.size(); j != eposj; j++) 
  {
    //m_midFitValues.getElement(idList[j], topElement);
    nodePtr = m_midFitValues.getNodePtr(idList[j], helper);
    //for(uint i=0,epos=topElement.size();i!=epos;i++) {
    for(uint i=0,epos=nodePtr->size();i!=epos;i++) {
      //fit = topElement.getDouble(i);
      fit = nodePtr->getDouble(i);
      if (fit < mins[i])
        mins[i] = fit;
      else if (fit > maxs[i])  
        maxs[i] = fit;
    }    
  }  

  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
    output.addChild("min-top-mid-fit-"+toString(i), new scDataNode(mins[i]));
  for(uint i=0,epos=fitnessVector.size();i!=epos;i++)
    output.addChild("max-top-mid-fit-"+toString(i), new scDataNode(maxs[i]));

  publishFitnessStats(avgTopMidFit, output);
}

void sgpOperatorMonitorBasic::publishFitnessStats(const sgpFitnessValue &avgTopMidFit, const scDataNode &fitStats)
{
  for(sgpMonitorFitStatsObserverList::iterator it = m_fitStatsObserverList.begin(), epos = m_fitStatsObserverList.end(); it != epos; ++it)
  {
    (*it)->handleFitnessStats(avgTopMidFit, fitStats);
  }
}

void sgpOperatorMonitorBasic::logIslandStats(const sgpGaGeneration &input)
{
  scDataNode islandItems;

  m_entityIslandTool->prepareIslandMap(input, islandItems);

  uint minSize, maxSize, totalSize, islandSize;

  minSize = maxSize = totalSize = 0;
  
  double avgSize;
  for(uint i=0, epos = islandItems.size(); i != epos; i++)
  {
    islandSize = islandItems[i].size();
    if (i == 0)
    {
      minSize = maxSize = totalSize = islandSize;
    } else {
      minSize = std::min<uint>(islandSize, minSize);
      maxSize = std::max<uint>(islandSize, maxSize);
      totalSize += islandSize;
    }
  }
  if (!islandItems.empty())
    avgSize = static_cast<double>(totalSize) / static_cast<double>(islandItems.size());
  else
    avgSize = 0.0;  
  
  m_genLogLine.addChild("island-size-min", new scDataNode(minSize));
  m_genLogLine.addChild("island-size-max", new scDataNode(maxSize));
  m_genLogLine.addChild("island-size-avg", new scDataNode(avgSize));
  m_genLogLine.addChild("island-migr-cnt", new scDataNode(Counter::getTotal("gp-island-mig-cnt-step")));
}

void sgpOperatorMonitorBasic::logDiversityStats(const sgpGaGeneration &input)
{
  uint cnt = calcShapeCount(input);
  m_genLogLine.addChild("shape-cnt", new scDataNode(cnt));  
}

void sgpOperatorMonitorBasic::performFinalReport(uint stepNo, const sgpGaGeneration &input)
{
  this->intExecute(stepNo, input);
  this->dumpExperimentStats(stepNo, input);
}

void sgpOperatorMonitorBasic::intExecute(uint stepNo, const sgpGaGeneration &input) 
{
#ifdef TRACE_TIME  
  Timer::start(TIMER_MONITOR);
#endif

  double maxFit;
  double avgFit;
  uint realStepNo = m_startStepNo + stepNo;
  uint popSize = getPopSize(input);

  prepareStepTime(stepNo);
  
#ifdef COUT_ENABLED
  if (realStepNo > 0) {
    cout << "-->> end of " << realStepNo - 1 << " <<--"<<"\n"; 
  }  
#endif
      
#ifdef TRACE_PARAMS
  if (realStepNo == 1)
    traceParams(popSize);      
#endif
    
  sgpOmBestInfo bestInfo;  
  
#ifdef DEBUG_OPER_MONITOR
  Log::addDebug("Fitness scan started..."); 
#ifdef COUT_ENABLED
  cout << "-- monitor.Execute: fitness scan start... -- " << "\n" << flush;    
#endif
#endif

  if (!m_genLogLine.isContainer())
    m_genLogLine.setAsParent();
        
  Timer::start("gx-mon-scan-fit");
  scanPopFitness(input, maxFit, avgFit, bestInfo, m_genLogLine); 
  Timer::stop("gx-mon-scan-fit");
  
#ifdef DEBUG_OPER_MONITOR
  Log::addDebug("Whole pop fitness scanned..."); 
#ifdef COUT_ENABLED
  cout << "-- monitor.Execute: fitness scan end... -- " << "\n" << flush;    
#endif
#endif

  invokeYield();

  sgpEntityBase *bestInputPtr = bestInfo.getEntity();
  uint bestCount = bestInfo.bestCount;
  uint bestIslandId = bestInfo.islandId;
  uint maxIdx = bestInfo.itemIndex;
  
  setLastBestIslandId(bestIslandId);
  
#ifdef REPORT_STEP_CHANGE
  cout << "-->> start of " << (realStepNo) << " / " << m_restartCount << " <<--"<<"\n"; 
  cout << "PS: "<<popSize;
  cout <<", avg: "<<avgFit;
  cout <<", max: "<<maxFit;
#endif

#ifdef REPORT_PERFORMANCE  
  reportPerformance(realStepNo); 
#endif
  
  if ((getFeatures() & mfLogPerformance) != 0)  
    logPerformance(realStepNo);  
        
  if (!bestInfo.isNull()) {
#ifdef REPORT_BEST_ENTITY
    reportBestItem(realStepNo, maxIdx, bestCount, bestInputPtr);
#endif
#ifdef TRACE_BEST_ENTITY
    traceBestItem(stepNo, maxIdx, bestInputPtr, 
      0.0);
#endif      
  }  
  
//-----------------------------------------------------------------------    
  logStepStats(realStepNo);
  m_genLogLine.clear();
  
//-----------------------------------------------------------------------    
#ifdef COUT_ENABLED
  cout << "---------------------------- " << "\n";
#endif

  scDataNode operatorCounters;
  collectOperatorCounters(operatorCounters);
  processOperatorCountersAll(operatorCounters);

  scDataNode reportedCounters(operatorCounters);
  reportedCounters.addElement("pop_size", scDataNode(popSize));
  execReportCounters(reportedCounters);

#ifdef REPORT_SURVIVAL_STEP_STATS
  reportSurvivalStats(bestCount);
#endif

#ifdef COUT_ENABLED
  cout << "\n";        
#endif
  
#ifdef REPORT_EXP_PARAMS_STEP
  if (m_experimentParams != SC_NULL)
  {
#ifdef COUT_ENABLED
  cout << "-- experiment params for best item -- " << "\n";    
#endif
    logExperimentParams(bestIslandId);
#ifdef COUT_ENABLED
    cout << "-- " << "\n";    
#endif
  }
#endif
  
  resetStepStats();    

#ifdef DEBUG_OPER_MONITOR
#ifdef COUT_ENABLED
    cout << "-- monitor.Execute: before check dump -- " << "\n" << flush;    
#endif
#endif
  checkFullDump(realStepNo, input);

#ifdef DEBUG_OPER_MONITOR
#ifdef COUT_ENABLED
    cout << "-- monitor.Execute: before notify -- " << "\n" << flush;    
#endif
#endif
  getNotifier()->notify(MONITOR_EVENT_EXEC_END);

#ifdef DEBUG_OPER_MONITOR
  Log::addDebug("monitor.Execute: end"); 
#endif

#ifdef TRACE_TIME  
  Timer::stop(TIMER_MONITOR);
#endif
}

void sgpOperatorMonitorBasic::collectOperatorCounters(scDataNode &output)
{
  output.clear();
  output.setAsParent();
  scDataNode operCounters;
  scString name;

  for(sgpMonitorStatsSrcList::iterator it = m_statsSrcList.begin(), epos = m_statsSrcList.end(); it != epos; ++it)
  {
    (*it)->getCounters(operCounters);
    (*it)->getName(name); 
    output.addElement(name, operCounters);
    operCounters.clear();
  }
}

void sgpOperatorMonitorBasic::collectOperatorTimers(scDataNode &output)
{
  output.clear();
  output.setAsParent();
  scDataNode operTimers;
  scString name;

  for(sgpMonitorStatsSrcList::iterator it = m_statsSrcList.begin(), epos = m_statsSrcList.end(); it != epos; ++it)
  {
    (*it)->getTimers(operTimers);
    (*it)->getName(name); 
    output.addElement(name, base::move<scDataNode>(operTimers));
    operTimers.clear();
  }
}

void sgpOperatorMonitorBasic::scanPopFitness(const sgpGaGeneration &input, double &maxFit, double &avgFit, sgpOmBestInfo &bestInfo, 
  scDataNode &fitnessStats)
{
  sgpEntityBasePtr bestInputPtr;

  bestInfo.scannedCount = 0;
  calcFitStats(input, maxFit, avgFit);

  findBestItemInfo(input, 
    bestInfo.itemIndex, bestInputPtr, bestInfo.bestCount, bestInfo.bestValue
  );
    
  bestInfo.scannedCount += input.size();

  bestInfo.islandId = getIslandId(bestInputPtr);  
  bestInfo.setEntity(dynamic_cast<sgpEntityBase *>(input.newItem(*bestInputPtr)));  

  fitnessStats.setElementSafe("pop-size", scDataNode(bestInfo.scannedCount));  
  fitnessStats.setElementSafe("best-count", scDataNode(bestInfo.bestCount));  
}
    
uint sgpOperatorMonitorBasic::getIslandId(sgpEntityBase *workInfo)
{
  uint res = m_islandLimit;
  
  if (m_experimentParams == SC_NULL)
    return res;
    
  uint islandId;
  
  if (!m_entityIslandTool->getIslandId(*workInfo, islandId))
    return res;

  res = islandId;    
  return res;
}
    
uint sgpOperatorMonitorBasic::getPopSize(const sgpGaGeneration &input)
{
  return input.size();
}

void sgpOperatorMonitorBasic::calcFitStats(const sgpGaGeneration &input, double &maxFit, double &avgFit)
{
  double totalFit = 0.0;
  double fit;
  uint maxIdx;
     
  maxFit = 0.0;
  avgFit = 0.0;

  if (input.size() > 0) {
    maxFit = input.at(0).getFitness(m_primaryObjectiveIndex); //fitness
  } 
      
  for(int it = input.beginPos(), epos = input.size(); it != epos; it++)
  {
    fit = input[it].getFitness(m_primaryObjectiveIndex);
    totalFit += fit;
    if (fit > maxFit) {
      maxFit = fit;
      maxIdx = it;
    }  
  }

  if (input.size() > 0) {
    avgFit = totalFit / input.size();
  } else {
    avgFit = 0.0;
  }
}

void sgpOperatorMonitorBasic::findBestItemInfo(const sgpGaGeneration &input, 
  uint &bestIndex, sgpEntityBasePtr &workInfo, uint &bestCount, double &bestValue
  )
{
  sgpWeightVector weights;
  m_fitnessFunc->getObjectiveWeights(weights);
  
  sgpFitnessScanner scanner(&input);
  bestIndex = scanner.findBestWithWeights(weights);  
  
  if (bestIndex < input.size()) {
    workInfo = 
      dynamic_cast<sgpEntityBase *>(
        &(const_cast<sgpEntityBase &>(
          input[bestIndex])));
  } else {
    workInfo = SC_NULL;
  } 
  
  bestValue = input.at(bestIndex).getFitness(m_primaryObjectiveIndex);
  bestCount = scanner.countGenomesByObjectiveValue(m_primaryObjectiveIndex, bestValue);
}  

void sgpOperatorMonitorBasic::reportBestItem(uint stepNo, uint bestIndex, uint bestCount, 
  sgpEntityBase *bestInputPtr
  )
{
#ifdef COUT_ENABLED
    cout << "\n-- Best item ("<<bestIndex<<" * " << bestCount << ") --"<<"\n";
#endif

    scStringList lines;
    sgpStringListWriter writer(lines);
    if (bestInputPtr != NULL)
      listEntity(bestInputPtr, melbCode + melbInfo + melbFitness + melbConsoleOut, &writer);

    writeLinesToConsole(lines);
}  

void sgpOperatorMonitorBasic::execReportCounters(const scDataNode &stepCounters)
{
  for(sgpMonitorReporterList::iterator it = m_reporterList.begin(), epos = m_reporterList.end(); it != epos; ++it)
  {
    (*it)->reportCounters(stepCounters, m_counters);
  }
}

void sgpOperatorMonitorBasic::processOperatorCountersAll(const scDataNode &values)  
{
  scDataNode helper;
  for(uint i=0, epos = values.size(); i != epos; i++)
  {
#ifdef COUT_ENABLED
    cout << "-- counters for " << values.getElementName(i) << " --" << "\n";    
#endif
    processOperatorCounters(*(const_cast<scDataNode &>(values).getNodePtr(i, helper)));
  }
}

void sgpOperatorMonitorBasic::processOperatorCounters(const scDataNode &values)  
{
  scString cname, cnameDisp;
  std::set<scString> sortedNameList;
  for(uint i=0,epos=values.size(); i!=epos; i++) {
    sortedNameList.insert(values.getElementName(i));
  }

  uint value;
  scString displayName;
  
  for(std::set<scString>::const_iterator it = sortedNameList.begin(),epos=sortedNameList.end(); it!=epos; ++it) {
    cname = *it;

    if (values[*it].isContainer())
    {
      displayName = values[*it].getString("label");
      value = values[*it].getUInt("value");
    } else {
      displayName = cname;
      value = values[*it].getAsUInt();
    }

    cnameDisp = displayName;       
#ifdef COUT_ENABLED    
    cout << strPadThis(cnameDisp, 32) << ": " << value << "\n";
#endif    
    m_counters.setUIntDef(cname, 
      m_counters.getUInt(cname, 0) + 
      value);
  }    
}

//TODO: implement as external counter source
void sgpOperatorMonitorBasic::addCountersToLogLine(scDataNode &logLine)
{
  logLine.addChild("mon-best-ignored-tour", new scDataNode(
    Counter::getTotal("gp-mon-fail-tour-for-best")));
  logLine.addChild("mon-best-ignored-match", new scDataNode(
    Counter::getTotal("gp-mon-fail-match-for-best")));
  logLine.addChild("mon-best-changed-mut", new scDataNode(
    Counter::getTotal("gp-mon-best-changed-mutate")));
  logLine.addChild("mon-best-changed-xover", new scDataNode(
    Counter::getTotal("gp-mon-best-changed-xover")));        
  logLine.addChild("mon-best-lost-cnt", new scDataNode(
    Counter::getTotal("gp-mon-best-lost")
    ));
  logLine.addChild("mon-best-lost-cnt-step", new scDataNode(
    Counter::getTotal("gp-mon-best-lost-step")
    ));
  logLine.addChild("gp-mon-best-lost-unk", new scDataNode(
    Counter::getTotal("gp-mon-best-lost-unk")
    ));
  logLine.addChild("gp-mon-best-lost-unk-step", new scDataNode(
    Counter::getTotal("gp-mon-best-lost-unk-step")
    ));
}

void sgpOperatorMonitorBasic::reportSurvivalStats(uint bestCount)
{
  uint rejectsCnt = 
    Counter::getTotal("gp-mon-fail-tour-for-best")+
    Counter::getTotal("gp-mon-fail-match-for-best")+
    Counter::getTotal("gp-mon-best-changed-mutate")+
    Counter::getTotal("gp-mon-best-changed-xover");      

  uint rejectsCntStep = 
    Counter::getTotal("gp-mon-fail-tour-for-best-step")+
    Counter::getTotal("gp-mon-fail-match-for-best-step")+
    Counter::getTotal("gp-mon-best-changed-mutate-step")+
    Counter::getTotal("gp-mon-best-changed-xover-step");      

  if ((rejectsCntStep == 0) && (Counter::getTotal("gp-mon-best-lost-step") > 0)) {
    Counter::inc("gp-mon-best-lost-unk");
    Counter::inc("gp-mon-best-lost-unk-step");
  }  

  float bestSurvivalProb;
  if (rejectsCntStep < bestCount) 
  // number of rejects / (total number of best items * (number of reject types = 4))
    bestSurvivalProb = 100.0 * (1.0 - float(rejectsCntStep)/float(REJECT_TYPE_CNT*bestCount));
  else
    bestSurvivalProb = 0.0;  
  
#ifdef COUT_ENABLED
  cout << "-- counters for monitor -- " << "\n";    
  cout << "Best could disappear because: " << "\n";
  cout << "... ignored by tournament: " << 
    Counter::getTotal("gp-mon-fail-tour-for-best") << " times " << "\n";      
  cout << "... failed by match: " << 
    Counter::getTotal("gp-mon-fail-match-for-best") << " times " << "\n";      
  cout << "... changed by mutation: " << 
    Counter::getTotal("gp-mon-best-changed-mutate") << " times " << "\n";      
  cout << "... changed by xover: " << 
    Counter::getTotal("gp-mon-best-changed-xover") << " times " << "\n";  

  cout << "Best rejects (all) : " << rejectsCnt << "\n";  
  cout << "Best rejects (step - all types) : " << rejectsCntStep << "\n";  
  cout << "Best lost (in fact): " << Counter::getTotal("gp-mon-best-lost") << " times" << "\n";
  cout << "... lost without rejects - all: " << Counter::getTotal("gp-mon-best-lost-unk") << "\n";
  cout << "... lost without rejects - step: " << Counter::getTotal("gp-mon-best-lost-unk-step") << "\n";
  
  float prcValue = (double(Counter::getTotal("tour-match-failed-count")) / Counter::getTotal("tour-match-count"))*100.0;
  
  cout << "Tournament matches: " << Counter::getTotal("tour-match-count") << "\n";
  cout << "... worse item won: " << Counter::getTotal("tour-match-failed-count") << "; " << prcValue << "%" << "\n";
  cout << "Best node survives with prob: " << bestSurvivalProb << "%" << "\n";  
#endif
}

void sgpOperatorMonitorBasic::initStepTime()
{
    m_lastStepNoForTime = 0;
    m_lastStepTime = 0;
    m_localTimer->stop(TIMER_EXPERIMENT_STEP);
    m_localTimer->reset(TIMER_EXPERIMENT_STEP);
    m_localTimer->start(TIMER_EXPERIMENT_STEP);
}

void sgpOperatorMonitorBasic::prepareStepTime(uint stepNo)
{
  if (m_lastStepNoForTime != stepNo) {
    m_lastStepNoForTime = stepNo;
    m_localTimer->stop(TIMER_EXPERIMENT_STEP);
    m_lastStepTime = m_localTimer->getTotal(TIMER_EXPERIMENT_STEP);
    m_localTimer->reset(TIMER_EXPERIMENT_STEP);
    m_localTimer->start(TIMER_EXPERIMENT_STEP);
  }
}

void sgpOperatorMonitorBasic::reportPerformance(uint stepNo)
{  
  for(sgpMonitorReporterList::iterator it = m_reporterList.begin(), epos = m_reporterList.end(); it != epos; ++it)
  {
    (*it)->reportPerformance(stepNo, m_lastStepTime);
  }

}  

void sgpOperatorMonitorBasic::reportStepStats(uint stepNo, const scDataNode &statsFilter)
{
  scDataNode timers, counters;
  Counter::getByFilter(statsFilter, counters);

#ifdef REPORT_COUNTER_STEP_STATS
  reportStats(stepNo, counters, "Counters");
#endif

  scDataNode stepCounters;
  calcStepCounters(counters, m_lastCounters, stepCounters); 

#ifdef REPORT_COUNTER_STEP_STATS
  reportStats(stepNo, stepCounters, "Step counters");
#endif

  Timer::getByFilter(statsFilter, timers);

#ifdef REPORT_TIMER_STEP_STATS
  reportStats(stepNo, timers, "Timers");
#endif
}

void sgpOperatorMonitorBasic::reportFinalStats(uint stepNo, const scDataNode &statsFilter)
{
  scDataNode timers, counters;
  Counter::getByFilter(statsFilter, counters);

  reportStats(stepNo, counters, "Counters");

  Timer::getByFilter(statsFilter, timers);

  reportStats(stepNo, timers, "Timers");
}

void sgpOperatorMonitorBasic::reportStats(uint stepNo, const scDataNode &statsValues, const scString &groupName)
{
  cout << "\n" << "-- " << groupName << " --" << "\n"; 

  std::set<scString> sortedNameList;  
  for(uint i=0,epos=statsValues.size(); i!=epos; i++) {
    sortedNameList.insert(statsValues.getElementName(i));
  }

  uint uvalue; 
  scString cname, cnameDisp;

  for(std::set<scString>::const_iterator it = sortedNameList.begin(),epos=sortedNameList.end(); it!=epos; ++it) {
    cname = *it;
    cnameDisp = cname;       
    uvalue = statsValues[*it].getAsUInt();
    cout << strPadRight(cnameDisp, 35, ' ') << ": " << uvalue;
    cout << "\n";
  }

  cout << "\n";
}

void sgpOperatorMonitorBasic::calcStepCounters(const scDataNode &totalCounters, scDataNode &lastCounters, scDataNode &stepCounters)
{
  ulong64 priorVal, newVal;
  scString name;
  stepCounters.clear();
  stepCounters.setAsParent();
  for(uint i=0, epos = totalCounters.size(); i != epos; i++) {
    name = totalCounters.getElementName(i);
    priorVal = lastCounters.getUInt64(name, 0);
    newVal = totalCounters.getUInt64(i);
    if (newVal > priorVal)
      stepCounters.setElementSafe(name, scDataNode(newVal - priorVal));
    else
      stepCounters.setElementSafe(name, scDataNode(0));
    lastCounters.setElementSafe(name, scDataNode(newVal));
  }
}

void sgpOperatorMonitorBasic::resetStepStats()
{  
  getNotifier()->notify(MONITOR_EVENT_RESET_STEP_STATS);
  
  Counter::reset("gp-mon-fail-tour-for-best-step"); 
  Counter::reset("gp-mon-fail-match-for-best-step");
  Counter::reset("gp-mon-best-changed-mutate-step");
  Counter::reset("gp-mon-best-changed-xover-step");  
  Counter::reset("gp-mon-best-lost-step");      
  Counter::reset("gp-mon-best-lost-unk-step");
  Counter::reset("gp-island-mig-cnt-step");
}

void sgpOperatorMonitorBasic::logStepStats(uint stepNo)
{  
  scDataNode logLine;
  logLine.addChild("step-no", new scDataNode(stepNo));
  logLine.addChild("when", new scDataNode(dateTimeToIsoStr(currentDateTime())));
  if (m_genLogLine.isParent())
    logLine.copyChildrenFrom(m_genLogLine);
  
  addCountersToLogLine(logLine);
  addToStatsLog(logLine);
}
  
void sgpOperatorMonitorBasic::checkFullDump(uint stepNo, const sgpGaGeneration &input)
{
  if ((m_fullDumpInterval > 0) && (stepNo % m_fullDumpInterval == 0) && (stepNo > 0))
  {
    Log::addDebug("Performing full dump: begin");
    performFullDump(stepNo, input);
    Log::addDebug("Performing full dump: end");
  }
}
         
void sgpOperatorMonitorBasic::performFullDump(uint stepNo, const sgpGaGeneration &input)
{
#ifndef TRACE_ENTITY_BIO    
    traceFullDump(stepNo, input);
#endif    
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::performDump();
#endif        
}

void sgpOperatorMonitorBasic::traceParams(uint popSize)
{
  scDataNode params;
  params.addChild("pop-size", new scDataNode(popSize));

  scDataNode reportedParams;
  getParamsFromParamReporters(reportedParams);
  if (!reportedParams.isNull())
    params.merge(reportedParams);

  int funcId = m_fitnessFunc->getFunctionId();
  params.addChild("function-id", new scDataNode(funcId));
  
  for(uint i = 0, epos = m_runParams.size(); i != epos; i++)
    params.addElement(m_runParams.getElementName(i), m_runParams.getElement(i));
  traceParams(params, "params");
}

void sgpOperatorMonitorBasic::traceParams(const scDataNode &paramsCollection, const scString &fname)
{
  scStringList lines;
  scString lineItem;  
  scString paramName;
  
  lineItem = "---- "+dateTimeToIsoStr(currentDateTime()) + " ----";
  lines.push_back(lineItem);

  uint maxLen = 0;
  
  for(uint i=0, epos = paramsCollection.size(); i != epos; i++) 
  {
    paramName = paramsCollection.getElementName(i);
    if (paramName.length() > maxLen)
      maxLen = paramName.length();
  }    
  
  for(uint i=0, epos = paramsCollection.size(); i != epos; i++) 
  {
    paramName = paramsCollection.getElementName(i);
    strPadThis(paramName, maxLen);   
    lineItem = paramName+": "+paramsCollection.getString(i);
    lines.push_back(lineItem);
  }
  m_experimentLog->addLineListToTxtFile(lines, fname, "txt");
}

void sgpOperatorMonitorBasic::traceFullDump(uint stepNo, const sgpGaGeneration &input)
{
  Timer::start("io-dump-work");

#ifdef DEBUG_DUMP
  Log::addDebug("traceFullDump - begin"); 
#endif

  grdEventTrace::addStep("before-dump", "dump", 1);

  const uint SHORT_YIELD_INTERVAL = 100;
  const uint LINES_PER_WRITE = 20000;

  scString fname = genPopDumpFileName(stepNo); 
  scString fext = "txt";
  //scStringList lines, entityLines;
  scString lineItem;  
  scDataNode shapes;

#ifdef DEBUG_DUMP
  Log::addDebug("traceFullDump - a1"); 
#endif

  Timer::start("io-dump-work-shapes");

  prepareShapeCollection(input, shapes);

  Timer::stop("io-dump-work-shapes");

#ifdef DEBUG_DUMP
  Log::addDebug("traceFullDump - a2"); 
#endif

  std::auto_ptr<sgpExperimentLogWriterIntf> logWriter(
    m_experimentLog->createTxtWriter(fname, fext));

  std::auto_ptr<sgpEntityListingWriter> entityWriter(
    new sgpEntityListingWriter(logWriter.get()));

  //addPopDumpHeader(stepNo, lines);
  Timer::start("io-dump-work-wr");
  Timer::start("io-dump-work-wr-hdr");
  addPopDumpHeader(stepNo, logWriter.get());
  Timer::stop("io-dump-work-wr-hdr");
  Timer::stop("io-dump-work-wr");
  
  sgpEntityBase *workInfo;
  std::vector<UIntDoublePair> shapeSort; 
  uint shapeIdx; 
  
  double fitValue;
  uint itemIndex;
   
  Timer::start("io-dump-work-shapes-col");

  shapeSort.clear();
  for(uint j=0, eposj = shapes.size(); j != eposj; j++) 
  {
    assert(!shapes[j].empty());
    itemIndex = shapes[j].getUInt(0);
    workInfo = 
      dynamic_cast<sgpEntityBase *>(
        &(const_cast<sgpEntityBase &>(
          input[itemIndex])));
    fitValue = workInfo->getFitness(m_shapeObjectiveIndex);      
    shapeSort.push_back(std::make_pair(j, fitValue));

    Timer::start("io-dump-work-shapes-yield");
    if (j % SHORT_YIELD_INTERVAL == 0)
      invokeYield();
    Timer::stop("io-dump-work-shapes-yield");
  }

  Timer::stop("io-dump-work-shapes-col");

  Timer::start("io-dump-work-shapes-sort");
  std::sort(shapeSort.begin(), shapeSort.end(), sort_second_pred_gt);         
  Timer::stop("io-dump-work-shapes-sort");


#ifdef DEBUG_DUMP
  Log::addDebug("traceFullDump - a3"); 
#endif

  //lines.clear();

  for(uint shapeIdxSorted = 0, eposj = shapeSort.size(); shapeIdxSorted != eposj; shapeIdxSorted++) 
  {
    shapeIdx = shapeSort[shapeIdxSorted].first;
    itemIndex = shapes[shapeIdx].getUInt(0);
    workInfo = 
      dynamic_cast<sgpEntityBase *>(
        &(const_cast<sgpEntityBase &>(
          input[itemIndex])));

    lineItem = "-- Shape: "+toString(shapeIdx)+ 
        "/ obj-"+toString(m_shapeObjectiveIndex)+"="+toString(workInfo->getFitness(m_shapeObjectiveIndex))+
        "/ cnt="+toString(shapes[shapeIdx].size());

    Timer::start("io-dump-work-wr");
    logWriter->writeLine(lineItem);
    Timer::stop("io-dump-work-wr");
    
    for(uint i=0, epos = shapes[shapeIdx].size(); i != epos; i++)
    {  
      itemIndex = shapes[shapeIdx].getUInt(i);
      lineItem = "-- Item: "+toString(itemIndex);

      Timer::start("io-dump-work-wr");
      logWriter->writeLine(lineItem);
      Timer::stop("io-dump-work-wr");

      Timer::start("io-dump-work-wr");
      dumpEntityToWriter(input, itemIndex, entityWriter.get());
      Timer::stop("io-dump-work-wr");

    }  
  }

  Timer::start("io-dump-work-close");
  logWriter->close();
  Timer::start("io-dump-work-close");

  grdEventTrace::addStep("after-dump", "dump", 2);

#ifdef DEBUG_DUMP
  Log::addDebug("traceFullDump - end"); 
#endif

  Timer::stop("io-dump-work");
}

scString sgpOperatorMonitorBasic::genPopDumpFileName(uint stepNo)
{
  return scString("pop_dump_")+toString(stepNo);
}

void sgpOperatorMonitorBasic::addPopDumpHeader(uint stepNo, scStringList &lines)
{
  scString lineItem;  
  lineItem = "---- "+dateTimeToIsoStr(currentDateTime()) + " ----";
  lines.push_back(lineItem);
  lineItem = "Step: "+toString(stepNo);
  lines.push_back(lineItem);
}

void sgpOperatorMonitorBasic::addPopDumpHeader(uint stepNo, sgpExperimentLogWriterIntf *writer)
{
  Timer::start("io-dump-work-hdr-1");
  scString lineItem;  
  lineItem = "---- "+dateTimeToIsoStr(currentDateTime()) + " ----";
  writer->writeLine(lineItem);
  Timer::stop("io-dump-work-hdr-1");
  Timer::start("io-dump-work-hdr-2");
  lineItem = "Step: "+toString(stepNo);
  writer->writeLine(lineItem);
  Timer::start("io-dump-work-hdr-2");
}

void sgpOperatorMonitorBasic::dumpEntityToWriter(const sgpGaGeneration &input, uint itemIndex, sgpEntityListingWriterIntf *writer)
{
  scDataNode code, itemData, infoBlock;
  
  sgpEntityBase *workInfo = 
    dynamic_cast<sgpEntityBase *>(
      &(const_cast<sgpEntityBase &>(
        input[itemIndex])));

  assert(m_entityLister != SC_NULL);
  m_entityLister->listEntity(workInfo, melbDump, writer);
}

void sgpOperatorMonitorBasic::traceBestItem(uint stepNo, uint bestIndex, sgpEntityBase *workInfo, 
  double finalFit)
{
  std::auto_ptr<sgpExperimentLogWriterIntf> logWriter(
    m_experimentLog->createTxtWriter("best_item", "txt"));

  std::auto_ptr<sgpEntityListingWriter> entityWriter(
    new sgpEntityListingWriter(logWriter.get()));

  entityWriter->writeLine("---- "+dateTimeToIsoStr(currentDateTime()) + " ----");
  entityWriter->writeLine("Step: "+toString(stepNo)+", best: "+toString(bestIndex));
  
  if (workInfo != NULL)
    listEntity(workInfo, melbCode + melbInfo + melbFitness, entityWriter.get());

  logWriter->close();  
}

// performed when worse item wins
void sgpOperatorMonitorBasic::traceFailedMatch(ulong64 stepNo, ulong64 groupNo, ulong64 matchNo, 
   const sgpGaGeneration &input, uint first, uint second, int matchRes)
{
  std::auto_ptr<sgpExperimentLogWriterIntf> logWriter(
    m_experimentLog->createTxtWriter("match_failed", "txt"));

  std::auto_ptr<sgpEntityListingWriter> entityWriter(
    new sgpEntityListingWriter(logWriter.get()));
   
  entityWriter->writeLine("---- "+dateTimeToIsoStr(currentDateTime()) + " ----");
  entityWriter->writeLine("Step: "+toString(stepNo)+", group: "+toString(groupNo)+", match: "+toString(matchNo));
  scString matchDesc;
  uint looserIndex;
  
  if (matchRes < 0) {
    matchDesc = "second";
    looserIndex = first;
  } else if (matchRes > 0) {
    matchDesc = "first";
    looserIndex = second;
  } else {
    matchDesc = "both";  
    looserIndex = input.size();
  }  

  if ((looserIndex < input.size()) && isSameAsBestEntity(input, looserIndex)) {
    Counter::inc("gp-mon-fail-match-for-best");
    Counter::inc("gp-mon-fail-match-for-best-step");
  }
      
  entityWriter->writeLine("Match result: "+toString(matchRes)+" ("+matchDesc+")");
  entityWriter->writeLine("");

  scStringList entityListing;

  entityWriter->writeLine("-- First --");
  listEntity(&(input[first]), melbCode, entityWriter.get());

  entityWriter->writeLine("-- Second --");
  listEntity(&(input[second]), melbCode, entityWriter.get());

  entityWriter->writeLine("-- Fitness --");

  sgpFitnessValue fitVectorFirst, fitVectorSecond;
  input.at(first).getFitness(fitVectorFirst);
  input.at(second).getFitness(fitVectorSecond);
  
  listFitnessPair(fitVectorFirst, fitVectorSecond, entityWriter.get());
  
  logWriter->close();  
}   

void sgpOperatorMonitorBasic::traceFailedTournament(const sgpGaGeneration &input, uint itemIndex)
{
  if (isSameAsBestEntity(input, itemIndex)) {
    Counter::inc("gp-mon-fail-tour-for-best");
    Counter::inc("gp-mon-fail-tour-for-best-step");
  }  
}

void sgpOperatorMonitorBasic::traceGenomeChanged(const sgpGaGeneration &input, uint itemIndex, const scString &sourceName)
{
  if (isSameAsBestEntity(input, itemIndex)) {
    if (sourceName == "mutate") {
      Counter::inc("gp-mon-best-changed-mutate");
      Counter::inc("gp-mon-best-changed-mutate-step");
    }  
    else if (sourceName == "xover") {
      Counter::inc("gp-mon-best-changed-xover");
      Counter::inc("gp-mon-best-changed-xover-step");
    }  
  }    
}

bool sgpOperatorMonitorBasic::isSameAsBestEntity(const sgpGaGeneration &input, uint itemIndex)
{
  bool res = true;
  sgpFitnessValue fitVector;
  input.at(itemIndex).getFitness(fitVector);
  
  uint minSize = std::min<uint>(fitVector.size(), m_lastBestFitness.size());

  if (minSize < 1) 
    res = false;
  else {
    for(uint i=sgpFitnessValue::SGP_OBJ_OFFSET, epos = minSize; i != epos; i++) {
      if (m_lastBestFitness[i] != fitVector[i]) {
        res = false;
        break;
      }
    }
  }
  return res;
}

void sgpOperatorMonitorBasic::logPerformance(uint stepNo)
{
  scDataNode perfData, perfLine;

  perfLine.clear();
  perfLine.setAsParent();

  for(sgpMonitorReporterList::iterator it = m_reporterList.begin(), epos = m_reporterList.end(); it != epos; ++it)
  {
    perfData.clear();
    (*it)->getPerformanceData(stepNo, m_lastStepTime, perfData);
    if (!perfData.isNull())
      perfLine.merge(perfData);
  }

  if (!perfData.empty())
    logPerformance(perfData);

}        

void sgpOperatorMonitorBasic::logPerformance(const scDataNode &lineData)
{
  m_experimentLog->addLineToCsvFile(lineData, "perf", "csv");
}

void sgpOperatorMonitorBasic::listFitnessPair(const sgpFitnessValue &fitVectorFirst, const sgpFitnessValue &fitVectorSecond, sgpEntityListingWriterIntf *writer)
{
  scString fitFirst, fitSecond;
  for(uint i=sgpFitnessValue::SGP_OBJ_OFFSET, epos = fitVectorFirst.size(); i != epos; i++) {
    fitFirst = toString(fitVectorFirst[i]);
    strPadThis(fitFirst, 32);   
    writer->writeLine("- Obj[" + toString(i) + "] = " + fitFirst + ", " + toString(fitVectorSecond[i]));
  }  
}

void sgpOperatorMonitorBasic::writeLinesToConsole(const scStringList &lines)
{
#ifdef COUT_ENABLED
  for(scStringList::const_iterator it = lines.begin(), epos = lines.end(); it != epos; ++it)
  {
    cout << *it << "\n";
  }  
#endif  
}

void sgpOperatorMonitorBasic::logWholePopFitFromGeneration(uint stepNo, const sgpGaGeneration &input, const scString &varName)
{
  sgpFitnessValue fitnessVector;
  scDataNode fitValueList, fitRow;

  for(uint j=input.beginPos(), eposj = input.size(); j != eposj; j++) 
  {
    input.at(j).getFitness(fitnessVector);
    for(uint i=0,epos=fitnessVector.size();i!=epos;i++) {
      fitRow.addChild("obj_"+toString(i), new scDataNode(fitnessVector[i]));
    }
    fitValueList.addChild(new scDataNode(fitRow));
    fitRow.clear();
  }  
  logWholePopFit(stepNo, fitValueList, varName);
}

// write whole population's fitness values to log
void sgpOperatorMonitorBasic::logWholePopFit(uint stepNo, const scDataNode &fitData, const scString &varName)
{
  scDataNode logLine, entityFit;
  scDataNode when(dateTimeToIsoStr(currentDateTime()));

  for(uint i=0,epos=fitData.size(); i != epos; i++) {
    logLine.addChild("step-no", new scDataNode(stepNo));
    logLine.addChild("when", new scDataNode(when));
    logLine.addChild("item", new scDataNode(i));
    fitData.getElement(i, entityFit);
    for(uint j=sgpFitnessValue::SGP_OBJ_OFFSET,eposj=entityFit.size(); j != eposj; j++) {
      logLine.addChild(scString("obj_")+toString(j), new scDataNode(entityFit.getElement(j)));
    }  
    m_experimentLog->addLineToCsvFile(logLine, varName, "csv");
    logLine.clear();
  }
}

void sgpOperatorMonitorBasic::prepareShapeCollection(const sgpGaGeneration &input, scDataNode &output)
{
  prepareShapeCollection(input, true, output);
}

void sgpOperatorMonitorBasic::prepareShapeCollection(const sgpGaGeneration &input, bool collectItems, scDataNode &output)
{
  double objValue;
  scString shapeName;
  std::map<double, scString> shapeMap;
  std::map<double, scString>::iterator shapeIt;
  StringBuilder builder;

  output.clear();
  for(uint i=0, epos = input.size(); i != epos; i++)
  {
    if (collectItems)
      for(uint j=0, eposj = output.size(); j != eposj; j++)
        assert(!output.getElement(j).empty());

    objValue = input[i].getFitness(m_shapeObjectiveIndex);
    shapeIt = shapeMap.find(objValue);
    if (shapeIt == shapeMap.end())
    {
      //shapeName = toString(objValue);
      builder.rewrite(objValue);
      builder.get(shapeName);

      if (!output.hasChild(shapeName)) {
        if (collectItems) {
          output.addChild(shapeName, new scDataNode(ict_array, vt_uint));
        } else {
          output.addChild(shapeName, new scDataNode());
        }
      }

      shapeMap.insert(std::make_pair(objValue, shapeName));
    } else {
      shapeName = (*shapeIt).second;
    }

    if (collectItems) 
      output[shapeName].addItem(i);  
  }

}  

uint sgpOperatorMonitorBasic::calcShapeCount(const sgpGaGeneration &input)
{
  scDataNode shapeCollection;
  prepareShapeCollection(input, false, shapeCollection);
  return shapeCollection.size();
}

void sgpOperatorMonitorBasic::logExperimentParams(uint bestIslandId)
{
  const uint OUT_WIDTH = 32;
  if (m_experimentParams == SC_NULL)
    return;
  scDataNode values;

  m_experimentParams->getParamValuesForGroup(bestIslandId, values);

#ifdef COUT_ENABLED
  scString nameDisp;
  nameDisp = "Island ID";
  strPadThis(nameDisp, OUT_WIDTH); 
  cout << nameDisp << ": " << bestIslandId << "\n";
  
  if (m_experimentParams != SC_NULL)
  {
    for(uint i=0, epos = values.size(); i != epos; i++)
    {
      nameDisp = values.getElementName(i);           
      cout << strPadThis(nameDisp, OUT_WIDTH) << ": " << values.getString(i) << "\n";
    }
  }  
#endif    
}
  
void sgpOperatorMonitorBasic::dumpExperimentStats(uint stepNo, const sgpGaGeneration &input) 
{
  //TODO: finish - log final experiment stats
}

uint sgpOperatorMonitorBasic::findBestIslandByWeights(const sgpGaGeneration &input)
{
  sgpWeightVector weights;
  m_fitnessFunc->getObjectiveWeights(weights);
  
  return sgpFitnessScanner(&input).findBestWithWeights(weights);
}

void sgpOperatorMonitorBasic::getParamsFromParamReporters(scDataNode &output)
{
  output.clear();
  scDataNode params;

  for(sgpMonitorParamReporterList::iterator it = m_paramReporterList.begin(), epos = m_paramReporterList.end(); it != epos; ++it)
  {
    (*it)->getReportedParams(params);

    if (!params.isNull()) {
      if (output.isNull()) {
        output.setAsParent();
      }
      output.merge(params);
    }

    params.clear();
  }
}

void sgpOperatorMonitorBasic::listEntity(const sgpEntityBase *entity, uint listBlocks, sgpEntityListingWriterIntf *writer) 
{
  assert(m_entityLister != SC_NULL);
  m_entityLister->listEntity(entity, listBlocks, writer);
}

void sgpOperatorMonitorBasic::listEntityCode(const scDataNode &code, sgpEntityListingWriterIntf *writer)
{
  assert(m_entityLister != SC_NULL);
  m_entityLister->listEntityCode(code, writer);
}

void sgpOperatorMonitorBasic::listInfoBlock(const scDataNode &code, sgpEntityListingWriterIntf *writer)
{
  assert(m_entityLister != SC_NULL);
  m_entityLister->listInfoBlock(code, writer);
}

