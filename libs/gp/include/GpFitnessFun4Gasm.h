/////////////////////////////////////////////////////////////////////////////
// Name:        GpFitnessFun4Gasm.h
// Project:     sgpLib
// Purpose:     Fitness function which executes Gasm program in order to 
//              calculate evolved function.
// Author:      Piotr Likus
// Modified by:
// Created:     12/06/2011
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPGPFITFUNGASM_H__
#define _SGPGPFITFUNGASM_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file GpFitnessFun4Gasm.h
\brief Fitness function which executes Gasm program

*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sgp/GaEvolver.h"
#include "sgp/GasmVMachine.h"
#include "sgp/GasmFunLib.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
const scString TIMER_RUNPRG = "gp-run-prg";
const scString TIMER_RUNPRG_CORE = "gp-run-prg-core";
const scString TIMER_PREPARE_CODE = "gp-prep-code";

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------

/// Class which keeps context of execution of Gasm program
/// This includes:
/// - function list
/// - library list
/// - vmachine
/// - feature list
/// - allowed data types
//class sgpFitGasmContext {
//};

class sgpFitnessFun4Gasm: public sgpFitnessFunction {
  typedef sgpFitnessFunction inherited;
public:
  // -- construct
  sgpFitnessFun4Gasm();
  virtual ~sgpFitnessFun4Gasm();

  // -- properties
  bool isFunctionSupported(const scString &aName) const;
  sgpFunction *getFunctorForInstrCode(uint instrCode) const;
  virtual uint getProgramStepLimit() const;
  sgpFunctionMapColn &getFunctions();
  void setSupportedDataTypes(uint mask);
  virtual uint getExpectedInstrCount() = 0;
  virtual uint getExpectedSize() = 0;

  // -- execute
  virtual void initProcess(sgpGaGeneration &newGeneration);
  virtual void prepare();
  void initVMachine(sgpVMachine &vmachine) const;
protected:
  virtual void initLibs(sgpFunLib &mainLib) = 0;
  virtual void getFunctionList(scStringList &output) = 0;
  void initFunctionList();
  void setVMachineProgram(sgpVMachine &vmachine, const scDataNode &code) const;
  void prepareFunctions();
  void runProgram(const scDataNode &input, scDataNode &output, uint startBlockNo) const;
  virtual void intPrepare();
protected:
  bool m_prepared;
  uint m_supportedDataTypes;
  sgpFunctionMapColn m_functions;
  std::auto_ptr<sgpFunLib> m_mainLib;
  std::auto_ptr<sgpVMachine> m_vmachine;
};

#endif // _SGPGPFITFUNGASM_H__
