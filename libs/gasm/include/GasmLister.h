/////////////////////////////////////////////////////////////////////////////
// Name:        GasmLister.h
// Project:     sgp
// Purpose:     List Gasm program as assembler code.
// Author:      Piotr Likus
// Modified by:
// Created:     08/02/2009
/////////////////////////////////////////////////////////////////////////////


#ifndef _GASMLISTER_H__
#define _GASMLISTER_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GasmLister.h
///
/// Convert byte code to assembler listing.
///
/// ; --------------------------
/// ; Sample GASM program
/// ; --------------------------
/// .block  
/// .input float,int
/// .output int
/// .label start
///     add.int #1, 2, 13U
///     add.double #76, 2.0D, 131.1F
///     xcmd "ala", true
///     jump_back @start
/// .end

//TODO: add auto-label support - use with jumps

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sgp/GasmVMachine.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
enum sgpLstFeatures {
  slfAllLabels = 1,
  slfDetectLabels = 2
};
  
const uint slfDefault = slfDetectLabels;
  
// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
class sgpLister {
public:
  sgpLister();
  virtual ~sgpLister();  
  void listProgram(const scDataNode &code, scStringList &output);
  uint getFeatures();
  void setFeatures(uint mask);
  void setFunctionList(const sgpFunctionMapColn &functions);
protected:  
  void listBlock(uint blockNo, const scDataNode &inputArgs, const scDataNode &outputArgs, const scDataNode &blockCode, scStringList &output);
  void listInstructions(const scDataNode &blockCode, scStringList &output);
  bool extractInstr(const scDataNode &blockCode, cell_size_t cellPos, uint &instrCode, uint &argCount, scString &name, bool &bIsJump);
  scString descArgs(const scDataNode &args);
  scString descArg(const scDataNode &cell);
  void listUnknownData(const scDataNode &blockCode, cell_size_t pos, cell_size_t aSize, scStringList &output);
  scString descUnknownData(const scDataNode &blockCode, cell_size_t pos, cell_size_t aSize);
  scString descArgValues(const scDataNode &blockCode, cell_size_t pos, cell_size_t aSize, const scDataNode *jumpArgs = SC_NULL);
  bool checkLabel(cell_size_t addr, const scDataNode &labels, scString &labelName);
  void checkJump(const scDataNode &blockCode, cell_size_t instrAddr, uint instrCode, uint argCount, 
    scDataNode &labels, scDataNode *outJumpArgs = SC_NULL);
protected:
  uint m_features;  
  sgpFunctionMapColn m_functions;
};

#endif // _GASMLISTER_H__
