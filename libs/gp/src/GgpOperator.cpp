/////////////////////////////////////////////////////////////////////////////
// Name:        GgpOperator.cpp
// Project:     scLib
// Purpose:     Operators for GGP (gramatic GP)
// Author:      Piotr Likus
// Modified by:
// Created:     13/10/2009
/////////////////////////////////////////////////////////////////////////////
#include "sgp\GgpOperator.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

// ----------------------------------------------------------------------------
// sgpGaOperatorInitGgp
// ----------------------------------------------------------------------------
sgpGaOperatorInitGgp::sgpGaOperatorInitGgp(): sgpGasmOperatorInit()
{
}

sgpGaOperatorInitGgp::~sgpGaOperatorInitGgp()
{
}

void sgpGaOperatorInitGgp::setGgpProcessor(sgpGgpProcessor *value)
{
  m_ggpProcessor = value;
}

void sgpGaOperatorInitGgp::addInfoVarPart(scDataNode &infoBlock)
{
  assert(m_ggpProcessor != SC_NULL);
  scDataNode varPart;
  infoBlock.transferChildrenFrom(varPart);
}

