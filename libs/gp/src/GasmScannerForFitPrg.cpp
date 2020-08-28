/////////////////////////////////////////////////////////////////////////////
// Name:        GasmScannerForFitPrg.h
// Project:     sgpLib
// Purpose:     GASM scanner for fitness calculations. Whole program version.
// Author:      Piotr Likus
// Modified by:
// Created:     12/06/2011
/////////////////////////////////////////////////////////////////////////////

#include "sc/defs.h"
#include "sc/smath.h"

#include "sgp/GasmScannerForFitPrg.h"
#include "sgp/GasmOperator.h"

sgpGasmScannerForFitPrg::sgpGasmScannerForFitPrg():
  m_code(SC_NULL),
  m_entity(SC_NULL),
  m_initDone(false)
{
}

sgpGasmScannerForFitPrg::~sgpGasmScannerForFitPrg()
{
}

// -- properties
void sgpGasmScannerForFitPrg::setEntity(sgpEntityForGasm *value)
{
  m_entity = value;
}

void sgpGasmScannerForFitPrg::setCode(scDataNode *value)
{
  m_code = value;
}

// -- execute
void sgpGasmScannerForFitPrg::init()
{
  assert(m_entity != SC_NULL);
  if (m_code == SC_NULL)
  {
    m_extractedCode.reset(new scDataNode());
    m_entity->getProgramCode(*m_extractedCode);
    m_code = m_extractedCode.get();
  }

  m_program.setFullCode(*m_code);
  m_initDone = true;
}

// calculate total program size (using all blocks)   
ulong64 sgpGasmScannerForFitPrg::calcProgramSize() const
{
  assert(m_initDone);
  return m_program.getCodeLength();
}

// calculate program size with limitation on minimum size
ulong64 sgpGasmScannerForFitPrg::calcProgramSizeEx(ulong64 minSize) const
{
  ulong64 res = calcProgramSize();
  if (res < minSize)
    res = minSize;
  return res;  
}
  
// calculate program size with help of degradation ratio
ulong64 sgpGasmScannerForFitPrg::calcProgramSizeDegrad(ulong64 minSize, double blockDegradRatio) const
{
  assert(m_initDone);

  ulong64 res = 0;
  ulong64 blockSize;
  uint blockCount;

  blockCount = m_program.getBlockCount();
  for(uint i=0,epos = blockCount; i != epos; ++i)
  {
    blockSize = m_program.getBlockLength(i);
    blockSize = round<ulong64>(static_cast<double>(blockSize) * ::calcBlockDegradFactor(i, blockDegradRatio));
    res += blockSize;
  }
  
  if (res < minSize)
    res = minSize;
    
  return res;  
}
  
// calculate program size with macro support
ulong64 sgpGasmScannerForFitPrg::calcProgramSizeWithMacroSup(ulong64 minSize) const
{
  ulong64 res = 0;
  uint blockCount;

  blockCount = m_program.getBlockCount();
  if (blockCount > 0) {
    res += m_program.getBlockLength(SGP_GASM_MAIN_BLOCK_INDEX);
  }  
  
#ifdef OPT_USE_MACRO_CNT_IN_SIZE_OBJ  
// make score higher if macro count > 2
  res = round<ulong64>(static_cast<double>(res) * std::fabs(log10(double(10 + std::max<uint>(2, blockCount) - 2))));
#endif
  
  if (res < minSize)
    res = minSize;
    
  return res;  
}
