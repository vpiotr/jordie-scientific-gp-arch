/////////////////////////////////////////////////////////////////////////////
// Name:        GasmScannerForFitUtils.cpp
// Project:     sgpLib
// Purpose:     GASM scanner for fitness calculations. Utility (abstract) 
//              functions. 
// Author:      Piotr Likus
// Modified by:
// Created:     12/06/2011
/////////////////////////////////////////////////////////////////////////////

#include "sgp/GasmScannerForFitUtils.h"
#include "sgp/GasmVMachine.h"

using namespace dtp;

bool sgpGasmScannerForFitUtils::verifyArgDir(const scDataNode &argMeta, uint argOffset, uint ioMode)
{
  if (argOffset >= argMeta.size())
    return false;
  uint argMetaDir = 
      sgpVMachine::getArgMetaParamUInt(argMeta, argOffset, GASM_ARG_META_IO_MODE); 
  if ((argMetaDir & ioMode) != 0)
    return true;
  else
    return false;     
}
  
// returns input and/or output mode specifier for a given arg number
uint sgpGasmScannerForFitUtils::getArgIoMode(const scDataNode &argMeta, uint argOffset)
{
  if (argOffset >= argMeta.size())
    return 0;
  uint res = 
      sgpVMachine::getArgMetaParamUInt(argMeta, argOffset, GASM_ARG_META_IO_MODE); 
  return res;        
}

// calc code of type basing on its similarity
int sgpGasmScannerForFitUtils::calcTypeKind(scDataNodeValueType vtype)
{
  int res; 
  switch (vtype) {
    case vt_null:
      res = 1;
      break;
    case vt_bool:
      res = 2;
      break;
    case vt_byte:
      res = 3;
      break;
    case vt_int:
      res = 4;
      break;
    case vt_uint:
      res = 5;
      break;
    case vt_int64:
      res = 6;
      break;
    case vt_uint64:
      res = 7;
      break;
    case vt_float:
      res = 8;
      break;
    case vt_double:
      res = 9;
      break;
    case vt_xdouble:
      res = 10;
      break;
    case vt_string: 
      res = 15;
      break;
    default: {
      res = 20;
      break; 
    }  
  } // switch  
  return res;  
}

// calculate numeric difference between two data types expressed as uint 
uint sgpGasmScannerForFitUtils::calcTypeDiff(scDataNodeValueType typ1, scDataNodeValueType typ2)
{
  int diff = 
    std::abs(calcTypeKind(typ1) - calcTypeKind(typ2));
  return uint(diff);
}

