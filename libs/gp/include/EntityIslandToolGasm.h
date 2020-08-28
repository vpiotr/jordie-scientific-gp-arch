/////////////////////////////////////////////////////////////////////////////
// Name:        EntityIslandToolGasm.h
// Project:     sgpLib
// Purpose:     Tool for island support - GASM-specific.
// Author:      Piotr Likus
// Modified by:
// Created:     23/07/2011
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPENTISLTLGP_H__
#define _SGPENTISLTLGP_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file EntityIslandToolGasm.h
\brief Short file description

Long description
*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sgp\EntityIslandTool.h"

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
class sgpEntityIslandToolGasm: public sgpEntityIslandToolIntf {
public:  
  sgpEntityIslandToolGasm():m_islandLimit(0) {}
  virtual ~sgpEntityIslandToolGasm() {} 
  void setIslandLimit(uint value);
  virtual void prepareIslandMap(const sgpGaGeneration &newGeneration, scDataNode &output);
  virtual bool getIslandId(const sgpEntityBase &entity, uint &output);
  virtual bool setIslandId(sgpEntityBase &entity, uint value);
private:
  uint m_islandLimit;
};


#endif // _SGPENTISLTLGP_H__
