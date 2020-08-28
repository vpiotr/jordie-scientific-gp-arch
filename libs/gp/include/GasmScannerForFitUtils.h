/////////////////////////////////////////////////////////////////////////////
// Name:        GasmScannerForFitUtils.h
// Project:     sgpLib
// Purpose:     GASM scanner for fitness calculations. Utility (abstract) 
//              functions. 
// Author:      Piotr Likus
// Modified by:
// Created:     12/06/2011
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPGSFORFITUT_H__
#define _SGPGSFORFITUT_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file GasmScannerForFitUtils.h
\brief GASM scanner for fitness calculations

*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sc/dtypes.h"

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
class sgpGasmScannerForFitUtils {
public:
  // construct
  sgpGasmScannerForFitUtils() {}
  ~sgpGasmScannerForFitUtils() {}
  // execute

  // verify IO mode of argumen is correct   
  static bool verifyArgDir(const scDataNode &argMeta, uint argOffset, uint ioMode);
  
  // returns input and/or output mode specifier for a given arg number
  static uint getArgIoMode(const scDataNode &argMeta, uint argOffset);

  // calc code of type basing on its similarity
  static int calcTypeKind(scDataNodeValueType vtype);

  // calculate numeric difference between two data types expressed as uint 
  static uint calcTypeDiff(scDataNodeValueType typ1, scDataNodeValueType typ2);
};


#endif // _SGPGSFORFITUT_H__
