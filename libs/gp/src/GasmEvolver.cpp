/////////////////////////////////////////////////////////////////////////////
// Name:        GasmEvolver.cpp
// Project:     sgpLib
// Purpose:     Evolver for genetic programs
// Author:      Piotr Likus
// Modified by:
// Created:     23/03/2009
/////////////////////////////////////////////////////////////////////////////

#include "perf/log.h"

#include "sc/defs.h"
#include "sc/utils.h"

#include "sgp/GasmEvolver.h"

#ifdef DEBUG_ON
#define DEBUG_ASM_CODE_STRUCT
//#define OPT_LOG_INFO_ACCESS_ERROR  
#endif

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

using namespace dtp;

const ulong64 SGP_GASM_EV_MAGIC_NO1 = 0xa50505a5UL;
const ulong64 SGP_GASM_EV_MAGIC_NO2 = 0xe70303e7UL;

// ----------------------------------------------------------------------------
// sgpGaOperatorAllocGasm
// ----------------------------------------------------------------------------
sgpGaOperatorAllocGasm::sgpGaOperatorAllocGasm(): sgpGaOperatorAlloc()
{
}

sgpGaOperatorAllocGasm::~sgpGaOperatorAllocGasm()
{
}

void sgpGaOperatorAllocGasm::setInfoMap(sgpInfoBlockVarMap *map)
{
  m_infoMap = map;
}

sgpGaGeneration *sgpGaOperatorAllocGasm::execute()
{
  std::auto_ptr<sgpGasmGeneration> genGuard(new sgpGasmGeneration());
  genGuard->setInfoMap(m_infoMap);
  return genGuard.release();
}

