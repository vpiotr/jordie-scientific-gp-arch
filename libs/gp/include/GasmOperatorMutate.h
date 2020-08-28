/////////////////////////////////////////////////////////////////////////////
// Name:        GasmOperatorMutate.h
// Project:     scLib
// Purpose:     Mutation operator for GP virtual machine.
// Author:      Piotr Likus
// Modified by:
// Created:     29/12/2009
/////////////////////////////////////////////////////////////////////////////

#ifndef _GASMOPERATORMUTATE_H__
#define _GASMOPERATORMUTATE_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GasmOperatorMutate.h
///
/// Mutation types for code:
/// - constant value change:
///   - bool: false <-> true
///   - xint: alphastring mutation
///   - real: 
///     abs(val) > 1.0 -> val +/- random(0.1*abs(val)) +/- 10*random(1.0) +/- 10^random(10.0)
///     abs(val) <= 1.0 -> val = val +/- random(abs(0.1*val))
///   - string: alphastring mutation
/// - constant <-> reg no
/// - constant type up (bool->byte->uint->uint64->int->int64->float->double->xdouble)
/// - constant type down reversed(bool->byte->uint->uint64->int->int64->float->double->xdouble)
/// - reg_no +/- n (n = 1..10) (?within reg_no limits?)
/// - instr code+/- n (n = 1..10) (?within instruction code limits?)
/// - insertion of random funct. after nth instruction with random args
/// - deletion of nth instruction 
/// 
/// Mutation types for info blocks:
/// - alphastring mutation for double values


// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sc/dtypes.h"
#include "sc/events/Events.h"

#include "sgp/GaOperatorBasic.h"
#include "sgp/GasmVMachine.h"
#include "sgp/GasmOperator.h"
#include "sgp/EntityIslandTool.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------
enum sgpGasmMutType {
  gmtUndef = 0,
  gmtValue = 1,
  gmtValueReg = 2,
  gmtValueTypeUp = 4,
  gmtValueTypeDown = 8,
  gmtValueNeg = 16,
  gmtRegValue = 32,
  gmtRegNo = 64,
  gmtRegNoZero = 128,
  gmtInstrCode = 256,
  gmtInsert = 512,
  gmtDelete = 1024,
  gmtReplace = 2048,
  gmtSwapArgs = 4096,
  gmtSwapInstr = 8192,
  gmtJoinInstr = 16384,
  gmtSplitInstr = 32768,
  gmtBlock = 65536,
  gmtNoChange = 131072,
  gmtLinkInstr = 262144,
  gmtMacroIns = 1 << 19,
  gmtMacroGen = 1 << 20,
  gmtMacroDel = 1 << 21,
  gmtMacroArgAdd = 1 << 22,
  gmtMacroArgDel = 1 << 23  
};

enum sgpGasmMutFeatures {
  gmfProtectBlockOutput    = 1,  // keep last write to block output
  gmfValidateOutputRegType = 2,  // validate type of output register
  gmfFadeMutTypeRatios     = 4,  // fade mutation levels in time
  gmfMacrosEnabled         = 8,  // if enabled, allows creation & usage of macros  
  gmfMacroDelEnabled       = 16, // if enabled, allows deletion of macros
  gmfAdfsEnabled           = 32, // if enabled, allows creation of subroutines
  gmfControlMutRate        = 64, // if enabled, keeps mutation ratio on pop level in a given range
  gmfMaxTailMutRate        = 128 // if enabled, last macros have highest mutation rate  
};

const uint gmfDefault = 0;

// maximum ID of mutation
//const uint gmtMutTypeMax = 2048;

typedef std::vector<sgpGasmMutType> sgpGasmTypeList;

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
const double SGP_GASM_DEF_OPER_PROB_MUTATE = 0.003;
const double SGP_GASM_MUT_PROB_INS_DEL = 0.01;
const double SGP_GASM_MUT_PROB_VALUE_VALUE = 0.5;
const double SGP_GASM_MUT_PROB_INS = SGP_GASM_MUT_PROB_INS_DEL * 0.5;
const double SGP_GASM_MUT_MAX_RAND_REG = 255;
const double SGP_GASM_MUT_REAL_SMALL_CHANGE_RATIO = 2.0;
const double SGP_GASM_MUT_REAL_LARGE_CHANGE_RATIO = 10.0;
const double SGP_GASM_MUT_SMALL_INT_CHANGE = 10.0;
const double SGP_GASM_MUT_MAX_INSTR_CODE = 512;

const double SGP_GASM_MUT_DEF_GLOBAL_RATIO = 0.5;
const double SGP_MUT_REAL_SPREAD = 40.0;
const double SGP_MUT_XINT_SPREAD = 20.0;
const double SGP_MUT_FADE_TYPE_PROB_RATIO = 0.5;
const double SGP_MUT_CHANGE_RATIO_FACTOR_DEC = 0.9;
const double SGP_MUT_CHANGE_RATIO_FACTOR_INC = 1.0 / SGP_MUT_CHANGE_RATIO_FACTOR_DEC;
const float SGP_GASM_MUT_CHANGE_RATE_MIN = 0.1f;
const float SGP_GASM_MUT_CHANGE_RATE_MAX = 0.8f;
//const double SGP_MUT_GLOBAL_RATIO_INFO_FILTER = 0.3;
const double SGP_MUT_NOMUT_RATIO_INFO_FILTER = 0.5;
const uint SGP_MUT_MAX_MACRO_INSTR_CNT_ON_GEN = 10;
const double SGP_MUT_MACRO_LEN_DEC_FACTOR = 1.7;
const uint SGP_GASM_MUT_DEF_BLOCK_SIZE_LIMIT = 4*500;
const double SGP_MUT_MAX_SIZE_INC_RATE = 1.2;
const double SGP_MUT_SIZE_LIMIT_APPLY_PROB = 0.5;

const uint SGP_GASM_MUT_CODE_AREA_COUNT = 3; // instruction, input, output
const uint SGP_GASM_MUT_PROB_SECTION_SIZE = 6;
const uint SGP_GASM_MUT_PROB_SECTION_COUNT = 5;

const uint SGP_GASM_MUT_PROB_SECTION_GLOBAL = 0;
const uint SGP_GASM_MUT_PROB_SECTION_VALUE = SGP_GASM_MUT_PROB_SECTION_SIZE;
const uint SGP_GASM_MUT_PROB_SECTION_REGNO = SGP_GASM_MUT_PROB_SECTION_SIZE*2;
const uint SGP_GASM_MUT_PROB_SECTION_INSTR = SGP_GASM_MUT_PROB_SECTION_SIZE*3;
const uint SGP_GASM_MUT_PROB_SECTION_MACRO = SGP_GASM_MUT_PROB_SECTION_SIZE*4;

const uint SGP_GASM_MUT_PROB_SECTION_GLOBAL_SIZE = 4;
const uint SGP_GASM_MUT_PROB_SECTION_VALUE_SIZE = 6;
const uint SGP_GASM_MUT_PROB_SECTION_REGNO_SIZE = 4;
const uint SGP_GASM_MUT_PROB_SECTION_INSTR_SIZE = 4;
const uint SGP_GASM_MUT_PROB_SECTION_MACRO_SIZE = 5;

const uint SGP_GASM_MUT_PROB_ALL_SECTION_SIZE = 
  SGP_GASM_MUT_PROB_SECTION_GLOBAL_SIZE +
  SGP_GASM_MUT_PROB_SECTION_VALUE_SIZE +
  SGP_GASM_MUT_PROB_SECTION_REGNO_SIZE +
  SGP_GASM_MUT_PROB_SECTION_INSTR_SIZE +
  SGP_GASM_MUT_PROB_SECTION_MACRO_SIZE;
  
const uint SGP_GASM_MUT_PROB_SECTION_GLOBAL_INFO_OFFSET = 0;
const uint SGP_GASM_MUT_PROB_SECTION_VALUE_INFO_OFFSET = 
  SGP_GASM_MUT_PROB_SECTION_GLOBAL_INFO_OFFSET +
  SGP_GASM_MUT_PROB_SECTION_GLOBAL_SIZE;
const uint SGP_GASM_MUT_PROB_SECTION_REGNO_INFO_OFFSET = 
  SGP_GASM_MUT_PROB_SECTION_VALUE_INFO_OFFSET + 
  SGP_GASM_MUT_PROB_SECTION_VALUE_SIZE;
const uint SGP_GASM_MUT_PROB_SECTION_INSTR_INFO_OFFSET = 
  SGP_GASM_MUT_PROB_SECTION_REGNO_INFO_OFFSET +
  SGP_GASM_MUT_PROB_SECTION_REGNO_SIZE;
const uint SGP_GASM_MUT_PROB_SECTION_MACRO_INFO_OFFSET = 
  SGP_GASM_MUT_PROB_SECTION_INSTR_INFO_OFFSET +
  SGP_GASM_MUT_PROB_SECTION_INSTR_SIZE;

const uint SGP_GASM_MUT_PROB_IDX_GLOBAL_DELETE   = 0;
const uint SGP_GASM_MUT_PROB_IDX_GLOBAL_REPLACE  = 1;
const uint SGP_GASM_MUT_PROB_IDX_GLOBAL_INSERT   = 2;
const uint SGP_GASM_MUT_PROB_IDX_GLOBAL_BLOCK    = 3;

const uint SGP_GASM_MUT_PROB_IDX_VALUE_VALUE     = 0;
const uint SGP_GASM_MUT_PROB_IDX_VALUE_VALUE_REG = 1;
const uint SGP_GASM_MUT_PROB_IDX_VALUE_TYPE_UP   = 2;
const uint SGP_GASM_MUT_PROB_IDX_VALUE_TYPE_DOWN = 3;
const uint SGP_GASM_MUT_PROB_IDX_VALUE_SWAP_ARGS = 4;
const uint SGP_GASM_MUT_PROB_IDX_VALUE_NEG       = 5;

const uint SGP_GASM_MUT_PROB_IDX_INSTR_INSTR_CODE  = 0;
const uint SGP_GASM_MUT_PROB_IDX_INSTR_JOIN_INSTR  = 1;
const uint SGP_GASM_MUT_PROB_IDX_INSTR_SPLIT_INSTR = 2;
const uint SGP_GASM_MUT_PROB_IDX_INSTR_LINK_INSTR  = 3;

const uint SGP_GASM_MUT_PROB_IDX_REGNO_REGNO       = 0;
const uint SGP_GASM_MUT_PROB_IDX_REGNO_REGNO_ZERO  = 1;
const uint SGP_GASM_MUT_PROB_IDX_REGNO_REG_VALUE   = 2;
const uint SGP_GASM_MUT_PROB_IDX_REGNO_SWAP_ARGS   = 3;

const uint SGP_GASM_MUT_PROB_IDX_MACRO_INS         = 0;
const uint SGP_GASM_MUT_PROB_IDX_MACRO_GEN         = 1;
const uint SGP_GASM_MUT_PROB_IDX_MACRO_DEL         = 2;
const uint SGP_GASM_MUT_PROB_IDX_MACRO_ARG_ADD     = 3;
const uint SGP_GASM_MUT_PROB_IDX_MACRO_ARG_DEL     = 4;

const uint SGP_MUT_OTHER_PARAM_IDX_NOMUT_RATIO          = 0;
const uint SGP_MUT_OTHER_PARAM_IDX_GLOBAL_RATIO         = 1;
const uint SGP_MUT_OTHER_PARAM_IDX_BLOCK_DEGRAD         = 2; // per block mutation ratio degradation
const uint SGP_MUT_OTHER_PARAM_IDX_VAL_CHG_STEP         = 3;
const uint SGP_MUT_OTHER_PARAM_IDX_MULTI_POINT_RATIO    = 4;
const uint SGP_MUT_OTHER_PARAM_IDX_MACRO_CHG_FILTER     = 5;
const uint SGP_MUT_OTHER_PARAM_IDX_MACRO_CHG_CENTER     = 6;
const uint SGP_MUT_OTHER_PARAM_IDX_MACRO_CHG_DIST_SHAPE = 7;
const uint SGP_MUT_OTHER_PARAM_IDX_GEN_CHG_CENTER       = 8;
const uint SGP_MUT_OTHER_PARAM_IDX_GEN_CHG_DIST_SHAPE   = 9;

const uint SGP_GASM_MUT_AREA_COUNT = 3; // code, output, input

const float SGP_MIN_MUT_PROB_VALUE = 0.001F;
const uint SGP_MUT_DEF_INFO_VAR_LEN = 12;

const uint SGP_MUT_AREA_IDX_INSTR   = 0;
const uint SGP_MUT_AREA_IDX_INPUT   = 1;
const uint SGP_MUT_AREA_IDX_INOUT   = 2;
const uint SGP_MUT_AREA_IDX_INCONST = 3;
const uint SGP_MUT_AREA_IDX_INREG   = 4;

const uint MUT_CODE_OUT_REG_TRY_LIMIT = 1000;

const uint SG_MUT_VAL_XINT_OPER_CODE_PLUS  = 0;
const uint SG_MUT_VAL_XINT_OPER_CODE_MINUS = 1;
const uint SG_MUT_VAL_XINT_OPER_CODE_MULT  = 2;
const uint SG_MUT_VAL_XINT_OPER_CODE_DIV   = 3;
const uint SG_MUT_VAL_XINT_OPER_CODE_XOR   = 4;
const uint SG_MUT_VAL_XINT_OPER_CODE_MAX   = 4;

const uint SG_MUT_MAIN_BLOCK_INDEX = 0;


// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// sgpGasmOperatorMutate
// ----------------------------------------------------------------------------
class sgpGasmOperatorMutate: public sgpOperatorMutateBase {
  typedef sgpOperatorMutateBase inherited;
public:
  // construction
  sgpGasmOperatorMutate();
  virtual ~sgpGasmOperatorMutate();

  // properties
  double getProbability();
  void setProbability(double aValue);
  double getInfoProbability();
  void setInfoProbability(double aValue);
  void setFunctions(const sgpFunctionMapColn &functions);
  void getFunctions(sgpFunctionMapColn &functions);
  void setVMachine(sgpVMachine *machine);
  void setSupportedDataTypes(uint mask);
  virtual void getCounters(scDataNode &output);  
  void setInstrCodeChangeSpread(uint value);
  void setRegNoChangeSpread(uint value);
  void setLargeFloatChangeRatio(double value);
  void setSmallFloatChangeRatio(double value);
  void setTypeProbs(const scDataNode &valueList);
  virtual uint getTypeProbsSize() const;
  virtual uint getCodeAreaFactorsCount() const;
  virtual void setMetaForInfoBlock(const sgpGaGenomeMetaList &value);
  void setMaxBlockCount(uint value);
  uint getMaxBlockCount();
  void setGenomeChangedTracer(sgpGenomeChangedTracer *tracer);
  uint getFeatures();
  void setFeatures(uint value);
  void setFadingInfoParamSet(const sgpGasmInfoParamSet &paramSet);
  void setFadingInfoParamSet2(const sgpGasmInfoParamSet &paramSet);
  void setFadingInfoParamSet3(const sgpGasmInfoParamSet &paramSet);
  void setIslandLimit(uint value);
  void setExperimentParams(const sgpGaExperimentParams *params);
  void setBlockSizeLimit(uint value);
  void setYieldSignal(scSignal *value);
  void setIslandTool(sgpEntityIslandToolIntf *value);
  // run
  virtual void init();  
  virtual void execute(sgpGaGeneration &newGeneration);
  //@static bool buildRandomInstr(const sgpFunctionMapColn &functions,
  //@  sgpGasmRegSet &writtenRegs, uint supportedDataTypes, sgpGaGenome &newCode);
protected:
  void initStep(sgpGaGeneration &newGeneration);
  void initStep(sgpGaGeneration &newGeneration, const scDataNode &itemList);
  void updateBlockSizeLimit(sgpGaGeneration &newGeneration);
  void updateBlockSizeLimit(sgpGaGeneration &newGeneration, const scDataNode &itemList);
  double calcCodeMutProb(const sgpEntityForGasm &workInfo, double baseProb);
  void executeByIslands(sgpGaGeneration &newGeneration);
  void executeOnIsland(sgpGaGeneration &newGeneration, scDataNode &islandItems, uint islandId);
  void executeOnAll(sgpGaGeneration &newGeneration);
  void executeOnBlock(sgpGaGeneration &newGeneration);
  void executeOnBlockByIds(sgpGaGeneration &newGeneration, scDataNode &itemList);  
  virtual void intPrepareIslandMap(const sgpGaGeneration &input, uint islandLimit, scDataNode &output);
  virtual bool processEntity(sgpGaGeneration &newGeneration, uint entityIndex, 
    double infoProb, double codeProb);
  virtual bool processGenome(sgpGaGenome &genome, int genomeCount, double aProb, 
    bool infoBlock, uint inputCount, sgpEntityForGasm *workInfo, uint genomeIndex, uint entityIndex, bool &mutPerformed);
  void mutateGenomeInfo(sgpGaGenome &genome, const sgpEntityForGasm *workInfo, int varIndex, uint offset);
  bool mutateGenomeCode(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, int varIndex, 
    uint offset, uint inputCount, const sgpGasmProbList &mutTypeProbs, sgpEntityForGasm *workInfo, 
    uint genomeIndex, uint entityIndex,
    double &mutCost);
  virtual bool doMutateGenomeCode(const sgpGaGenomeMetaList &info, 
    sgpGaGenome &genome, int varIndex, uint offset, uint inputCount, 
    sgpGasmMutType mutType, uint instrOffset, uint ioMode, scDataNode &argMeta, uint argNo, 
    sgpEntityForGasm *workInfo, uint genomeIndex, uint entityIndex,
    double &mutCost);
  void calcMetaForInfo(sgpGaGenomeMetaList &info);
  uint countGenomeTypes(const sgpGaGenomeMetaList &info, uint userTypeMask, uint instrOffset = 0) const;  
  inline void calcMetaForCode(const sgpGaGenome &genome, 
    const sgpEntityForGasm *workInfo,
    sgpGaGenomeMetaList &info, uint &genomeSize);
  inline void calcMetaForCodeScan(const sgpGaGenome &genome, sgpGaGenomeMetaList &info);
  void calcGenSizeFactors(const sgpGaGenome &genome, 
    const sgpEntityForGasm *workInfo,
    const sgpGaGenomeMetaList &info,
    sgpGasmFactorList &factors);
  inline void mutateVar(const sgpGaGenomeMetaInfo &metaInfo, uint offset, scDataNode &var);
  sgpGasmMutType selectRandomMutateMethodUniform(uint varType, uint ioMode, uint instrOffset); 
  sgpGasmMutType selectRandomMutateMethodWithProbs(uint varType, uint ioMode, uint instrOffset); 
  uint findInstrBeginBackward(const sgpGaGenomeMetaList &info, uint startOffset);
  bool mutateGenomeCodeValue(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    int varIndex, uint offset, sgpEntityForGasm *workInfo);
  bool mutateGenomeCodeRegValue(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    uint instrOffset, int varIndex, uint offset, scDataNode &argMeta);
  bool mutateGenomeCodeSwapArgs(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    int varIndex, scDataNode &argMeta, uint argNo, uint ioMode);
  bool mutateGenomeCodeTypeUp(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    uint instrOffset, int varIndex, uint offset);
  void mutateGenomeCodeTypeUp(scDataNodeValue &value);
  bool mutateGenomeCodeTypeDown(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    uint instrOffset, int varIndex, uint offset);
  void mutateGenomeCodeTypeDown(scDataNodeValue &value);
  bool mutateGenomeCodeInsert(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    uint initInstrOffset, int varIndex, uint offset, 
    uint blockInputCount, const sgpEntityForGasm *workInfo, uint genomeIndex);
  bool mutateGenomeCodeDelete(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    uint instrOffset,
    int varIndex, uint offset);
  bool mutateGenomeCodeReplace(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    uint instrOffset,  
    int varIndex, uint offset, uint blockInputCount, const sgpEntityForGasm *workInfo, uint genomeIndex);
  void mutateGenomeCodeGenBlock(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    int varIndex, uint offset, uint blockInputCount, sgpEntityForGasm *workInfo, uint genomeIndex, double &mutCost);
  bool mutateGenomeCodeSwapInstr(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    int varIndex, uint offset, uint instrOffset);
  bool mutateGenomeCodeJoinInstr(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    int varIndex, uint offset, uint instrOffset);
  bool mutateGenomeCodeSplitInstr(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    int varIndex, uint offset, uint instrOffset, uint blockInputCount, const scDataNode &argMeta,
    const sgpEntityForGasm *workInfo, uint genomeIndex);
  bool mutateGenomeCodeLinkInstr(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    int varIndex, uint instrOffset, const scDataNode &argMeta, 
    const sgpEntityForGasm *workInfo);
  bool mutateGenomeCodeInstrCode(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    int varIndex, uint offset, const sgpEntityForGasm *workInfo);
  bool mutateGenomeCodeValueReg(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    int varIndex, uint offset, uint instrOffset, uint blockInputCount, uint ioMode, scDataNode &argMeta);
  bool mutateGenomeCodeRegNo(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    int varIndex, uint offset, uint instrOffset, uint blockInputCount, uint ioMode);
  bool mutateGenomeCodeRegNoZero(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    int varIndex, uint offset, uint instrOffset, uint blockInputCount, uint ioMode);
  bool mutateGenomeCodeValueNeg(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    int varIndex, uint offset);
  bool mutateGenomeCodeMacroGenerate(const sgpGaGenomeMetaList &initInfo, sgpGaGenome &initGenome, 
    uint initInstrOffset, uint blockInputCount, uint genomeIndex, sgpEntityForGasm *workInfo, double &mutCost);
  bool mutateGenomeCodeMacroInsert(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    uint instrOffset, uint blockInputCount, sgpEntityForGasm *workInfo, uint genomeIndex, double &mutCost);
  bool mutateGenomeCodeMacroDelete(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    sgpEntityForGasm *workInfo, uint genomeIndex, double &mutCost);

  uint findInstrBeginForward(const sgpGaGenomeMetaList &info, uint aStart);
  uint findNthInstrForward(const sgpGaGenomeMetaList &info, uint aStart, uint n);
  void getBlockMeta(sgpGaGeneration &newGeneration, 
    uint entityIndex, uint genomeIndex, scDataNode &output);
  uint getInputCountFromBlockMeta(const scDataNode &blockMeta);
  void findWrittenRegs(const sgpGaGenomeMetaList &info, 
    sgpGaGenome &genome, uint endPos, uint blockInputCount, sgpGasmRegSet &writtenRegs);
  uint getInstrOutputDataTypeSet(const scDataNode &argMeta);
  void findOutRegCellMap(const sgpGaGenomeMetaList &info, 
    sgpGaGenome &genome, uint endPos, sgpGasmOutRegCellMap &outputMap);
  void swapGenomeBlocks(sgpGaGenome &genome, uint offset1, uint size1, uint offset2, uint size2);
  bool getArgMetaForInstr(const sgpGaGenome &genome, uint instrOffset, scDataNode &argMeta);
  bool getOutputRegNo(const sgpGaGenome &genome, uint instrOffset, const scDataNode &argMeta, uint &regNo);
  bool setRandomInputArgAsRegNo(sgpGaGenome &genome, uint instrOffset, const scDataNode &argMeta, uint regNo);
  void setOutputRegNo(sgpGaGenome &genome, uint instrOffset, const scDataNode &argMeta, uint regNo);
  void monitorDupsFromOperator(const sgpGaGenome &genomeBefore, const sgpGaGenome &genomeAfter, 
    const scString &operTypeName);
  void calcInfoBlockSize(const sgpGaGenomeMetaList &metaInfo, uint &output);
  void fillTypeList();
  sgpGasmMutType selectRandomMutateMethodWithProbsFromInfo(
    const sgpGasmProbList &probList, uint varType, uint ioMode, uint instrOffset, uint blockCount, 
    const sgpEntityForGasm *workInfo,
    bool mainBlock, uint blockSize);
  void prepareMutGenProbs(sgpGasmProbList &output, const sgpEntityForGasm *workInfo, uint genomeSize);
  void readMutTypeProbs(const sgpEntityForGasm *workInfo, sgpGasmProbList &mutTypeProbs);
  bool readMutInstrCodeProbs(const sgpEntityForGasm *workInfo, sgpGasmProbList &outProbs);
  void copyProbs(sgpGasmProbList &output, const sgpGasmProbList &input, uint startOffset, uint blockSize);
  bool isFunctionSupported(const scString &aName);
  uint getInstrCode(const scString &aName);
  void buildRandomBlock(uint blockInputCount, uint instrLimit, 
    const sgpEntityForGasm *workInfo, uint targetBlockIndex,
    scDataNode &output);
  void updateWrittenRegs(sgpGaGenome &newCode, uint instrOffset, sgpGasmRegSet &writtenRegs);
  void traceEntityChanged(const sgpGaGeneration &newGeneration, 
    uint itemIndex) const;
  uint getRegChangeCount(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, uint regNo);
  void getArgMetaInfo(uint instrCode, uint argNo, uint &dataType, uint &ioMode);
  void getArgDataTypes(sgpGaGenome &genome, uint instrOffset, int varIndex, uint &dataTypes);
  void getArgTypesAsSetFinal(sgpGaGenome &genome, uint instrOffset, int varIndex, sgpGasmDataNodeTypeSet &outset);
  double getStepSizeRatio(const sgpEntityForGasm *workInfo, double spread);
  uint getMultiPointRatio(const sgpEntityForGasm *workInfo);
  void performFadeMutTypeProbs(sgpGaGeneration &newGeneration,
    uint entityIndex);
  void readFadeParams(sgpEntityForGasm *workInfo, uint paramIdx, double &baseFadeRatio, bool &useFade);
  void fadeInfoParams(sgpEntityForGasm *workInfo, const sgpGasmInfoParamSet &paramSet, 
    double baseFadeRatio);
  void findWrittenRegsWithDist(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    uint instrOffset, scDataNode &writtenRegWithDistance);
  void processRegDataTypes(scDataNode &regsWithDistance);
  void appendInputArgsWithDist(const sgpProgramCode& code, uint codeBlockIndex, uint distance,
    scDataNode &writtenRegWithDistance);
  void macroReplaceInputRegs(const scDataNode& macroInputMeta, const scDataNode& writtenRegsWithDist, 
    const sgpGaGenomeMetaList &macroInfo, sgpGaGenome& blockCode);
  void macroReplaceArgRegReadInCode(uint oldRegNo, uint newRegNo, cell_size_t instrOffset, const sgpGaGenomeMetaList& info, sgpGaGenome& genome);
  uint macroFindMatchingInputArgReg(const sgpGaGenomeMetaList &info, sgpGaGenome &genome, 
    cell_size_t instrOffset, uint dataTypes, uint defValue);
  void macroReplaceOutRegInCode(uint oldRegNo, uint newRegNo, cell_size_t instrOffset, 
    const sgpGaGenomeMetaList &info, sgpGaGenome &genome);
  uint getCodeBlockOffset(const sgpEntityForGasm *workInfo);
  bool canAddCodeToGenome(const sgpEntityForGasm *workInfo, const sgpProgramCode &code, const sgpGaGenome &genome, uint genomeIndex);
  uint genomeToBlockIndex(const sgpEntityForGasm *workInfo, uint value);
  sgpFunction *getFunctor(const sgpGaGenome &genome, uint instrOffset) const;
  void updateMutRate(uint totalPopSize, uint changedEntityCount);
  void buildMacroInstr(sgpGaGenome &newCode, uint macroNo, uint outputRegNo, const scDataNode &inputArgs);
  void invokeNextEntity();
protected:  
//configuration
  double m_probability;
  double m_infoProbability;
  uint m_maxBlockCount;
  uint m_instrCodeChangeSpread;
  int m_instrCodeChangeMin;
  int m_instrCodeChangeMax;
  uint m_regNoChangeSpread;
  int m_regNoChangeMin;
  int m_regNoChangeMax;
  scDataNode m_typeProbs;
  uint m_supportedDataTypes;
  sgpGasmDataNodeTypeSet m_supportedDataNodeTypes;
  sgpFunctionMapColn m_functions;
  sgpGaGenomeMetaList m_metaForInfoBlock;
  uint m_features;
  double m_globalMutRatio;
  sgpGasmInfoParamSet m_fadingParamSet;
  sgpGasmInfoParamSet m_fadingParamSet2;
  sgpGasmInfoParamSet m_fadingParamSet3;
  float m_changeRateMin;
  float m_changeRateMax;
  uint m_islandLimit;
  uint m_blockSizeLimit;
  uint m_blockSizeLimitStep;
  const sgpGaExperimentParams *m_experimentParams;
  scSignal *m_yieldSignal;
//state  
  scDataNode m_functionNameMap;
  float m_changeRatio;
  uint m_changedEntityCount;
  uint m_scannedCount;  
//other
  double m_largeFloatChangeRatio;
  double m_smallFloatChangeRatio;  
  uint m_infoBlockSize;
  sgpVMachine *m_machine;  
  scDataNode m_counters;
  sgpGasmProbList m_typeProbSectionGlobal;
  sgpGasmProbList m_typeProbSectionValue;
  sgpGasmProbList m_typeProbSectionInstr;
  sgpGasmProbList m_typeProbSectionRegNo;
  sgpGasmProbList m_typeProbSectionMacro;
  sgpGasmTypeList m_typeListGlobal;
  sgpGasmTypeList m_typeListValue;
  sgpGasmTypeList m_typeListInstr;
  sgpGasmTypeList m_typeListRegNo;
  sgpGasmTypeList m_typeListMacro;
  sgpGenomeChangedTracer *m_genomeChangedTracer;
  sgpEntityIslandToolIntf *m_islandTool;
};


#endif // _GASMOPERATORMUTATE_H__
