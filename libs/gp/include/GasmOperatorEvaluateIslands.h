/////////////////////////////////////////////////////////////////////////////
// Name:        GasmOperatorEvaluateIslands.h
// Project:     sgmLib
// Purpose:     Evaluate operator that processes island by island.
//              Abstract class. 
// Author:      Piotr Likus
// Modified by:
// Created:     05/06/2010
/////////////////////////////////////////////////////////////////////////////

#ifndef _GASMOPERATOREVALUATEISLANDS_H__
#define _GASMOPERATOREVALUATEISLANDS_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GasmOperatorEvaluateIslands.h
///
/// File description

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sgp/GaOperatorBasic.h"

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
class sgpGasmOperatorEvaluateIslands: public sgpGaOperatorEvaluateBasic {
public:
  // create
  sgpGasmOperatorEvaluateIslands(); 
  virtual ~sgpGasmOperatorEvaluateIslands();
  // properties
  void setIslandLimit(uint value);
  // run
  virtual bool execute(uint stepNo, bool isNewGen, sgpGaGeneration &generation);
protected:
  virtual bool processIsland(uint stepNo, uint islandNo, bool isNewGen, sgpGaGeneration &generation);
  virtual bool processBuffer(sgpGaGeneration &generation);    
protected:
  uint m_islandLimit;  
};

#endif // _COREMODULE_H__
