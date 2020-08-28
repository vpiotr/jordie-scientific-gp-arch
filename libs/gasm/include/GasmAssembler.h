/////////////////////////////////////////////////////////////////////////////
// Name:        GasmAssembler.h
// Project:     sgpLib
// Purpose:     Class for converting text to bytecode.
// Author:      Piotr Likus
// Modified by:
// Created:     09/02/2009
/////////////////////////////////////////////////////////////////////////////


#ifndef _GASMASSEMBLER_H__
#define _GASMASSEMBLER_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GasmAssembler.h
///
/// Parse text and convert it to byte code program.
/// Process is performed in two passes for correct label detection.
/// Note: keep synchronized with sgpLister.
///
/// ; --------------------------
/// ; Sample GASM program
/// ; --------------------------
/// .block  
/// .input float,int
/// .output int
/// :start
///     add.int #1, 2, 13U
///     add.double #76, 2.0D, 131.1F
///     xcmd "ala", true
///     jump_back @start
/// .end
///
/// Supported macros:
/// .block - start of new block
/// .end - end of block
/// .input - define list of input arguments
/// .output - define list of output arguments
/// .data - define block of data
///
/// - comments are started on the beggining of line with ';'
/// - registers are reffered as "#13" - starting with hash (#) followed by integer number
/// - labels are refered as "@name" - starting with "@" followed by alphanumeric name
/// 
/// Supported argument types:
///   int,int64,byte,uint,uint64,float,double,xdouble,bool,string,variant,struct,array
///  
/// Supported constant formats:
/// 2      - integer number, 
/// 2.0    - floating point number, does not have to contain decimal point 
///         (2D is correct double value = 2.0)
/// "text" - string
/// true   - boolean
/// false
/// null   - null
/// 
/// Allowed suffixes that if following value, will pricise it's data type:
/// I - int (optional), L - int64, B - byte, U - uint, UL - uint64
/// F - float, D - double (optional), X - xdouble
///
/// Labels:
/// - if backward - calculated as diff between next address and label address
/// - if forward - calculated in the same way, converted to integer value
///

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

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
class sgpAssembler {
public:
  sgpAssembler();
  virtual ~sgpAssembler();
  bool parseText(const scStringList &lines, scDataNode &outputCode);
  void setFunctionList(const sgpFunctionMapColn &functions);
protected:
  bool isSpecial(int c);  
  bool isStartOfName(int c);
  void throwSyntaxError(const scString &text, int a_pos, int a_char) const;
  bool parseLine(const scString &aLine, cell_size_t currAddr, scDataNode &labels, scString &cmd, scDataNode &paramList);
  void processBlockArgs(const scDataNode &params, scDataNode &blockParams);
  void processBlockCode(const scDataNode &lexBlock, const scDataNode &blockLabels, scDataNode &blockCode);
  int getCommandCode(const scString &cmdName);
  void decodeArgs(const scDataNode &lexArgs, const scDataNode &blockLabels, cell_size_t currAddr, scDataNode &output);
  bool checkDataTypeName(const scString &name, sgpGvmDataTypeFlag &output);
protected:    
  sgpFunctionMapColn m_functions;
  scDataNode m_typeNames;
  scDataNode m_functionNameMap;
};

#endif // _GASMASSEMBLER_H__
