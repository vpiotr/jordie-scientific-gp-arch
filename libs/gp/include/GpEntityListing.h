/////////////////////////////////////////////////////////////////////////////
// Name:        GpEntityListing.h
// Project:     sgpLib
// Purpose:     Functions for listing entities for export to file / reports.
// Author:      Piotr Likus
// Modified by:
// Created:     08/06/2011
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPENTITYLIST_H__
#define _SGPENTITYLIST_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file FileName.h
\brief Short file description

Long description
*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
//sc
#include "sc/dtypes.h"
//sgp
#include "sgp/GasmVMachine.h"
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

// ----------------------------------------------------------------------------
// Public functions
// ----------------------------------------------------------------------------
void listProgramToLines(const scDataNode &prg, sgpFunctionMapColn &functions, scStringList &lines);
void listInfoBlockToLines(const scDataNode &infoBlock, const sgpInfoBlockVarMap *infoMap, 
  scStringList &lines);

#endif // _SGPENTITYLIST_H__
