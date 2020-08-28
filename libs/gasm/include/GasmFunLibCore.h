/////////////////////////////////////////////////////////////////////////////
// Name:        GasmFunLibCore.h
// Project:     sgp
// Purpose:     Core functions for use with Gasm vmachine
// Author:      Piotr Likus
// Modified by:
// Created:     05/02/2009
/////////////////////////////////////////////////////////////////////////////

#ifndef _GASMFUNLIBCORE_H__
#define _GASMFUNLIBCORE_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GasmFunLibCore.h
///
/// Implemented functions:
/// - noop
/// - move #out, value
/// - init #out
/// - call, return
/// - set_next_block, set_next_block_rel, clr_next_block
/// - push_next_result
/// - push, pop
/// - ref.build #out, #in[, idx]
/// - ref.clear #out
/// - define #reg, a_type:uint
/// - data #reg, type, size - build data array from code segment
/// - cast
/// - data_type
/// - logic.* : iif, and, or, xor, not
/// - comp.* : compare funcs: equ, gt, gte, ls, lse, cmp, same (all scalar types + string)
///----------
/// - add.* (int,int64,byte,uint,uint64,float,double,xdouble)
/// - sub.*
/// - mult.*
/// - div.*
/// - multdiv.* #out,a1,a2,divider (int,byte,uint,float,double)
/// - pow.uint
///----------
/// - mod.* (int,int64,byte,uint,uint64)
/// - bit.and, bit.or, bit.xor, bit.not (byte, uint, uint64)
/// - shr, shl (byte, uint, uint64)
/// - abs, sign (int, int64, float, double, xdouble) 
/// - ceil.*, floor, frac (float, double, xdouble)
//  - trunc, trunc64, truncf (float, double, xdouble)
//  - round, round64, roundf (float, double, xdouble)
///----------
/// - skipifn, skip, jump_back
///----------
/// - array.define #out, type, size
/// - array.* (size, add_item, erase_item, set_item, get_item, index_of), later: range, merge
/// - struct.* (size, get_item, set_item, add_item, erase_item, index_of) 
/// - string.* (size, merge, get_char, set_char, range, find, replace)
/// - vector.* (min, max, avg, sum, sort, distinct, norm)
 
// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sgp/GasmFunLib.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
const uint FUNCID_CORE_BASE = 0;
const uint FUNCID_CORE_CALL = FUNCID_CORE_BASE + 1;

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
class sgpFunLibCore: public sgpFunLib {
public:
  sgpFunLibCore();
  virtual ~sgpFunLibCore();
protected:
  virtual void registerFuncs();
};

#endif // _GASMFUNLIBCORE_H__
