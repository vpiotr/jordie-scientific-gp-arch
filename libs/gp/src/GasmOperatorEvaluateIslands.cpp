/////////////////////////////////////////////////////////////////////////////
// Name:        GasmOperatorEvaluateIslands.cpp
// Project:     sgmLib
// Purpose:     Evaluate operator that processes island by island.
//              Can be used as base for split-by-islands processing.
// Author:      Piotr Likus
// Modified by:
// Created:     05/06/2010
/////////////////////////////////////////////////////////////////////////////

#include "sgp/GasmOperatorEvaluateIslands.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

sgpGasmOperatorEvaluateIslands::sgpGasmOperatorEvaluateIslands(): sgpGaOperatorEvaluateBasic() {
}
 
sgpGasmOperatorEvaluateIslands::~sgpGasmOperatorEvaluateIslands()
{
}

// properties
void sgpGasmOperatorEvaluateIslands::setIslandLimit(uint value)
{
  m_islandLimit = value;
}

// run
bool sgpGasmOperatorEvaluateIslands::execute(uint stepNo, bool isNewGen, sgpGaGeneration &generation)
{
  bool res = true;
  for(uint i = 0, epos = m_islandLimit; i != epos; i++)  
  {
    if (!processIsland(stepNo, i, isNewGen, generation))
      res = false;
  }    
  return res;    
}

bool sgpGasmOperatorEvaluateIslands::processIsland(uint stepNo, uint islandNo, bool isNewGen, sgpGaGeneration &generation)
{
  return processBuffer(generation);
}

bool sgpGasmOperatorEvaluateIslands::processBuffer(sgpGaGeneration &generation)
{
#ifdef DEBUG_OPER_EVAL
  scLog::addDebug("gasm-oper-island-eval-a0");  
#endif  

  bool res = false;
  if (m_fitnessFunc == SC_NULL) 
    return res;

  m_fitnessFunc->initProcess(generation);    

#ifdef DEBUG_OPER_EVAL
  scLog::addDebug("gasm-oper-island-eval-a1");  
#endif  
    
  //res = m_fitnessFunc->evaluateAll(&generation);
  res = evaluateAll(&generation);

#ifdef DEBUG_OPER_EVAL
  scLog::addDebug("gasm-oper-island-eval-a2");  
#endif  
    
#ifdef DEBUG_OPER_EVAL
  scLog::addDebug("gasm-oper-island-eval-a2");  
#endif  

  if (!m_fitnessFunc->postProcess(generation))
    res = false;
    
  return res;  
}

