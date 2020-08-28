/////////////////////////////////////////////////////////////////////////////
// Name:        GasmEvolver.h
// Project:     sgpLib
// Purpose:     Evolver for genetic programs
// Author:      Piotr Likus
// Modified by:
// Created:     23/03/2009
/////////////////////////////////////////////////////////////////////////////

#ifndef _GASMEVOLVER_H__
#define _GASMEVOLVER_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GasmEvolver.h
///
/// Evolver for Gasm programs.
///
/// Gasm uses multi-block code. 
/// Special GA operators are required for this code.
/// Here you can find a base for fitness function. 
/// Program can be build with RNA+DNA or directly from genome.
///
/// If RNA+DNA:
/// - initial program outputs code as an array of scDataValue
/// - in basic version data is generated using "data" command (from code block)
/// - output array is used as code for evaluation
/// 
/// If direct version:
/// - code is provided as array of scDataValue or 
///   standard Gasm program code - multi-block scDataNode
///
/// Gasm byte code program is build from gene using the following:
/// - each block is evolved separately, only code inside it 
/// - multi-point mutation
///   - constant <-> reg
///   - constant <-> function call with params
///   - constant <-> constant
///   - reg <-> reg
///   - function code <-> function code // force param count
///   - function code+/- n 
/// - multi-point cross-over
///   - possible for each block several times 
/// - optionally include GA params in first block of code:
///   - structure:
///     - 0xa50505a5: marker (constant ulong64)
///     - parameters for GA: 
///       - mut prob: -1,0..1,0 
///       - cross-over prob: -1,0..1,0
///       - species ratio: -1,0..1,0
///     - 0xe51515e5: marker (constant ulong64)
///   - if such a block is detected, it is not used for evaluation, only for breeding
///   - block is generated on start if special initialization function is used
/// - genome code can contain every possible value for scDataValue, so operators need to handle this 
/// - final values = base value + genome value  
/// - if prob > 1.0 then operation is performed several times (1.1 = 2x0.55, 2.1 = 3x0.7)
/// - operators work directly on code 
///   - dynamic meta info (cell size, type)
///   - constants mutated as bit- or alpha-strings
/// or 
/// ? conversion from program code to gene:
///   - int,uint: 32-long bitstring
///   - float, double, long, ulong: 64-long bitstring
///   - xdouble: 100-long bitstring
///   - byte: 8-long bitstring
///   - string: alphastring
///   - bool: 4-bit bitstring (0..3 - false, 4..15 - true) 
///
/// Initial code can be generated as follows: 
/// - select maximum number of instructions
/// - generate n = random value between 2 and maximum
/// - generate n random instructions + header + footer
///
///  
// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
// bost
#include <boost/ptr_container/ptr_list.hpp>
// sgp
#include "sgp/GaEvolver.h"
// for program code
#include "sgp/GasmVMachine.h"
#include "sgp/EntityForGasm.h"
#include "sgp/GasmGeneration.h"


// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
// index in infoblock variable array ignoring prefix existance
const uint SGP_INFOBLOCK_IDX_MUT_PROB = 1;
const uint SGP_INFOBLOCK_IDX_XCROSS_PROB = 2;
const uint SGP_INFOBLOCK_IDX_MATCH_RATE = 3;

enum sgpGasmGenomeType {
  ggtValue = 1,
  ggtInstrCode = 2,
  ggtRegNo = 4,
  ggtMeta = 8,
  ggtUnknInstrCode = 16
};

const uint DEF_INFO_BLOCK_SIZE = 31;

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------

typedef sgpEntityForGasm *sgpGaGenomeWorkInfoGasmPtr;

// ----------------------------------------------------------------------------
// sgpGasmEvolver
// ----------------------------------------------------------------------------
class sgpGaOperatorAllocGasm: public sgpGaOperatorAlloc {
public:  
  sgpGaOperatorAllocGasm();
  virtual ~sgpGaOperatorAllocGasm();
  void setInfoMap(sgpInfoBlockVarMap *map);
  virtual sgpGaGeneration *execute();
protected:  
  sgpInfoBlockVarMap *m_infoMap;
};

#endif // _GASMEVOLVER_H__
