/////////////////////////////////////////////////////////////////////////////
// Name:        GasmFunLibMacros.h
// Project:     sgp
// Purpose:     Functions for macro support
// Author:      Piotr Likus
// Modified by:
// Created:     11/12/2009
/////////////////////////////////////////////////////////////////////////////

#ifndef _GASMFUNLIBMAC_H__
#define _GASMFUNLIBMAC_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GasmFunLibMacros.h
///
/// Implemented functions:
/// -     macro 
 
// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
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
const uint FUNCID_MACROS_BASE = 200;
const uint FUNCID_MACROS_MACRO = FUNCID_MACROS_BASE + 1;
const uint FUNCID_MACROS_MACRO_REL = FUNCID_MACROS_BASE + 2;

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
class sgpFunLibMacros: public sgpFunLib {
public:
  sgpFunLibMacros();
  virtual ~sgpFunLibMacros();
protected:
  virtual void registerFuncs();
};

#endif // _GASMFUNLIBMAC_H__
