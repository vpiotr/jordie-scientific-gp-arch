/////////////////////////////////////////////////////////////////////////////
// Name:        GgpOperator.h
// Project:     scLib
// Purpose:     Operators for GGP (gramatic GP)
// Author:      Piotr Likus
// Modified by:
// Created:     13/10/2009
/////////////////////////////////////////////////////////////////////////////

#ifndef _GGPOPERATOR_H__
#define _GGPOPERATOR_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GgpOperator.h
///
/// File description

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sc\dtypes.h"
#include "sgp\GasmOperator.h"
#include "sgp\GasmOperatorInit.h"

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
// sgpGgpProcessor
// ----------------------------------------------------------------------------
class sgpGgpProcessor {
public:
  // construct
  sgpGgpProcessor();
  virtual ~sgpGgpProcessor();
  // properties
  void setSeparator(const scDataNode &valueList);
  void getSeparator(scDataNode &valueList);
  // run
  void buildGgpInfoPart(scDataNode &list);  
protected:
  scDataNode m_separator;  
};

// ----------------------------------------------------------------------------
// sgpGaOperatorInitGgp
// ----------------------------------------------------------------------------
class sgpGaOperatorInitGgp: public sgpGasmOperatorInit {
public:
  // construction
  sgpGaOperatorInitGgp();
  virtual ~sgpGaOperatorInitGgp();
  // properties
  void setGgpProcessor(sgpGgpProcessor *value);
protected:  
  virtual void addInfoVarPart(scDataNode &infoBlock);
protected:
  sgpGgpProcessor *m_ggpProcessor;  
};


#endif // _GGPOPERATOR_H__
