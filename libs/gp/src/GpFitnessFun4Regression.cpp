/////////////////////////////////////////////////////////////////////////////
// Name:        GpFitnessFun4Regression.cpp
// Project:     sgpLib
// Purpose:     Fitness function for Gp evolver - regression.
// Author:      Piotr Likus
// Modified by:
// Created:     02/05/2009
/////////////////////////////////////////////////////////////////////////////

// std
#include <limits>
#include <algorithm>
#include <numeric>

// sc
#include "sc/defs.h"
#include "sc/ompdefs.h"

#include "sc/log.h"
#include "sc/rand.h"
#include "sc/utils.h"
#include "sc/smath.h"
#include "sc/counter.h"
#include "sc/timer.h"
#include "sc/series.h"

// sgp
#include "sgp/GasmFunLibCore.h"
#include "sgp/GasmFunLibFMath.h"
#include "sgp/GasmFunLibComplex.h"
#include "sgp/GasmFunLibAnn.h"
#include "sgp/GasmFunLibMacros.h"
#include "sgp/GasmOperator.h"
#include "sgp/GasmOperatorMutate.h"

#ifdef TRACE_ENTITY_BIO
#include "sgp\GpEntityTracer.h"
#endif

#include "sgp/GpFitnessFun4Regression.h"
#include "sgp/GasmScannerForFitBlock.h"
#include "sgp/GasmScannerForFitUtils.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

using namespace dtp;

#define COUT_ENABLED
#define OPT_ADF_DEGRAD_SIZE  
//#define OPT_DYNAMIC_SAMPLE_RANGE
// define to verify output for #IND
#define SAFE_OUTPUT
//#define DEBUG_FITFUN_TEST
#define OPT_OBSERVE_PRG_OUTPUT
#define OPT_NARROW_OBJS

#ifdef USE_OPENMP
//#define USE_OPENMP_IN_RUNPRG_LOOP
#endif

#ifdef COUT_ENABLED
#include <iostream>
using namespace std;
#endif

const double HIT_THRESHOLD = 0.025;
const double SAMPLE_MIN_X = -3.14;
const double SAMPLE_MAX_X = 3.14;

const uint SAMPLE_RESET_INTERVAL = 1; // number of iterations between sample set reset

const uint RESTART_MIN_AVG_GENOME_SIZE = 2*4;
const double RESTART_MIN_PROGRESS_RATIO = 0.01;
const double RESTART_MIN_BEST_STD_DEV_RATIO = 0.01;    
const uint RESTART_BUFFER_SIZE = 25;
const uint RESTART_BUFFER_MARGIN = 5;

const ulong64 MIN_GENOME_SIZE = 4*2;
const double MIN_REQ_CONST_TO_ARG_RATIO = 0.5;
const double MAX_UNIQ_INSTR_CODES_RATIO = 0.5;
const double MIN_CONST_ONLY_INSTR_RATIO = 0.2;  
const double MIN_FAILED_WRITES = 0.1;
const double MIN_OUT_DIST_TO_END = 0.33;
const uint EXREME_NOISE_FILTER_STEP = 5;

const double CONST_OUT_ERROR_STD_DEV = 1E+128;
const double CONST_OUT_ERROR_SSE = 1E+128;
const double CONST_OUT_ERROR_SSE_ABS = 1E+128;
const double CONST_OUT_CODE_COST = 1E+100;
const double CONST_OUT_SIZE = 1000;
const double CONST_OUT_FAILED_WRITES = 100.0;
const double CONST_OUT_FADING_DIFF = 1E+128;
const double CONST_OUT_CORREL = 1E-100;
const double CONST_OUT_EXTREME_REL = 1.0;
const double CONST_OUT_FREQ_DIFF = 1E+128;
const double CONST_OUT_AMPLI_DIFF = 1E+128;
const double CONST_OUT_INC_DIFF = 1E+128;
const double CONST_OUT_REV_FX = 100.0;
const double CONST_OUT_REG_IO_DIST = 0.0;

const double NULL_INC_DIFF = 100.0;

const uint EXTREME_COUNT_GAP = 1;
const uint REV_FX_NULL_MACRO_NO = 0;



//#define COUT_ENABLED
#define EXTRA_VALIDATION
#ifdef EXTRA_VALIDATION
#define VALIDATE_NAN
#endif
#define STOP_ON_HITS  
//#define TEST_DYNAMIC

typedef std::pair<double, double> dim2rec;

bool less_2dims( dim2rec const &lhs, dim2rec const &rhs )
{
  return ((lhs.second < rhs.second) || ((lhs.second == rhs.second) && (lhs.first < rhs.first)));
}

sgpGpFitnessFun4Regression::sgpGpFitnessFun4Regression(): sgpFitnessFun4Gasm() {
  m_restartsEnabled = true;   

  m_sampleSide = 0; // no limits
  m_sampleChangeInterval = 0; //SAMPLE_RESET_INTERVAL

  m_minConstToArgRatio = MIN_REQ_CONST_TO_ARG_RATIO;
  m_maxReqUniqInstrCodeRatio = MAX_UNIQ_INSTR_CODES_RATIO; 

  m_onSamplesChanged = SC_NULL;
  m_onHandleProgramOutput = SC_NULL;
  m_yieldSignal = SC_NULL;

  m_nonFuncInstrEnabled = true;
  m_adfsEnabled = false;
  m_stepCounter = 0;
  m_totalCalc = 0; 

  m_expectedSize = 0;
}


sgpGpFitnessFun4Regression::~sgpGpFitnessFun4Regression()
{
}

void sgpGpFitnessFun4Regression::setAdfsEnabled(bool value)
{
  m_adfsEnabled = value;
}

void sgpGpFitnessFun4Regression::setNonFuncInstrEnabled(bool value)
{
  m_nonFuncInstrEnabled = value;
}

void sgpGpFitnessFun4Regression::setYieldSignal(scSignal *value)
{
  m_yieldSignal = value; 
}

void sgpGpFitnessFun4Regression::setExpectedSize(uint value)
{
  m_expectedSize = value;
}

uint sgpGpFitnessFun4Regression::getExpectedSize()
{
  return m_expectedSize;
}

uint sgpGpFitnessFun4Regression::getExpectedSize() const
{
  return m_expectedSize;
}
  
void sgpGpFitnessFun4Regression::setMacrosEnabled(bool value)
{
  m_macrosEnabled = value;
}

void sgpGpFitnessFun4Regression::setMinConstToArgRatio(double value) {
  m_minConstToArgRatio = value;
}
      
void sgpGpFitnessFun4Regression::setMaxReqUniqInstrCodeRatio(double value) {
  m_maxReqUniqInstrCodeRatio = value;       
}  

void sgpGpFitnessFun4Regression::fillObjectiveSet()
{
  m_objectiveSet.resize(getObjectiveCount());
  for(uint i=0, epos = m_objectiveSet.size(); i != epos; i++)
      m_objectiveSet[i] = true;
}

void sgpGpFitnessFun4Regression::getObjectiveSet(sgpObjectiveSet &output) const
{
  output = m_objectiveSet;
}

void sgpGpFitnessFun4Regression::setObjectiveSet(const sgpObjectiveSet &value)
{
  m_objectiveSet = value;
}

void sgpGpFitnessFun4Regression::setRestartsEnabled(bool value)
{
  m_restartsEnabled = value;
}

bool sgpGpFitnessFun4Regression::getRestartsEnabled()
{
  return m_restartsEnabled;
}
	
void sgpGpFitnessFun4Regression::prepareObjExtremeCount(const sgpFitDoubleVector &yVect, uint &output)
{
  output = countExtremeValuesWithNoiseFilter(yVect, EXREME_NOISE_FILTER_STEP);
}

void sgpGpFitnessFun4Regression::prepareObjDerives(const sgpFitDoubleVector &xVect, const sgpFitDoubleVector &yVect, uint level, sgpFitDoubleVector &output)
{
  output.clear();
  calcNthDerives(xVect, yVect, level, output);
}

void sgpGpFitnessFun4Regression::prepareObjFreqY(const sgpFitDoubleVector &yVect, sgpFitDoubleVector &output)
{
  output.clear();
  calcFreqVector(yVect, output);
}

void sgpGpFitnessFun4Regression::prepareObjAmpliY(const sgpFitDoubleVector &yVect, sgpFitDoubleVector &output)
{
  output.clear();
  calcAmplitudeVector(yVect, output);
}

// objectives:
// 0 - total
// 1 - size of code
// 2 - cost of execution
// 3 - % of input usage
// 4 - % of steps where output was <> null
// 5 - SSE from output type distance to desired
// - aproximation:
//   6 - sum of squared errors
//   7 - standard deviation of abs(error)
// 8 - minimal distance to input+output
// 9 - minimal distance to input+output with io mode validation
uint sgpGpFitnessFun4Regression::getObjectiveCount() const {
  return FUN_REGR_OBJ_IDX_ERROR_MAX+1;
}

void sgpGpFitnessFun4Regression::setSampleChangeInterval(uint value)
{
  m_sampleChangeInterval = value;
}

void sgpGpFitnessFun4Regression::setSampleSide(int value)
{
  m_sampleSide = value;
}  

void sgpGpFitnessFun4Regression::getStats(uint &totalCalc) {
  totalCalc = m_totalCalc;
}  

void sgpGpFitnessFun4Regression::getObjectiveWeights(sgpWeightVector &output) const
{
  output.resize(getObjectiveCount());
  for(uint i=0, epos = output.size(); i != epos; i++)
      output[i] = 100.0;

  // higher - less important
  
  output[FUN_REGR_OBJ_IDX_ERROR_Sse] = 3.0; //133; //3.0; //7.0;
  output[FUN_REGR_OBJ_IDX_ERROR_StdDev] = 5.0; //134; //4.0; //8.0; 
  output[FUN_REGR_OBJ_IDX_ERROR_RegDistRead] = 0.0;
  output[FUN_REGR_OBJ_IDX_ERROR_RegDistWrite] = 0.0;
  output[FUN_REGR_OBJ_IDX_ERROR_VarLevel] = 1.0;
  //output[FUN_REGR_OBJ_IDX_ERROR_TypeDiff] = 10.0;
  output[FUN_REGR_OBJ_IDX_ERROR_TotalCost] = 20.0;
  output[FUN_REGR_OBJ_IDX_ERROR_ProgramSize] = 60.0;

  output[FUN_REGR_OBJ_IDX_ERROR_FailedReads] = 24.0;
  output[FUN_REGR_OBJ_IDX_ERROR_WriteToInput] = 6.0; //3.0;
  //output[FUN_REGR_OBJ_IDX_ERROR_RelHitCount] = 6.0;
  output[FUN_REGR_OBJ_IDX_ERROR_DistanceToEndOfLastWrite] = 25.0; //11.0;
  //output[FUN_REGR_OBJ_IDX_ERROR_ConstToArgRatio] = 9.0;
  //output[FUN_REGR_OBJ_IDX_ERROR_StdDevDiffDfx] = 11.0;
  //output[FUN_REGR_OBJ_IDX_ERROR_StdDevDiffD2fx] = 12.0;
  //output[FUN_REGR_OBJ_IDX_ERROR_WriteToOutToTotalWriteCnt] = 13.0;
  output[FUN_REGR_OBJ_IDX_ERROR_FadingMaOutDiff] = 3.0;
  output[FUN_REGR_OBJ_IDX_ERROR_ExtremaRelError] = 3.0;
  output[FUN_REGR_OBJ_IDX_ERROR_ErrorSseAbs] = 3.0;
  output[FUN_REGR_OBJ_IDX_ERROR_FreqDiff] = 5.0;
  output[FUN_REGR_OBJ_IDX_ERROR_AmpliDiff] = 5.0;
  output[FUN_REGR_OBJ_IDX_ERROR_RevFx] = 5.0; 
  output[FUN_REGR_OBJ_IDX_ERROR_RegIoDist] = 7.0;
  //output[28] = 5.0;
}

void sgpGpFitnessFun4Regression::getObjectiveSigns(sgpObjectiveSigns &output) const
{
  output.resize(getObjectiveCount());
  for(uint i=0, epos = output.size(); i != epos; i++)
      output[i] = -1;

  output[FUN_REGR_OBJ_IDX_ERROR_VarLevel] = 1;
  output[FUN_REGR_OBJ_IDX_ERROR_UniqInstrCodeRatio] = 1;
  output[FUN_REGR_OBJ_IDX_ERROR_MaxInstrSeq] = 1;
  output[FUN_REGR_OBJ_IDX_ERROR_RelHitCount] = 1;
  output[FUN_REGR_OBJ_IDX_ERROR_OutCorrel] = 1;
  output[FUN_REGR_OBJ_IDX_ERROR_RegIoDist] = 1;
}

void sgpGpFitnessFun4Regression::initLibs(sgpFunLib &mainLib)
{
  mainLib.addLib(new sgpFunLibCore());
  mainLib.addLib(new sgpFunLibFMath());
  mainLib.addLib(new sgpFunLibComplex());
  mainLib.addLib(new sgpFunLibAnn());
  mainLib.addLib(new sgpFunLibMacros());
}

void sgpGpFitnessFun4Regression::prepare() {
  if (m_prepared)
    return;

  inherited::prepare();
  fillObjectiveSet();
  initInputValues();
}

void sgpGpFitnessFun4Regression::initProcess(sgpGaGeneration &newGeneration) {
  inherited::initProcess(newGeneration);

  if (m_stepCounter == 0) {
    generateInputValues();
  } 
   
  if (m_sampleChangeInterval > 0)
    m_stepCounter = (m_stepCounter + 1) % m_sampleChangeInterval;    
  else 
    m_stepCounter = 1;     
}

void sgpGpFitnessFun4Regression::invokeEntityHandled() const
{
  signalYield();
}

void sgpGpFitnessFun4Regression::signalYield() const
{
  if (m_yieldSignal != SC_NULL)
    m_yieldSignal->execute();
}

bool sgpGpFitnessFun4Regression::calc(uint entityIndex, const sgpEntityBase *entity, sgpFitnessValue &fitness) const
{
  fitness.resize(getObjectiveCount()); 
  scDataNode code;
  const sgpEntityForGasm *gasmEntity = checked_cast<const sgpEntityForGasm *>(entity);
  gasmEntity->getProgramCode(code);
  bool res = evaluateProgram(*gasmEntity, code, fitness, entityIndex);
  m_totalCalc++;
  return res; 
}

bool sgpGpFitnessFun4Regression::evaluateProgram(const sgpEntityForGasm &info, const scDataNode &code, sgpFitnessValue &fitness, 
  uint entityIndex
) const
{     
#ifdef DEBUG_FITFUN_TEST
  scLog::addDebug("fit-fun-e-begin");
#endif  
  sgpGpEvalPrgOutput prgOutput;

  prgOutput.fxVect().resize(SAMPLE_COUNT);
  prgOutput.yVect().resize(SAMPLE_COUNT);
  prgOutput.revFxVect().resize(SAMPLE_COUNT);

#ifdef DEBUG_FITFUN_TEST
  scLog::addDebug("fit-fun-e-1");
#endif  

  uint revFxMacroNo = getReverseFuncMacroNo(info);
  
#ifdef TRACE_TIME
  scTimer::start(TIMER_RUNPRG);
#endif
  
  runProgramForSamplesSeq(
    code, 
    prgOutput.fxVect(), prgOutput.yVect(), 
    prgOutput.notNullCnt, prgOutput.totalCost, prgOutput.totalTypeDiff);
    
  runProgramForRevFxInRange(        
    0, getSampleCount() - 1, 
    prgOutput.fxVect(), prgOutput.yVect(), prgOutput.revFxVect(), 
    revFxMacroNo);

#ifdef TRACE_TIME
  scTimer::stop(TIMER_RUNPRG);
#endif

#ifdef DEBUG_FITFUN_TEST
  scLog::addDebug("fit-fun-e-2");
#endif  

#ifdef DEBUG_FITFUN_TEST
  scLog::addDebug("fit-fun-e-3");
#endif  
  bool res =
    rateProgram(entityIndex, info, code, prgOutput, fitness);

#ifdef OPT_OBSERVE_PRG_OUTPUT
  handleProgramOutput(info, prgOutput);
#endif
       
#ifdef DEBUG_FITFUN_TEST
  scLog::addDebug("fit-fun-e-end");
#endif  
  return res;    
}

void sgpGpFitnessFun4Regression::runProgramForRevFxInRange(
  uint first, uint last,
  sgpFitDoubleVector &fxVect, sgpFitDoubleVector &yVect, sgpFitDoubleVector &revFxVect, 
  uint revFxMacroNo) const
{  
  double y;
  const double DEF_REV_FX_VALUE = 0.0;
  scDataNode revInput, revOutput;
  uint blockCount = m_vmachine->blockGetCount();  
  bool revFxEnabled = (revFxMacroNo != REV_FX_NULL_MACRO_NO) && (revFxMacroNo < blockCount);
  
  revInput.clear();
  revInput.addChild(new scDataNode(float(0.0)));

  if (revFxEnabled)    
  {
    for(uint i = first; i <= last; i++)
    {    
      y = yVect[i];
      revInput.setFloat(0, y);
      revOutput.clear();
      runProgram(revInput, revOutput, revFxMacroNo);
      revFxVect[i] = readPrgOutputAsFloat(revOutput, DEF_REV_FX_VALUE);
    }
  } else {
  // fill result with const value
    for(uint i = first; i <= last; i++)
      revFxVect[i] = DEF_REV_FX_VALUE;
  }
}  

bool sgpGpFitnessFun4Regression::rateProgram(uint entityIndex, const sgpEntityForGasm &info, const scDataNode &code, 
  const sgpGpEvalPrgOutput &prgOutput, 
  sgpFitnessValue &fitness) const  
{  
  _TRCSTEP_;
  bool res = true;
  double errorSse;
  double errorSseAbs;
  double errorStdDev = 0.0;
  double errorSum;
  scDataNode input, output;    
  sgpFitDoubleVector errors(SAMPLE_COUNT);    
  const sgpFitDoubleVector &fxVect = prgOutput.fxVect();    
  const sgpFitDoubleVector &yVect = prgOutput.yVect();    
  const sgpFitDoubleVector &revFxVect = prgOutput.revFxVect();    
  long64 totalCost = prgOutput.totalCost;
  uint countNotNullOutput = prgOutput.notNullCnt;
  long64 totalTypeDiff = prgOutput.totalTypeDiff;
  uint hitCount = 0;
  
  _TRCSTEP_;
  // calculate statistics
  calcOutputStats(fxVect, yVect, errorSse, errorSseAbs, errorSum, hitCount, errors);  
  _TRCSTEP_;

  if (hitCount == SAMPLE_COUNT) {
#ifdef COUT_ENABLED  
    cout << "--> 100% hit found! " << endl;
#endif    
#ifdef STOP_ON_HITS  
    res = false;
#endif    
  }

  sgpGaGenome genome;
  sgpGaGenomeMetaList codeMeta;

  sgpGasmScannerForFitPrg prgScanner;
  sgpGasmScannerForFitBlock blockScanner;

  prgScanner.setEntity(const_cast<sgpEntityForGasm *>(&info));
  prgScanner.setCode(const_cast<scDataNode *>(&code));
  prgScanner.init();

  blockScanner.setFunctions(const_cast<sgpFunctionMapColn *>(&m_functions));
  blockScanner.scanFirstBlockCode(info, code);
  blockScanner.init();

  ulong64 stepSize = prgScanner.calcProgramSize();
  scCounter::inc("gp-prg-size-step", stepSize);
  scCounter::inc("gx-workload-step", stepSize * SAMPLE_COUNT);

//TODO: scan all blocks of code if jumping to other blocks is possible (set_next_block, call)    
  double fxStdDev;
  
  //fxStdDev = calcStdDev(fxVect);
  fxStdDev = std_dev(fxVect.begin(), fxVect.end(), 0.0);
  
  uint fxDistinctCnt = calcDistinctCount(fxVect);
  double constToAllArgRatio = blockScanner.calcConstantToArgRatio(m_minConstToArgRatio);       

  sgpVMachine &vmachine = *m_vmachine;

  double regDistRead = 
    blockScanner.calcRegDistance(SGP_REGB_INPUT, SGP_REGB_INPUT + getInputArgCount() - 1, SGP_MAX_REG_NO + 1, gatfInput, vmachine);

  double regDistWrite = 
    blockScanner.calcRegDistance(SGP_REGB_OUTPUT, SGP_REGB_OUTPUT, SGP_MAX_REG_NO + 1, gatfOutput, vmachine);

  bool constOutput;
  double variableOutputRating;

  calcOutputVarRating(countNotNullOutput, fxDistinctCnt, fxStdDev, constToAllArgRatio, 
    regDistRead, regDistWrite, variableOutputRating, constOutput);
  
  if (constOutput) {    
    errorSse = CONST_OUT_ERROR_SSE;
    errorSseAbs = CONST_OUT_ERROR_SSE_ABS;
  }  
  
  if (constOutput)
    errorStdDev = CONST_OUT_ERROR_STD_DEV;
  else  
    errorStdDev = std_dev(errors.begin(), errors.end(), 0.0);

  double revFxObj;
  if (constOutput || !m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_RevFx]) {
    revFxObj = CONST_OUT_REV_FX * getSampleCount();
  } else { 
    revFxObj = calcReverseFuncObj(info, fxVect, yVect, revFxVect);  
  }  

  double regIoDist;
  if (constOutput || !m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_RegIoDist]) {
    regIoDist = CONST_OUT_REG_IO_DIST;
  } else { 
    regIoDist = blockScanner.calcRegIoDist();  
  }  

  _TRCSTEP_;

  double regDist1 = (
    blockScanner.calcRegDistance(SGP_REGB_INPUT, SGP_REGB_INPUT + getInputArgCount() - 1, SGP_MAX_REG_NO + 1, gatfAnyIo, vmachine)
    +
    blockScanner.calcRegDistance(SGP_REGB_OUTPUT, SGP_REGB_OUTPUT, SGP_MAX_REG_NO + 1, gatfAnyIo, vmachine)
  );
        
  double failedWrites;
  
  if (constOutput)
    failedWrites = CONST_OUT_FAILED_WRITES;
  else  
    failedWrites = blockScanner.calcWritesNotUsed(SGP_MAX_REG_NO + 1, MIN_FAILED_WRITES);
  
  failedWrites = sqrt(failedWrites);  
    
  double failedReads = 
    blockScanner.calcUninitializedReads(SGP_MAX_REG_NO + 1, SGP_REGB_INPUT, SGP_REGB_INPUT + getInputArgCount() - 1);

  double writesToInput = 
    blockScanner.calcWritesToInput(SGP_MAX_REG_NO + 1, SGP_REGB_INPUT, SGP_REGB_INPUT + getInputArgCount() - 1);
    
  double writesToOutput =
    blockScanner.calcWritesToOutput(SGP_MAX_REG_NO + 1, SGP_REGB_OUTPUT, SGP_REGB_OUTPUT);  

  double outputDistToEnd =
    blockScanner.calcOutputDistanceToEnd(SGP_MAX_REG_NO + 1, SGP_REGB_OUTPUT, SGP_REGB_OUTPUT, MIN_OUT_DIST_TO_END);  
    
  double uniqueInstrCodesRatio = 
    blockScanner.calcUniqueInstrCodes(m_maxReqUniqInstrCodeRatio);        

  double sameInstrCodeRatio = 
    blockScanner.calcSameInstrCodeRatio();  

  _TRCSTEP_;

  // calc number of extreme values
  double extremeCntRel;
  
  if (constOutput || (!m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_ExtremaRelError]))
    extremeCntRel = CONST_OUT_EXTREME_REL;    
  else
    extremeCntRel =     
    relativeErrorMinMax<double>(
      static_cast<double>(m_objCacheExtremeCount),
      static_cast<double>(countExtremeValuesWithNoiseFilter(fxVect, EXREME_NOISE_FILTER_STEP))
    );  
    
  // calc error on derive level 1 = stdDev(sub(dfx(i) - dy(i))
  double stdErrorDerive1;
  
  if (constOutput || !m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_StdDevDiffDfx])
    stdErrorDerive1 = CONST_OUT_ERROR_STD_DEV;
  else  
    stdErrorDerive1 = calcStdErrorDeriveNForTargetFuncNI1O(yVect, fxVect, 1);
      
  // calcStdErrorDeriveN(m_inputValues, yVect, fxVect, 1);
  
  // calc error on derive level 2 = stdDev(sub(d2fx(i) - d2y(i))
  double stdErrorDerive2;
  
  if (constOutput || !m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_StdDevDiffD2fx])
    stdErrorDerive2 = CONST_OUT_ERROR_STD_DEV;
  else  
    stdErrorDerive2 = calcStdErrorDeriveNForTargetFuncNI1O(yVect, fxVect, 2);

  double correlYNFx;
  
  if (constOutput)
    correlYNFx = CONST_OUT_CORREL;
  else
    correlYNFx = 2.0+calcCorrel(yVect, fxVect);
  
  double constOnlyInstrRatio = blockScanner.calcConstOnlyInstr(MIN_CONST_ONLY_INSTR_RATIO);
  
//  double foundRegs = calcUsedRegs(info, code, genome, codeMeta, 
//    SGP_MAX_REG_NO + 1, SGP_REGB_INPUT, SGP_REGB_INPUT + m_inputArgCount - 1);

  long64 totalSizeRawL = stepSize; //calcProgramSize(code);      
  double totalSize;
  
  if (constOutput)
    totalSize = CONST_OUT_SIZE;
  else {  
    ulong64 totalSizeL = calcProgramSizeScore(info, &prgScanner);  
    totalSize = std::fabs(log10(double(10+totalSizeL)));
  }  

  double fadingMaDiff;
  
  if (constOutput || (!m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_FadingMaOutDiff]))
    fadingMaDiff = CONST_OUT_FADING_DIFF;
  else  
    fadingMaDiff = calcFadingMaDiff(yVect, fxVect);
  
  double freqDiff;
  
  if (constOutput || !m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_FreqDiff])
    freqDiff = CONST_OUT_FREQ_DIFF;
  else
    //freqDiff = calcFreqDiff(yVect, fxVect);
    freqDiff = calcFreqDiffPrepared(m_objCacheFreqY, fxVect);
    
  double ampliDiff;
  
  if (constOutput || !m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_AmpliDiff])
    ampliDiff = CONST_OUT_AMPLI_DIFF;
  else
    //ampliDiff = calcAmplitudeDiff(yVect, fxVect);
    ampliDiff = calcAmplitudeDiffPrepared(m_objCacheAmpliY, fxVect);
    
  //double incDiff = NULL_INC_DIFF; //calcIncreasesDiff(yVect, fxVect);
  
  double totalCostD;

  if (constOutput)
    totalCostD = CONST_OUT_CODE_COST;  
  else
    totalCostD = std::fabs(log10(10.0 + (double(totalCost) / double(totalSizeRawL))));
             
  _TRCSTEP_;
  fitness[FUN_REGR_OBJ_IDX_ERROR_Sse] = -errorSse; 

  fitness[FUN_REGR_OBJ_IDX_ERROR_StdDev] = -std::fabs(errorStdDev);

  fitness[FUN_REGR_OBJ_IDX_ERROR_RegDistRead] = -regDistRead;

  fitness[FUN_REGR_OBJ_IDX_ERROR_RegDistWrite] = -regDistWrite;

  fitness[FUN_REGR_OBJ_IDX_ERROR_RegDist] = -regDist1;

  fitness[FUN_REGR_OBJ_IDX_ERROR_VarLevel] = variableOutputRating;

  fitness[FUN_REGR_OBJ_IDX_ERROR_TypeDiff] = -std::fabs(double(totalTypeDiff));

  fitness[FUN_REGR_OBJ_IDX_ERROR_TotalCost] = -totalCostD;

  fitness[FUN_REGR_OBJ_IDX_ERROR_ProgramSize] = -totalSize;

  fitness[FUN_REGR_OBJ_IDX_ERROR_FailedWrites] = -failedWrites;

  fitness[FUN_REGR_OBJ_IDX_ERROR_FailedReads] = -failedReads;

  fitness[FUN_REGR_OBJ_IDX_ERROR_WriteToInput] = -writesToInput;

  fitness[FUN_REGR_OBJ_IDX_ERROR_UniqInstrCodeRatio] = uniqueInstrCodesRatio;    

  fitness[FUN_REGR_OBJ_IDX_ERROR_MaxInstrSeq] = sameInstrCodeRatio;

  fitness[FUN_REGR_OBJ_IDX_ERROR_RelHitCount] = double(hitCount)/double(SAMPLE_COUNT); //was fxStdDev;

  fitness[FUN_REGR_OBJ_IDX_ERROR_DistanceToEndOfLastWrite] = -outputDistToEnd;

  fitness[FUN_REGR_OBJ_IDX_ERROR_ConstToArgRatio] = -constToAllArgRatio;

  fitness[FUN_REGR_OBJ_IDX_ERROR_ConstOnlyArgInstrToTotal] = -constOnlyInstrRatio;
  
  fitness[FUN_REGR_OBJ_IDX_ERROR_StdDevDiffDfx] = -std::fabs(log10(double(10.0+
      stdErrorDerive1
  )));
    
  fitness[FUN_REGR_OBJ_IDX_ERROR_StdDevDiffD2fx] = -std::fabs(log10(double(10.0+
      stdErrorDerive2
  )));
    
  fitness[FUN_REGR_OBJ_IDX_ERROR_WriteToOutToTotalWriteCnt] = -writesToOutput; //was (-foundRegs)
    
  fitness[FUN_REGR_OBJ_IDX_ERROR_OutCorrel] = correlYNFx;

  fitness[FUN_REGR_OBJ_IDX_ERROR_FadingMaOutDiff] = -fadingMaDiff;
    
  fitness[FUN_REGR_OBJ_IDX_ERROR_ExtremaRelError] = -(1.0 + extremeCntRel);  
    
  fitness[FUN_REGR_OBJ_IDX_ERROR_ErrorSseAbs] = -errorSseAbs;

  fitness[FUN_REGR_OBJ_IDX_ERROR_FreqDiff] = -(1.0 + freqDiff);
    
  fitness[FUN_REGR_OBJ_IDX_ERROR_AmpliDiff] = -(1.0 + ampliDiff);

  fitness[FUN_REGR_OBJ_IDX_ERROR_RevFx] = -(1.0 + revFxObj);  
  
  fitness[FUN_REGR_OBJ_IDX_ERROR_RegIoDist] = 1.0 + regIoDist;
  
  //fitness[28] = -(1.0 + incDiff);

  for (uint i=1, epos = getObjectiveCount(); i < epos; i++)
    if (!m_objectiveSet[i])
      fitness[i] = 1.0;
    
  _TRCSTEP_;

#ifdef TRACE_ENTITY_BIO
  if (entityIndex != UINT_MAX) {
    sgpEntityTracer::handleEntityEvaluated(entityIndex, fitness, info);
    sgpEntityTracer::handleSetEntityClass(entityIndex, 
      calcEntityClassForTargetFuncNI1O(fxVect, yVect));
  } 
#endif
  
  return res;
}

void sgpGpFitnessFun4Regression::handleProgramOutput(const sgpEntityForGasm &workInfo, const sgpGpEvalPrgOutput &prgOutput) const
{
  if (m_onHandleProgramOutput != SC_NULL)
    m_onHandleProgramOutput->execute(workInfo, prgOutput);
}

void sgpGpFitnessFun4Regression::runProgramForSamplesSeq(
  const scDataNode &code,
  sgpFitDoubleVector &fxVect, sgpFitDoubleVector &yVect, 
  uint &notNullCnt, long64 &totalCost, long64 &totalTypeDiff) const
{
  setVMachineProgram(*m_vmachine, code);
  
  runProgramForSamplesInRange(0, getSampleCount() - 1, 
    fxVect, yVect, 
    notNullCnt, totalCost, totalTypeDiff);
}

void sgpGpFitnessFun4Regression::calcOutputStats(const sgpFitDoubleVector &fxVect, const sgpFitDoubleVector &yVect, double &errorSse, double &errorSseAbs, double &errorSum, uint &hitCount, sgpFitDoubleVector &errors) const
{
  double fx, y;
  double sampleError, sampleErrorForSse;

  hitCount = 0;
  errorSse = errorSseAbs = errorSum = 0;
  
  for(int i=0,epos = SAMPLE_COUNT; i != epos; i++)
  {
    fx = fxVect[i];
    y = yVect[i];
    
    sampleError = (fx - y);
    sampleErrorForSse = 1.0 + relativeErrorMinMax<double>(fx, y);
    errorSse += (sampleErrorForSse * sampleErrorForSse);
    errorSseAbs += (sampleError * sampleError);
    errorSum += std::fabs(sampleError);
    if (std::fabs(fx - y) < HIT_THRESHOLD * std::fabs(y))
      hitCount++;
    errors[i] = sampleError;
  }  
}

// calculate how much output varies across samples
void sgpGpFitnessFun4Regression::calcOutputVarRating(uint countNotNullOutput, uint fxDistinctCnt, double fxStdDev, double constToAllArgRatio, 
  double regDistRead, double regDistWrite,
  double &variableOutputRating, bool &constOutput) const
{
  constOutput = false;
  variableOutputRating = double(1+countNotNullOutput) / double(SAMPLE_COUNT);;
  
  if ((fxDistinctCnt >= MIN_OUTPUT_DISTINCT_CNT) && !isnan(fxStdDev) && !isinf(fxStdDev)) {      
    variableOutputRating += 2.0;    
  } else { // constant output 
    variableOutputRating += 1.0/constToAllArgRatio;
    constOutput = true;
  }  
  variableOutputRating += 1/regDistRead + 1/regDistWrite;      
}

double sgpGpFitnessFun4Regression::calcReverseFuncObj(const sgpEntityForGasm &info, 
  const sgpFitDoubleVector &fxVect, const sgpFitDoubleVector &yVect, const sgpFitDoubleVector &revFxVect) const
{
  const double NULL_OBJ_VALUE = 1.0; // maximum error
  double res;
  uint macroNo;

  macroNo = getReverseFuncMacroNo(info);

  if (macroNo == REV_FX_NULL_MACRO_NO)
    res = NULL_OBJ_VALUE * static_cast<double>(fxVect.size());
  else  
  {
    double fx, y, revFx;
    res = 0.0;
    for(int i=0,epos = fxVect.size(); i != epos; i++)
    {
      fx = fxVect[i];
      y = yVect[i];
      revFx = revFxVect[i];
      res += relativeErrorMinMax(fx, revFx);
    }
    //res = res / static_cast<double>(fxVect.size()); // avg
  }
  return res;
}

// return macro number to be used as reversed function (auto-objective)
// 0 means "no reverse function available"
uint sgpGpFitnessFun4Regression::getReverseFuncMacroNo(const sgpEntityForGasm &info) const
{
  const double MIN_LEVEL = 0.5;
  uint res;
  double macroFactor;

  if (!info.getInfoDouble(SGP_INFOBLOCK_IDX_OTHER_PARAM_BASE + SGP_INFOPARAM_IDX_OTHER_PARAM_REV_FUN_MACRO_NO, macroFactor)) 
  {
    macroFactor = 0.0;    
  }

  if (macroFactor < MIN_LEVEL) {
    res = REV_FX_NULL_MACRO_NO;  
  } else {
  // each 0.1 is another macro no
    macroFactor -= MIN_LEVEL;
    res = 1 + round(macroFactor * 10.0); // 1..5      
  }  
  
  return res;
}

void sgpGpFitnessFun4Regression::handleSamplesChanged()
{
#ifdef TRACE_ENTITY_BIO
    sgpEntityTracer::handleSamplesChanged();
#endif        
  if (m_onSamplesChanged != SC_NULL)
    m_onSamplesChanged->execute(this);
  prepareObjData();  
}

void sgpGpFitnessFun4Regression::prepareObjData()
{  // empty here
}

void sgpGpFitnessFun4Regression::getSampleRange(int dimNo, double &minVal, double &maxVal)
{
  minVal = SAMPLE_MIN_X; 
  maxVal = SAMPLE_MAX_X; 
}

float sgpGpFitnessFun4Regression::readPrgOutputAsFloat(const scDataNode &value, float defValue) const
{
  float res;
  if (!value.isNull()) {
    switch (value.getValueType()) {
      case vt_double:
      case vt_xdouble:
      case vt_float:
      case vt_int:
      case vt_uint:
      case vt_int64:
      case vt_uint64:
      case vt_byte: 
        res = value.getAsFloat(); 
        break;
      default: 
        res = defValue; 
        break;
    }
  } else { // null
    res = defValue; 
  }

#ifdef SAFE_OUTPUT
    if (isnan(res))
      res = defValue;
#endif            
  return res;
}

ulong64 sgpGpFitnessFun4Regression::calcProgramSizeScore(const sgpEntityForGasm &info, sgpGasmScannerForFitPrg *scanner) const
{
  double degradRatio;
  ulong64 res = 0;
  
  if (m_adfsEnabled ) {
#ifdef OPT_ADF_DEGRAD_SIZE  
  if (info.getInfoDouble(SGP_INFOBLOCK_IDX_MUT_OTHER_PARAM_BASE + SGP_MUT_OTHER_PARAM_IDX_BLOCK_DEGRAD, degradRatio)) {
    res = scanner->calcProgramSizeDegrad(m_expectedSize, degradRatio);    
  }  
#endif  
  } else if (m_macrosEnabled) {
    res = scanner->calcProgramSizeWithMacroSup(m_expectedSize);    
  } 
  
  if (res == 0) { // default calculation 
    res = scanner->calcProgramSizeEx(m_expectedSize);    
  } 
  return res; 
}

// ----------------------------------------------------------------------------
// sgpGpFitnessFun1I1O
// ----------------------------------------------------------------------------
uint sgpGpFitnessFun1I1O::getInputCount()
{
  return 1;
}

void sgpGpFitnessFun1I1O::getInputValues(scDataNode &output)
{
  output.clear();
  output.setAsArray(vt_double);
  for(uint i=0, epos = m_inputValues.size(); i != epos; i++)
    output.addItemAsDouble(m_inputValues[i]);
}

void sgpGpFitnessFun1I1O::setInputValues(const scDataNode &values)
{
  m_inputValues.clear();
  m_inputValues.resize(values.size());    
  for(uint i=0, epos = values.size(); i != epos; i++)
    m_inputValues[i] = values.getDouble(i);
  handleSamplesChanged();  
}

void sgpGpFitnessFun1I1O::initInputValues()
{
  m_inputValues.resize(SAMPLE_COUNT);    
}

void sgpGpFitnessFun1I1O::fillObjectiveSet()
{
  m_objectiveSet.clear();
  for(uint i=0, epos = getObjectiveCount(); i != epos; i++)
    m_objectiveSet.push_back(true);

#ifdef OPT_NARROW_OBJS
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_Sse] = false;                      
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_StdDev] = false;                   
  m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_RegDistRead] = false;                        
  m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_RegDistWrite] = false;             
  m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_RegDist] = false;                  
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_VarLevel] = false;                 
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_TypeDiff] = false;                           
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_TotalCost] = false;                
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_ProgramSize] = false;              
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_FailedWrites] = false;             
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_FailedReads] = false;              
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_WriteToInput] = false;             
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_UniqInstrCodeRatio] = false;       
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_MaxInstrSeq] = false;                 
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_RelHitCount] = false;              
  m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_DistanceToEndOfLastWrite] = false; 
  m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_ConstToArgRatio] = false;                   
  m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_ConstOnlyArgInstrToTotal] = false; 
  m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_StdDevDiffDfx] = false;                     
  m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_StdDevDiffD2fx] = false;                              
  m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_WriteToOutToTotalWriteCnt] = false;
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_OutCorrel] = false;                          
  m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_FadingMaOutDiff] = false;          
  m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_ExtremaRelError] = false;          
  m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_ErrorSseAbs] = false;              
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_FreqDiff] = false;                 
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_AmpliDiff] = false;                
#endif  
}

void sgpGpFitnessFun1I1O::generateInputValues()
{
    double minVal, maxVal;
#ifdef OPT_DYNAMIC_SAMPLE_RANGE
    double val1, val2;
    val1 = randomDouble(SAMPLE_MIN_X, SAMPLE_MAX_X);
    val2 = randomDouble(SAMPLE_MIN_X, SAMPLE_MAX_X);
    minVal = std::min<double>(val1, val2); 
    maxVal = std::max<double>(val1, val2); 
#else    
    getSampleRange(0, minVal, maxVal); 
#endif    

    if (m_sampleSide < 0)
      maxVal = 0.0;
    else if (m_sampleSide > 0)
      minVal = 0.0;
        
    for(int i=0,epos = SAMPLE_COUNT; i != epos; i++)
      m_inputValues[i] = randomDouble(minVal, maxVal);
    // sort - for derivatives  
    sort(m_inputValues.begin(), m_inputValues.end());  
    handleSamplesChanged();
}

uint sgpGpFitnessFun1I1O::getSampleCount() const
{
  return SAMPLE_COUNT;
}  

double sgpGpFitnessFun1I1O::calcStdErrorDeriveNForTargetFuncNI1O(const sgpFitDoubleVector &yVect, const sgpFitDoubleVector &fxVect, uint level) const
{
  if (level == 1)
    return calcStdErrorDeriveNPrepared(m_inputValues, m_objCacheDerivedY1, fxVect, level);
  else if (level == 2)  
    return calcStdErrorDeriveNPrepared(m_inputValues, m_objCacheDerivedY2, fxVect, level);
  else  
    return calcStdErrorDeriveN(m_inputValues, yVect, fxVect, level);
}

uint sgpGpFitnessFun1I1O::calcEntityClassForTargetFuncNI1O(const sgpFitDoubleVector &fxVect, const sgpFitDoubleVector &yVect) const
{
  return calcEntityClass(m_inputValues, fxVect, yVect);
}

void sgpGpFitnessFun1I1O::runProgramForSamplesInRange(
  uint first, uint last,
  sgpFitDoubleVector &fxVect, sgpFitDoubleVector &yVect, 
  uint &notNullCnt, long64 &totalCost, long64 &totalTypeDiff) const
{  
  double x, y, fx;
  scDataNode input, output;
  uint typeDiff;
  const sgpFitDoubleVector &inputValues = m_inputValues; 
  
  notNullCnt = 0;
  totalCost = 0;
  totalTypeDiff = 0;
    
  sgpVMachine &vmachine = *const_cast<sgpVMachine *>(m_vmachine.get());

  input.clear();
  input.addChild(new scDataNode(float(0.0)));

  for(uint i = first; i <= last; i++)
  {
    x = inputValues[i];
    y = calcTargetFunction(x); 
    input.setFloat(0, x);
    
    runProgram(input, output, SGP_GASM_MAIN_BLOCK_INDEX);
    
    totalCost += vmachine.getTotalCost();
    typeDiff = sgpGasmScannerForFitUtils::calcTypeDiff(output.getValueType(), vt_float);
    totalTypeDiff += (typeDiff * typeDiff);
    
    fx = readPrgOutputAsFloat(output, 0.0);

    if (!output.isNull()) 
      notNullCnt++;
    
    fxVect[i] = fx;
    yVect[i] = y;

    output.clear();
  }
}  

void sgpGpFitnessFun1I1O::prepareObjData()
{
  sgpFitDoubleVector yVect;
  sgpFitDoubleVector &xVect = m_inputValues;
  
  prepareTargetOutput(xVect, yVect);

  prepareObjExtremeCount(yVect, m_objCacheExtremeCount);
  prepareObjDerives(xVect, yVect, 1, m_objCacheDerivedY1); 
  prepareObjDerives(xVect, yVect, 2, m_objCacheDerivedY2); 
  prepareObjFreqY(yVect, m_objCacheFreqY); 
  prepareObjAmpliY(yVect, m_objCacheAmpliY); 
}

void sgpGpFitnessFun1I1O::prepareTargetOutput(const sgpFitDoubleVector &xVect, sgpFitDoubleVector &yVect)
{
  double x, y;

  yVect.resize(xVect.size());
  
  for(uint i = 0, epos = xVect.size(); i != epos; i++)
  {
    x = xVect[i];
    y = calcTargetFunction(x); 
    yVect[i] = y;
  }
}

// ----------------------------------------------------------------------------
// sgpGpFitnessFun2I1O
// ----------------------------------------------------------------------------
uint sgpGpFitnessFun2I1O::getInputCount()
{
  return 2;
}

void sgpGpFitnessFun2I1O::getInputValues(scDataNode &output)
{
  output.clear();
  for(uint i=0, epos = m_inputValuesX1.size(); i != epos; i++)
    output.addChild(
      (new scDataNode(ict_list))->
        addChild(new scDataNode(m_inputValuesX1[i])). 
        addChild(new scDataNode(m_inputValuesX2[i]))
    );
}

void sgpGpFitnessFun2I1O::setInputValues(const scDataNode &values)
{
  m_inputValuesX1.clear();
  m_inputValuesX2.clear();

  if (values.size() > 0)
  {
    m_inputValuesX1.resize(values.size());        
    m_inputValuesX2.resize(values.size());        
    
    for(uint i=0, epos = values.size(); i != epos; i++)
    {
      m_inputValuesX1[i] = values[i].getDouble(0);
      m_inputValuesX2[i] = values[i].getDouble(1);
    }  
  }
  handleSamplesChanged();  
}

void sgpGpFitnessFun2I1O::initInputValues()
{
  m_inputValuesX1.resize(SAMPLE_COUNT);    
  m_inputValuesX2.resize(SAMPLE_COUNT);    
}

void sgpGpFitnessFun2I1O::generateInputValues()
{  
    double minVal1, maxVal1;
    double minVal2, maxVal2;
#ifdef OPT_DYNAMIC_SAMPLE_RANGE
    double val1, val2;
    val1 = randomDouble(SAMPLE_MIN_X, SAMPLE_MAX_X);
    val2 = randomDouble(SAMPLE_MIN_X, SAMPLE_MAX_X);
    minVal = std::min<double>(val1, val2); 
    maxVal = std::max<double>(val1, val2); 
#else    
    getSampleRange(0, minVal1, maxVal1); 
    getSampleRange(1, minVal2, maxVal2); 
#endif    
    if (m_sampleSide < 0) {
      maxVal1 = maxVal2 = 0.0;
    }  
    else if (m_sampleSide > 0) {
      minVal1 = minVal2 = 0.0;
    }  

    // sort values in two dimensions
    std::vector<dim2rec> sampleVect;    

    uint sqrtValue = round(sqrt(static_cast<double>(SAMPLE_COUNT)));
    sampleVect.reserve(sqrtValue);
    double x2(0.0);
    for(int i=0,epos = SAMPLE_COUNT; i != epos; i++) 
    {
      if (i % sqrtValue == 0)
        x2 = randomDouble(minVal2, maxVal2);
      sampleVect.push_back(make_pair(randomDouble(minVal1, maxVal1), x2));
    }  
    sort(sampleVect.begin(), sampleVect.end(), less_2dims);

    for(int i=0,epos = SAMPLE_COUNT; i != epos; i++) {
      m_inputValuesX1[i] = sampleVect[i].first;
      m_inputValuesX2[i] = sampleVect[i].second;
    }  

    handleSamplesChanged();
}

uint sgpGpFitnessFun2I1O::getSampleCount() const
{
  return SAMPLE_COUNT;
}  

double sgpGpFitnessFun2I1O::calcStdErrorDeriveNForTargetFuncNI1O(const sgpFitDoubleVector &yVect, const sgpFitDoubleVector &fxVect, uint level) const
{
  return 0.0; //calcStdErrorDeriveN(m_inputValues, yVect, fxVect, level);
}

uint sgpGpFitnessFun2I1O::calcEntityClassForTargetFuncNI1O(const sgpFitDoubleVector &fxVect, const sgpFitDoubleVector &yVect) const
{
  return calcEntityClass(m_inputValuesX1, m_inputValuesX2, fxVect, yVect);
}

uint sgpGpFitnessFun2I1O::calcEntityClass(const sgpFitDoubleVector &input1, const sgpFitDoubleVector &input2, 
  const sgpFitDoubleVector &output, const sgpFitDoubleVector &target) const
{
  return 0;
}    

void sgpGpFitnessFun2I1O::runProgramForSamplesInRange(
  uint first, uint last,
  sgpFitDoubleVector &fxVect, sgpFitDoubleVector &yVect,
  uint &notNullCnt, long64 &totalCost, long64 &totalTypeDiff) const
{  
  double x1, x2, y, fx;
  scDataNode input, output;
  uint typeDiff;
  const sgpFitDoubleVector &inputValues1 = m_inputValuesX1; 
  const sgpFitDoubleVector &inputValues2 = m_inputValuesX2; 
  
  sgpVMachine &vmachine = *const_cast<sgpVMachine *>(m_vmachine.get());
  notNullCnt = 0;
  totalCost = 0;
  totalTypeDiff = 0;

  input.clear();
  input.addChild(new scDataNode(float(0.0)));
  input.addChild(new scDataNode(float(0.0)));
  
  for(uint i = first; i <= last; i++)
  {
    // aproximate f(x) = (x^2)/2
    x1 = inputValues1[i];
    x2 = inputValues2[i];
    y = calcTargetFunction(x1, x2); 
    
    input.setFloat(0, x1);
    input.setFloat(1, x2);
    
    runProgram(input, output, SGP_GASM_MAIN_BLOCK_INDEX);

    totalCost += vmachine.getTotalCost();
    typeDiff = sgpGasmScannerForFitUtils::calcTypeDiff(output.getValueType(), vt_float);
    totalTypeDiff += (typeDiff * typeDiff);
    
    fx = 0.0;
    if (!output.isNull()) {
      switch (output.getValueType()) {
        case vt_double:
        case vt_xdouble:
        case vt_float:
        case vt_int:
        case vt_uint:
        case vt_int64:
        case vt_uint64:
        case vt_byte: 
          fx = output.getAsFloat(); 
          break;
        default: 
          fx = 0.0; // for max error
          //y = fx + i * 100.0;
          break;
      }
              
      notNullCnt++;
    } else { // null
      fx = 0.0; // for max error
      //y = fx + i * 200.0;
    }

#ifdef SAFE_OUTPUT
    if (isnan(fx))
      fx = 0.0;
#endif      
    
    fxVect[i] = fx;
    yVect[i] = y;
    output.clear();
  }
}  

void sgpGpFitnessFun2I1O::fillObjectiveSet()
{
  m_objectiveSet.clear();
  for(uint i=0, epos = getObjectiveCount(); i != epos; i++)
    m_objectiveSet.push_back(true);

  m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_StdDevDiffDfx] = false;
  m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_StdDevDiffD2fx] = false;
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_FadingMaOutDiff] = false;
  //m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_ExtremaRelError] = false;
  ///m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_FreqDiff] = false;
  ///m_objectiveSet[FUN_REGR_OBJ_IDX_ERROR_AmpliDiff] = false;
  ///m_objectiveSet[28] = false;
}

void sgpGpFitnessFun2I1O::prepareObjData()
{
  sgpFitDoubleVector yVect;  
  prepareTargetOutput(m_inputValuesX1, m_inputValuesX2, yVect);

  prepareObjExtremeCount(yVect, m_objCacheExtremeCount);
  prepareObjFreqY(yVect, m_objCacheFreqY); 
  prepareObjAmpliY(yVect, m_objCacheAmpliY); 
}

void sgpGpFitnessFun2I1O::prepareTargetOutput(const sgpFitDoubleVector &xVect1, const sgpFitDoubleVector &xVect2, sgpFitDoubleVector &yVect)
{
  double x1, x2, y;

  yVect.resize(xVect1.size());
  
  for(uint i = 0, epos = xVect1.size(); i != epos; i++)
  {
    x1 = xVect1[i];
    x2 = xVect2[i];
    y = calcTargetFunction(x1, x2); 
    yVect[i] = y;
  }
}
