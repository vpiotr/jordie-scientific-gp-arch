/////////////////////////////////////////////////////////////////////////////
// Name:        GasmOperatorSelectTourDynObj.h
// Project:     scLib
// Purpose:     Select operator - tournament with dynamic objective set.
// Author:      Piotr Likus
// Modified by:
// Created:     29/12/2009
/////////////////////////////////////////////////////////////////////////////


#ifndef _GASMOPERATORSELECTTOURDYNOBJ_H__
#define _GASMOPERATORSELECTTOURDYNOBJ_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file core.h
///
/// File description

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sc/dtypes.h"
#include "sgp/GaOperatorBasic.h"
#include "sgp/GaOperatorSelectTourProb.h"

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
// sgpGasmOperatorSelectTournamentDynObj
// ----------------------------------------------------------------------------
class sgpGasmOperatorSelectTournamentDynObj: public sgpGaOperatorSelectTourProb {
public:
  // construct
  sgpGasmOperatorSelectTournamentDynObj();
  virtual ~sgpGasmOperatorSelectTournamentDynObj();
  // properties
  virtual void setObjectiveWeights(const sgpWeightVector &value);
  void setMinErrorObjWeight(double value);
  void setMaxErrorObjWeight(double value);
protected:  
  virtual double getObjectiveWeight(const sgpGaGeneration &input, uint first, uint second,
    uint objIndex, double defValue);
protected:
  double m_minErrorObjWeight;
  double m_maxErrorObjWeight;    
};


#endif // _GASMOPERATORSELECTTOURDYNOBJ_H__
