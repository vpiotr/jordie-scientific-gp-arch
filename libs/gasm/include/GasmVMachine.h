/////////////////////////////////////////////////////////////////////////////
// Name:        GasmVMachine.h
// Project:     scLib
// Purpose:     Virtual machine for Gasm language. 
//              Includes memory management and interpreter.
// Author:      Piotr Likus
// Modified by:
// Created:     29/01/2009
/////////////////////////////////////////////////////////////////////////////

#ifndef _GASMVMACHINE_H__
#define _GASMVMACHINE_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GasmVMachine.h
///
/// SGP - scheduled GP
///
/// Includes interpreter for Gasm language + parser and writer.
/// 
/// Langage features:
///   set #1, 2 // copy constant to build-in variable #1
///   add #1, #2, 123.2 // add float to variable #2 and copy result to var. #1
///   // c++ comments
///   - instructions are user-defined (external)
///   - available value types can be specified by:
///     - pre-defined variables (provided when calling interpreter)
///     - special type mask
///   - build-in types include:
///     - signed integers: char, int, long64
///     - unsigned int: byte, uint, ulong64
///     - floats: float, double, xdouble
///     - strings
///     - arrays
///     - dynamic structures
///   - arguments can be:
///     - constant - number, string
///     - build-in variable 
///     - reference to variable
///     - null reference
///   - instructions are encoded as 16-bit constants, where:
///     - lower 11 bits are instruction codes
///     - higher 5 bits is a argument count (=0 for default)
///   - there is registry of defined instructions (each instruction is a object)
///   - program is provided as array of blocks where each block is a array of "cells"
///   - each block is a kind of procedure with input, output and local variables  
///   - each "cell" contains an constant or reference
///   - reference handling is included
///   - recurrance calls are allowed (block state is pushed onto stack)   
///   - build-in variables:
///     - #0 - return value(s) - type depends on block
///     - #1..#33 - input params
///     - #34..#60 - restricted type accumulators (signed int, unsigned int, float)
///     - #61..#249 - variant variables (no definition required)
///     - #250..#255 - system variables:
///          #250 - +global variables (persistent array)
///          #251 - +program code
///          #252..#255 - unused
/// 
/// Interpreter - main features
/// - initialize for a specified program
/// - get / set whole program state and code
/// - configure starting point: block number, instruction number
/// - run n instructions of program 
/// - set function set
/// - set allowed data types
/// - set "unknown instruction" handling (default: perform noop) 
/// - set template for local variables (array of cells)
/// - get/set build-in variable x
/// - control references
/// - flag: break on error
/// - statistics: 
///   - program cost depending on instruction codes and argument types
///   - number of instructions already performed
///   - number of errors
///   - error log
///
/// Parser - main features
/// - convert text to program code with all blocks included and variables defined
///   - support instructions parsing with constants, build-in vars and references
///   - support comments
///   - support multiple blocks
///   - support constant structures
///   - support meta information - block argument definition as "const" instruction.
///   - support macro instructions like .dtypes - list of data types, executed during parsing
///
/// Writer - main features:
/// - convert program to text form 
/// - include:
///   - program code   
///  
/// Program structure
/// - program code - list of blocks
///   - block:
///     - list of cells
///     - #0 - meta info: #0 - array of input params, #1 - array of output params
///     - #1 - first instruction code
///     - each cell is an instruction or it's argument
///
/// Program state:
///   - call stack (last block, last instruction, block's build-in vars, local vars)
///   - instruction / cell number
///   - active block's build-in variable values
///   - active block's defined variables 
///
/// Instruction set:
/// - array of pairs: instruction code, function object
/// - function object includes: 
///   - min and max arg count
///   - default argument values - input + output
///   - "execute" method (mnemonic, arguments, vmachine)
///
/// Allowed data types:
/// - specified as: .dtypes 'byte,int,float'

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#define GVM_USE_COUNTERS

// stl
#include <map>
#include <set>
#include <iostream>

// TR1
//#include <unordered_set>

// boost
#include "boost/intrusive_ptr.hpp"
#include "boost/ptr_container/ptr_map.hpp"
#include "boost/unordered_set.hpp"

// other
#include "LRUCache.h"
// sc
#include "sc/dtypes.h"

#ifdef GVM_USE_COUNTERS
#include "perf/counter.h"
#endif

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------
typedef uint cell_size_t; ///<-- as big as cell memory

/// flags of supported virtual machine features
enum sgpGvmFeatures {
  //ggfVariantsEnabled = 1,   ///<-- variant is a variable without type specification
  ggfGlobalVarsEnabled = 2,
  ggfDynamicRegsEnabled = 4, ///<-- allow "on-demand" register allocation
  //ggfBreakOnError,
  ggfValidateArgs = 8,
  ggfCodeAccessRead = 16,
  ggfCodeAccessWrite = 32,
  ggfLogReadErrors = 64,
  ggfFixedBlockArgCount = 128,
  ggfNotesEnabled = 256, ///<-- world-scope values
  ggfTraceEnabled = 512, ///<-- world-scope values
  ggfLogErrors = 1024,
  ggfRecurrenceEnabled = 2048
};

/// flags of supported argument types
enum sgpGvmArgTypeFlag {
  gatfNull = 1,
  gatfConst = 2,
  gatfRef = 4,
  gatfRegister = 8,
  gatfAny = 0xffff
};

enum sgpGvmArgIoMode {
  gatfInput = 1,
  gatfOutput = 2,
  gatfAnyIo = 0xffff
};

/// flags of supported data types
enum sgpGvmDataTypeFlag {
  gdtfNull = 1,  
  gdtfFloat = 2,
  gdtfDouble = 4,
  gdtfXDouble = 8,
  gdtfInt = 16,
  gdtfInt64 = 32,
  gdtfByte = 64,
  gdtfUInt = 128,
  gdtfUInt64 = 256,
  gdtfString = 512,
  gdtfStruct = 1024,
  gdtfArray = 2048,
  gdtfRef = 4096,  
  gdtfBool = 8192,  
  gdtfVariant = 16384,  
  gdtfMax = gdtfVariant,
  gdtfAll = 0xffff
};

enum sgpProgramStateFlag {
  psfPushNextResult = 1
};
  
// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------
class sgpVMachine;

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------

const uint sgpGvmFeaturesDefault = ggfValidateArgs+ggfLogReadErrors+ggfGlobalVarsEnabled+ggfLogErrors; //ggfVariantsEnabled + 

const uint gdtfAllBaseInts = gdtfInt + gdtfInt64;
const uint gdtfAllBaseUInts = gdtfByte + gdtfUInt + gdtfUInt64;
const uint gdtfAllBaseXInts = gdtfAllBaseInts + gdtfAllBaseUInts;
const uint gdtfAllBaseFloats = gdtfFloat + gdtfDouble + gdtfXDouble;
const uint gdtfAllScalars = gdtfAllBaseXInts+gdtfAllBaseFloats+gdtfBool;
const uint gatfLValue = gatfRef+gatfRegister+gatfNull;

const uint gicNoop = 0;

const uint GVM_ERROR_WRONG_PARAM_COUNT = 1;
const uint GVM_ERROR_WRONG_PARAMS      = 2;
const uint GVM_ERROR_FUNCT_FAIL        = 3;
const uint GVM_ERROR_WRONG_LVALUE_TYPE = 4;
const uint GVM_ERROR_UNDEF_ARG_TYPE    = 5;
const uint GVM_ERROR_REG_LIMIT_REACHED = 6;
const uint GVM_ERROR_DYN_REGS_DISABLED = 7;
const uint GVM_ERROR_REG_WRITE_DISABLED= 8;
const uint GVM_ERROR_REG_READ_DISABLED = 9;
const uint GVM_ERROR_UNK_INSTRUCTION   = 10;
const uint GVM_ERROR_STD_EXCEPT        = 11;
const uint GVM_ERROR_SC_EXCEPT         = 12;
const uint GVM_ERROR_REG_WRITE_WRONG_TYPE = 13;
const uint GVM_ERROR_VAR_ALREADY_DEFINED = 14;
const uint GVM_ERROR_WRONG_CALL_INPUT_ARG_CNT = 15;
const uint GVM_ERROR_CALL_TO_WRONG_BLOCK_NO = 16;
const uint GVM_ERROR_STACK_OVERFLOW = 17;
const uint GVM_ERROR_ACCESS_PATH_TOO_LONG_RD = 18;
const uint GVM_ERROR_ACCESS_PATH_TOO_LONG_WR = 19;
const uint GVM_ERROR_ARITH_DIV_BY_ZERO = 20;
const uint GVM_ERROR_WRONG_JUMP_BACK = 21;
const uint GVM_ERROR_ARRAY_ALREADY_DEFINED = 22;
const uint GVM_ERROR_ARRAY_NOT_AVAILABLE = 23;
const uint GVM_ERROR_STRUCT_NOT_AVAILABLE = 24;
const uint GVM_ERROR_STRUCT_NOT_APPLICABLE = 25;
const uint GVM_ERROR_WRONG_SKIP = 26;
const uint GVM_ERROR_WRONG_ARRAY_TYPE = 27;
const uint GVM_ERROR_ARRAY_LIMIT_REACHED = 28;
const uint GVM_ERROR_STRUCT_LIMIT_REACHED = 29;
const uint GVM_ERROR_RECURRENCE_FORBID = 30;

const uint SGP_DEF_ERROR_LIMIT = 20;
const uint SGP_DEF_DYNAMIC_REGS_LIMIT = 512;
const uint SGP_DEF_MAX_STACK_DEPTH = 255;
const uint SGP_DEF_MAX_ACCESS_PATH_LEN = 32;
const uint SGP_MAX_INSTR_ARG_COUNT = 31;
const uint SGP_DEF_ITEM_LIMIT = 100000;
const uint SGP_DEF_INSTR_CACHE_SIZE = 1000;
const uint SGP_MAX_REG_NO = 1000;
const uint SGP_DEF_VALUE_STACK_LIMIT = 1000;
const uint SGP_DEF_BLOCK_SIZE_LIMIT = 4*500;

// default registry layout
const uint SGP_REGB_OUTPUT   = 0;
const uint SGP_REGB_OUTPUT_MAX = 0;
const uint SGP_REGB_INPUT    = 1;
const uint SGP_REGB_INPUT_MAX = 33;
const uint SGP_REGB_ACCUMS   = 34;
const uint SGP_REGB_INT      = SGP_REGB_ACCUMS;
const uint SGP_REGB_INT64    = SGP_REGB_ACCUMS + 3;
const uint SGP_REGB_BYTE     = SGP_REGB_ACCUMS + 6;
const uint SGP_REGB_UINT     = SGP_REGB_ACCUMS + 9;
const uint SGP_REGB_UINT64   = SGP_REGB_ACCUMS + 12;
const uint SGP_REGB_FLOAT    = SGP_REGB_ACCUMS + 15;
const uint SGP_REGB_DOUBLE   = SGP_REGB_ACCUMS + 18;
const uint SGP_REGB_XDOUBLE  = SGP_REGB_ACCUMS + 21;
const uint SGP_REGB_BOOL     = SGP_REGB_ACCUMS + 24;
const uint SGP_REGB_VARIANTS = SGP_REGB_ACCUMS + 27;
const uint SGP_REGB_VIRTUAL  = 250;
const uint SGP_REGB_VIRTUAL_MAX  = 255;

// register numbers
const uint SGP_REGNO_GLOBAL_VARS   = SGP_REGB_VIRTUAL + 0;
const uint SGP_REGNO_PROGRAM_CODE  = SGP_REGB_VIRTUAL + 1;
const uint SGP_REGNO_NOTES         = SGP_REGB_VIRTUAL + 2;

const uint SGP_REGNO_OUTPUT = SGP_REGB_OUTPUT + 0;

// operation costs
const uint SGP_OPER_COST_FUNC_FAILED     = 1;
const uint SGP_OPER_COST_GET_ACT_INSTR   = 2;
const uint SGP_OPER_COST_SET_LVALUE      = 3;
const uint SGP_OPER_COST_EVALUATE_ARG    = 4;
const uint SGP_OPER_COST_GET_REF_VALUE   = 5;
const uint SGP_OPER_COST_SET_REF_VALUE   = 6;
const uint SGP_OPER_COST_EXCEPTION       = 7;
const uint SGP_OPER_COST_ERROR           = 8;
const uint SGP_OPER_COST_UNK_INSTR       = 9;
const uint SGP_OPER_COST_EVALUATE_CONST  = 10;
const uint SGP_OPER_COST_EVALUATE_STRUCT_ARG = 11;
const uint SGP_OPER_COST_EVALUATE_VARIANT = 12;
const uint SGP_OPER_COST_DYNAMIC_TYPE_ARGS = 13; // argument type is dynamic - less optimal solution
const uint SGP_OPER_COST_ARG_CNT_ERROR = 14;

const uint GASM_ARG_META_IO_MODE  = 0;
const uint GASM_ARG_META_ARG_TYPE  = 1;
const uint GASM_ARG_META_DATA_TYPE = 2;

// ----------------------------------------------------------------------------
// sgpProgramState
// ----------------------------------------------------------------------------    
typedef std::map<uint, scDataNode> sgpRegReferences;
typedef boost::unordered_set<uint> sgpRegUSet;
 
/// Full program state (like processor + memory snapshot)
struct sgpProgramState {
  uint activeBlockNo;
  int nextBlockNo;
  cell_size_t activeCellNo;
  cell_size_t nextCellNo;
  uint flags;
  sgpRegUSet modifiedRegs;
  scDataNode activeRegs;  ///<-- build-in, vmachine-generated registers
  scDataNode definedRegs; ///<-- registers defined dynamically
  sgpRegReferences references;
  scDataNode callStack;
  scDataNode valueStack;
  scDataNode globalVars;
  scDataNode notes;
  scDataNode reserve;
  
  void reset();
  void resetWarmWay();
  void clearRegisters();
  void restoreRegisters(const scDataNode &src);
  void markRegAsModified(uint regNo);
};

typedef std::set<uint> sgpGasmRegSet;

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
class sgpProgramCode;

// ----------------------------------------------------------------------------
// sgpFunction
// ----------------------------------------------------------------------------    
/// Single supported function object
class sgpFunction: public scReferenceCounter {
public:
  // -- construction
  sgpFunction();
  virtual ~sgpFunction();
  // -- properties
  virtual void getArgCount(uint &a_min, uint &a_max) const;
  // array of 3x uint: io_mode, arg_type_mask, data_type_mask 
  virtual bool getArgMeta(scDataNode &output) const;
  virtual const scDataNode *getArgMeta() const;
  virtual scString getName() const = 0;
  virtual scString getExportName() const;
  virtual uint getId() const;  
  void setMachine(sgpVMachine *machine);
  // -- execution
  virtual bool execute(const scDataNode &args) const = 0;
  virtual bool execute(const scDataNode &args, uint &execCost) const;
  // returns <true> if function changes current execution point
  virtual bool isJumpAction();
  // returns <true> if function can be stripped on code prepare
  virtual bool isStrippable();
  // returns array of: (index of jump argument + 1, negative sign if jump backward)
  virtual void getJumpArgs(scDataNode &args);
  // optional step used to speed-up "execute" performance
  virtual void prepare();
  // returns <true> if arguments cannot be auto-generated basing on meta info
  virtual bool hasDynamicArgs() const; 
protected:
  virtual uint getFunctionCost() const;
  void addArgMeta(uint ioMode, uint argTypeMask, uint dataTypeMask, scDataNode &output) const;
protected:
  sgpVMachine *m_machine;  
  std::auto_ptr<scDataNode> m_argMetaGuard;
};

// ----------------------------------------------------------------------------
// sgpFunctionForExpand
// ----------------------------------------------------------------------------    
// function that needs to be expanded
class sgpFunctionForExpand: public sgpFunction {
  typedef sgpFunction inherited;
public:
  static scInterfaceId getInterfaceId() { return 0xB7904F77A68FC1C4;}
  sgpFunctionForExpand();
  virtual ~sgpFunctionForExpand();
  virtual scInterface *queryInterface(scInterfaceId intId) {      
     if (intId == getInterfaceId())
       return this;
     else
       return DTP_NULL;
  }
  virtual bool expand(const scDataNode &args, sgpProgramCode &programCode, uint blockNo, cell_size_t instrOffset, bool validateOrder) =0;
};

typedef boost::intrusive_ptr<sgpFunction> sgpFunctionTransporter;
typedef std::map<uint,sgpFunctionTransporter> sgpFunctionMapColn;
typedef std::set<ulong64> sgpValidationCacheSet;

void addFunctionToList(uint instrCode, sgpFunction *funct, sgpFunctionMapColn &list);

typedef sgpFunctionMapColn::iterator sgpFunctionMapColnIterator;

// ----------------------------------------------------------------------------
// scProgramCode
// ----------------------------------------------------------------------------    
// Object containing full program code (no state)
class sgpProgramCode {
public:
  // copy constructor, assignment, default constructor   
  sgpProgramCode();
  sgpProgramCode(const sgpProgramCode &rhs);
  virtual ~sgpProgramCode();
  virtual sgpProgramCode &operator=(const sgpProgramCode &rhs);
  // properties
  cell_size_t getMaxBlockLength();
  uint getBlockCount() const;
  // public methods
  void clear();
  scDataNode &getFullCode();
  void setFullCode(const scDataNode &code);
  cell_size_t getBlockLength(uint blockNo) const; 
  /// returns size of code for all blocks in total
  cell_size_t getCodeLength() const;
  bool blockExists(uint blockNo);
  scDataNode &getBlock(uint blockNo);
  void getBlock(uint blockNo, scDataNode &output);
  bool getBlockCodePart(uint blockNo, cell_size_t beginPos, cell_size_t count, scDataNode &output) const;
  void getBlockCode(uint blockNo, scDataNode &output) const;
  void setBlockCode(uint blockNo, const scDataNode &code);
  void getBlockMetaInfo(uint blockNo, scDataNode &inputArgs, scDataNode &outputArgs) const;
  void getBlockMetaInfoForInput(uint blockNo, scDataNode &inputArgs) const;
  void getBlockMetaInfoForOutput(uint blockNo, scDataNode &outputArgs) const;
  void getBlockMetaInfo(uint blockNo, scDataNode &output) const;
  void setBlockMetaInfo(uint blockNo, const scDataNode &inputArgs, const scDataNode &outputArgs);
  void setBlockMetaInfo(uint blockNo, const scDataNode &metaInfo);
  void setDefBlockMetaInfo(uint blockNo);
  void addBlock();
  void addBlock(scDataNode *block, scDataNode *metaInfo = SC_NULL);
  void addBlock(const scDataNode &inputArgs, const scDataNode &outputArgs, const scDataNode &code);
  void eraseLastBlock();
  void eraseBlock(uint blockIndex);
  void setBlock(uint blockNo, const scDataNode &inputArgs, const scDataNode &outputArgs, const scDataNode &code);
  void clearBlock(uint blockNo);
  void getBlockCell(uint blockNo, uint cellOffset, scDataNode &output);
  scDataNode *cloneBlockCell(uint blockNo, uint cellOffset);
protected:
  virtual void codeChanged();  
protected:  
  scDataNode m_code;
  cell_size_t m_maxBlockLength;
};
  
typedef std::auto_ptr<scDataNode> sgpArgsGuard;
     
class sgpInstructionInfo {
public:
  sgpInstructionInfo(uint instrCode, uint argCount, scDataNode* args, sgpFunction *functor): 
    m_instrCode(instrCode), m_argCount(argCount), m_args(args), m_functor(functor) {};
  virtual ~sgpInstructionInfo() {
  };
  void getActiveInstr(uint &instrCode, uint &argCount, scDataNode* &args, sgpFunction* &functor) {
    instrCode = m_instrCode;
    argCount = m_argCount;
    args = m_args.get();
    functor = m_functor;
  }
  size_t size() { return 1; }
protected:
  uint m_instrCode;
  uint m_argCount;
  sgpArgsGuard m_args;
  sgpFunction *m_functor;
};

typedef boost::shared_ptr<sgpInstructionInfo> sgpInstructionInfoTransporter;
typedef std::vector<uint> sgpDataTypeVector;

typedef LRUCache<ulong64,sgpInstructionInfo> sgpInstrCache;
       
sgpFunction *getFunctorForInstrCode(const sgpFunctionMapColn &functions, 
  uint instrCodeRaw);
       
// ----------------------------------------------------------------------------
// sgpVMachine
// ----------------------------------------------------------------------------    
class sgpVMachine {
public:  
  // public methods
  sgpVMachine();
  virtual ~sgpVMachine();

  // execution
  void run(uint instrLimit = 0);  
  virtual void runInstr(uint code, const scDataNode &args);
  void resetProgram();
  void resetWarmWay();  
  bool isLastRunOk();
  bool isFinished();   
  ulong64 getTotalCost();
  scDataNode getOutput();
  void setInput(const scDataNode &value);
  void setOutput(const scDataNode &value);
  void forceInstrCache();
  void getCounters(scDataNode &output);
  void resetCounters();

  // helper functions
  void prepareCode(sgpProgramCode &code) const;
  void expandCode(sgpProgramCode &code, uint firstBlockNo = 0, bool validateOrder = true) const;
  void getArgValue(const scDataNode &argInfo, scDataNode &output);
  void setArgValue(const scDataNode &argInfo, const scDataNode &value);

  static uint getArgType(const scDataNode &argInfo);
  static uint getArgDataType(const scDataNode &argInfo);
  static void initDataNodeAs(sgpGvmDataTypeFlag a_type, scDataNode &output);
  static uint getArgMetaParamUInt(const scDataNode &meta, uint argNo, uint argMetaId);
  static void buildFunctionArgMeta(scDataNode &output, uint ioMode, uint argTypes, uint dataTypes);
  static bool isRegisterNo(uint value);
  static uint getRegisterNo(const scDataNode &lvalue);
  static uint getRegisterNo(const scDataNodeValue &lvalue);
  static scDataNodeValueType castGasmToDataNodeType(sgpGvmDataTypeFlag a_type);

  void prepareRegisterSet(sgpGasmRegSet &regSet, uint allowedDataTypes, uint ioMode) const;

  bool castValue(const scDataNode &value, sgpGvmDataTypeFlag targetType, scDataNode &output);
  cell_size_t getInstrCount();
  scString formatAddress(uint blockNo, cell_size_t cellAddr);

  bool isRegisterTypeAllowed(uint regNo) const;
  bool isRegisterTypeIo(uint regNo) const;
  bool isRegisterTypeVirtual(uint regNo) const;
  
  // properties
  void setProgramState(const sgpProgramState &state);
  void getProgramState(sgpProgramState &state) const;
  void setProgramCode(const sgpProgramCode &code);
  void getProgramCode(sgpProgramCode &code) const;
  void setFunctionList(const sgpFunctionMapColn &functions);
  uint getFeatures();
  void setFeatures(uint features);
  void getErrorLog(scStringList &output);
  void getTraceLog(scStringList &output);
  void setDynamicRegsLimit(uint limit);
  void setErrorLimit(uint limit);
  uint getSupportedArgTypes();
  void setSupportedArgTypes(uint types);
  void setSupportedDataTypes(uint types);
  void setDefaultDataType(sgpGvmDataTypeFlag a_type);
  uint getMaxStackDepth();
  void setMaxStackDepth(uint value);
  uint getMaxAccessPathLength();
  void setMaxAccessPathLength(uint value);
  uint getExtraRegDataTypes();
  void setExtraRegDataTypes(uint value);  
  bool getNotes(scDataNode &output);
  bool setNotes(const scDataNode &value);
  void setMaxItemCount(uint a_value);
  uint getMaxItemCount();
  void setValueStackLimit(uint value);
  void setBlockSizeLimit(uint value);
  void setStartBlockNo(uint blockNo);
  //--- evolver support
  virtual bool canReadRegister(uint regNo) const;
  virtual bool canWriteRegister(uint regNo) const;
  //--- function handling
  virtual void handleInstrError(const scString &name, const scDataNode &args, uint errorCode, const scString &errorMsg = "");
  static uint encodeInstr(uint code, uint argCount);
  static void decodeInstr(uint instrCode, uint &rawCode, uint &argCount);
  static bool isEncodedInstrCode(uint instrCode);
  static void buildRegisterArg(scDataNode &ref, uint regNo);
  static scDataNode buildRegisterArg(uint regNo);
  bool checkArgDataType(const scDataNode &value, uint allowedTypes);
  bool checkArgType(const scDataNode &value, uint allowedTypes);
  virtual void setLValue(const scDataNode &lvalue, const scDataNode &value, bool forceCast = false);
  virtual void clearRegisterValue(uint regNo);

  virtual bool getRegisterValue(uint regNo, scDataNode &value, bool ignoreRef = false);

  bool getRegisterValueInternal(uint regNo, scDataNode &regValue);
  bool getReferencedValueInternal(const scDataNode &ref, scDataNode &value);
  
  virtual bool getRegisterDataType(uint &dataType, uint regNo, bool ignoreRef = false) const;
  virtual uint getRegisterDefaultDataType(uint regNo) const;  
  virtual scDataNode *getRegisterPtr(uint regNo);
  virtual void setRegisterValue(uint regNo, const scDataNode &value, bool ignoreRef = false, bool forceCast = false);
  virtual bool getReferencedValue(const scDataNode &ref, scDataNode &value);
  virtual void setReferencedValue(const scDataNode &ref, const scDataNode &value, bool forceCast = false);
  bool argValidationEnabled();
  void evaluateArg(const scDataNode &input, scDataNode &output);
  bool isRefInRegister(uint regNo);
  void addRef(uint outputRegNo, uint inputRegNo, int elementNo = -1, const scString &elementName = "");
  void defineVar(uint outputRegNo, sgpGvmDataTypeFlag a_dataType);
  bool defineArray(uint outputRegNo, sgpGvmDataTypeFlag a_dataType, cell_size_t a_size);
  void defineArrayFromCode(uint outputRegNo, sgpGvmDataTypeFlag a_dataType, cell_size_t a_size);
  void clearRefs(uint regNo);
  void clearRefsBelow(uint regNo, const scDataNode &path);
  bool callBlock(uint blockNo, const scDataNode &args);
  void exitBlock();
  bool setNextBlockNo(uint blockNo);
  bool setNextBlockNoRel(uint blockNoOffset);
  void clearNextBlockNo();
  uint calcDataType(const scDataNode &value) const;
  static sgpGvmDataTypeFlag castDataNodeToGasmType(scDataNodeValueType a_type);  
  void skipCells(cell_size_t a_value);
  void jumpBack(cell_size_t a_value);
  bool verifyArgs(const scDataNode &args, sgpFunction *functor);
  //------------------------------------------------------------------------
  ulong64 arrayGetSize(uint regNo);
  void arrayAddItem(uint regNo, const scDataNode &value);
  void arrayEraseItem(uint regNo, ulong64 idx);
  void arraySetItem(uint regNo, ulong64 idx, const scDataNode &value);
  void arrayGetItem(uint regNo, ulong64 idx, scDataNode &output);
  long64 arrayIndexOf(uint regNo, const scDataNode &value);
  void arrayMerge(uint regNo1, uint regNo2, scDataNode &output);        
  void arrayRange(uint regNo, ulong64 aStart, ulong64 aSize, scDataNode &output);        
  //------------------------------------------------------------------------
  ulong64 structGetSize(uint regNo);
  void structAddItem(uint regNo, const scDataNode &value);
  void structAddItem(uint regNo, const scString &aName, const scDataNode &value);
  void structEraseItem(uint regNo, const scString &idx);
  void structEraseItem(uint regNo, ulong64 idx);
  void structSetItem(uint regNo, const scString &idx, const scDataNode &value);
  void structSetItem(uint regNo, ulong64 idx, const scDataNode &value);
  void structGetItem(uint regNo, const scString &idx, scDataNode &output);
  void structGetItem(uint regNo, ulong64 idx, scDataNode &output);
  long64 structIndexOf(uint regNo, const scDataNode &value);
  void structMerge(uint regNo1, uint regNo2, scDataNode &output);        
  void structRange(uint regNo, ulong64 aStart, ulong64 aSize, scDataNode &output);        
  void structGetItemName(uint regNo, ulong64 aIndex, scString &output);        
  //------------------------------------------------------------------------  
  void vectorFindMinValue(uint regNo, scDataNode &output);        
  void vectorFindMaxValue(uint regNo, scDataNode &output);        
  void vectorSum(uint regNo, scDataNode &output);        
  void vectorAvg(uint regNo, scDataNode &output);        
  void vectorSort(uint regNo);        
  void vectorDistinct(uint regNo);        
  void vectorNorm(uint regNo);
  void vectorDotProduct(uint regNo1, uint regNo2, scDataNode &output);        
  void vectorStdDev(uint regNo1, scDataNode &output);        
  //------------------------------------------------------------------------  
  void setPushNextResult();
  bool pushValueToValueStack(const scDataNode &value);
  bool popValueFromValueStack(scDataNode &output);
  //------------------------------------------------------------------------  
  void setRandomInit(bool value);
  bool getRandomInit();
  //------------------------------------------------------------------------  
  uint blockAdd();    
  bool blockInit(uint blockNo);    
  bool blockErase(uint blockNo);    
  cell_size_t blockGetSize(uint blockNo);    
  uint blockGetCount();    
  uint blockActiveId();          
  virtual bool expandMacro(uint macroNo, const scDataNode &args, sgpProgramCode &programCode, uint blockNo, cell_size_t instrOffset, bool validateOrder);
  //------------------------------------------------------------------------  
protected:
  virtual void handleUnknownInstr(uint instrCode, const scDataNode &args);
  virtual void handleError(uint msgCode, const scString &msgText, const scDataNode *context = SC_NULL);
  virtual void handleInstrError(uint instrCode, const scDataNode &args, uint errorCode, const scString &errorMsg);
  virtual void handleInstrException(const std::exception& error, 
    uint code, const scDataNode &args, sgpFunction *functor);
  virtual void handleInstrException(const scError &error, 
    uint code, const scDataNode &args, sgpFunction *functor);
  virtual void handleInstrException(const scString &errorType, 
    uint code, const scDataNode &args, sgpFunction *functor);
  void handleWrongParamCount(uint instrCode);
  void handleWrongArgumentError(uint instrCode, const scDataNode &args, uint argNo);
  void logInstrError(uint instrCode, const scDataNode &args, uint errorCode, const scString &errorMsg);
  void handleInstrErrorCost(uint instrCode, uint errorCode);
  void handleReadDisabled(uint regNo);
  void logError(uint msgCode, const scString &msgText, const scDataNode *context = SC_NULL);
  void handleErrorCost(uint errorCode);
  void handleRegWriteDisabled(uint regNo);
  void handleRegWriteTypeError(uint regNo, const scDataNode &value);
  void handleRecurrenceForbidden(uint blockNo);
  void handleWrongBlockNo(uint blockNo);

  bool isReferenceEnabled();

//  virtual void addInstrError(uint instrCode, const scDataNode &args, uint errorCode, const scString &errorMsg);
  
  void clearInput();
  void clearOutput();
  virtual void initRegisters();
  virtual void initRegisterTemplate();
  void addRegisterTplBlock(sgpGvmDataTypeFlag a_type, uint a_size);
  scDataNode buildRegisterValue(sgpGvmDataTypeFlag a_type);  
  void releaseRef(uint refRegNo, const scDataNode &ref);

  bool isRegisterTypeMatched(uint regNo, const scDataNode &value, bool ignoreSizeDiff, bool &typeRestricted) const;  
  bool isRegisterTypeMatched(uint regNo, uint valueTypeSet, bool ignoreSizeDiff, bool &typeRestricted) const;
  bool isRegisterTypeMatched(uint regNo, const scDataNode *value, uint valueTypeSet, bool ignoreSizeDiff, 
    bool &typeRestricted) const;
  bool checkArgMeta(const scDataNode &args, const scDataNode &meta, uint ioMode, uint &argNo, bool &staticTypes);
    
  virtual void beforeRun();
  virtual void afterRun();
  void clearErrors();
  void clearTrace();
  void prepareRun();
  void intRun();
  bool getActiveInstrForLog(uint &instrCode, uint &argCount, scString &name);
  bool getActiveInstr(uint &instrCode, uint &argCount, scDataNode &args, sgpFunction *&functor);
  bool getActiveInstrFromCache(uint &instrCode, uint &argCount, scDataNode *&args, sgpFunction *&functor);
  void intRunInstr(uint code, const scDataNode &args, sgpFunction *functor);
  void addOperCost(ulong64 a_cost);
  void addOperCostById(uint a_subOperCostId, uint mult = 1);
  scString getCurrentAddressCtx();
  scString getCurrentCtx();
  scString formatErrorMsg(uint msgCode, const scString &msgText);
  bool checkArgsSupported(const scDataNode &args, uint &argNo);
  bool checkArgCount(const scDataNode &args, sgpFunction *functor);  
  bool checkArgCount(uint argCount, sgpFunction *functor);
  void lockReadRegError();
  void unlockReadRegError();
  bool isReadRegErrorLocked() const;  
  void pushBlockState(const scDataNode &extraValue);
  void popBlockState(scDataNode &extraValue);
  void pushValue(const scDataNode &value);
  scDataNode popValue();
  void prepareBlockRun(uint blockNo);
  void initBlockState(uint blockNo);
  bool isVariantRegisterFast(uint regNo) const;  
  bool isStaticTypeRegister(uint regNo) const;  
  void addToTraceLogInstr(uint instrCode, const scDataNode &args, const sgpFunction *functor);
  void addToTraceLogChange(uint regNo, const scDataNode &value);
  void addToTraceLogChangeByRef(uint regNo, const scDataNode &value);
  bool getArgsValidFromCache(ulong64 keyNum);
  void setArgsValidInCache(ulong64 keyNum);  
  void clearArgValidCache();
  void clearInstrCache();
  ulong64 calcActiveInstrCacheKey();
  void addInstrToCache(ulong64 keyNum, uint instrCode, uint argCount, scDataNode *args, sgpFunction *functor);
  void stripUnusedCodeAfterLastOutWrite(sgpProgramCode &code) const;
  void macroReplaceInputRegsWithArgs(const scDataNode &macroInputMeta, const scDataNode &args, scDataNode &code);
  bool macroReplaceArgRegInCode(uint oldRegNo, const scDataNode &newValue, uint searchIoMode, 
    cell_size_t instrOffset, scDataNode &blockCode);
  void checkNextBlock();
protected:
// vmachine configuration
  uint m_supportedArgTypes;
  uint m_supportedDataTypes;
  uint m_features;
  cell_size_t m_instrLimit;
  uint m_errorLimit;
  uint m_dynamicRegsLimit;
  sgpGvmDataTypeFlag m_defDataType;
  uint m_maxStackDepth;
  uint m_maxAccessPathLength;
  uint m_extraRegDataTypes;
  bool m_randomInit;
  uint m_maxItemCount; /// maximum number of items in arrays
  uint m_valueStackLimit;
  uint m_blockSizeLimit;
  scStringList m_errorLog;
  scStringList m_traceLog;  
  sgpFunctionMapColn m_functions;
// vmachine state
  sgpProgramCode m_programCode;
  sgpProgramState m_programState;
  scDataNode m_registerTemplate;
  uint m_registerTemplateSize;
  sgpDataTypeVector m_registerDataTypes;
  cell_size_t m_instrCount;
  uint m_errorCount;
  ulong64 m_lastOperCost;
  ulong64 m_totalCost;
  mutable uint m_readRegErrorLock;
  scDataNode m_notes;
  sgpValidationCacheSet m_validArgCache;
  sgpInstrCache m_instrCache;
  bool m_instrCacheRequired;
#ifdef GVM_USE_COUNTERS  
  perf::LocalCounter m_counters;
#endif
};

#endif // _GASMVMACHINE_H__
