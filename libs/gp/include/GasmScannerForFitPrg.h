/////////////////////////////////////////////////////////////////////////////
// Name:        GasmScannerForFitPrg.h
// Project:     sgpLib
// Purpose:     GASM scanner for fitness calculations. Whole program version.
// Author:      Piotr Likus
// Modified by:
// Created:     12/06/2011
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPGSFORFITPRG_H__
#define _SGPGSFORFITPRG_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file GasmScannerForFitPrg.h
\brief GASM scanner for fitness calculations. Whole program version.

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
/// Whole GASM program scanner functions.
class sgpGasmScannerForFitPrg {
public:
  // -- construct
  sgpGasmScannerForFitPrg();
  ~sgpGasmScannerForFitPrg();

  // -- properties
  void setEntity(sgpEntityForGasm *value);
  void setCode(scDataNode *value);

  // -- execute
  void init();

  // calculate total program size (using all blocks)   
  ulong64 calcProgramSize() const;

  // calculate program size with limitation on minimum size
  ulong64 calcProgramSizeEx(ulong64 minSize) const;
  
  // calculate program size with help of degradation ratio
  ulong64 calcProgramSizeDegrad(ulong64 minSize, double blockDegradRatio) const;
  
  // calculate program size with macro support
  ulong64 calcProgramSizeWithMacroSup(ulong64 minSize) const;
protected:
private:
  bool m_initDone;
  sgpEntityForGasm *m_entity;
  scDataNode *m_code;
  std::auto_ptr<scDataNode> m_extractedCode;
  sgpProgramCode m_program;
};


#endif // _SGPGSFORFITPRG_H__
