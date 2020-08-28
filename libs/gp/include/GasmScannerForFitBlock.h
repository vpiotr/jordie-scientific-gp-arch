/////////////////////////////////////////////////////////////////////////////
// Name:        GasmScannerForFitBlock.h
// Project:     sgpLib
// Purpose:     GASM scanner for fitness calculations. Single block version.
// Author:      Piotr Likus
// Modified by:
// Created:     12/06/2011
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPGSFORFITBLOCK_H__
#define _SGPGSFORFITBLOCK_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file GasmScannerForFitBlock.h
\brief GASM scanner for fitness calculations. Single block version.

*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sc/dtypes.h"
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

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
class sgpGasmScannerForFitBlock {
public:
  // -- construct
  sgpGasmScannerForFitBlock();
  ~sgpGasmScannerForFitBlock();

  // -- properties
  void setGenome(const sgpGaGenome &value);
  void setCodeMeta(const sgpGaGenomeMetaList &value);
  void setInstrCodeCount(uint value);
  uint getInstrCodeCount() const;
  void setFunctions(sgpFunctionMapColn *value);

  // -- execute
  void init();

  // read details of first block
  bool scanFirstBlockCode(const sgpEntityForGasm &info, const scDataNode &code);

  // read details of nth genome/block
  bool scanBlockCode(const sgpEntityForGasm &info, const scDataNode &code, uint codeIndex);
  
  // calculate how many input arguments has been used
  uint countInputUsed(uint startRegNo, uint endRegNo);

  // calculate min(distance to specified regs)
  // input: range of required regs + global range scope (using mod), reg type: input,output,any
  // output: 
  //  0: all specified regs were used
  //  0..1: some regs found to be used
  //  1..n: minimal distance to specified regs (mod n)
  // How to calc:
  //  f(x) = 2 + min(distance)/max-distance - used-regs/req-regs
  //  if input mode <> any -> verify if correct, otherwise do not use point 
  //  for calculation
  double calcRegDistance(uint searchMinRegNo, uint searchMaxRegNo, uint regNoMod, 
    sgpGvmArgIoMode ioMode, const sgpVMachine &vmachine) const;

  // Rate for all instructions:
  // Calculates how much input is different then output (regs only)
  // Protects against unbreakable instructions like "neg #230, #230"
  // if input reg found in output arg: instr-rate = 1/2
  // otherwise: 1.0 (if no regs, or reg not found)
  // result: avg(instr-rate)
  double calcRegIoDist() const;
    
  // returns number of writes to regs# that are not used later divided by total # of writes
  double calcWritesNotUsed(uint regNoMod, double minRatio) const;

  // calculate how many writes are performed to input args
  double calcWritesToInput(uint regNoMod, uint minInputRegNo, uint maxInputRegNo) const;

  // calculate number of writes to output reg#
  // higher value - worse
  double calcWritesToOutput(uint regNoMod, uint minOutputRegNo, uint maxOutputRegNo) const;

  // calculate number of writes after last write to output reg#
  // higher value - worse
  double calcOutputDistanceToEnd(uint regNoMod, uint minOutputRegNo, uint maxOutputRegNo, double minRate) const;

  // calculate how many different registers has been used, exclude input args
  double calcUsedRegs(uint regNoMod, uint minInputRegNo, uint maxInputRegNo) const;

  // returns number of reads that are using uninitialized regs# divided by total # of reads
  double calcUninitializedReads(uint regNoMod, uint minInputRegNo, uint maxInputRegNo) const;

  // calculate how many unique instruction codes are used / total-instr-count
  double calcUniqueInstrCodes(double maxReqRatio) const;

  // calc constant to all argument count ratio (50% is maximum required)
  double calcConstantToArgRatio(double minReqRatio) const;

  // calculate instructions with constant-only arguments
  double calcConstOnlyInstr(double minRatio) const;
  
  // calc ratio of too long sequences of the same instructions
  // too long means > estimated correct maximum = 3  
  // if sequence is shorter it is calculated as len = 1
  double calcSameInstrCodeRatio() const;
  
  void getArgMeta(cell_size_t instrOffset, scDataNode &argMeta) const;
  void getInstrInfo(cell_size_t instrOffset, uint &instrCodeRaw, uint &argCount) const;
 
protected:
  sgpFunction *getFunctorForInstrCode(uint instrCode) const;
private:
  bool m_initDone;
  uint m_instrCodeCount;
  sgpGaGenome m_genome;
  sgpGaGenomeMetaList m_codeMeta;
  sgpFunctionMapColn *m_functions;
};

#endif // _SGPGSFORFITBLOCK_H__
