/////////////////////////////////////////////////////////////////////////////
// Name:        GpFitnessFun4Regression.h
// Project:     sgpLib
// Purpose:     Fitness function for Gp evolver - regression.
// Author:      Piotr Likus
// Modified by:
// Created:     02/05/2009
/////////////////////////////////////////////////////////////////////////////

#ifndef _GPFITNESSFUN4REGRESSION_H__
#define _GPFITNESSFUN4REGRESSION_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GpFitnessFun4Regression.h
///
/// Fitness function for regression

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
//std
#include <list>
#include <queue>

//base
#include "base/algorithm.h"

//sc
#include "sc/events/Events.h"

//sgp
#include "sgp/GasmFunLib.h"
#include "sgp/GaEvolver.h"
#include "sgp/GasmEvolver.h"
#include "sgp/GpFitnessFun4Gasm.h"
#include "sgp/GasmScannerForFitPrg.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------
typedef sgpFitnessValue sgpFitDoubleVector;
typedef boost::shared_ptr<sgpFitDoubleVector> sgpFitDoubleVectorGuard;

const uint SGP_OBJ_IDX_SSE           = 1;
const uint SGP_OBJ_IDX_ERROR_STDDEV  = 2;
const uint SGP_OBJ_IDX_REL_HIT_COUNT = 15;
const uint SGP_OBJ_IDX_DFX_STDDEV    = 19;
const uint SGP_OBJ_IDX_D2FX_STDDEV   = 20;
const uint SGP_OBJ_IDX_CORREL        = 22;
const uint SGP_OBJ_IDX_FADING_MA     = 23;
const uint SGP_OBJ_IDX_EXTREMA_CNT   = 24;
const uint SGP_OBJ_IDX_SSE_ABS       = 25;
const uint SGP_OBJ_IDX_FREQ_DIFF     = 26;
const uint SGP_OBJ_IDX_AMPLI_DIFF    = 27;
const uint SGP_OBJ_IDX_REV_FX        = 28;

//const uint SGP_OBJ_IDX_INC_DIFF      = 29;

const uint FUN_REGR_OBJ_IDX_ERROR_Sse                      = 1;
const uint FUN_REGR_OBJ_IDX_ERROR_StdDev                   = 2;
const uint FUN_REGR_OBJ_IDX_ERROR_RegDistRead              = 3;
const uint FUN_REGR_OBJ_IDX_ERROR_RegDistWrite             = 4;
const uint FUN_REGR_OBJ_IDX_ERROR_RegDist                  = 5;
const uint FUN_REGR_OBJ_IDX_ERROR_VarLevel                 = 6;
const uint FUN_REGR_OBJ_IDX_ERROR_TypeDiff                 = 7;
const uint FUN_REGR_OBJ_IDX_ERROR_TotalCost                = 8;
const uint FUN_REGR_OBJ_IDX_ERROR_ProgramSize              = 9;
const uint FUN_REGR_OBJ_IDX_ERROR_FailedWrites             = 10;
const uint FUN_REGR_OBJ_IDX_ERROR_FailedReads              = 11;
const uint FUN_REGR_OBJ_IDX_ERROR_WriteToInput             = 12;
const uint FUN_REGR_OBJ_IDX_ERROR_UniqInstrCodeRatio       = 13;
const uint FUN_REGR_OBJ_IDX_ERROR_MaxInstrSeq              = 14;
const uint FUN_REGR_OBJ_IDX_ERROR_RelHitCount              = 15;
const uint FUN_REGR_OBJ_IDX_ERROR_DistanceToEndOfLastWrite = 16;
const uint FUN_REGR_OBJ_IDX_ERROR_ConstToArgRatio          = 17;
const uint FUN_REGR_OBJ_IDX_ERROR_ConstOnlyArgInstrToTotal = 18;
const uint FUN_REGR_OBJ_IDX_ERROR_StdDevDiffDfx            = 19;
const uint FUN_REGR_OBJ_IDX_ERROR_StdDevDiffD2fx           = 20;
const uint FUN_REGR_OBJ_IDX_ERROR_WriteToOutToTotalWriteCnt = 21;
const uint FUN_REGR_OBJ_IDX_ERROR_OutCorrel                = 22;
const uint FUN_REGR_OBJ_IDX_ERROR_FadingMaOutDiff          = 23;
const uint FUN_REGR_OBJ_IDX_ERROR_ExtremaRelError          = 24;
const uint FUN_REGR_OBJ_IDX_ERROR_ErrorSseAbs              = 25;
const uint FUN_REGR_OBJ_IDX_ERROR_FreqDiff                 = 26;
const uint FUN_REGR_OBJ_IDX_ERROR_AmpliDiff                = 27;
const uint FUN_REGR_OBJ_IDX_ERROR_RevFx                    = 28;
const uint FUN_REGR_OBJ_IDX_ERROR_RegIoDist                = 29;

const uint FUN_REGR_OBJ_IDX_ERROR_MAX                      = FUN_REGR_OBJ_IDX_ERROR_RegIoDist;

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------

const uint SAMPLE_COUNT = 200;
const uint MIN_OUTPUT_DISTINCT_CNT = 2 + (SAMPLE_COUNT / 100);

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
class sgpGpFitnessFun4Regression;

class sgpSamplesChangedEvent {
public:
  virtual void execute(const sgpGpFitnessFun4Regression *sender) = 0;
};

class sgpGpEvalPrgOutput {
public:
  sgpGpEvalPrgOutput() {
    fxVectGuard.reset(new sgpFitDoubleVector());
    yVectGuard.reset(new sgpFitDoubleVector());
    revFxVectGuard.reset(new sgpFitDoubleVector());
  }
  sgpFitDoubleVector &fxVect() { return *fxVectGuard; }
  sgpFitDoubleVector &yVect() { return *yVectGuard; }
  sgpFitDoubleVector &revFxVect() { return *revFxVectGuard; }
  const sgpFitDoubleVector &fxVect() const { return *fxVectGuard; }
  const sgpFitDoubleVector &yVect() const { return *yVectGuard; }
  const sgpFitDoubleVector &revFxVect() const { return *revFxVectGuard; }
public:  
  sgpFitDoubleVectorGuard fxVectGuard;
  sgpFitDoubleVectorGuard yVectGuard;
  sgpFitDoubleVectorGuard revFxVectGuard;
  uint notNullCnt;
  long64 totalCost;
  long64 totalTypeDiff;
};

typedef std::queue<sgpGpEvalPrgOutput> sgpGpEvalPrgOutputList; 

class sgpHandleProgramOutputEvent {
public:
  virtual void execute(const sgpEntityForGasm &workInfo, const sgpGpEvalPrgOutput &prgOutput) = 0;
};

class sgpGpFitnessFun4Regression: public sgpFitnessFun4Gasm {
  typedef sgpFitnessFun4Gasm inherited;
public:
    //-- construction
    sgpGpFitnessFun4Regression();
    virtual ~sgpGpFitnessFun4Regression();
    //-- properties      
    void setSampleSide(int value);
    void setSampleChangeInterval(uint value);
    void setAdfsEnabled(bool value);
    void setNonFuncInstrEnabled(bool value);
    void setYieldSignal(scSignal *value);
    void setExpectedSize(uint value);
    uint getExpectedSize();
    void setMacrosEnabled(bool value);
    void setMinConstToArgRatio(double value);
    void setMaxReqUniqInstrCodeRatio(double value);
    void setRestartsEnabled(bool value);
    bool getRestartsEnabled();

    virtual uint getObjectiveCount() const;
    virtual void getObjectiveWeights(sgpWeightVector &output) const;
    virtual void getObjectiveSigns(sgpObjectiveSigns &output) const;
    virtual uint getExpectedInstrCount() = 0;
    virtual uint getExpectedSize() const;
    virtual uint getInputCount() = 0;
    void getStats(uint &totalCalc);

    //-- run
    void init() {if (!m_prepared) prepare(); }
    virtual void prepare();
protected:
    virtual void fillObjectiveSet();
    virtual void getObjectiveSet(sgpObjectiveSet &output) const;
    virtual void setObjectiveSet(const sgpObjectiveSet &value);

    virtual void initInputValues() = 0;
    virtual void generateInputValues() = 0;
    virtual uint getSampleCount() const = 0;
    //virtual void initSubLibs();
    void initLibs(sgpFunLib &mainLib);

    virtual void initProcess(sgpGaGeneration &newGeneration);
    void invokeEntityHandled() const;

    virtual bool calc(uint entityIndex, const sgpEntityBase *entity, sgpFitnessValue &fitness) const;

    virtual bool evaluateProgram(const sgpEntityForGasm &info, const scDataNode &code, sgpFitnessValue &fitness, 
      uint entityIndex) const;

    void runProgramForSamplesSeq(
      const scDataNode &code,
      sgpFitDoubleVector &fxVect, sgpFitDoubleVector &yVect, 
      uint &notNullCnt, long64 &totalCost, long64 &totalTypeDiff) const;

    void runProgramForRevFxInRange(
      uint first, uint last,
      sgpFitDoubleVector &fxVect, sgpFitDoubleVector &yVect, sgpFitDoubleVector &revFxVect, 
      uint revFxMacroNo) const;

    bool rateProgram(uint entityIndex, const sgpEntityForGasm &info, const scDataNode &code, 
      const sgpGpEvalPrgOutput &prgOutput, 
      sgpFitnessValue &fitness) const;

    void handleProgramOutput(const sgpEntityForGasm &workInfo, const sgpGpEvalPrgOutput &prgOutput) const;

    virtual void runProgramForSamplesInRange(
      uint first, uint last,
      sgpFitDoubleVector &fxVect, sgpFitDoubleVector &yVect, 
      uint &notNullCnt, long64 &totalCost, long64 &totalTypeDiff) const = 0;        

    void handleSamplesChanged();

    double calcReverseFuncObj(const sgpEntityForGasm &info, 
      const sgpFitDoubleVector &fxVect, const sgpFitDoubleVector &yVect, const sgpFitDoubleVector &revFxVect) const;

    uint getReverseFuncMacroNo(const sgpEntityForGasm &info) const;

    void calcOutputStats(const sgpFitDoubleVector &fxVect, const sgpFitDoubleVector &yVect, 
      double &errorSse, double &errorSseAbs, double &errorSum, uint &hitCount, sgpFitDoubleVector &errors) const;

    void calcOutputVarRating(uint countNotNullOutput, uint fxDistinctCnt, double fxStdDev, double constToAllArgRatio, 
      double regDistRead, double regDistWrite,
      double &variableOutputRating, bool &constOutput) const;

    virtual void getSampleRange(int dimNo, double &minVal, double &maxVal);

    void prepareObjExtremeCount(const sgpFitDoubleVector &yVect, uint &output);
    void prepareObjDerives(const sgpFitDoubleVector &xVect, const sgpFitDoubleVector &yVect, uint level, sgpFitDoubleVector &output);
    void prepareObjFreqY(const sgpFitDoubleVector &yVect, sgpFitDoubleVector &output);
    void prepareObjAmpliY(const sgpFitDoubleVector &yVect, sgpFitDoubleVector &output);

protected:
  void signalYield() const;
  float readPrgOutputAsFloat(const scDataNode &value, float defValue) const;
  virtual void prepareObjData();
  virtual double calcStdErrorDeriveNForTargetFuncNI1O(const sgpFitDoubleVector &yVect, const sgpFitDoubleVector &fxVect, uint level) const = 0;
  ulong64 calcProgramSizeScore(const sgpEntityForGasm &info, sgpGasmScannerForFitPrg *scanner) const;
  virtual uint getInputArgCount() const = 0;
protected:
  bool m_restartsEnabled;
  uint m_stepCounter;
  int m_sampleSide;
  bool m_nonFuncInstrEnabled;
  bool m_adfsEnabled;
  bool m_macrosEnabled;
  uint m_sampleChangeInterval;
  uint m_expectedSize;
  mutable uint m_totalCalc;  
  double m_minConstToArgRatio;
  double m_maxReqUniqInstrCodeRatio;

  scSignal *m_yieldSignal;
  sgpSamplesChangedEvent *m_onSamplesChanged;
  sgpHandleProgramOutputEvent *m_onHandleProgramOutput;

  uint m_objCacheExtremeCount;
  sgpFitDoubleVector m_objCacheFreqY;   
  sgpFitDoubleVector m_objCacheAmpliY; 
  sgpObjectiveSet m_objectiveSet;
};

// ----------------------------------------------------------------------------
// sgpGpFitnessFun1I1O
// ----------------------------------------------------------------------------
class sgpGpFitnessFun1I1O: public sgpGpFitnessFun4Regression {
public:
  // construction
  sgpGpFitnessFun1I1O(): sgpGpFitnessFun4Regression() {};
  virtual ~sgpGpFitnessFun1I1O() {};
  virtual uint getInputCount();
  virtual void getInputValues(scDataNode &output);
  virtual void setInputValues(const scDataNode &values);
protected:  
  virtual void initInputValues();
  virtual void fillObjectiveSet();
  virtual void generateInputValues();
  virtual uint getSampleCount() const;
  virtual double calcStdErrorDeriveNForTargetFuncNI1O(const sgpFitDoubleVector &yVect, const sgpFitDoubleVector &fxVect, uint level) const;
  virtual uint calcEntityClass(const sgpFitDoubleVector &input, const sgpFitDoubleVector &output, const sgpFitDoubleVector &target) const = 0;
  virtual uint calcEntityClassForTargetFuncNI1O(const sgpFitDoubleVector &fxVect, const sgpFitDoubleVector &yVect) const;
  virtual void runProgramForSamplesInRange(
    uint first, uint last,
    sgpFitDoubleVector &fxVect, sgpFitDoubleVector &yVect, 
    uint &notNullCnt, long64 &totalCost, long64 &totalTypeDiff) const;        
  virtual double calcTargetFunction(double x) const = 0;
  virtual void prepareObjData();
  void prepareTargetOutput(const sgpFitDoubleVector &xVect, sgpFitDoubleVector &yVect);
  virtual uint getInputArgCount() const { return 1; }
protected:
  sgpFitDoubleVector m_inputValues;
  sgpFitDoubleVector m_objCacheDerivedY1; 
  sgpFitDoubleVector m_objCacheDerivedY2;
};

// ----------------------------------------------------------------------------
// sgpGpFitnessFun2I1O
// ----------------------------------------------------------------------------
class sgpGpFitnessFun2I1O: public sgpGpFitnessFun4Regression {
  typedef sgpGpFitnessFun4Regression inherited;
public:
  // construction
  sgpGpFitnessFun2I1O(): sgpGpFitnessFun4Regression() {};
  virtual ~sgpGpFitnessFun2I1O() {};
  virtual uint getInputCount();
  virtual void getInputValues(scDataNode &output);
  virtual void setInputValues(const scDataNode &values);
protected:  
  virtual void initInputValues();
  virtual void generateInputValues();
  virtual uint getSampleCount() const;
  virtual double calcStdErrorDeriveNForTargetFuncNI1O(const sgpFitDoubleVector &yVect, const sgpFitDoubleVector &fxVect, uint level) const;
  virtual uint calcEntityClassForTargetFuncNI1O(const sgpFitDoubleVector &fxVect, const sgpFitDoubleVector &yVect) const;
  virtual void runProgramForSamplesInRange(
    uint first, uint last,
    sgpFitDoubleVector &fxVect, sgpFitDoubleVector &yVect, 
    uint &notNullCnt, long64 &totalCost, long64 &totalTypeDiff) const;        
  virtual uint calcEntityClass(const sgpFitDoubleVector &input1, const sgpFitDoubleVector &input2, 
    const sgpFitDoubleVector &output, const sgpFitDoubleVector &target) const;
  virtual double calcTargetFunction(double x1, double x2) const = 0;

  virtual void fillObjectiveSet();
  virtual void prepareObjData();
  void prepareTargetOutput(const sgpFitDoubleVector &xVect1, const sgpFitDoubleVector &xVect2, sgpFitDoubleVector &yVect);
  virtual uint getInputArgCount() const { return 2; }
protected:
  sgpFitDoubleVector m_inputValuesX1;
  sgpFitDoubleVector m_inputValuesX2;
};


#endif // _GPFITNESSFUN4REGRESSION_H__
