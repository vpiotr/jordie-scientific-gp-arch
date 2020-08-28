/////////////////////////////////////////////////////////////////////////////
// Name:        GasmOperator.h
// Project:     scLib
// Purpose:     Operators for evolver for genetic programs
// Author:      Piotr Likus
// Modified by:
// Created:     23/03/2009
/////////////////////////////////////////////////////////////////////////////

#ifndef _GASMOPERATOR_H__
#define _GASMOPERATOR_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GasmOperator.h
///
/// Gasm-oriented operators:
/// - mutation
/// - insertion (inserts random instruction at random position)
/// - deletion (deletes instruction at random position)
/// - xover
///
// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
// stl
#include <map>
#include <set>
// sc
#include "sc/utils.h"
// sgp
#include "sgp/GasmVMachine.h"
#include "sgp/GaEvolver.h"
#include "sgp/GaOperatorBasic.h"
#include "sgp/GasmEvolver.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
// factors for differences
const double SGP_XOVER_DIFF_RATIO_META_TYPE = 1.0;
const double SGP_XOVER_DIFF_RATIO_INSTR     = 0.95;
const double SGP_XOVER_DIFF_RATIO_REG_NO    = 0.85;
const double SGP_XOVER_DIFF_RATIO_ARG_VALUE = 0.8;
  
const uint SGP_GASM_DEF_GENOME_MAX_LEN_4_DIFF = 20;
const uint SGP_GASM_DEF_INSTR_CODE_CHANGE_SPREAD = 10;
const uint SGP_GASM_DEF_REG_NO_CHANGE_SPREAD = 100;
const uint SGP_GASM_DEF_DATA_TYPES = gdtfAll ^ (gdtfVariant + gdtfString + gdtfStruct + gdtfArray + gdtfNull + gdtfRef);
const uint SGP_INFO_DATA_OFFSET = 1; // magic-number-header, data, magic-number-tail

const uint SGP_INFOBLOCK_IDX_MUT_MAIN_PARAMS_BASE    = 50;
const uint SGP_INFOBLOCK_IDX_MUT_CODE_AREA_BASE      = 100;
const uint SGP_INFOBLOCK_IDX_MUT_TYPE_PROB_BASE      = 200;
const uint SGP_INFOBLOCK_IDX_MUT_OTHER_PARAM_BASE    = 300;
const uint SGP_INFOBLOCK_IDX_MUT_ERR_OBJ_WEIGHT_BASE = 400;
const uint SGP_INFOBLOCK_IDX_MUT_INSTR_PROBS_BASE    = 500;
const uint SGP_INFOBLOCK_IDX_OTHER_PARAM_BASE        = 600;

const uint SGP_INFOPARAM_IDX_OTHER_PARAM_INFO_FADE_RATIO  = 0;
const uint SGP_INFOPARAM_IDX_OTHER_PARAM_INFO_FADE_RATIO2 = 1;
const uint SGP_INFOPARAM_IDX_OTHER_PARAM_INFO_FADE_RATIO3 = 2;
const uint SGP_INFOPARAM_IDX_OTHER_PARAM_ISLAND_ID        = 3;
const uint SGP_INFOPARAM_IDX_OTHER_PARAM_REV_FUN_MACRO_NO = 4;

const uint SGP_GASM_MAIN_BLOCK_INDEX = 0;
const double SGP_GASM_RAND_ARG_PROB = 0.5;
const double SGP_GASM_MAX_RAND_STR_LEN = 50;

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
typedef std::multimap<uint, cell_size_t> sgpGasmOutRegCellMap;
typedef std::set<scDataNodeValueType> sgpGasmDataNodeTypeSet;
typedef std::vector<double> sgpGasmProbList;
typedef std::vector<double> sgpGasmFactorList;
typedef std::auto_ptr<strDiffFunctor> sgpStrDiffFunctorGuard;
typedef std::set<uint> sgpGasmInfoParamSet;

// ----------------------------------------------------------------------------
// sgpGaGenomeCompareToolForGasm
// ----------------------------------------------------------------------------
class sgpGaGenomeCompareToolForGasm: public sgpGaGenomeCompareTool {
public:
  sgpGaGenomeCompareToolForGasm();
  virtual ~sgpGaGenomeCompareToolForGasm() {};
  virtual double calcGenomeDiff(const sgpGaGeneration &newGeneration, uint first, uint second);
  virtual double calcGenomeDiff(const sgpGaGeneration &newGeneration, uint first, uint second, uint genNo);
protected:  
  void checkMaxGenomeLength(uint value);
  void setMaxGenomeLength(uint value);
  double calcGenomeDiffForValues(const sgpGaGenome &gen1, const sgpGaGenome &gen2);
  double calcGenomeDiffForCode(const sgpGaGenome &gen1, const sgpGaGenome &gen2) const;
protected:  
  uint m_maxGenomeLength;
  sgpStrDiffFunctorGuard m_diffFunct;
};

class sgpDistanceFunctionLinearToTorus: public sgpDistanceFunction {
public:
  // construction
  sgpDistanceFunctionLinearToTorus();
  virtual ~sgpDistanceFunctionLinearToTorus();
  // properties
  void setSourceLength(uint value);
  // run
  virtual void calcDistanceAbs(uint first, uint second, double &distance);
  virtual void calcDistanceRel(uint first, uint second, double &distance);
  virtual void calcDistanceXY(uint first, uint second, int &distX, int &distY);
protected:  
  inline void calcPosOnTorus(uint linearPos, int &posX, int &posY);
protected:
  uint m_sourceLength;  
  uint m_rowLength;
};
  
// ----------------------------------------------------------------------------
// sgpGasmCodeProcessor
// ----------------------------------------------------------------------------
// parse & generate GASM code for genetic operators
class sgpGasmCodeProcessor {
public:
  static double calcDupsRatio(const sgpGaGenomeMetaList &info, const sgpGaGenome &genome);
  static void checkMainNotEmpty(const sgpEntityForGasm *workInfo);
  static void castTypeSet(uint supportedGasmTypes, sgpGasmDataNodeTypeSet &dataNodeTypes);
  static bool buildRandomInstr(const sgpFunctionMapColn &functions,
    sgpGasmRegSet &writtenRegs, uint supportedDataTypes, sgpGasmDataNodeTypeSet &dataNodeTypes, 
    const sgpGasmProbList &instrCodeProbs,  
    const sgpVMachine *vmachine, 
    const sgpEntityForGasm *workInfo, const uint *targetBlockIndex,
    sgpGaGenome &newCode);    
  static void getArguments(const sgpGaGenome &genome, 
    int start, uint argCount, scDataNode &output);
  static void prepareRegisterSet(sgpGasmRegSet &regSet, 
    uint supportedDataTypes, uint ioMode, const sgpVMachine *vmachine);
  static scDataNodeValue randomRegNoAsValue(const sgpGasmRegSet &regSet);  
  static uint randomRegNo(const sgpGasmRegSet &regSet);
};

// ----------------------------------------------------------------------------
// Forward function definitions
// ----------------------------------------------------------------------------
// calculate degradation factor for block level (1 -> 1, 2 -> factor, 3 -> factor^2...)
double calcBlockDegradFactor(uint blockNo, double factor);  

// returns random bitset element
uint getRandomBitSetItem(uint bitSet);

// select index of item basing on it's probability
uint selectProbItem(const sgpGasmProbList &list);

// --- island support ---
uint calcIslandId(uint rawValue, uint islandCount);
// ---

#endif // _GASMOPERATOR_H__
