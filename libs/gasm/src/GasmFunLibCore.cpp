/////////////////////////////////////////////////////////////////////////////
// Name:        GasmFunLibCore.cpp
// Project:     sgp
// Purpose:     Core functions for use with Gasm vmachine
// Author:      Piotr Likus
// Modified by:
// Created:     05/02/2009
/////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <limits>

#include "base/rand.h"
#include "base/bmath.h"

#include "sc/smath.h"
#include "sc/utils.h"

#include "sgp/GasmFunLibCore.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

// define to allow data transmission between blocks using data stack
#define ALLOW_INTERBLOCK_DATA_STACK

using namespace dtp;

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

// noop [args]: do nothing, 0..n arguments
class sgpFuncNoop: public sgpFunction {
public:
  virtual scString getName() const { return "noop"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 0; a_max = SGP_MAX_INSTR_ARG_COUNT; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    return true;
  }
};

// move #out, value : copy value to destination
class sgpFuncMove: public sgpFunction {
public:
  virtual scString getName() const { return "move"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1;

    m_machine->evaluateArg(args[1], arg1);
    m_machine->setLValue(args[0], arg1);
    return true;
  }
};

// init #out : initialize register using zero/empty value 
class sgpFuncInit: public sgpFunction {
public:
  virtual scString getName() const { return "init"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode value;
    sgpGvmDataTypeFlag aType;

    aType = static_cast<sgpGvmDataTypeFlag>(m_machine->getRegisterDefaultDataType(m_machine->getRegisterNo(args[0])));
    m_machine->initDataNodeAs(aType, value);
    m_machine->setLValue(args[0], value);
    return true;
  }
};

// call #out, block_no[, input_args] :call block
class sgpFuncCall: public sgpFunction {
public:
  virtual scString getName() const { return "call"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 2; a_max = SGP_MAX_INSTR_ARG_COUNT; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfInput, gatfConst, gdtfAllBaseXInts, output);
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    return true;
  }
  
  virtual bool hasDynamicArgs() const
  {
    return true;
  }

  virtual uint getId() const
  {
    return FUNCID_CORE_CALL;
  }

  virtual bool execute(const scDataNode &args) const {
    scDataNode arg0;
    scDataNode inputArgs = args;
    scDataNode callArgs, argx, argx2; 
    
    inputArgs.eraseElement(0);
    
    callArgs.setAsArray(vt_datanode);
    
    m_machine->evaluateArg(args[0], arg0);
    for(int i=0, epos = inputArgs.size(); i!=epos; i++) {
      inputArgs.getElement(i, argx);
      if (i == 0) {
        callArgs.addItem(argx);
      } else {
        m_machine->evaluateArg(argx, argx2);
        callArgs.addItem(argx2);
      }  
    }    

    bool res = m_machine->callBlock(arg0.getAsUInt(), callArgs);
    return res;
  }
};

// return :return from block
class sgpFuncReturn: public sgpFunction {
public:
  virtual scString getName() const { return "return"; };

  virtual bool isStrippable() {
    return false;
  }  
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 0; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const 
  {
    m_machine->exitBlock();
    return true;
  }
};

// set_next_block block_no :set block which should be performed as next
class sgpFuncSetNextBlock: public sgpFunction {
public:
  virtual scString getName() const { return "set_next_block"; };
  
  virtual bool isStrippable() {
    return false;
  }  
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 1; a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfInput, gatfConst, gdtfAllBaseXInts, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg0;
    m_machine->evaluateArg(args[0], arg0);
    bool res = m_machine->setNextBlockNo(arg0.getAsUInt());
    return res;
  }
};

// set_next_block_rel block_no :set relative block no which should be performed as next
class sgpFuncSetNextBlockRel: public sgpFunction {
public:
  virtual scString getName() const { return "set_next_block_rel"; };
  
  virtual bool isStrippable() {
    return false;
  }  
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 1; a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfInput, gatfConst, gdtfAllBaseXInts, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg0;
    m_machine->evaluateArg(args[0], arg0);
    bool res = m_machine->setNextBlockNoRel(arg0.getAsUInt());
    return res;
  }
};

// clr_next_block :clear "next block" information
class sgpFuncClrNextBlock: public sgpFunction {
public:
  virtual scString getName() const { return "clr_next_block"; };
  
  virtual bool isStrippable() {
    return false;
  }  
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 0; a_max = 0; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    m_machine->clearNextBlockNo();
    return true;
  }
};

// value stack
//--------------------------------
// push_next_result : force next instruction with output to store it on stack
class sgpFuncPushNextResult: public sgpFunction {
public:
  virtual scString getName() const { return "push_next_result"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 0; a_max = 0; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    m_machine->setPushNextResult();
    return true;
  }
};

// push value : push value to value stack
class sgpFuncPush: public sgpFunction {
public:
  virtual scString getName() const { return "push"; };

#ifdef ALLOW_INTERBLOCK_DATA_STACK
  virtual bool isStrippable() {
    return false;
  }  
#endif
    
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfInput, gatfAny, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg0;

    m_machine->evaluateArg(args[0], arg0);
    return m_machine->pushValueToValueStack(arg0);
  }
};

// pop #out : pop value from value stack
class sgpFuncPop: public sgpFunction {
public:
  virtual scString getName() const { return "pop"; };
  
#ifdef ALLOW_INTERBLOCK_DATA_STACK
  virtual bool isStrippable() {
    return false; // leave pop even the instruction reads something unused
  }  
#endif

  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode value;
    if (m_machine->popValueFromValueStack(value))
    {
      m_machine->setLValue(args[0], value);
      return true;
    } else {
      return false;
    }    
  }
};


// ref.build #out, #src_reg [, item_index: uint]
class sgpFuncRefBuild: public sgpFunction {
public:
  virtual scString getName() const { return "ref.build"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 2; a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfRegister, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts+gdtfVariant+gdtfString, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg2;
    uint outRegNo, inRegNo;
    
    outRegNo = m_machine->getRegisterNo(args[0]);
    inRegNo = m_machine->getRegisterNo(args[1]);
    
    if (args.size() > 2) {
      m_machine->evaluateArg(args[2], arg2);

      if (arg2.getValueType() == vt_string)
        m_machine->addRef(outRegNo, inRegNo, -1, arg2.getAsString());
      else    
        m_machine->addRef(outRegNo, inRegNo, arg2.getAsInt());
    } else {   
      m_machine->addRef(outRegNo, inRegNo);
    }
    return true;
  }
};

// ref.clear #out : clear reference
class sgpFuncRefClear: public sgpFunction {
public:
  virtual scString getName() const { return "ref.clear"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 1; a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    m_machine->clearRegisterValue(m_machine->getRegisterNo(args[0]));
    return true;
  }
};

// ref.is_ref #out, #regno : returns <true> if reg value is a ref
class sgpFuncRefIsRef: public sgpFunction {
public:
  virtual scString getName() const { return "ref.is_ref"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 2; a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfRegister, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1;

    bool outValue = m_machine->isRefInRegister(m_machine->getRegisterNo(args[1]));
    m_machine->setLValue(args[0], scDataNode(outValue));
    return true;
  }
};

// define #out, data_type: define new variable stored in new register "out"
class sgpFuncDefine: public sgpFunction {
public:
  virtual scString getName() const { return "define"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 2; a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAllBaseXInts+gdtfVariant, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts+gdtfVariant, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    sgpGvmDataTypeFlag atyp;    
    scDataNode arg1;
    uint outRegNo;
    
    m_machine->evaluateArg(args[1], arg1);

    atyp = sgpGvmDataTypeFlag(arg1.getAsUInt());    

    outRegNo = m_machine->getRegisterNo(args[0]);

    m_machine->defineVar(outRegNo, atyp);    
    return true;
  }
};

// data #reg, type, size - build data array from code segment
class sgpFuncData: public sgpFunction {
public:
  virtual scString getName() const { return "data"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 3; a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts+gdtfVariant, output);
    addArgMeta(gatfInput, gatfConst, gdtfAllBaseXInts+gdtfVariant, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    sgpGvmDataTypeFlag atyp;    
    scDataNode arg1, arg2;
    uint outRegNo;
    cell_size_t size;
    
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);

    atyp = sgpGvmDataTypeFlag(arg1.getAsUInt());    
    size = arg2.getAsUInt64();

    outRegNo = m_machine->getRegisterNo(args[0]);

    m_machine->defineArrayFromCode(outRegNo, atyp, size);    
    return true;
  }
};

// cast #out, value, data_type: convert value to a different type
class sgpFuncCast: public sgpFunction {
public:
  virtual scString getName() const { return "cast"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 3; a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts+gdtfVariant, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    sgpGvmDataTypeFlag atyp;    
    scDataNode arg1, arg2;
    scDataNode newValue;
    bool res;
    
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);

    atyp = sgpGvmDataTypeFlag(arg2.getAsUInt());    
    
    res = m_machine->castValue(arg1, atyp, newValue);

    m_machine->setLValue(args[0], newValue);
    return res;
  }
};

// rtti.data_type #out, value: returns data type of a given value
class sgpFuncDataType: public sgpFunction {
public:
  virtual scString getName() const { return "rtti.data_type"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 2; a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    sgpGvmDataTypeFlag atyp;        
    atyp = sgpGvmDataTypeFlag(m_machine->calcDataType(args[1]));

    m_machine->setLValue(args[0], scDataNode(uint(atyp)));
    return true;
  }
};


// ----------------------------------------------------------------------------
// functions - logic
// ----------------------------------------------------------------------------

// iif #out, flag, true_value, false_value : move to output true- or false-value depending on flag
class sgpFuncIif: public sgpFunction {
public:
  virtual scString getName() const { return "iif"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 4; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfBool+gdtfVariant, output);
    addArgMeta(gatfInput, gatfAny, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2, arg3;
    
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);
    m_machine->evaluateArg(args[3], arg3);

    if (arg1.getAsBool())
      m_machine->setLValue(args[0], arg2);
    else  
      m_machine->setLValue(args[0], arg3);
      
    return true;
  }
};

// and #out, arg1, arg2: logical AND
class sgpFuncAnd: public sgpFunction {
public:
  virtual scString getName() const { return "and"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfBool+gdtfNull, output);
    addArgMeta(gatfInput, gatfAny, gdtfBool+gdtfVariant, output);
    addArgMeta(gatfInput, gatfAny, gdtfBool+gdtfVariant, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;
    
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);

    bool outValue = (arg1.getAsBool() && arg2.getAsBool());
    
    m_machine->setLValue(args[0], scDataNode(outValue));
      
    return true;
  }
};

// or #out, arg1, arg2: logical OR
class sgpFuncOr: public sgpFunction {
public:
  virtual scString getName() const { return "or"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfBool+gdtfNull, output);
    addArgMeta(gatfInput, gatfAny, gdtfBool+gdtfVariant, output);
    addArgMeta(gatfInput, gatfAny, gdtfBool+gdtfVariant, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;
    
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);

    bool outValue = (arg1.getAsBool() || arg2.getAsBool());
    
    m_machine->setLValue(args[0], scDataNode(outValue));
      
    return true;
  }
};

// xor #out, arg1, arg2: logical XOR
class sgpFuncXor: public sgpFunction {
public:
  virtual scString getName() const { return "xor"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfBool+gdtfNull, output);
    addArgMeta(gatfInput, gatfAny, gdtfBool+gdtfVariant, output);
    addArgMeta(gatfInput, gatfAny, gdtfBool+gdtfVariant, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;
    
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);

    bool p = arg1.getAsBool();
    bool q = arg2.getAsBool();
    
    bool outValue = (p || q) && !(p && q);
    
    m_machine->setLValue(args[0], scDataNode(outValue));
      
    return true;
  }
};

// not #out, arg1: logical NOT
class sgpFuncNot: public sgpFunction {
public:
  virtual scString getName() const { return "not"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfBool+gdtfNull, output);
    addArgMeta(gatfInput, gatfAny, gdtfBool+gdtfVariant, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;
    
    m_machine->evaluateArg(args[1], arg1);

    bool outValue = !arg1.getAsBool();
    
    m_machine->setLValue(args[0], scDataNode(outValue));
      
    return true;
  }
};


// ----------------------------------------------------------------------------
// functions - compare
// ----------------------------------------------------------------------------

// generic compare function
class sgpFuncComp: public sgpFunction {
public:
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2, outValue;
    
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);
    
    calcValue(arg1, arg2, outValue);
    m_machine->setLValue(args[0], outValue);
      
    return true;
  }
  
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const = 0;
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    bool outValueBool = calcValue(arg1, arg2);
    output = scDataNode(outValueBool);
  }
};

// equ #out, arg1, arg2: verify arg1 == arg2
//   for types: int, int64, byte, uint, uint64, float, double, xdouble, string, bool, variant
class sgpFuncEqu: public sgpFuncComp {
};

class sgpFuncCompInt: public sgpFuncEqu {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfBool, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfInt+gdtfByte, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfInt+gdtfByte, output);
    return true;
  }
};

class sgpFuncCompInt64: public sgpFuncEqu {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfBool, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfInt+gdtfUInt+gdtfInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfInt+gdtfUInt+gdtfInt64, output);
    return true;
  }
};

class sgpFuncCompByte: public sgpFuncEqu {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfBool, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte, output);
    return true;
  }
};

class sgpFuncCompUInt: public sgpFuncEqu {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfBool, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfUInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfUInt, output);
    return true;
  }
};

class sgpFuncCompUInt64: public sgpFuncEqu {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfBool, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfInt+gdtfUInt+gdtfUInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfInt+gdtfUInt+gdtfUInt64, output);
    return true;
  }
};

class sgpFuncCompBool: public sgpFuncEqu {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfBool, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfBool, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfBool, output);
    return true;
  }
};

class sgpFuncCompFloat: public sgpFuncEqu {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfBool, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat, output);
    return true;
  }
};

class sgpFuncCompDouble: public sgpFuncEqu {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfBool, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble, output);
    return true;
  }
};

class sgpFuncCompXDouble: public sgpFuncEqu {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfBool, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble+gdtfXDouble, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble+gdtfXDouble, output);
    return true;
  }
};

class sgpFuncCompString: public sgpFuncEqu {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfBool, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfString+gdtfAllScalars, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfString+gdtfAllScalars, output);
    return true;
  }
};

//-----
class sgpFuncEquInt: public sgpFuncCompInt {
public:
  virtual scString getName() const { return "equ.int"; };  
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const
  {
    return (arg1.getAsInt() == arg2.getAsInt());
  }    
};

class sgpFuncEquInt64: public sgpFuncCompInt64 {
public:
  virtual scString getName() const { return "equ.int64"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const
  {
    return (arg1.getAsInt64() == arg2.getAsInt64());
  }    
};

class sgpFuncEquByte: public sgpFuncCompByte {
public:
  virtual scString getName() const { return "equ.byte"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const
  {
    return (arg1.getAsByte() == arg2.getAsByte());
  }    
};

class sgpFuncEquUInt: public sgpFuncCompUInt {
public:
  virtual scString getName() const { return "equ.uint"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const
  {
    return (arg1.getAsUInt() == arg2.getAsUInt());
  }    
};

class sgpFuncEquUInt64: public sgpFuncCompUInt64 {
public:
  virtual scString getName() const { return "equ.uint64"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const
  {
    return (arg1.getAsUInt64() == arg2.getAsUInt64());
  }    
};

class sgpFuncEquBool: public sgpFuncCompBool {
public:
  virtual scString getName() const { return "equ.bool"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const
  {
    return (arg1.getAsBool() == arg2.getAsBool());
  }    
};

class sgpFuncEquFloat: public sgpFuncCompFloat {
public:
  virtual scString getName() const { return "equ.float"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const
  {
    //return (arg1.getAsFloat() == arg2.getAsFloat());
    return (std::fabs(arg1.getAsFloat() - arg2.getAsFloat()) < std::numeric_limits<float>::epsilon());        
  }    
};

class sgpFuncEquDouble: public sgpFuncCompDouble {
public:
  virtual scString getName() const { return "equ.double"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const
  {
    //return (arg1.getAsDouble() == arg2.getAsDouble());
    return (std::fabs(arg1.getAsDouble() - arg2.getAsDouble()) < std::numeric_limits<double>::epsilon());        
  }    
};

class sgpFuncEquXDouble: public sgpFuncCompXDouble {
public:
  virtual scString getName() const { return "equ.xdouble"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const
  {
    //return (arg1.getAsXDouble() == arg2.getAsXDouble());
    return (std::fabs(arg1.getAsXDouble() - arg2.getAsXDouble()) < std::numeric_limits<long double>::epsilon());        
  }    
};

class sgpFuncEquString: public sgpFuncCompString {
public:
  virtual scString getName() const { return "equ.string"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const
  {
    return (arg1.getAsString() == arg2.getAsString());
  }    
};

class sgpFuncEquVariant: public sgpFuncEquString {
public:
  virtual scString getName() const { return "equ.variant"; };
};  

//----------------------------------------------------------------------------
class sgpFuncGtInt: public sgpFuncCompInt {
public:
  virtual scString getName() const { return "gt.int"; };  
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const
  {
    return (arg1.getAsInt() > arg2.getAsInt());
  }    
};

class sgpFuncGtInt64: public sgpFuncCompInt64 {
public:
  virtual scString getName() const { return "gt.int64"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const
  {
    return (arg1.getAsInt64() > arg2.getAsInt64());
  }    
};

class sgpFuncGtByte: public sgpFuncCompByte {
public:
  virtual scString getName() const { return "gt.byte"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const
  {
    return (arg1.getAsByte() > arg2.getAsByte());
  }    
};

class sgpFuncGtUInt: public sgpFuncCompUInt {
public:
  virtual scString getName() const { return "gt.uint"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsUInt() > arg2.getAsUInt());
  }    
};

class sgpFuncGtUInt64: public sgpFuncCompUInt64 {
public:
  virtual scString getName() const { return "gt.uint64"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsUInt64() > arg2.getAsUInt64());
  }    
};

class sgpFuncGtBool: public sgpFuncCompBool {
public:
  virtual scString getName() const { return "gt.bool"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsBool() > arg2.getAsBool());
  }    
};

class sgpFuncGtFloat: public sgpFuncCompFloat {
public:
  virtual scString getName() const { return "gt.float"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsFloat() > arg2.getAsFloat());
  }    
};

class sgpFuncGtDouble: public sgpFuncCompDouble {
public:
  virtual scString getName() const { return "gt.double"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsDouble() > arg2.getAsDouble());
  }    
};

class sgpFuncGtXDouble: public sgpFuncCompXDouble {
public:
  virtual scString getName() const { return "gt.xdouble"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsXDouble() > arg2.getAsXDouble());
  }    
};

class sgpFuncGtString: public sgpFuncCompString {
public:
  virtual scString getName() const { return "gt.string"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsString() > arg2.getAsString());
  }    
};

class sgpFuncGtVariant: public sgpFuncGtString {
public:
  virtual scString getName() const { return "gt.variant"; };
};  

//----------------------------------------------------------------------------
class sgpFuncGteInt: public sgpFuncCompInt {
public:
  virtual scString getName() const { return "gte.int"; };  
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsInt() >= arg2.getAsInt());
  }    
};

class sgpFuncGteInt64: public sgpFuncCompInt64 {
public:
  virtual scString getName() const { return "gte.int64"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsInt64() >= arg2.getAsInt64());
  }    
};

class sgpFuncGteByte: public sgpFuncCompByte {
public:
  virtual scString getName() const { return "gte.byte"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsByte() >= arg2.getAsByte());
  }    
};

class sgpFuncGteUInt: public sgpFuncCompUInt {
public:
  virtual scString getName() const { return "gte.uint"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsUInt() >= arg2.getAsUInt());
  }    
};

class sgpFuncGteUInt64: public sgpFuncCompUInt64 {
public:
  virtual scString getName() const { return "gte.uint64"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsUInt64() >= arg2.getAsUInt64());
  }    
};

class sgpFuncGteBool: public sgpFuncCompBool {
public:
  virtual scString getName() const { return "gte.bool"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsBool() >= arg2.getAsBool());
  }    
};

class sgpFuncGteFloat: public sgpFuncCompFloat {
public:
  virtual scString getName() const { return "gte.float"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsFloat() >= arg2.getAsFloat());
  }    
};

class sgpFuncGteDouble: public sgpFuncCompDouble {
public:
  virtual scString getName() const { return "gte.double"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsDouble() >= arg2.getAsDouble());
  }    
};

class sgpFuncGteXDouble: public sgpFuncCompXDouble {
public:
  virtual scString getName() const { return "gte.xdouble"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsXDouble() >= arg2.getAsXDouble());
  }    
};

class sgpFuncGteString: public sgpFuncCompString {
public:
  virtual scString getName() const { return "gte.string"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsString() >= arg2.getAsString());
  }    
};

class sgpFuncGteVariant: public sgpFuncGteString {
public:
  virtual scString getName() const { return "gte.variant"; };
};  

//----------------------------------------------------------------------------
class sgpFuncLtInt: public sgpFuncCompInt {
public:
  virtual scString getName() const { return "lt.int"; };  
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsInt() < arg2.getAsInt());
  }    
};

class sgpFuncLtInt64: public sgpFuncCompInt64 {
public:
  virtual scString getName() const { return "lt.int64"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsInt64() < arg2.getAsInt64());
  }    
};

class sgpFuncLtByte: public sgpFuncCompByte {
public:
  virtual scString getName() const { return "lt.byte"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsByte() < arg2.getAsByte());
  }    
};

class sgpFuncLtUInt: public sgpFuncCompUInt {
public:
  virtual scString getName() const { return "lt.uint"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsUInt() < arg2.getAsUInt());
  }    
};

class sgpFuncLtUInt64: public sgpFuncCompUInt64 {
public:
  virtual scString getName() const { return "lt.uint64"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsUInt64() < arg2.getAsUInt64());
  }    
};

class sgpFuncLtBool: public sgpFuncCompBool {
public:
  virtual scString getName() const { return "lt.bool"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsBool() < arg2.getAsBool());
  }    
};

class sgpFuncLtFloat: public sgpFuncCompFloat {
public:
  virtual scString getName() const { return "lt.float"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsFloat() < arg2.getAsFloat());
  }    
};

class sgpFuncLtDouble: public sgpFuncCompDouble {
public:
  virtual scString getName() const { return "lt.double"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsDouble() < arg2.getAsDouble());
  }    
};

class sgpFuncLtXDouble: public sgpFuncCompXDouble {
public:
  virtual scString getName() const { return "lt.xdouble"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsXDouble() < arg2.getAsXDouble());
  }    
};

class sgpFuncLtString: public sgpFuncCompString {
public:
  virtual scString getName() const { return "lt.string"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsString() < arg2.getAsString());
  }    
};

class sgpFuncLtVariant: public sgpFuncLtString {
public:
  virtual scString getName() const { return "lt.variant"; };
};  

//----------------------------------------------------------------------------
class sgpFuncLteInt: public sgpFuncCompInt {
public:
  virtual scString getName() const { return "lse.int"; };  
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsInt() <= arg2.getAsInt());
  }    
};

class sgpFuncLteInt64: public sgpFuncCompInt64 {
public:
  virtual scString getName() const { return "lse.int64"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsInt64() <= arg2.getAsInt64());
  }    
};

class sgpFuncLteByte: public sgpFuncCompByte {
public:
  virtual scString getName() const { return "lse.byte"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsByte() <= arg2.getAsByte());
  }    
};

class sgpFuncLteUInt: public sgpFuncCompUInt {
public:
  virtual scString getName() const { return "lse.uint"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsUInt() <= arg2.getAsUInt());
  }    
};

class sgpFuncLteUInt64: public sgpFuncCompUInt64 {
public:
  virtual scString getName() const { return "lse.uint64"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsUInt64() <= arg2.getAsUInt64());
  }    
};

class sgpFuncLteBool: public sgpFuncCompBool {
public:
  virtual scString getName() const { return "lse.bool"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsBool() <= arg2.getAsBool());
  }    
};

class sgpFuncLteFloat: public sgpFuncCompFloat {
public:
  virtual scString getName() const { return "lse.float"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsFloat() <= arg2.getAsFloat());
  }    
};

class sgpFuncLteDouble: public sgpFuncCompDouble {
public:
  virtual scString getName() const { return "lse.double"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsDouble() <= arg2.getAsDouble());
  }    
};

class sgpFuncLteXDouble: public sgpFuncCompXDouble {
public:
  virtual scString getName() const { return "lse.xdouble"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsXDouble() <= arg2.getAsXDouble());
  }    
};

class sgpFuncLteString: public sgpFuncCompString {
public:
  virtual scString getName() const { return "lse.string"; };
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return (arg1.getAsString() <= arg2.getAsString());
  }    
};

class sgpFuncLteVariant: public sgpFuncLteString {
public:
  virtual scString getName() const { return "lse.variant"; };
};  

// same #out, value1, value2: same value and same type
class sgpFuncSame: public sgpFuncEquString {
public:
  virtual scString getName() const { return "same"; };

  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const 
  {
    return 
     (
      (arg1.getAsString() == arg2.getAsString())
      &&
      (arg1.getValueType() == arg2.getValueType())
     );
  }    
};  

// ----------------------------------------------------------------------------
// ---- comp.cmp #out, arg1, arg2
// ----------------------------------------------------------------------------
class sgpFuncCmpInt: public sgpFuncCompInt {
public:
  virtual scString getName() const { return "cmp.int"; };  
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfInt+gdtfByte, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfInt+gdtfByte, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    int val1 = arg1.getAsInt();
    int val2 = arg2.getAsInt();
    int res;
    if (val1 < val2) res = -1;
    else if (val1 > val2) res = 1;
    else res = 0;
    output.copyValueFrom(scDataNode(res));
  }    
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const {return false;};
};

class sgpFuncCmpInt64: public sgpFuncCompInt64 {
public:
  virtual scString getName() const { return "cmp.int64"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt+gdtfInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfInt+gdtfByte+gdtfInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfInt+gdtfByte+gdtfInt64, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    long64 val1 = arg1.getAsInt64();
    long64 val2 = arg2.getAsInt64();
    int res;
    if (val1 < val2) res = -1;
    else if (val1 > val2) res = 1;
    else res = 0;
    output.copyValueFrom(scDataNode(res));
  }    
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const {return false;};
};

class sgpFuncCmpByte: public sgpFuncCompByte {
public:
  virtual scString getName() const { return "cmp.byte"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    byte val1 = arg1.getAsByte();
    byte val2 = arg2.getAsByte();
    int res;
    if (val1 < val2) res = -1;
    else if (val1 > val2) res = 1;
    else res = 0;
    output.copyValueFrom(scDataNode(res));
  }    
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const {return false;};
};

class sgpFuncCmpUInt: public sgpFuncCompUInt {
public:
  virtual scString getName() const { return "cmp.uint"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfUInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfUInt, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    uint val1 = arg1.getAsUInt();
    uint val2 = arg2.getAsUInt();
    int res;
    if (val1 < val2) res = -1;
    else if (val1 > val2) res = 1;
    else res = 0;
    output.copyValueFrom(scDataNode(res));
  }    
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const {return false;};
};

class sgpFuncCmpUInt64: public sgpFuncCompUInt64 {
public:
  virtual scString getName() const { return "cmp.uint64"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfUInt+gdtfInt+gdtfUInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfUInt+gdtfInt+gdtfUInt64, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    ulong64 val1 = arg1.getAsUInt64();
    ulong64 val2 = arg2.getAsUInt64();
    int res;
    if (val1 < val2) res = -1;
    else if (val1 > val2) res = 1;
    else res = 0;
    output.copyValueFrom(scDataNode(res));
  }    
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const {return false;};
};

class sgpFuncCmpFloat: public sgpFuncCompFloat {
public:
  virtual scString getName() const { return "cmp.float"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    float val1 = arg1.getAsFloat();
    float val2 = arg2.getAsFloat();
    int res;
    if (val1 < val2) res = -1;
    else if (val1 > val2) res = 1;
    else res = 0;
    output.copyValueFrom(scDataNode(res));
  }    
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const {return false;};
};

class sgpFuncCmpDouble: public sgpFuncCompDouble {
public:
  virtual scString getName() const { return "cmp.double"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    double val1 = arg1.getAsDouble();
    double val2 = arg2.getAsDouble();
    int res;
    if (val1 < val2) res = -1;
    else if (val1 > val2) res = 1;
    else res = 0;
    output.copyValueFrom(scDataNode(res));
  }    
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const {return false;};
};

class sgpFuncCmpXDouble: public sgpFuncCompXDouble {
public:
  virtual scString getName() const { return "cmp.xdouble"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble+gdtfXDouble, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble+gdtfXDouble, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    xdouble val1 = arg1.getAsXDouble();
    xdouble val2 = arg2.getAsXDouble();
    int res;
    if (val1 < val2) res = -1;
    else if (val1 > val2) res = 1;
    else res = 0;
    output.copyValueFrom(scDataNode(res));
  }    
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const {return false;};
};

class sgpFuncCmpString: public sgpFuncCompString {
public:
  virtual scString getName() const { return "cmp.string"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfString, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfString, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    scString val1 = arg1.getAsString();
    scString val2 = arg2.getAsString();
    int res;
    if (val1 < val2) res = -1;
    else if (val1 > val2) res = 1;
    else res = 0;
    output.copyValueFrom(scDataNode(res));
  }    
  virtual bool calcValue(const scDataNode &arg1, const scDataNode &arg2) const {return false;};
};

class sgpFuncCmpVariant: public sgpFuncCmpString {
public:
  virtual scString getName() const { return "cmp.variant"; };
};  

// ----------------------------------------------------------------------------
// functions - arithmetic
// ----------------------------------------------------------------------------
// generic arithmetic, 1-input argument function
class sgpFuncArith1a: public sgpFunction {
public:
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, outValue;
    bool res;
    m_machine->evaluateArg(args[1], arg1);
    
    res = checkArgs(arg1);
    
    if (res) {          
      calcValue(arg1, outValue);
      
      m_machine->setLValue(args[0], outValue);
    }  
    return res;
  }
  
  virtual bool checkArgs(const scDataNode &arg1) const {return true;}
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const = 0;
};

// generic arithmetic, 2-input argument function
class sgpFuncArith2a: public sgpFunction {
public:
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2, outValue;
    bool res;
    
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);
    
    res = checkArgs(arg1, arg2);
    
    if (res) {          
      calcValue(arg1, arg2, outValue);
      
      m_machine->setLValue(args[0], outValue);
    }  
    return res;
  }
  
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2) const {return true;}
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const = 0;
};

//----------------------------------------------------------------------------
class sgpFuncArith1aInt: public sgpFuncArith1a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfInt+gdtfByte, output);
    return true;
  }
};

class sgpFuncArith1aInt64: public sgpFuncArith1a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfInt+gdtfUInt+gdtfInt64, output);
    return true;
  }
};

class sgpFuncArith1aByte: public sgpFuncArith1a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfByte, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte, output);
    return true;
  }
};

class sgpFuncArith1aUInt: public sgpFuncArith1a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfUInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfUInt, output);
    return true;
  }
};

class sgpFuncArith1aUInt64: public sgpFuncArith1a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfUInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfInt+gdtfUInt+gdtfUInt64, output);
    return true;
  }
};

class sgpFuncArith1aFloat: public sgpFuncArith1a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfFloat, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat, output);
    return true;
  }
};

class sgpFuncArith1aDouble: public sgpFuncArith1a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfDouble, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble, output);
    return true;
  }
};

class sgpFuncArith1aXDouble: public sgpFuncArith1a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfXDouble, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble+gdtfXDouble, output);
    return true;
  }
};

//----------------------------------------------------------------------------
class sgpFuncArith2aInt: public sgpFuncArith2a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfInt+gdtfByte, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfInt+gdtfByte, output);
    return true;
  }
};

class sgpFuncArith2aInt64: public sgpFuncArith2a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfInt+gdtfUInt+gdtfInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfInt+gdtfUInt+gdtfInt64, output);
    return true;
  }
};

class sgpFuncArith2aByte: public sgpFuncArith2a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfByte, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte, output);
    return true;
  }
};

class sgpFuncArith2aUInt: public sgpFuncArith2a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfUInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfUInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfUInt, output);
    return true;
  }
};

class sgpFuncArith2aUInt64: public sgpFuncArith2a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfUInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfInt+gdtfUInt+gdtfUInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfInt+gdtfUInt+gdtfUInt64, output);
    return true;
  }
};

class sgpFuncArith2aFloat: public sgpFuncArith2a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfFloat, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat, output);
    return true;
  }
};

class sgpFuncArith2aDouble: public sgpFuncArith2a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfDouble, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble, output);
    return true;
  }
};

class sgpFuncArith2aXDouble: public sgpFuncArith2a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfXDouble, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble+gdtfXDouble, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble+gdtfXDouble, output);
    return true;
  }
};

//----------------------------------------------------------------------------

// generic arithmetic, 3-input argument function
class sgpFuncArith3a: public sgpFunction {
public:
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 4; }

  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2, arg3, outValue;
    bool res;
    
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);
    m_machine->evaluateArg(args[3], arg3);

    res = checkArgs(arg1, arg2, arg3);
    
    if (res) {          
      calcValue(arg1, arg2, arg3, outValue);
      
      m_machine->setLValue(args[0], outValue);
    }  
    return res;
  }
  
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2, const scDataNode &arg3) const {return true;}
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, const scDataNode &arg3, scDataNode &output) const = 0;
};


//----------------------------------------------------------------------------
class sgpFuncArith3aInt: public sgpFuncArith3a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfInt+gdtfByte, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfInt+gdtfByte, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfInt+gdtfByte, output);
    return true;
  }
};

class sgpFuncArith3aInt64: public sgpFuncArith3a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfInt+gdtfUInt+gdtfInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfInt+gdtfUInt+gdtfInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfInt+gdtfUInt+gdtfInt64, output);
    return true;
  }
};

class sgpFuncArith3aByte: public sgpFuncArith3a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfByte, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte, output);
    return true;
  }
};

class sgpFuncArith3aUInt: public sgpFuncArith3a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfUInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfUInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfUInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfUInt, output);
    return true;
  }
};

class sgpFuncArith3aUInt64: public sgpFuncArith3a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfUInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfInt+gdtfUInt+gdtfUInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfInt+gdtfUInt+gdtfUInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfInt+gdtfUInt+gdtfUInt64, output);
    return true;
  }
};

class sgpFuncArith3aFloat: public sgpFuncArith3a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfFloat, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat, output);
    return true;
  }
};

class sgpFuncArith3aDouble: public sgpFuncArith3a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfDouble, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble, output);
    return true;
  }
};

class sgpFuncArith3aXDouble: public sgpFuncArith3a {
public:
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfXDouble, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble+gdtfXDouble, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble+gdtfXDouble, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble+gdtfXDouble, output);
    return true;
  }
};

//----------------------------------------------------------------------------

//-------  ADD
class sgpFuncAddInt: public sgpFuncArith2aInt {
public:
  virtual scString getName() const { return "add.int"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    output.copyValueFrom(scDataNode( 
      int(arg1.getAsInt() + 
          arg2.getAsInt())));
  }
};

class sgpFuncAddInt64: public sgpFuncArith2aInt64 {
public:
  virtual scString getName() const { return "add.int64"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    output.copyValueFrom(scDataNode( 
      long64(arg1.getAsInt64() + 
             arg2.getAsInt64())));
  }
};

class sgpFuncAddByte: public sgpFuncArith2aByte {
public:
  virtual scString getName() const { return "add.byte"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    output.copyValueFrom(scDataNode( 
      byte(arg1.getAsByte() + 
           arg2.getAsByte())));
  }
};

class sgpFuncAddUInt: public sgpFuncArith2aUInt {
public:
  virtual scString getName() const { return "add.uint"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    output.copyValueFrom(scDataNode( 
      uint(arg1.getAsUInt() + 
           arg2.getAsUInt())));
  }
};

class sgpFuncAddUInt64: public sgpFuncArith2aUInt64 {
public:
  virtual scString getName() const { return "add.uint64"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    output.copyValueFrom(scDataNode( 
      ulong64(arg1.getAsUInt64() + 
              arg2.getAsUInt64())));
  }
};

class sgpFuncAddFloat: public sgpFuncArith2aFloat {
public:
  virtual scString getName() const { return "add.float"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    output.copyValueFrom(scDataNode( 
      float(arg1.getAsFloat() + 
            arg2.getAsFloat())));
  }
};

class sgpFuncAddDouble: public sgpFuncArith2aDouble {
public:
  virtual scString getName() const { return "add.double"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    output.copyValueFrom(scDataNode( 
      double(arg1.getAsDouble() + 
             arg2.getAsDouble())));
  }
};

class sgpFuncAddXDouble: public sgpFuncArith2aXDouble {
public:
  virtual scString getName() const { return "add.xdouble"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    output.copyValueFrom(scDataNode( 
      xdouble(arg1.getAsXDouble() + 
              arg2.getAsXDouble())));
  }
};

//------ SUB
class sgpFuncSubInt: public sgpFuncArith2aInt {
public:
  virtual scString getName() const { return "sub.int"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    output.copyValueFrom(scDataNode(
      int(arg1.getAsInt() - 
          arg2.getAsInt())));
  }
};

class sgpFuncSubInt64: public sgpFuncArith2aInt64 {
public:
  virtual scString getName() const { return "sub.int64"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    output.copyValueFrom(scDataNode( 
      long64(arg1.getAsInt64() - 
             arg2.getAsInt64())));
  }
};

class sgpFuncSubByte: public sgpFuncArith2aByte {
public:
  virtual scString getName() const { return "sub.byte"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    output.copyValueFrom(scDataNode(
      byte(arg1.getAsByte() - 
           arg2.getAsByte())));
  }
};

class sgpFuncSubUInt: public sgpFuncArith2aUInt {
public:
  virtual scString getName() const { return "sub.uint"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    output.copyValueFrom(scDataNode( 
      uint(arg1.getAsUInt() - 
           arg2.getAsUInt())));
  }
};

class sgpFuncSubUInt64: public sgpFuncArith2aUInt64 {
public:
  virtual scString getName() const { return "sub.uint64"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    output.copyValueFrom(scDataNode( 
      ulong64(arg1.getAsUInt64() - 
              arg2.getAsUInt64())));
  }
};

class sgpFuncSubFloat: public sgpFuncArith2aFloat {
public:
  virtual scString getName() const { return "sub.float"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    output.copyValueFrom(scDataNode(
      float(arg1.getAsFloat() - 
            arg2.getAsFloat())));
  }
};

class sgpFuncSubDouble: public sgpFuncArith2aDouble {
public:
  virtual scString getName() const { return "sub.double"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const 
  {
    output.copyValueFrom(scDataNode( 
      double(arg1.getAsDouble() - 
             arg2.getAsDouble())));
  }
};

class sgpFuncSubXDouble: public sgpFuncArith2aXDouble {
public:
  virtual scString getName() const { return "sub.xdouble"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode( 
      xdouble(arg1.getAsXDouble() - 
              arg2.getAsXDouble())));
  }
};

//------ MULT
class sgpFuncMultInt: public sgpFuncArith2aInt {
public:
  virtual scString getName() const { return "mult.int"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(
      int(arg1.getAsInt() * 
          arg2.getAsInt())));
  }
};

class sgpFuncMultInt64: public sgpFuncArith2aInt64 {
public:
  virtual scString getName() const { return "mult.int64"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode( 
      long64(arg1.getAsInt64() * 
             arg2.getAsInt64())));
  }
};

class sgpFuncMultByte: public sgpFuncArith2aByte {
public:
  virtual scString getName() const { return "mult.byte"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(
      byte(arg1.getAsByte() * 
           arg2.getAsByte())));
  }
};

class sgpFuncMultUInt: public sgpFuncArith2aUInt {
public:
  virtual scString getName() const { return "mult.uint"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(
      uint(arg1.getAsUInt() * 
           arg2.getAsUInt())));
  }
};

class sgpFuncMultUInt64: public sgpFuncArith2aUInt64 {
public:
  virtual scString getName() const { return "mult.uint64"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode( 
      ulong64(arg1.getAsUInt64() * 
              arg2.getAsUInt64())));
  }
};

class sgpFuncMultFloat: public sgpFuncArith2aFloat {
public:
  virtual scString getName() const { return "mult.float"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode( 
      float(arg1.getAsFloat() * 
            arg2.getAsFloat())));
  }
};

class sgpFuncMultDouble: public sgpFuncArith2aDouble {
public:
  virtual scString getName() const { return "mult.double"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode( 
      double(arg1.getAsDouble() * 
             arg2.getAsDouble())));
  }
};

class sgpFuncMultXDouble: public sgpFuncArith2aXDouble {
public:
  virtual scString getName() const { return "mult.xdouble"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode( 
      xdouble(arg1.getAsXDouble() * 
              arg2.getAsXDouble())));
  }
};

//------ DIV
class sgpFuncDivInt: public sgpFuncArith2aInt {
public:
  virtual scString getName() const { return "div.int"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode( 
      int(arg1.getAsInt() / 
          arg2.getAsInt())));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2) const
  {
    bool res = (arg2.getAsInt() != 0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg2, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

class sgpFuncDivInt64: public sgpFuncArith2aInt64 {
public:
  virtual scString getName() const { return "div.int64"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode( 
      long64(arg1.getAsInt64() / 
             arg2.getAsInt64())));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2) const
  {
    bool res = (arg2.getAsInt64() != 0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg2, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

class sgpFuncDivByte: public sgpFuncArith2aByte {
public:
  virtual scString getName() const { return "div.byte"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode( 
      byte(arg1.getAsByte() / 
           arg2.getAsByte())));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2) const
  {
    bool res = (arg2.getAsByte() != 0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg2, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

class sgpFuncDivUInt: public sgpFuncArith2aUInt {
public:
  virtual scString getName() const { return "div.uint"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(
      uint(arg1.getAsUInt() / 
           arg2.getAsUInt())));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2) const
  {
    bool res = (arg2.getAsUInt() != 0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg2, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

class sgpFuncDivUInt64: public sgpFuncArith2aUInt64 {
public:
  virtual scString getName() const { return "div.uint64"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode( 
      ulong64(arg1.getAsUInt64() / 
              arg2.getAsUInt64())));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2) const
  {
    bool res = (arg2.getAsUInt64() != 0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg2, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

class sgpFuncDivFloat: public sgpFuncArith2aFloat {
public:
  virtual scString getName() const { return "div.float"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode( 
      float(arg1.getAsFloat() / 
            arg2.getAsFloat())));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2) const
  {
    bool res = (arg2.getAsFloat() != 0.0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg2, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

class sgpFuncDivDouble: public sgpFuncArith2aDouble {
public:
  virtual scString getName() const { return "div.double"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode( 
      double(arg1.getAsDouble() / 
             arg2.getAsDouble())));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2) const
  {
    bool res = (arg2.getAsDouble() != 0.0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg2, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

class sgpFuncDivXDouble: public sgpFuncArith2aXDouble {
public:
  virtual scString getName() const { return "div.xdouble"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(
      xdouble(arg1.getAsXDouble() / 
              arg2.getAsXDouble())));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2) const
  {
    bool res = (arg2.getAsXDouble() != 0.0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg2, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

// ----------------------------------------------------------------------------
// ------ invs: f(x) = 1/x (safe version)
// ----------------------------------------------------------------------------
// invs #out, arg1
class sgpFuncInvsFloat: public sgpFuncArith1aFloat {
public:
  virtual scString getName() const { return "invs.float"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    float val1 = arg1.getAsFloat();
    val1 = fpSafeInv(val1);
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncInvsDouble: public sgpFuncArith1aDouble {
public:
  virtual scString getName() const { return "invs.double"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    double val1 = arg1.getAsDouble();
    val1 = fpSafeInv(val1);
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncInvsXDouble: public sgpFuncArith1aXDouble {
public:
  virtual scString getName() const { return "invs.xdouble"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    xdouble val1 = arg1.getAsXDouble();
    val1 = fpSafeInv(val1);
    output.copyValueFrom(scDataNode(val1));
  }
};

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

//------ MultDiv
// multdiv.int #out, arg1, arg2, arg3 : calculate f(x) = arg1 * arg2 / arg3
class sgpFuncMultDivInt: public sgpFuncArith3aInt {
public:
  virtual scString getName() const { return "multdiv.int"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, const scDataNode &arg3, scDataNode &output) const
  {
    long64 outValue = arg1.getAsInt();
    outValue = outValue * arg2.getAsInt();
    outValue = outValue / arg3.getAsInt();
    output.copyValueFrom(scDataNode(int(outValue)));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2, const scDataNode &arg3) const
  {
    bool res = (arg3.getAsInt() != 0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg3, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

class sgpFuncMultDivByte: public sgpFuncArith3aByte {
public:
  virtual scString getName() const { return "multdiv.byte"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, const scDataNode &arg3, scDataNode &output) const
  {
    int outValue = arg1.getAsByte();
    outValue = outValue * arg2.getAsByte();
    outValue = outValue / arg3.getAsByte();
    output.copyValueFrom(scDataNode(byte(outValue)));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2, const scDataNode &arg3) const
  {
    bool res = (arg3.getAsByte() != 0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg3, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

class sgpFuncMultDivUInt: public sgpFuncArith3aUInt {
public:
  virtual scString getName() const { return "multdiv.uint"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, const scDataNode &arg3, scDataNode &output) const
  {
    ulong64 outValue = arg1.getAsUInt();
    outValue = outValue * arg2.getAsUInt();
    outValue = outValue / arg3.getAsUInt();
    output.copyValueFrom(scDataNode(uint(outValue)));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2, const scDataNode &arg3) const
  {
    bool res = (arg3.getAsUInt() != 0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg3, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

class sgpFuncMultDivFloat: public sgpFuncArith3aFloat {
public:
  virtual scString getName() const { return "multdiv.float"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, const scDataNode &arg3, scDataNode &output) const
  {
    double outValue = arg1.getAsFloat();
    outValue = outValue * arg2.getAsFloat();
    outValue = outValue / arg3.getAsFloat();
    output.copyValueFrom(scDataNode(float(outValue)));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2, const scDataNode &arg3) const
  {
    bool res = (arg2.getAsFloat() != 0.0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg3, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

class sgpFuncMultDivDouble: public sgpFuncArith3aDouble {
public:
  virtual scString getName() const { return "multdiv.double"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, const scDataNode &arg3, scDataNode &output) const
  {
    xdouble outValue = arg1.getAsDouble();
    outValue = outValue * arg2.getAsDouble();
    outValue = outValue / arg3.getAsDouble();
    output.copyValueFrom(scDataNode(double(outValue)));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2, const scDataNode &arg3) const
  {
    bool res = (arg3.getAsDouble() != 0.0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg3, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

// ----------------------------------------------------------------------------
// ------ pow.uint
// ----------------------------------------------------------------------------
// pow.uint #out, arg1, arg2 : calculate f(x) = arg1 ^ arg2 
class sgpFuncPowUInt: public sgpFuncArith2a {
public:
  virtual scString getName() const { return "pow.uint"; };

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfUInt+gdtfUInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfUInt+gdtfUInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfByte+gdtfUInt+gdtfUInt64, output);
    return true;
  }
  
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    uint leftCnt = arg2.getAsUInt();
    uint base = arg1.getAsUInt();
    ulong64 resValue = ((leftCnt != 0)?base:1);
    if (leftCnt > 1) {
      do {
        resValue = resValue * base;
        leftCnt--;
      } while (leftCnt > 1);
    }
    output.copyValueFrom(scDataNode(uint(resValue)));
  }
};

// ----------------------------------------------------------------------------
// ------ MOD
// ----------------------------------------------------------------------------
// mod #out, arg1, arg2
class sgpFuncModInt: public sgpFuncArith2aInt {
public:
  virtual scString getName() const { return "mod.int"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(int(arg1.getAsInt() % arg2.getAsInt())));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2) const
  {
    bool res = (arg2.getAsInt() != 0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg2, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

class sgpFuncModInt64: public sgpFuncArith2aInt64 {
public:
  virtual scString getName() const { return "mod.int64"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(long64(arg1.getAsInt64() % arg2.getAsInt64())));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2) const
  {
    bool res = (arg2.getAsInt64() != 0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg2, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

class sgpFuncModByte: public sgpFuncArith2aByte {
public:
  virtual scString getName() const { return "mod.byte"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(byte(arg1.getAsByte() % arg2.getAsByte())));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2) const
  {
    bool res = (arg2.getAsByte() != 0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg2, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

class sgpFuncModUInt: public sgpFuncArith2aUInt {
public:
  virtual scString getName() const { return "mod.uint"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(uint(arg1.getAsUInt() % arg2.getAsUInt())));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2) const
  {
    bool res = (arg2.getAsUInt() != 0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg2, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

class sgpFuncModUInt64: public sgpFuncArith2aUInt64 {
public:
  virtual scString getName() const { return "mod.uint64"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(ulong64(arg1.getAsUInt64() % arg2.getAsUInt64())));
  }
  virtual bool checkArgs(const scDataNode &arg1, const scDataNode &arg2) const
  {
    bool res = (arg2.getAsUInt64() != 0);
    if (!res) 
      m_machine->handleInstrError(getName(), arg2, GVM_ERROR_ARITH_DIV_BY_ZERO);    
    return res;
  }
};

// ----------------------------------------------------------------------------
// ------ bit.and
// ----------------------------------------------------------------------------
// bit.and #out, arg1, arg2
class sgpFuncBitAndByte: public sgpFuncArith2aByte {
public:
  virtual scString getName() const { return "bit.and.byte"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(byte(arg1.getAsByte() & arg2.getAsByte())));
  }
};

class sgpFuncBitAndUInt: public sgpFuncArith2aUInt {
public:
  virtual scString getName() const { return "bit.and.uint"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(uint(arg1.getAsUInt() & arg2.getAsUInt())));
  }
};

class sgpFuncBitAndUInt64: public sgpFuncArith2aUInt64 {
public:
  virtual scString getName() const { return "bit.and.uint64"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(ulong64(arg1.getAsUInt64() & arg2.getAsUInt64())));
  }
};

// ----------------------------------------------------------------------------
// ------ bit.or
// ----------------------------------------------------------------------------
// bit.or #out, arg1, arg2
class sgpFuncBitOrByte: public sgpFuncArith2aByte {
public:
  virtual scString getName() const { return "bit.or.byte"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(byte(arg1.getAsByte() | arg2.getAsByte())));
  }
};

class sgpFuncBitOrUInt: public sgpFuncArith2aUInt {
public:
  virtual scString getName() const { return "bit.or.uint"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(uint(arg1.getAsUInt() | arg2.getAsUInt())));
  }
};

class sgpFuncBitOrUInt64: public sgpFuncArith2aUInt64 {
public:
  virtual scString getName() const { return "bit.or.uint64"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(ulong64(arg1.getAsUInt64() | arg2.getAsUInt64())));
  }
};

// ----------------------------------------------------------------------------
// ------ bit.xor
// ----------------------------------------------------------------------------
// bit.xor #out, arg1, arg2
class sgpFuncBitXorByte: public sgpFuncArith2aByte {
public:
  virtual scString getName() const { return "bit.xor.byte"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(byte(arg1.getAsByte() ^ arg2.getAsByte())));
  }
};

class sgpFuncBitXorUInt: public sgpFuncArith2aUInt {
public:
  virtual scString getName() const { return "bit.xor.uint"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(uint(arg1.getAsUInt() ^ arg2.getAsUInt())));
  }
};

class sgpFuncBitXorUInt64: public sgpFuncArith2aUInt64 {
public:
  virtual scString getName() const { return "bit.xor.uint64"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(ulong64(arg1.getAsUInt64() ^ arg2.getAsUInt64())));
  }
};

// ----------------------------------------------------------------------------
// ------ bit.not
// ----------------------------------------------------------------------------
// bit.not #out, arg1
class sgpFuncBitNotByte: public sgpFuncArith1aByte {
public:
  virtual scString getName() const { return "bit.not.byte"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(byte(~arg1.getAsByte())));
  }
};

class sgpFuncBitNotUInt: public sgpFuncArith1aUInt {
public:
  virtual scString getName() const { return "bit.not.uint"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(uint(~arg1.getAsUInt())));
  }
};

class sgpFuncBitNotUInt64: public sgpFuncArith1aUInt64 {
public:
  virtual scString getName() const { return "bit.not.uint64"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(ulong64(~arg1.getAsUInt64())));
  }
};

// ----------------------------------------------------------------------------
// ------ bit.shr
// ----------------------------------------------------------------------------
// bit.shr #out, arg1, arg2
class sgpFuncBitShrByte: public sgpFuncArith2aByte {
public:
  virtual scString getName() const { return "bit.shr.byte"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(byte(arg1.getAsByte() >> byte(arg2.getAsByte()))));
  }
};

class sgpFuncBitShrUInt: public sgpFuncArith2aUInt {
public:
  virtual scString getName() const { return "bit.shr.uint"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(uint(arg1.getAsUInt() >> byte(arg2.getAsUInt()))));
  }
};

class sgpFuncBitShrUInt64: public sgpFuncArith2aUInt64 {
public:
  virtual scString getName() const { return "bit.shr.uint64"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(ulong64(arg1.getAsUInt64() >> byte(arg2.getAsUInt64()))));
  }
};

// ----------------------------------------------------------------------------
// ------ bit.shl
// ----------------------------------------------------------------------------
// bit.shl #out, arg1, arg2
class sgpFuncBitShlByte: public sgpFuncArith2aByte {
public:
  virtual scString getName() const { return "bit.shl.byte"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(byte(arg1.getAsByte() << byte(arg2.getAsByte()))));
  }
};

class sgpFuncBitShlUInt: public sgpFuncArith2aUInt {
public:
  virtual scString getName() const { return "bit.shl.uint"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(uint(arg1.getAsUInt() << byte(arg2.getAsUInt()))));
  }
};

class sgpFuncBitShlUInt64: public sgpFuncArith2aUInt64 {
public:
  virtual scString getName() const { return "bit.shl.uint64"; };
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const
  {
    output.copyValueFrom(scDataNode(ulong64(arg1.getAsUInt64() << byte(arg2.getAsUInt64()))));
  }
};

// ----------------------------------------------------------------------------
// ------ abs
// ----------------------------------------------------------------------------
// abs #out, arg1
class sgpFuncAbsInt: public sgpFuncArith1aInt {
public:
  virtual scString getName() const { return "abs.int"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    int val1 = arg1.getAsInt();
    if (val1 < 0) val1 = -val1;
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncAbsInt64: public sgpFuncArith1aInt64 {
public:
  virtual scString getName() const { return "abs.int64"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    long64 val1 = arg1.getAsInt64();
    if (val1 < 0) val1 = -val1;
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncAbsFloat: public sgpFuncArith1aFloat {
public:
  virtual scString getName() const { return "abs.float"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    float val1 = arg1.getAsFloat();
    if (val1 < 0.0) val1 = -val1;
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncAbsDouble: public sgpFuncArith1aDouble {
public:
  virtual scString getName() const { return "abs.double"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    double val1 = arg1.getAsDouble();
    if (val1 < 0.0) val1 = -val1;
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncAbsXDouble: public sgpFuncArith1aXDouble {
public:
  virtual scString getName() const { return "abs.xdouble"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    xdouble val1 = arg1.getAsXDouble();
    if (val1 < 0.0) val1 = -val1;
    output.copyValueFrom(scDataNode(val1));
  }
};

// ----------------------------------------------------------------------------
// ------ neg: f(x) = -x
// ----------------------------------------------------------------------------
// neg #out, arg1
class sgpFuncNegInt: public sgpFuncArith1aInt {
public:
  virtual scString getName() const { return "neg.int"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    int val1 = arg1.getAsInt();
    val1 = -val1;
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncNegInt64: public sgpFuncArith1aInt64 {
public:
  virtual scString getName() const { return "neg.int64"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    long64 val1 = arg1.getAsInt64();
    val1 = -val1;
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncNegFloat: public sgpFuncArith1aFloat {
public:
  virtual scString getName() const { return "neg.float"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    float val1 = arg1.getAsFloat();
    val1 = -val1;
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncNegDouble: public sgpFuncArith1aDouble {
public:
  virtual scString getName() const { return "neg.double"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    double val1 = arg1.getAsDouble();
    val1 = -val1;
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncNegXDouble: public sgpFuncArith1aXDouble {
public:
  virtual scString getName() const { return "neg.xdouble"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    xdouble val1 = arg1.getAsXDouble();
    val1 = -val1;
    output.copyValueFrom(scDataNode(val1));
  }
};

// ----------------------------------------------------------------------------
// ------ sgn
// ----------------------------------------------------------------------------
// sgn #out, arg1
class sgpFuncSgnInt: public sgpFuncArith1aInt {
public:
  virtual scString getName() const { return "sgn.int"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    int val1 = arg1.getAsInt();
    int sgnVal;
    if (val1 < 0) sgnVal = -1;
    else if (val1 > 0) sgnVal = 1;
    else sgnVal = 1;
    output.copyValueFrom(scDataNode(sgnVal));
  }
};

class sgpFuncSgnInt64: public sgpFuncArith1aInt64 {
public:
  virtual scString getName() const { return "sgn.int64"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    long64 val1 = arg1.getAsInt64();
    int sgnVal;
    if (val1 < 0) sgnVal = -1;
    else if (val1 > 0) sgnVal = 1;
    else sgnVal = 1;
    output.copyValueFrom(scDataNode(sgnVal));
  }
};

class sgpFuncSgnFloat: public sgpFuncArith1aFloat {
public:
  virtual scString getName() const { return "sgn.float"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    float val1 = arg1.getAsFloat();
    float sgnVal;
    if (val1 < 0.0) sgnVal = -1;
    else if (val1 > 0.0) sgnVal = 1;
    else sgnVal = 1;
    output.copyValueFrom(scDataNode(sgnVal));
  }
};

class sgpFuncSgnDouble: public sgpFuncArith1aDouble {
public:
  virtual scString getName() const { return "sgn.double"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    double val1 = arg1.getAsDouble();
    double sgnVal;
    if (val1 < 0.0) sgnVal = -1.0;
    else if (val1 > 0.0) sgnVal = 1.0;
    else sgnVal = 1.0;
    output.copyValueFrom(scDataNode(sgnVal));
  }
};

class sgpFuncSgnXDouble: public sgpFuncArith1aXDouble {
public:
  virtual scString getName() const { return "sgn.xdouble"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    xdouble val1 = arg1.getAsXDouble();
    xdouble sgnVal;
    if (val1 < 0.0) sgnVal = -1.0;
    else if (val1 > 0.0) sgnVal = 1.0;
    else sgnVal = 1.0;
    output.copyValueFrom(scDataNode(sgnVal));
  }
};

// ----------------------------------------------------------------------------
// ceil #out, arg1
// ----------------------------------------------------------------------------
// The smallest integral value not less than x. Round up.
class sgpFuncCeilFloat: public sgpFuncArith1aFloat {
public:
  virtual scString getName() const { return "ceil.float"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    float val1 = arg1.getAsFloat();
    val1 = ceil(val1);
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncCeilDouble: public sgpFuncArith1aDouble {
public:
  virtual scString getName() const { return "ceil.double"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    double val1 = arg1.getAsDouble();
    val1 = ceil(val1);
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncCeilXDouble: public sgpFuncArith1aXDouble {
public:
  virtual scString getName() const { return "ceil.xdouble"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    xdouble val1 = arg1.getAsXDouble();
    val1 = ceil(val1);
    output.copyValueFrom(scDataNode(val1));
  }
};

// ----------------------------------------------------------------------------
// floor #out, arg1
// ----------------------------------------------------------------------------
// The largest integral value not greater than x. Round down.
class sgpFuncFloorFloat: public sgpFuncArith1aFloat {
public:
  virtual scString getName() const { return "floor.float"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    float val1 = arg1.getAsFloat();
    val1 = floor(val1);
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncFloorDouble: public sgpFuncArith1aDouble {
public:
  virtual scString getName() const { return "floor.double"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    double val1 = arg1.getAsDouble();
    val1 = floor(val1);
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncFloorXDouble: public sgpFuncArith1aXDouble {
public:
  virtual scString getName() const { return "floor.xdouble"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    xdouble val1 = arg1.getAsXDouble();
    val1 = floor(val1);
    output.copyValueFrom(scDataNode(val1));
  }
};


// ----------------------------------------------------------------------------
// frac #out, arg1
// ----------------------------------------------------------------------------
// Returns fractional part of the FP value
class sgpFuncFracFloat: public sgpFuncArith1aFloat {
public:
  virtual scString getName() const { return "frac.float"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    float val1 = arg1.getAsFloat();
    float intpart;
    float fracPart = modf(val1, &intpart);
    output.copyValueFrom(scDataNode(fracPart));
  }
};

class sgpFuncFracDouble: public sgpFuncArith1aDouble {
public:
  virtual scString getName() const { return "frac.double"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    double val1 = arg1.getAsDouble();
    double intpart;
    double fracPart = modf(val1, &intpart);
    output.copyValueFrom(scDataNode(fracPart));
  }
};

class sgpFuncFracXDouble: public sgpFuncArith1aXDouble {
public:
  virtual scString getName() const { return "frac.xdouble"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    xdouble val1 = arg1.getAsXDouble();
    xdouble intpart;
    xdouble fracPart = modf(val1, &intpart);
    output.copyValueFrom(scDataNode(fracPart));
  }
};

// ----------------------------------------------------------------------------
// trunc #out, arg1
// ----------------------------------------------------------------------------
// Returns integer part of the FP value
class sgpFuncTruncFloat: public sgpFuncArith1aFloat {
public:
  virtual scString getName() const { return "trunc.float"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    float val1 = arg1.getAsFloat();
    int intVal = int(val1);
    output.copyValueFrom(scDataNode(intVal));
  }
};

class sgpFuncTruncDouble: public sgpFuncArith1aDouble {
public:
  virtual scString getName() const { return "trunc.double"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    double val1 = arg1.getAsDouble();
    int intVal = int(val1);
    output.copyValueFrom(scDataNode(intVal));
  }
};

class sgpFuncTruncXDouble: public sgpFuncArith1aXDouble {
public:
  virtual scString getName() const { return "trunc.xdouble"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble+gdtfXDouble, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    xdouble val1 = arg1.getAsXDouble();
    int intVal = int(val1);
    output.copyValueFrom(scDataNode(intVal));
  }
};

// ----------------------------------------------------------------------------
// trunc64 #out, arg1
// ----------------------------------------------------------------------------
// Returns integer part of the FP value (int64 version)
class sgpFuncTrunc64Float: public sgpFuncArith1aFloat {
public:
  virtual scString getName() const { return "trunc64.float"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfAllBaseInts, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    float val1 = arg1.getAsFloat();
    long64 intVal = long64(val1);
    output.copyValueFrom(scDataNode(intVal));
  }
};

class sgpFuncTrunc64Double: public sgpFuncArith1aDouble {
public:
  virtual scString getName() const { return "trunc64.double"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfAllBaseInts, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    double val1 = arg1.getAsDouble();
    long64 intVal = long64(val1);
    output.copyValueFrom(scDataNode(intVal));
  }
};

class sgpFuncTrunc64XDouble: public sgpFuncArith1aXDouble {
public:
  virtual scString getName() const { return "trunc64.xdouble"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfAllBaseInts, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble+gdtfXDouble, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    xdouble val1 = arg1.getAsXDouble();
    long64 intVal = long64(val1);
    output.copyValueFrom(scDataNode(intVal));
  }
};

// ----------------------------------------------------------------------------
// truncf #out, arg1
// ----------------------------------------------------------------------------
// Returns integer part of the FP value - as FP value
class sgpFuncTruncfFloat: public sgpFuncArith1aFloat {
public:
  virtual scString getName() const { return "truncf.float"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    float val1 = arg1.getAsFloat();
    val1 = long64(val1);
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncTruncfDouble: public sgpFuncArith1aDouble {
public:
  virtual scString getName() const { return "truncf.double"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    double val1 = arg1.getAsDouble();
    val1 = long64(val1);
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncTruncfXDouble: public sgpFuncArith1aXDouble {
public:
  virtual scString getName() const { return "truncf.xdouble"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    xdouble val1 = arg1.getAsXDouble();
    val1 = long64(val1);
    output.copyValueFrom(scDataNode(val1));
  }
};

// ----------------------------------------------------------------------------
// round #out, arg1
// ----------------------------------------------------------------------------
// Returns FP value rounded mathematically and returns result as int
class sgpFuncRoundFloat: public sgpFuncArith1aFloat {
public:
  virtual scString getName() const { return "round.float"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    float val1 = arg1.getAsFloat();
    if (val1 < 0.0f)
      val1 = val1 - 0.5f;
    else  
      val1 = val1 + 0.5f;
     
    int intPart = int(val1);
    output.copyValueFrom(scDataNode(intPart));
  }
};

class sgpFuncRoundDouble: public sgpFuncArith1aDouble {
public:
  virtual scString getName() const { return "round.double"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    double val1 = arg1.getAsDouble();
    if (val1 < 0.0f)
      val1 = val1 - 0.5f;
    else  
      val1 = val1 + 0.5f;
     
    int intPart = int(val1);
    output.copyValueFrom(scDataNode(intPart));
  }
};

class sgpFuncRoundXDouble: public sgpFuncArith1aXDouble {
public:
  virtual scString getName() const { return "round.xdouble"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble+gdtfXDouble, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    xdouble val1 = arg1.getAsXDouble();
    if (val1 < 0.0f)
      val1 = val1 - 0.5f;
    else  
      val1 = val1 + 0.5f;
     
    int intPart = int(val1);
    output.copyValueFrom(scDataNode(intPart));
  }
};

// ----------------------------------------------------------------------------
// round64 #out, arg1
// ----------------------------------------------------------------------------
// Returns FP value rounded mathematically and returns result as int64
class sgpFuncRound64Float: public sgpFuncArith1aFloat {
public:
  virtual scString getName() const { return "round64.float"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    float val1 = arg1.getAsFloat();
    if (val1 < 0.0f)
      val1 = val1 - 0.5f;
    else  
      val1 = val1 + 0.5f;
     
    long64 intPart = long64(val1);
    output.copyValueFrom(scDataNode(intPart));
  }
};

class sgpFuncRound64Double: public sgpFuncArith1aDouble {
public:
  virtual scString getName() const { return "round64.double"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    double val1 = arg1.getAsDouble();
    if (val1 < 0.0f)
      val1 = val1 - 0.5f;
    else  
      val1 = val1 + 0.5f;
     
    long64 intPart = long64(val1);
    output.copyValueFrom(scDataNode(intPart));
  }
};

class sgpFuncRound64XDouble: public sgpFuncArith1aXDouble {
public:
  virtual scString getName() const { return "round64.xdouble"; };
  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfVariant+gdtfNull+gdtfInt64, output);
    addArgMeta(gatfInput, gatfAny, gdtfVariant+gdtfFloat+gdtfDouble+gdtfXDouble, output);
    return true;
  }
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    xdouble val1 = arg1.getAsXDouble();
    if (val1 < 0.0f)
      val1 = val1 - 0.5f;
    else  
      val1 = val1 + 0.5f;
     
    long64 intPart = long64(val1);
    output.copyValueFrom(scDataNode(intPart));
  }
};

// ----------------------------------------------------------------------------
// roundf #out, arg1
// ----------------------------------------------------------------------------
// Returns FP value rounded mathematically and returns result as FP value
class sgpFuncRoundfFloat: public sgpFuncArith1aFloat {
public:
  virtual scString getName() const { return "roundf.float"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    float val1 = arg1.getAsFloat();
    if (val1 < 0.0f)
      val1 = val1 - 0.5f;
    else  
      val1 = val1 + 0.5f;
     
    val1 = long64(val1);
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncRoundfDouble: public sgpFuncArith1aDouble {
public:
  virtual scString getName() const { return "roundf.double"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    double val1 = arg1.getAsDouble();
    if (val1 < 0.0f)
      val1 = val1 - 0.5f;
    else  
      val1 = val1 + 0.5f;
     
    val1 = long64(val1);
    output.copyValueFrom(scDataNode(val1));
  }
};

class sgpFuncRoundfXDouble: public sgpFuncArith1aXDouble {
public:
  virtual scString getName() const { return "roundf.xdouble"; };
  virtual void calcValue(const scDataNode &arg1, scDataNode &output) const
  {
    xdouble val1 = arg1.getAsXDouble();
    if (val1 < 0.0f)
      val1 = val1 - 0.5f;
    else  
      val1 = val1 + 0.5f;
     
    val1 = long64(val1);
    output.copyValueFrom(scDataNode(val1));
  }
};

// ----------------------------------------------------------------------------
// control functions
// ----------------------------------------------------------------------------
// skip.ifn value1 jump_size : skip if value is <false>
class sgpFuncSkipIfn: public sgpFunction {
public:
  virtual scString getName() const { return "skip.ifn"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfInput, gatfAny, gdtfBool, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseUInts, output);
    return true;
  }

  virtual bool isJumpAction() { return true;}

  virtual void getJumpArgs(scDataNode &args) 
  {
    args.addItem(scDataNode(2));
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;

    m_machine->evaluateArg(args[0], arg1);
    m_machine->evaluateArg(args[1], arg2);
    
    if (!arg1.getAsBool())
      m_machine->skipCells(arg2.getAsUInt64());
    return true;
  }
};

// skip.if value1 jump_size : skip if value is <true>
class sgpFuncSkipIf: public sgpFunction {
public:
  virtual scString getName() const { return "skip.if"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfInput, gatfAny, gdtfBool, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseUInts, output);
    return true;
  }

  virtual bool isJumpAction() { return true;}

  virtual void getJumpArgs(scDataNode &args) 
  {
    args.addItem(scDataNode(2));
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;

    m_machine->evaluateArg(args[0], arg1);
    m_machine->evaluateArg(args[1], arg2);
    
    if (arg1.getAsBool())
      m_machine->skipCells(arg2.getAsUInt64());
    return true;
  }
};

// skip jump_size1 : skip n cells
class sgpFuncSkip: public sgpFunction {
public:
  virtual scString getName() const { return "skip"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseUInts, output);
    return true;
  }

  virtual bool isJumpAction() { return true;}

  virtual void getJumpArgs(scDataNode &args) 
  {
    args.addItem(scDataNode(1));
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1;

    m_machine->evaluateArg(args[0], arg1);
    
    m_machine->skipCells(arg1.getAsUInt64());
    return true;
  }
};

//  jump.back jump_size1 : go back n cells
class sgpFuncJumpBack: public sgpFunction {
public:
  virtual scString getName() const { return "jump.back"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseUInts, output);
    return true;
  }

  virtual bool isJumpAction() { return true;}

  virtual void getJumpArgs(scDataNode &args) 
  {
    args.addItem(scDataNode(-1));
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1;

    m_machine->evaluateArg(args[0], arg1);
    
    m_machine->jumpBack(arg1.getAsUInt64());
    return true;
  }
};

// repnz #reg jump_size : decrement #reg, if zero jump forward
class sgpFuncRepnz: public sgpFunction {
public:
  virtual scString getName() const { return "repnz"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfInput+gatfOutput, gatfLValue, gdtfAllBaseXInts, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseUInts, output);
    return true;
  }

  virtual bool isJumpAction() { return true;}
  
  virtual void getJumpArgs(scDataNode &args) 
  {
    args.addItem(scDataNode(2));
  }

  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;

    m_machine->evaluateArg(args[0], arg1);
    m_machine->evaluateArg(args[1], arg2);
    
    int rValue = arg1.getAsInt();
    
    if (rValue <= 0)
      m_machine->skipCells(arg2.getAsUInt64());
      
    rValue--;
    m_machine->setLValue(args[0], scDataNode(rValue));
      
    return true;
  }
};

// repnz.back #reg jump_size : decrement #reg, if non-zero jump backward
class sgpFuncRepnzBack: public sgpFunction {
public:
  virtual scString getName() const { return "repnz.back"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfInput+gatfOutput, gatfLValue, gdtfAllBaseXInts, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseUInts, output);
    return true;
  }

  virtual bool isJumpAction() { return true;}
  
  virtual void getJumpArgs(scDataNode &args) 
  {
    args.addItem(scDataNode(-2));
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;

    m_machine->evaluateArg(args[0], arg1);
    m_machine->evaluateArg(args[1], arg2);
    
    int rValue = arg1.getAsInt();
    rValue--;
    
    m_machine->setLValue(args[0], scDataNode(rValue), true);
    
    if (rValue > 0)
      m_machine->jumpBack(arg2.getAsUInt64());
    return true;
  }
};

// ----------------------------------------------------------------------------
// array functions
// ----------------------------------------------------------------------------

//  array.init #out[, type] : initialization of array, clears contents
class sgpFuncArrayInit: public sgpFunction {
public:
  virtual scString getName() const { return "array.init"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 1; a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput+gatfInput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseUInts, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg0, arg1, outValue;
    uint valueType = vt_datanode;

    if (args.size() > 1) {
      m_machine->evaluateArg(args[1], arg1);
      valueType = arg1.getAsUInt();
      // convert vmachine data type to scDataNode data type
      scDataNode tempVal;
      m_machine->initDataNodeAs(sgpGvmDataTypeFlag(valueType), tempVal);
      valueType = tempVal.getValueType();
    } else {      
      m_machine->evaluateArg(args[0], arg0);

      if (arg0.isArray())
        valueType = arg0.getArray()->getValueType();
    }
          
    outValue.setAsArray(scDataNodeValueType(valueType));  
    m_machine->setLValue(args[0], outValue);
    return true;
  }
};

//  array.define #out, type, size : define an array variable
class sgpFuncArrayDefine: public sgpFunction {
public:
  virtual scString getName() const { return "array.define"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseUInts, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseUInts, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;
    uint outRegNo = m_machine->getRegisterNo(args[0]);

    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);
    
    m_machine->defineArray(
      outRegNo,
      sgpGvmDataTypeFlag(arg1.getAsUInt()),
      arg1.getAsUInt64()
    );
    return true;
  }
};

//  array.size #out, #in_reg : returns size of array
class sgpFuncArraySize: public sgpFunction {
public:
  virtual scString getName() const { return "array.size"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfRegister, gdtfArray, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;

    uint inRegNo = m_machine->getRegisterNo(args[1]);
    ulong64 asize = m_machine->arrayGetSize(inRegNo);
    m_machine->setLValue(args[0], scDataNode(asize));
    return true;
  }

  virtual uint getLastCost() {return 3;}; // less recommended operation
};

//  array.add_item #in_reg, value : add an item to array, reg-only
class sgpFuncArrayAddItem: public sgpFunction {
public:
  virtual scString getName() const { return "array.add_item"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfRegister, gdtfArray, output);
    addArgMeta(gatfInput, gatfAny, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1;

    uint outRegNo = m_machine->getRegisterNo(args[0]);
    m_machine->evaluateArg(args[1], arg1);
        
    m_machine->arrayAddItem(outRegNo, arg1);
    return true;
  }
};

//  array.erase_item #in_reg, index : remove item from array, reg-only
class sgpFuncArrayEraseItem: public sgpFunction {
public:
  virtual scString getName() const { return "array.erase_item"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfRegister, gdtfArray, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1;

    uint outRegNo = m_machine->getRegisterNo(args[0]);
    m_machine->evaluateArg(args[1], arg1);
        
    m_machine->arrayEraseItem(outRegNo, arg1.getAsUInt());
    return true;
  }
};

//  array.set_item #in_reg, index, value : remove item from array, reg-only
class sgpFuncArraySetItem: public sgpFunction {
public:
  virtual scString getName() const { return "array.set_item"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfRegister, gdtfArray, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts, output);
    addArgMeta(gatfInput, gatfAny, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;

    uint outRegNo = m_machine->getRegisterNo(args[0]);
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);
        
    m_machine->arraySetItem(outRegNo, arg1.getAsUInt(), arg2);
    return true;
  }
};

//  array.get_item #out, #in_reg, index : returns item from array
class sgpFuncArrayGetItem: public sgpFunction {
public:
  virtual scString getName() const { return "array.get_item"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfRegister, gdtfArray, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg2, outValue;

    uint inRegNo = m_machine->getRegisterNo(args[1]);
    m_machine->evaluateArg(args[2], arg2);
                
    m_machine->arrayGetItem(inRegNo, arg2.getAsUInt(), outValue);
    m_machine->setLValue(args[0], outValue);
    
    return true;
  }
};

//  array.index_of #out, #in_reg, value : returns index of value in array
class sgpFuncArrayIndexOf: public sgpFunction {
public:
  virtual scString getName() const { return "array.index_of"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfRegister, gdtfArray, output);
    addArgMeta(gatfInput, gatfAny, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg2, outValue;

    uint inRegNo = m_machine->getRegisterNo(args[1]);
    m_machine->evaluateArg(args[2], arg2);
                
    long64 idx = m_machine->arrayIndexOf(inRegNo, arg2);
    m_machine->setLValue(args[0], scDataNode(idx));
    
    return true;
  }
};

//  array.merge #out, #in_reg1, #in_reg2 : returns merged arrays, types must be equal
class sgpFuncArrayMerge: public sgpFunction {
public:
  virtual scString getName() const { return "array.merge"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfRegister, gdtfArray, output);
    addArgMeta(gatfInput, gatfRegister, gdtfArray, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg2, outValue;

    uint inRegNo1 = m_machine->getRegisterNo(args[1]);
    uint inRegNo2 = m_machine->getRegisterNo(args[2]);
                                
    m_machine->arrayMerge(inRegNo1, inRegNo2, outValue);
    m_machine->setLValue(args[0], outValue);
    
    return true;
  }
};

//  array.range #out, #in_reg1, start[, size] : returns merged arrays, types must be equal
class sgpFuncArrayRange: public sgpFunction {
public:
  virtual scString getName() const { return "array.range"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 3; a_max = 4; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfRegister, gdtfArray, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg2, arg3, outValue;
    ulong64 aStart, aSize;

    m_machine->evaluateArg(args[2], arg2);
    uint inRegNo1 = m_machine->getRegisterNo(args[1]);
    aStart = arg2.getAsUInt64();
    if (args.size() > 3) {
      m_machine->evaluateArg(args[3], arg3);
      aSize = arg3.getAsUInt64();
    } else {
      aSize = 0;
    }  
    
    m_machine->arrayRange(inRegNo1, aStart, aSize, outValue);
    m_machine->setLValue(args[0], outValue);
    
    return true;
  }
};


// ----------------------------------------------------------------------------
// struct functions
// ----------------------------------------------------------------------------

//  struct.init #out : initialization of struct, clears contents
class sgpFuncStructInit: public sgpFunction {
public:
  virtual scString getName() const { return "struct.init"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode outValue;
    outValue.setAsParent(); 
    m_machine->setLValue(args[0], outValue);
    return true;
  }
};

//  struct.size #out, #in_reg : returns size of array
class sgpFuncStructSize: public sgpFunction {
public:
  virtual scString getName() const { return "struct.size"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfRegister, gdtfStruct, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;

    uint inRegNo = m_machine->getRegisterNo(args[1]);
    ulong64 asize = m_machine->structGetSize(inRegNo);
    m_machine->setLValue(args[0], scDataNode(asize));
    return true;
  }

  virtual uint getLastCost() {return 3;}; // less recommended operation
};

//  struct.add_item #in_reg, value[, name] : add an item to array, reg-only
class sgpFuncStructAddItem: public sgpFunction {
public:
  virtual scString getName() const { return "struct.add_item"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 2; a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfRegister, gdtfStruct, output);
    addArgMeta(gatfInput, gatfAny, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfString+gdtfVariant+gdtfAllBaseXInts, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1;

    uint outRegNo = m_machine->getRegisterNo(args[0]);
    m_machine->evaluateArg(args[1], arg1);
    scString name;
    
    if (args.size() > 2) {
      scDataNode arg2;
      m_machine->evaluateArg(args[2], arg2);
      name = arg2.getAsString();
    }          

    if (name.empty())
      m_machine->structAddItem(outRegNo, arg1);
    else
      m_machine->structAddItem(outRegNo, name, arg1);

    return true;
  }
};

//  struct.erase_item #in_reg, index : remove item from struct
class sgpFuncStructEraseItem: public sgpFunction {
public:
  virtual scString getName() const { return "struct.erase_item"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfRegister, gdtfStruct, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts+gdtfString, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1;

    uint outRegNo = m_machine->getRegisterNo(args[0]);
    m_machine->evaluateArg(args[1], arg1);

    if ((m_machine->calcDataType(arg1) & gdtfAllBaseXInts) == 0)
      m_machine->structEraseItem(outRegNo, arg1.getAsString());
    else  
      m_machine->structEraseItem(outRegNo, arg1.getAsUInt64());
    return true;
  }
};

//  struct.set_item #in_reg, index, value : change value of item in struct
class sgpFuncStructSetItem: public sgpFunction {
public:
  virtual scString getName() const { return "struct.set_item"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfRegister, gdtfStruct, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts+gdtfString, output);
    addArgMeta(gatfInput, gatfAny, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;

    uint outRegNo = m_machine->getRegisterNo(args[0]);
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);

    if ((m_machine->calcDataType(arg1) & gdtfAllBaseXInts) == 0)
      m_machine->structSetItem(outRegNo, arg1.getAsString(), arg2);
    else  
      m_machine->structSetItem(outRegNo, arg1.getAsUInt64(), arg2);
    return true;
  }
};

//  struct.get_item #out, #in_reg, index : returns item from struct
class sgpFuncStructGetItem: public sgpFunction {
public:
  virtual scString getName() const { return "struct.get_item"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfRegister, gdtfStruct, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts+gdtfString, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg2, outValue;

    uint inRegNo = m_machine->getRegisterNo(args[1]);
    m_machine->evaluateArg(args[2], arg2);

    if ((m_machine->calcDataType(arg2) & gdtfAllBaseXInts) == 0)
      m_machine->structGetItem(inRegNo, arg2.getAsString(), outValue);
    else  
      m_machine->structGetItem(inRegNo, arg2.getAsUInt(), outValue);

    m_machine->setLValue(args[0], outValue);    
    return true;
  }
};

//  struct.index_of #out, #in_reg, value : returns index of value in array
class sgpFuncStructIndexOf: public sgpFunction {
public:
  virtual scString getName() const { return "struct.index_of"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfRegister, gdtfStruct, output);
    addArgMeta(gatfInput, gatfAny, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg2, outValue;

    uint inRegNo = m_machine->getRegisterNo(args[1]);
    m_machine->evaluateArg(args[2], arg2);
                
    long64 idx = m_machine->structIndexOf(inRegNo, arg2);
    m_machine->setLValue(args[0], scDataNode(idx));
    
    return true;
  }
};

//  struct.merge #out, #in_reg1, #in_reg2 : returns merged arrays, types must be equal
class sgpFuncStructMerge: public sgpFunction {
public:
  virtual scString getName() const { return "struct.merge"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfRegister, gdtfStruct, output);
    addArgMeta(gatfInput, gatfRegister, gdtfStruct, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg2, outValue;

    uint inRegNo1 = m_machine->getRegisterNo(args[1]);
    uint inRegNo2 = m_machine->getRegisterNo(args[2]);
                                
    m_machine->structMerge(inRegNo1, inRegNo2, outValue);
    m_machine->setLValue(args[0], outValue);
    
    return true;
  }
};

//  struct.range #out, #in_reg1, start[, size] : returns merged arrays, types must be equal
class sgpFuncStructRange: public sgpFunction {
public:
  virtual scString getName() const { return "struct.range"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 3; a_max = 4; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfRegister, gdtfStruct, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg2, arg3, outValue;
    ulong64 aStart, aSize;

    m_machine->evaluateArg(args[2], arg2);
    uint inRegNo1 = m_machine->getRegisterNo(args[1]);
    aStart = arg2.getAsUInt64();
    if (args.size() > 3) {
      m_machine->evaluateArg(args[3], arg3);
      aSize = arg3.getAsUInt64();
    } else {
      aSize = 0;
    }  
    
    m_machine->structRange(inRegNo1, aStart, aSize, outValue);
    m_machine->setLValue(args[0], outValue);
    
    return true;
  }
};

//  struct.get_item_name #out, #in_reg1, index : returns name of item
class sgpFuncStructGetItemName: public sgpFunction {
public:
  virtual scString getName() const { return "struct.get_item_name"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 3; a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfRegister, gdtfStruct, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseUInts, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg2;
    ulong64 aStart;

    m_machine->evaluateArg(args[2], arg2);
    uint inRegNo1 = m_machine->getRegisterNo(args[1]);
    aStart = arg2.getAsUInt64();
    
    scString outValueS;
    m_machine->structGetItemName(inRegNo1, aStart, outValueS);
    m_machine->setLValue(args[0], scDataNode(outValueS));
    
    return true;
  }
};

// ----------------------------------------------------------------------------
// string functions
// ----------------------------------------------------------------------------

//  string.size #out, value : returns length of string
class sgpFuncStringSize: public sgpFunction {
public:
  virtual scString getName() const { return "string.size"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllScalars+gdtfString, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1;
    m_machine->evaluateArg(args[1], arg1);
    ulong64 asize = arg1.getAsString().length();
    m_machine->setLValue(args[0], scDataNode(asize));
    return true;
  }
};

// string.merge #out, value1, value2
class sgpFuncStringMerge: public sgpFunction {
public:
  virtual scString getName() const { return "string.merge"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllScalars+gdtfString, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllScalars+gdtfString, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);
    scString outValue = arg1.getAsString() + arg2.getAsString();
    m_machine->setLValue(args[0], scDataNode(outValue));
    return true;
  }
};

//  string.get_char #out, index, value : returns nth char as byte
class sgpFuncStringGetChar: public sgpFunction {
public:
  virtual scString getName() const { return "string.get_char"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllScalars+gdtfString, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);
    
    ulong64 pos = arg1.getAsUInt64();
    scString inValue = arg2.getAsString();
    byte outValue = inValue[int(pos)];
    
    m_machine->setLValue(args[0], scDataNode(outValue));
    return true;
  }
};

//  string.set_char #out, index, value, new_char_val : modifies nth char
class sgpFuncStringSetChar: public sgpFunction {
public:
  virtual scString getName() const { return "string.set_char"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 4; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllScalars+gdtfString, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2, arg3;
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);
    m_machine->evaluateArg(args[3], arg3);
    
    int pos = arg1.getAsInt();
    scString inValue = arg2.getAsString();
    byte newValue = arg3.getAsByte();
    
    inValue[pos] = newValue;
    
    m_machine->setLValue(args[0], scDataNode(inValue));
    return true;
  }
};

//  string.range #out, value, pos[, size] : returns part of string (substring)
class sgpFuncStringRange: public sgpFunction {
public:
  virtual scString getName() const { return "string.range"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 3; a_max = 4; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllScalars+gdtfString, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    ulong64 size;
    scDataNode arg1, arg2; 
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);

    scString inValue = arg1.getAsString();
    
    if (args.size() > 3) {
      scDataNode arg3;
      m_machine->evaluateArg(args[3], arg3);
      size = arg3.getAsUInt64();
    } else {
      size = inValue.length();
    }  
    
    ulong64 pos = arg2.getAsUInt64();
    scString outValue = inValue.substr(pos, size);
    
    m_machine->setLValue(args[0], scDataNode(outValue));
    return true;
  }
};

//  string.find #out, value, find_text, pos : returns position of find_text in value
class sgpFuncStringFind: public sgpFunction {
public:
  virtual scString getName() const { return "string.find"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 3; a_max = 4; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllScalars+gdtfString, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllScalars+gdtfString, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2, arg3;
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);
    
    ulong64 pos = 0;
    
    if (args.size() > 3) {
      m_machine->evaluateArg(args[3], arg3);
      pos = arg3.getAsUInt64();
    }  

    scString inValue = arg1.getAsString();
    scString findValue = arg2.getAsString();
    
    size_t findPos = inValue.find(findValue, pos);
    
    long64 outValue = ((findPos != scString::npos)?findPos:-1);
    
    m_machine->setLValue(args[0], scDataNode(outValue));
    return true;
  }
};

//  string.replace #out, value, find_text, new_text : replace text
class sgpFuncStringReplace: public sgpFunction {
public:
  virtual scString getName() const { return "string.replace"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 4; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllScalars+gdtfString, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllScalars+gdtfString, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllScalars+gdtfString, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2, arg3;
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);
    m_machine->evaluateArg(args[3], arg3);
    
    scString inValue = arg1.getAsString();
    scString findValue = arg2.getAsString();
    scString newValue = arg3.getAsString();
    
    //inValue.Replace(findValue, newValue, true);
    strReplaceThis(inValue, findValue, newValue, true);
    
    m_machine->setLValue(args[0], scDataNode(inValue));
    return true;
  }
};

// ----------------------------------------------------------------------------
// vector functions
// ----------------------------------------------------------------------------
//TODO: implement vector.* functions in seperate module with possibility to use optimized libs

//  vector.min #out, #in_reg: find minimum value in array
class sgpFuncVectorMinValue: public sgpFunction {
public:
  virtual scString getName() const { return "vector.min"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfArray, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode outValue;
    uint inRegNo = m_machine->getRegisterNo(args[1]);

    m_machine->vectorFindMinValue(inRegNo, outValue);        
    m_machine->setLValue(args[0], scDataNode(outValue));
    return true;
  }
};

//  vector.max #out, #in_reg: find maximum value in array
class sgpFuncVectorMaxValue: public sgpFunction {
public:
  virtual scString getName() const { return "vector.max"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfArray, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode outValue;
    uint inRegNo = m_machine->getRegisterNo(args[1]);

    m_machine->vectorFindMaxValue(inRegNo, outValue);        
    m_machine->setLValue(args[0], scDataNode(outValue));
    return true;
  }
};

//  vector.sum #out, #in_reg: find sum of values in array
class sgpFuncVectorSum: public sgpFunction {
public:
  virtual scString getName() const { return "vector.sum"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfArray, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode outValue;
    uint inRegNo = m_machine->getRegisterNo(args[1]);

    m_machine->vectorSum(inRegNo, outValue);        
    m_machine->setLValue(args[0], scDataNode(outValue));
    return true;
  }
};

//  vector.avg #out, #in_reg: find average of values in array
class sgpFuncVectorAvg: public sgpFunction {
public:
  virtual scString getName() const { return "vector.avg"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfArray, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode outValue;
    uint inRegNo = m_machine->getRegisterNo(args[1]);

    m_machine->vectorAvg(inRegNo, outValue);        
    m_machine->setLValue(args[0], scDataNode(outValue));
    return true;
  }
};

//  vector.sort #in_reg: sort items of vector
class sgpFuncVectorSort: public sgpFunction {
public:
  virtual scString getName() const { return "vector.sort"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput+gatfInput, gatfAny, gdtfArray, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode outValue;
    uint inRegNo = m_machine->getRegisterNo(args[0]);

    m_machine->vectorSort(inRegNo);        
    return true;
  }
};

//  vector.distinct #in_reg: make items of vector unique
class sgpFuncVectorDistinct: public sgpFunction {
public:
  virtual scString getName() const { return "vector.distinct"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput+gatfInput, gatfAny, gdtfArray, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode outValue;
    uint inRegNo = m_machine->getRegisterNo(args[0]);

    m_machine->vectorDistinct(inRegNo);        
    return true;
  }
};

//  vector.norm #in_reg: normalize vector
class sgpFuncVectorNorm: public sgpFunction {
public:
  virtual scString getName() const { return "vector.norm"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput+gatfInput, gatfAny, gdtfArray, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode outValue;
    uint inRegNo = m_machine->getRegisterNo(args[0]);

    m_machine->vectorNorm(inRegNo);        
    return true;
  }
};

//  vector.dot_product #out, #in_reg1, #in_reg2: calculate dot product of two vectors
class sgpFuncVectorDotProduct: public sgpFunction {
public:
  virtual scString getName() const { return "vector.dot_product"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfArray, output);
    addArgMeta(gatfInput, gatfAny, gdtfArray, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode outValue;
    uint inRegNo1 = m_machine->getRegisterNo(args[1]);
    uint inRegNo2 = m_machine->getRegisterNo(args[2]);

    m_machine->vectorDotProduct(inRegNo1, inRegNo2, outValue);        
    m_machine->setLValue(args[0], scDataNode(outValue));
    return true;
  }
};

//  vector.std_dev #out, #in_reg1: calculate standard dev 
class sgpFuncVectorStdDev: public sgpFunction {
public:
  virtual scString getName() const { return "vector.std_dev"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfArray, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode outValue;
    uint inRegNo1 = m_machine->getRegisterNo(args[1]);

    m_machine->vectorStdDev(inRegNo1, outValue);        
    m_machine->setLValue(args[0], scDataNode(outValue));
    return true;
  }
};

// ----------------------------------------------------------------------------
// dates
// ----------------------------------------------------------------------------

//  date.nowf #out : returns current date & time in floating point format
class sgpFuncDateNowf: public sgpFunction {
public:
  virtual scString getName() const { return "date.nowf"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode outValue(dtp::currentDateTime());
    m_machine->setLValue(args[0], outValue);
    return true;
  }
};

//  date.nows #out : returns current date & time in string format
class sgpFuncDateNows: public sgpFunction {
public:
  virtual scString getName() const { return "date.nows"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode outValue(dateTimeToIsoStr(dtp::currentDateTime()));
    m_machine->setLValue(args[0], outValue);
    return true;
  }
};

//  date.nowi #out : returns current date & time in ticks (uint64) format
class sgpFuncDateNowi: public sgpFunction {
public:
  virtual scString getName() const { return "date.nowi"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode outValue(cpu_time_ms());
    m_machine->setLValue(args[0], outValue);
    return true;
  }
};

//  date.datei #out : returns current date as yymmdd (uint) format
class sgpFuncDateDatei: public sgpFunction {
public:
  virtual scString getName() const { return "date.datei"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    uint oyear, omon, oday, ohour, omin, osecs;
    scDateTime now = dtp::currentDateTime();
    decodeDateTime(now, oyear, omon, oday, ohour, omin, osecs);
    
    scDataNode outValue(uint(oyear*10000+omon*100+oday));
    m_machine->setLValue(args[0], outValue);
    return true;
  }
};

// ----------------------------------------------------------------------------
// rand.* - random values
// ----------------------------------------------------------------------------

//  rand.init : initialize random number generator
class sgpFuncRandInit: public sgpFunction {
public:
  virtual scString getName() const { return "rand.init"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 0; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    m_machine->setRandomInit(true);
    return true;
  }
};

//  rand.randomf #out : returns random value 0..1
class sgpFuncRandRandomf: public sgpFunction {
public:
  virtual scString getName() const { return "rand.randomf"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode outValue(randomDouble(0, 1.0));
    m_machine->setLValue(args[0], outValue);
    return true;
  }

  virtual uint getLastCost() {return 3;}; 
};

//  rand.randomi #out, min, max : returns random integer value min..max
class sgpFuncRandRandomi: public sgpFunction {
public:
  virtual scString getName() const { return "rand.randomi"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfInt+gdtfByte, output);
    addArgMeta(gatfInput, gatfAny, gdtfInt+gdtfByte, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;
    
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);
    
    
    scDataNode outValue(randomInt(arg1.getAsInt(), arg2.getAsInt()));
    m_machine->setLValue(args[0], outValue);
    return true;
  }

  virtual uint getLastCost() {return 3;}; 
};

// ----------------------------------------------------------------------------
// block.*
// ----------------------------------------------------------------------------
// Dynamic code blocks support

class sgpFuncBlockBase: public sgpFunction {
public:
  virtual uint getLastCost() {return 10;}; 
};

//  block.add #out: creates new block, returns it's id or zero if failed
class sgpFuncBlockAdd: public sgpFuncBlockBase {
public:
  virtual scString getName() const { return "block.add"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    uint res = m_machine->blockAdd();    
    scDataNode outValue(res);
    m_machine->setLValue(args[0], outValue);
    return (res > 0);
  }
};

//  block.init #out, block_no: initialize block code, block_no must be != active, returns action result as bool
class sgpFuncBlockInit: public sgpFuncBlockBase {
public:
  virtual scString getName() const { return "block.init"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1;
    m_machine->evaluateArg(args[1], arg1);
    bool res = m_machine->blockInit(arg1.getAsUInt());    
    scDataNode outValue(res);
    m_machine->setLValue(args[0], outValue);
    return res;
  }
};

//  block.erase block_no: remove block of code, block_no must be != active
class sgpFuncBlockErase: public sgpFuncBlockBase {
public:
  virtual scString getName() const { return "block.erase"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1;
    m_machine->evaluateArg(args[0], arg1);
    bool res = m_machine->blockErase(arg1.getAsUInt());    
    return res;
  }
};

//  block.size #out, block_no: returns size of a given block 
class sgpFuncBlockSize: public sgpFuncBlockBase {
public:
  virtual scString getName() const { return "block.size"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseXInts, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1;
    m_machine->evaluateArg(args[1], arg1);
    cell_size_t bsize = m_machine->blockGetSize(arg1.getAsUInt());    
    scDataNode outValue(bsize);
    m_machine->setLValue(args[0], outValue);    
    return true;
  }
};

//  block.count #out: returns number of defined blocks
class sgpFuncBlockCount: public sgpFuncBlockBase {
public:
  virtual scString getName() const { return "block.count"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    uint cnt = m_machine->blockGetCount();    
    scDataNode outValue(cnt);
    m_machine->setLValue(args[0], outValue);    
    return true;
  }
};

//  block.active_id #out: returns id of active block
class sgpFuncBlockActiveId: public sgpFuncBlockBase {
public:
  virtual scString getName() const { return "block.active_id"; };
  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    return true;
  }
  
  virtual bool execute(const scDataNode &args) const {
    uint bid = m_machine->blockActiveId();    
    scDataNode outValue(bid);
    m_machine->setLValue(args[0], outValue);    
    return true;
  }
};

// ----------------------------------------------------------------------------
// functions - rest
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// function factories
// ----------------------------------------------------------------------------
typedef std::map<scString,int> sgpFFFuncMap; 

const int GASM_INSTR_CODE_NOOP = 10;     
const int GASM_INSTR_CODE_MOVE = 20;
const int GASM_INSTR_CODE_INIT = 21;
const int GASM_INSTR_CODE_CALL = 30;
const int GASM_INSTR_CODE_RETURN = 40;
const int GASM_INSTR_CODE_SET_NEXT_BLOCK = 41;
const int GASM_INSTR_CODE_SET_NEXT_BLOCK_REL = 42;
const int GASM_INSTR_CODE_CLR_NEXT_BLOCK = 43;
const int GASM_INSTR_CODE_PUSH_NEXT_RESULT = 45;
const int GASM_INSTR_CODE_PUSH = 46;
const int GASM_INSTR_CODE_POP = 47;
const int GASM_INSTR_CODE_DATA = 50;
const int GASM_INSTR_CODE_REF_BUILD = 60;
const int GASM_INSTR_CODE_REF_CLEAR = 70;
const int GASM_INSTR_CODE_REF_IS_REF = 71;
const int GASM_INSTR_CODE_DEFINE = 80;
const int GASM_INSTR_CODE_CAST = 90;
const int GASM_INSTR_CODE_SKIPIFN = 100;
const int GASM_INSTR_CODE_SKIPIF = 101;
const int GASM_INSTR_CODE_SKIP = 110;
const int GASM_INSTR_CODE_JUMP_BACK = 120;
const int GASM_INSTR_CODE_REPNZ = 121;
const int GASM_INSTR_CODE_REPNZ_BACK = 122;
const int GASM_INSTR_CODE_RTTI_DATA_TYPE = 130;
const int GASM_INSTR_CODE_LOGIC_IIF = 140;
const int GASM_INSTR_CODE_LOGIC_AND = 150;
const int GASM_INSTR_CODE_LOGIC_OR = 160;
const int GASM_INSTR_CODE_LOGIC_XOR = 170;
const int GASM_INSTR_CODE_LOGIC_NOT = 180;
const int GASM_INSTR_CODE_EQU_INT = 190;
const int GASM_INSTR_CODE_EQU_INT64 = 200;
const int GASM_INSTR_CODE_EQU_BYTE = 210;
const int GASM_INSTR_CODE_EQU_UINT = 220;
const int GASM_INSTR_CODE_EQU_UINT64 = 230;
const int GASM_INSTR_CODE_EQU_FLOAT = 240;
const int GASM_INSTR_CODE_EQU_DOUBLE = 250;
const int GASM_INSTR_CODE_EQU_XDOUBLE = 260;
const int GASM_INSTR_CODE_EQU_STRING = 270;
const int GASM_INSTR_CODE_EQU_BOOL = 280;
const int GASM_INSTR_CODE_EQU_VARIANT = 290;
const int GASM_INSTR_CODE_GT_INT = 300;
const int GASM_INSTR_CODE_GT_INT64 = 310;
const int GASM_INSTR_CODE_GT_BYTE = 320;
const int GASM_INSTR_CODE_GT_UINT = 330;
const int GASM_INSTR_CODE_GT_UINT64 = 340;
const int GASM_INSTR_CODE_GT_FLOAT = 350;
const int GASM_INSTR_CODE_GT_DOUBLE = 360;
const int GASM_INSTR_CODE_GT_XDOUBLE = 370;
const int GASM_INSTR_CODE_GT_STRING = 380;
const int GASM_INSTR_CODE_GT_BOOL = 390;
const int GASM_INSTR_CODE_GT_VARIANT = 400;
const int GASM_INSTR_CODE_GTE_INT = 410;
const int GASM_INSTR_CODE_GTE_INT64 = 420;
const int GASM_INSTR_CODE_GTE_BYTE = 430;
const int GASM_INSTR_CODE_GTE_UINT = 440;
const int GASM_INSTR_CODE_GTE_UINT64 = 450;
const int GASM_INSTR_CODE_GTE_FLOAT = 460;
const int GASM_INSTR_CODE_GTE_DOUBLE = 470;
const int GASM_INSTR_CODE_GTE_XDOUBLE = 480;
const int GASM_INSTR_CODE_GTE_STRING = 490;
const int GASM_INSTR_CODE_GTE_BOOL = 500;
const int GASM_INSTR_CODE_GTE_VARIANT = 510;
const int GASM_INSTR_CODE_LT_INT = 520;
const int GASM_INSTR_CODE_LT_INT64 = 530;
const int GASM_INSTR_CODE_LT_BYTE = 540;
const int GASM_INSTR_CODE_LT_UINT = 550;
const int GASM_INSTR_CODE_LT_UINT64 = 560;
const int GASM_INSTR_CODE_LT_FLOAT = 570;
const int GASM_INSTR_CODE_LT_DOUBLE = 580;
const int GASM_INSTR_CODE_LT_XDOUBLE = 590;
const int GASM_INSTR_CODE_LT_STRING = 600;
const int GASM_INSTR_CODE_LT_BOOL = 610;
const int GASM_INSTR_CODE_LT_VARIANT = 620;
const int GASM_INSTR_CODE_LTE_INT = 630;
const int GASM_INSTR_CODE_LTE_INT64 = 640;
const int GASM_INSTR_CODE_LTE_BYTE = 650;
const int GASM_INSTR_CODE_LTE_UINT = 660;
const int GASM_INSTR_CODE_LTE_UINT64 = 670;
const int GASM_INSTR_CODE_LTE_FLOAT = 680;
const int GASM_INSTR_CODE_LTE_DOUBLE = 690;
const int GASM_INSTR_CODE_LTE_XDOUBLE = 700;
const int GASM_INSTR_CODE_LTE_STRING = 710;
const int GASM_INSTR_CODE_LTE_BOOL = 720;
const int GASM_INSTR_CODE_LTE_VARIANT = 730;
const int GASM_INSTR_CODE_SAME = 740;
const int GASM_INSTR_CODE_CMP_INT = 750;
const int GASM_INSTR_CODE_CMP_INT64 = 760;
const int GASM_INSTR_CODE_CMP_BYTE = 770;
const int GASM_INSTR_CODE_CMP_UINT = 780;
const int GASM_INSTR_CODE_CMP_UINT64 = 790;
const int GASM_INSTR_CODE_CMP_FLOAT = 800;
const int GASM_INSTR_CODE_CMP_DOUBLE = 810;
const int GASM_INSTR_CODE_CMP_XDOUBLE = 820;
const int GASM_INSTR_CODE_CMP_STRING = 830;
const int GASM_INSTR_CODE_CMP_VARIANT = 840;
const int GASM_INSTR_CODE_ADD_INT = 850;
const int GASM_INSTR_CODE_ADD_INT64 = 860;
const int GASM_INSTR_CODE_ADD_BYTE = 870;
const int GASM_INSTR_CODE_ADD_UINT = 880;
const int GASM_INSTR_CODE_ADD_UINT64 = 890;
const int GASM_INSTR_CODE_ADD_FLOAT = 900;
const int GASM_INSTR_CODE_ADD_DOUBLE = 910;
const int GASM_INSTR_CODE_ADD_XDOUBLE = 920;
const int GASM_INSTR_CODE_SUB_INT = 930;
const int GASM_INSTR_CODE_SUB_INT64 = 940;
const int GASM_INSTR_CODE_SUB_BYTE = 950;
const int GASM_INSTR_CODE_SUB_UINT = 960;
const int GASM_INSTR_CODE_SUB_UINT64 = 970;
const int GASM_INSTR_CODE_SUB_FLOAT = 980;
const int GASM_INSTR_CODE_SUB_DOUBLE = 990;
const int GASM_INSTR_CODE_SUB_XDOUBLE = 1000;
const int GASM_INSTR_CODE_MULT_INT = 1010;
const int GASM_INSTR_CODE_MULT_INT64 = 1020;
const int GASM_INSTR_CODE_MULT_BYTE = 1030;
const int GASM_INSTR_CODE_MULT_UINT = 1040;
const int GASM_INSTR_CODE_MULT_UINT64 = 1050;
const int GASM_INSTR_CODE_MULT_FLOAT = 1060;
const int GASM_INSTR_CODE_MULT_DOUBLE = 1070;
const int GASM_INSTR_CODE_MULT_XDOUBLE = 1080;
const int GASM_INSTR_CODE_DIV_INT = 1090;
const int GASM_INSTR_CODE_DIV_INT64 = 1100;
const int GASM_INSTR_CODE_DIV_BYTE = 1110;
const int GASM_INSTR_CODE_DIV_UINT = 1120;
const int GASM_INSTR_CODE_DIV_UINT64 = 1130;
const int GASM_INSTR_CODE_DIV_FLOAT = 1140;
const int GASM_INSTR_CODE_DIV_DOUBLE = 1150;
const int GASM_INSTR_CODE_DIV_XDOUBLE = 1160;

const int GASM_INSTR_CODE_INVS_FLOAT = 1161;
const int GASM_INSTR_CODE_INVS_DOUBLE = 1162;
const int GASM_INSTR_CODE_INVS_XDOUBLE = 1163;

const int GASM_INSTR_CODE_MULTDIV_INT = 1170;
const int GASM_INSTR_CODE_MULTDIV_BYTE = 1180;
const int GASM_INSTR_CODE_MULTDIV_UINT = 1190;
const int GASM_INSTR_CODE_MULTDIV_FLOAT = 1200;
const int GASM_INSTR_CODE_MULTDIV_DOUBLE = 1210;
const int GASM_INSTR_CODE_POW_UINT = 1211;
const int GASM_INSTR_CODE_MOD_INT = 1220;
const int GASM_INSTR_CODE_MOD_INT64 = 1230;
const int GASM_INSTR_CODE_MOD_BYTE = 1240;
const int GASM_INSTR_CODE_MOD_UINT = 1250;
const int GASM_INSTR_CODE_MOD_UINT64 = 1260;
const int GASM_INSTR_CODE_BIT_AND_BYTE = 1270;
const int GASM_INSTR_CODE_BIT_AND_UINT = 1280;
const int GASM_INSTR_CODE_BIT_AND_UINT64 = 1290;
const int GASM_INSTR_CODE_BIT_OR_BYTE = 1300;
const int GASM_INSTR_CODE_BIT_OR_UINT = 1310;
const int GASM_INSTR_CODE_BIT_OR_UINT64 = 1320;
const int GASM_INSTR_CODE_BIT_XOR_BYTE = 1330;
const int GASM_INSTR_CODE_BIT_XOR_UINT = 1340;
const int GASM_INSTR_CODE_BIT_XOR_UINT64 = 1350;
const int GASM_INSTR_CODE_BIT_NOT_BYTE = 1360;
const int GASM_INSTR_CODE_BIT_NOT_UINT = 1370;
const int GASM_INSTR_CODE_BIT_NOT_UINT64 = 1380;
const int GASM_INSTR_CODE_BIT_SHR_BYTE = 1390;
const int GASM_INSTR_CODE_BIT_SHR_UINT = 1400;
const int GASM_INSTR_CODE_BIT_SHR_UINT64 = 1410;
const int GASM_INSTR_CODE_BIT_SHL_BYTE = 1420;
const int GASM_INSTR_CODE_BIT_SHL_UINT = 1430;
const int GASM_INSTR_CODE_BIT_SHL_UINT64 = 1440;
const int GASM_INSTR_CODE_ABS_INT = 1450;
const int GASM_INSTR_CODE_ABS_INT64 = 1460;
const int GASM_INSTR_CODE_ABS_FLOAT = 1470;
const int GASM_INSTR_CODE_ABS_DOUBLE = 1480;
const int GASM_INSTR_CODE_ABS_XDOUBLE = 1490;

const int GASM_INSTR_CODE_NEG_INT = 1491;
const int GASM_INSTR_CODE_NEG_INT64 = 1492;
const int GASM_INSTR_CODE_NEG_FLOAT = 1493;
const int GASM_INSTR_CODE_NEG_DOUBLE = 1494;
const int GASM_INSTR_CODE_NEG_XDOUBLE = 1495;

const int GASM_INSTR_CODE_SGN_INT = 1500;
const int GASM_INSTR_CODE_SGN_INT64 = 1510;
const int GASM_INSTR_CODE_SGN_FLOAT = 1520;
const int GASM_INSTR_CODE_SGN_DOUBLE = 1530;
const int GASM_INSTR_CODE_SGN_XDOUBLE = 1540;
const int GASM_INSTR_CODE_CEIL_FLOAT = 1550;
const int GASM_INSTR_CODE_CEIL_DOUBLE = 1560;
const int GASM_INSTR_CODE_CEIL_XDOUBLE = 1570;
const int GASM_INSTR_CODE_FLOOR_FLOAT = 1580;
const int GASM_INSTR_CODE_FLOOR_DOUBLE = 1590;
const int GASM_INSTR_CODE_FLOOR_XDOUBLE = 1600;
const int GASM_INSTR_CODE_FRAC_FLOAT = 1610;
const int GASM_INSTR_CODE_FRAC_DOUBLE = 1620;
const int GASM_INSTR_CODE_FRAC_XDOUBLE = 1630;
const int GASM_INSTR_CODE_TRUNC_FLOAT = 1640;
const int GASM_INSTR_CODE_TRUNC_DOUBLE = 1650;
const int GASM_INSTR_CODE_TRUNC_XDOUBLE = 1660;
const int GASM_INSTR_CODE_TRUNC64_FLOAT = 1670;
const int GASM_INSTR_CODE_TRUNC64_DOUBLE = 1680;
const int GASM_INSTR_CODE_TRUNC64_XDOUBLE = 1690;
const int GASM_INSTR_CODE_TRUNCF_FLOAT = 1700;
const int GASM_INSTR_CODE_TRUNCF_DOUBLE = 1710;
const int GASM_INSTR_CODE_TRUNCF_XDOUBLE = 1720;
const int GASM_INSTR_CODE_ROUND_FLOAT = 1730;
const int GASM_INSTR_CODE_ROUND_DOUBLE = 1740;
const int GASM_INSTR_CODE_ROUND_XDOUBLE = 1750;
const int GASM_INSTR_CODE_ROUND64_FLOAT = 1760;
const int GASM_INSTR_CODE_ROUND64_DOUBLE = 1770;
const int GASM_INSTR_CODE_ROUND64_XDOUBLE = 1780;
const int GASM_INSTR_CODE_ROUNDF_FLOAT = 1790;
const int GASM_INSTR_CODE_ROUNDF_DOUBLE = 1800;
const int GASM_INSTR_CODE_ROUNDF_XDOUBLE = 1810;
const int GASM_INSTR_CODE_ARRAY_INIT = 1811;
const int GASM_INSTR_CODE_ARRAY_DEFINE = 1820;
const int GASM_INSTR_CODE_ARRAY_SIZE = 1830;
const int GASM_INSTR_CODE_ARRAY_GET_ITEM = 1840;
const int GASM_INSTR_CODE_ARRAY_SET_ITEM = 1850;
const int GASM_INSTR_CODE_ARRAY_ADD_ITEM = 1860;
const int GASM_INSTR_CODE_ARRAY_ERASE_ITEM = 1870;
const int GASM_INSTR_CODE_ARRAY_INDEX_OF = 1880;
const int GASM_INSTR_CODE_ARRAY_MERGE = 1881;
const int GASM_INSTR_CODE_ARRAY_RANGE = 1882;
const int GASM_INSTR_CODE_STRUCT_INIT = 1883;
const int GASM_INSTR_CODE_STRUCT_SIZE = 1890;
const int GASM_INSTR_CODE_STRUCT_GET_ITEM = 1900;
const int GASM_INSTR_CODE_STRUCT_SET_ITEM = 1910;
const int GASM_INSTR_CODE_STRUCT_ADD_ITEM = 1920;
const int GASM_INSTR_CODE_STRUCT_ERASE_ITEM = 1930;
const int GASM_INSTR_CODE_STRUCT_INDEX_OF = 1940;
const int GASM_INSTR_CODE_STRUCT_MERGE = 1941;
const int GASM_INSTR_CODE_STRUCT_RANGE = 1942;
const int GASM_INSTR_CODE_STRUCT_GET_ITEM_NAME = 1943;
const int GASM_INSTR_CODE_STRING_SIZE = 1950;
const int GASM_INSTR_CODE_STRING_MERGE = 1960;
const int GASM_INSTR_CODE_STRING_SET_CHAR = 1970;
const int GASM_INSTR_CODE_STRING_GET_CHAR = 1980;
const int GASM_INSTR_CODE_STRING_RANGE = 1990;
const int GASM_INSTR_CODE_STRING_FIND = 2000;
const int GASM_INSTR_CODE_STRING_REPLACE = 2010;                  
const int GASM_INSTR_CODE_VECTOR_MIN = 2020;
const int GASM_INSTR_CODE_VECTOR_MAX = 2030;
const int GASM_INSTR_CODE_VECTOR_SUM = 2040;
const int GASM_INSTR_CODE_VECTOR_AVG = 2050;
const int GASM_INSTR_CODE_VECTOR_SORT = 2060;
const int GASM_INSTR_CODE_VECTOR_DISTINCT = 2070;
const int GASM_INSTR_CODE_VECTOR_NORM = 2071;
const int GASM_INSTR_CODE_VECTOR_DOT_PRODUCT = 2080;
const int GASM_INSTR_CODE_VECTOR_STD_DEV = 2090;
const int GASM_INSTR_CODE_DATE_NOWF = 2100;
const int GASM_INSTR_CODE_DATE_NOWS = 2110;
const int GASM_INSTR_CODE_DATE_NOWI = 2120;
const int GASM_INSTR_CODE_DATE_DATEI = 2130;
const int GASM_INSTR_CODE_RAND_INIT = 2140;
const int GASM_INSTR_CODE_RAND_RANDOMF = 2150;
const int GASM_INSTR_CODE_RAND_RANDOMI = 2160;
const int GASM_INSTR_CODE_BLOCK_ADD = 2170;
const int GASM_INSTR_CODE_BLOCK_INIT = 2171;
const int GASM_INSTR_CODE_BLOCK_ERASE = 2180;
const int GASM_INSTR_CODE_BLOCK_SIZE = 2190;
const int GASM_INSTR_CODE_BLOCK_COUNT = 2200;
const int GASM_INSTR_CODE_BLOCK_ACTIVE_ID = 2210;

class sgpFFCore: public sgpFunFactory {
public:
  void init() {
    m_funcMap.insert(std::make_pair<scString, int>("noop", GASM_INSTR_CODE_NOOP));
    m_funcMap.insert(std::make_pair<scString, int>("move", GASM_INSTR_CODE_MOVE));
    m_funcMap.insert(std::make_pair<scString, int>("init", GASM_INSTR_CODE_INIT));
    m_funcMap.insert(std::make_pair<scString, int>("call", GASM_INSTR_CODE_CALL));
    m_funcMap.insert(std::make_pair<scString, int>("return", GASM_INSTR_CODE_RETURN));

    m_funcMap.insert(std::make_pair<scString, int>("set_next_block", GASM_INSTR_CODE_SET_NEXT_BLOCK));    
    m_funcMap.insert(std::make_pair<scString, int>("set_next_block_rel", GASM_INSTR_CODE_SET_NEXT_BLOCK_REL));    
    m_funcMap.insert(std::make_pair<scString, int>("clr_next_block", GASM_INSTR_CODE_CLR_NEXT_BLOCK));    

    m_funcMap.insert(std::make_pair<scString, int>("push_next_result", GASM_INSTR_CODE_PUSH_NEXT_RESULT));        
    m_funcMap.insert(std::make_pair<scString, int>("push", GASM_INSTR_CODE_PUSH));        
    m_funcMap.insert(std::make_pair<scString, int>("pop", GASM_INSTR_CODE_POP));        
    
    m_funcMap.insert(std::make_pair<scString, int>("data", GASM_INSTR_CODE_DATA));
    m_funcMap.insert(std::make_pair<scString, int>("ref.build", GASM_INSTR_CODE_REF_BUILD));
    m_funcMap.insert(std::make_pair<scString, int>("ref.clear", GASM_INSTR_CODE_REF_CLEAR));
    m_funcMap.insert(std::make_pair<scString, int>("ref.is_ref", GASM_INSTR_CODE_REF_IS_REF));
    m_funcMap.insert(std::make_pair<scString, int>("define", GASM_INSTR_CODE_DEFINE));
    m_funcMap.insert(std::make_pair<scString, int>("cast", GASM_INSTR_CODE_CAST));
    m_funcMap.insert(std::make_pair<scString, int>("skip.ifn", GASM_INSTR_CODE_SKIPIFN));   
    m_funcMap.insert(std::make_pair<scString, int>("skip.if", GASM_INSTR_CODE_SKIPIF));
    m_funcMap.insert(std::make_pair<scString, int>("skip", GASM_INSTR_CODE_SKIP));
    m_funcMap.insert(std::make_pair<scString, int>("jump.back", GASM_INSTR_CODE_JUMP_BACK));
    m_funcMap.insert(std::make_pair<scString, int>("repnz", GASM_INSTR_CODE_REPNZ));
    m_funcMap.insert(std::make_pair<scString, int>("repnz.back", GASM_INSTR_CODE_REPNZ_BACK));    
    m_funcMap.insert(std::make_pair<scString, int>("rtti.data_type", GASM_INSTR_CODE_RTTI_DATA_TYPE));
    m_funcMap.insert(std::make_pair<scString, int>("logic.iif", GASM_INSTR_CODE_LOGIC_IIF));
    m_funcMap.insert(std::make_pair<scString, int>("logic.and", GASM_INSTR_CODE_LOGIC_AND));
    m_funcMap.insert(std::make_pair<scString, int>("logic.or", GASM_INSTR_CODE_LOGIC_OR));
    m_funcMap.insert(std::make_pair<scString, int>("logic.xor", GASM_INSTR_CODE_LOGIC_XOR));
    m_funcMap.insert(std::make_pair<scString, int>("logic.not", GASM_INSTR_CODE_LOGIC_NOT));
    m_funcMap.insert(std::make_pair<scString, int>("equ.int", GASM_INSTR_CODE_EQU_INT));
    m_funcMap.insert(std::make_pair<scString, int>("equ.int64", GASM_INSTR_CODE_EQU_INT64));
    m_funcMap.insert(std::make_pair<scString, int>("equ.byte", GASM_INSTR_CODE_EQU_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("equ.uint", GASM_INSTR_CODE_EQU_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("equ.uint64", GASM_INSTR_CODE_EQU_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("equ.float", GASM_INSTR_CODE_EQU_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("equ.double", GASM_INSTR_CODE_EQU_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("equ.xdouble", GASM_INSTR_CODE_EQU_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("equ.string", GASM_INSTR_CODE_EQU_STRING));
    m_funcMap.insert(std::make_pair<scString, int>("equ.bool", GASM_INSTR_CODE_EQU_BOOL));
    m_funcMap.insert(std::make_pair<scString, int>("equ.variant", GASM_INSTR_CODE_EQU_VARIANT));
    m_funcMap.insert(std::make_pair<scString, int>("gt.int", GASM_INSTR_CODE_GT_INT));
    m_funcMap.insert(std::make_pair<scString, int>("gt.int64", GASM_INSTR_CODE_GT_INT64));
    m_funcMap.insert(std::make_pair<scString, int>("gt.byte", GASM_INSTR_CODE_GT_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("gt.uint", GASM_INSTR_CODE_GT_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("gt.uint64", GASM_INSTR_CODE_GT_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("gt.float", GASM_INSTR_CODE_GT_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("gt.double", GASM_INSTR_CODE_GT_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("gt.xdouble", GASM_INSTR_CODE_GT_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("gt.string", GASM_INSTR_CODE_GT_STRING));
    m_funcMap.insert(std::make_pair<scString, int>("gt.bool", GASM_INSTR_CODE_GT_BOOL));
    m_funcMap.insert(std::make_pair<scString, int>("gt.variant", GASM_INSTR_CODE_GT_VARIANT));
    m_funcMap.insert(std::make_pair<scString, int>("gte.int", GASM_INSTR_CODE_GTE_INT));
    m_funcMap.insert(std::make_pair<scString, int>("gte.int64", GASM_INSTR_CODE_GTE_INT64));
    m_funcMap.insert(std::make_pair<scString, int>("gte.byte", GASM_INSTR_CODE_GTE_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("gte.uint", GASM_INSTR_CODE_GTE_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("gte.uint64", GASM_INSTR_CODE_GTE_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("gte.float", GASM_INSTR_CODE_GTE_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("gte.double", GASM_INSTR_CODE_GTE_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("gte.xdouble", GASM_INSTR_CODE_GTE_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("gte.string", GASM_INSTR_CODE_GTE_STRING));
    m_funcMap.insert(std::make_pair<scString, int>("gte.bool", GASM_INSTR_CODE_GTE_BOOL));
    m_funcMap.insert(std::make_pair<scString, int>("gte.variant", GASM_INSTR_CODE_GTE_VARIANT));
    m_funcMap.insert(std::make_pair<scString, int>("lt.int", GASM_INSTR_CODE_LT_INT));
    m_funcMap.insert(std::make_pair<scString, int>("lt.int64", GASM_INSTR_CODE_LT_INT64));
    m_funcMap.insert(std::make_pair<scString, int>("lt.byte", GASM_INSTR_CODE_LT_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("lt.uint", GASM_INSTR_CODE_LT_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("lt.uint64", GASM_INSTR_CODE_LT_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("lt.float", GASM_INSTR_CODE_LT_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("lt.double", GASM_INSTR_CODE_LT_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("lt.xdouble", GASM_INSTR_CODE_LT_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("lt.string", GASM_INSTR_CODE_LT_STRING));
    m_funcMap.insert(std::make_pair<scString, int>("lt.bool", GASM_INSTR_CODE_LT_BOOL));
    m_funcMap.insert(std::make_pair<scString, int>("lt.variant", GASM_INSTR_CODE_LT_VARIANT));
    m_funcMap.insert(std::make_pair<scString, int>("lse.int", GASM_INSTR_CODE_LTE_INT));
    m_funcMap.insert(std::make_pair<scString, int>("lse.int64", GASM_INSTR_CODE_LTE_INT64));
    m_funcMap.insert(std::make_pair<scString, int>("lse.byte", GASM_INSTR_CODE_LTE_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("lse.uint", GASM_INSTR_CODE_LTE_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("lse.uint64", GASM_INSTR_CODE_LTE_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("lse.float", GASM_INSTR_CODE_LTE_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("lse.double", GASM_INSTR_CODE_LTE_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("lse.xdouble", GASM_INSTR_CODE_LTE_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("lse.string", GASM_INSTR_CODE_LTE_STRING));
    m_funcMap.insert(std::make_pair<scString, int>("lse.bool", GASM_INSTR_CODE_LTE_BOOL));
    m_funcMap.insert(std::make_pair<scString, int>("lse.variant", GASM_INSTR_CODE_LTE_VARIANT));
    m_funcMap.insert(std::make_pair<scString, int>("same", GASM_INSTR_CODE_SAME));
    m_funcMap.insert(std::make_pair<scString, int>("cmp.int", GASM_INSTR_CODE_CMP_INT));
    m_funcMap.insert(std::make_pair<scString, int>("cmp.int64", GASM_INSTR_CODE_CMP_INT64));
    m_funcMap.insert(std::make_pair<scString, int>("cmp.byte", GASM_INSTR_CODE_CMP_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("cmp.uint", GASM_INSTR_CODE_CMP_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("cmp.uint64", GASM_INSTR_CODE_CMP_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("cmp.float", GASM_INSTR_CODE_CMP_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("cmp.double", GASM_INSTR_CODE_CMP_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("cmp.xdouble", GASM_INSTR_CODE_CMP_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("cmp.string", GASM_INSTR_CODE_CMP_STRING));
    m_funcMap.insert(std::make_pair<scString, int>("cmp.variant", GASM_INSTR_CODE_CMP_VARIANT));
    m_funcMap.insert(std::make_pair<scString, int>("add.int", GASM_INSTR_CODE_ADD_INT));
    m_funcMap.insert(std::make_pair<scString, int>("add.int64", GASM_INSTR_CODE_ADD_INT64));
    m_funcMap.insert(std::make_pair<scString, int>("add.byte", GASM_INSTR_CODE_ADD_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("add.uint", GASM_INSTR_CODE_ADD_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("add.uint64", GASM_INSTR_CODE_ADD_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("add.float", GASM_INSTR_CODE_ADD_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("add.double", GASM_INSTR_CODE_ADD_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("add.xdouble", GASM_INSTR_CODE_ADD_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("sub.int", GASM_INSTR_CODE_SUB_INT));
    m_funcMap.insert(std::make_pair<scString, int>("sub.int64", GASM_INSTR_CODE_SUB_INT64));
    m_funcMap.insert(std::make_pair<scString, int>("sub.byte", GASM_INSTR_CODE_SUB_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("sub.uint", GASM_INSTR_CODE_SUB_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("sub.uint64", GASM_INSTR_CODE_SUB_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("sub.float", GASM_INSTR_CODE_SUB_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("sub.double", GASM_INSTR_CODE_SUB_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("sub.xdouble", GASM_INSTR_CODE_SUB_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("mult.int", GASM_INSTR_CODE_MULT_INT));
    m_funcMap.insert(std::make_pair<scString, int>("mult.int64", GASM_INSTR_CODE_MULT_INT64));
    m_funcMap.insert(std::make_pair<scString, int>("mult.byte", GASM_INSTR_CODE_MULT_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("mult.uint", GASM_INSTR_CODE_MULT_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("mult.uint64", GASM_INSTR_CODE_MULT_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("mult.float", GASM_INSTR_CODE_MULT_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("mult.double", GASM_INSTR_CODE_MULT_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("mult.xdouble", GASM_INSTR_CODE_MULT_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("div.int", GASM_INSTR_CODE_DIV_INT));
    m_funcMap.insert(std::make_pair<scString, int>("div.int64", GASM_INSTR_CODE_DIV_INT64));
    m_funcMap.insert(std::make_pair<scString, int>("div.byte", GASM_INSTR_CODE_DIV_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("div.uint", GASM_INSTR_CODE_DIV_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("div.uint64", GASM_INSTR_CODE_DIV_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("div.float", GASM_INSTR_CODE_DIV_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("div.double", GASM_INSTR_CODE_DIV_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("div.xdouble", GASM_INSTR_CODE_DIV_XDOUBLE));

    m_funcMap.insert(std::make_pair<scString, int>("invs.float", GASM_INSTR_CODE_INVS_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("invs.double", GASM_INSTR_CODE_INVS_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("invs.xdouble", GASM_INSTR_CODE_INVS_XDOUBLE));

    m_funcMap.insert(std::make_pair<scString, int>("multdiv.int", GASM_INSTR_CODE_MULTDIV_INT));
    m_funcMap.insert(std::make_pair<scString, int>("multdiv.byte", GASM_INSTR_CODE_MULTDIV_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("multdiv.uint", GASM_INSTR_CODE_MULTDIV_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("multdiv.float", GASM_INSTR_CODE_MULTDIV_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("multdiv.double", GASM_INSTR_CODE_MULTDIV_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("pow.uint", GASM_INSTR_CODE_POW_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("mod.int", GASM_INSTR_CODE_MOD_INT));
    m_funcMap.insert(std::make_pair<scString, int>("mod.int64", GASM_INSTR_CODE_MOD_INT64));
    m_funcMap.insert(std::make_pair<scString, int>("mod.byte", GASM_INSTR_CODE_MOD_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("mod.uint", GASM_INSTR_CODE_MOD_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("mod.uint64", GASM_INSTR_CODE_MOD_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("bit.and.byte", GASM_INSTR_CODE_BIT_AND_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("bit.and.uint", GASM_INSTR_CODE_BIT_AND_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("bit.and.uint64", GASM_INSTR_CODE_BIT_AND_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("bit.or.byte", GASM_INSTR_CODE_BIT_OR_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("bit.or.uint", GASM_INSTR_CODE_BIT_OR_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("bit.or.uint64", GASM_INSTR_CODE_BIT_OR_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("bit.xor.byte", GASM_INSTR_CODE_BIT_XOR_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("bit.xor.uint", GASM_INSTR_CODE_BIT_XOR_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("bit.xor.uint64", GASM_INSTR_CODE_BIT_XOR_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("bit.not.byte", GASM_INSTR_CODE_BIT_NOT_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("bit.not.uint", GASM_INSTR_CODE_BIT_NOT_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("bit.not.uint64", GASM_INSTR_CODE_BIT_NOT_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("bit.shr.byte", GASM_INSTR_CODE_BIT_SHR_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("bit.shr.uint", GASM_INSTR_CODE_BIT_SHR_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("bit.shr.uint64", GASM_INSTR_CODE_BIT_SHR_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("bit.shl.byte", GASM_INSTR_CODE_BIT_SHL_BYTE));
    m_funcMap.insert(std::make_pair<scString, int>("bit.shl.uint", GASM_INSTR_CODE_BIT_SHL_UINT));
    m_funcMap.insert(std::make_pair<scString, int>("bit.shl.uint64", GASM_INSTR_CODE_BIT_SHL_UINT64));
    m_funcMap.insert(std::make_pair<scString, int>("abs.int", GASM_INSTR_CODE_ABS_INT));
    m_funcMap.insert(std::make_pair<scString, int>("abs.int64", GASM_INSTR_CODE_ABS_INT64));
    m_funcMap.insert(std::make_pair<scString, int>("abs.float", GASM_INSTR_CODE_ABS_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("abs.double", GASM_INSTR_CODE_ABS_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("abs.xdouble", GASM_INSTR_CODE_ABS_XDOUBLE));

    m_funcMap.insert(std::make_pair<scString, int>("neg.int", GASM_INSTR_CODE_NEG_INT));
    m_funcMap.insert(std::make_pair<scString, int>("neg.int64", GASM_INSTR_CODE_NEG_INT64));
    m_funcMap.insert(std::make_pair<scString, int>("neg.float", GASM_INSTR_CODE_NEG_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("neg.double", GASM_INSTR_CODE_NEG_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("neg.xdouble", GASM_INSTR_CODE_NEG_XDOUBLE));

    m_funcMap.insert(std::make_pair<scString, int>("sgn.int", GASM_INSTR_CODE_SGN_INT));
    m_funcMap.insert(std::make_pair<scString, int>("sgn.int64", GASM_INSTR_CODE_SGN_INT64));
    m_funcMap.insert(std::make_pair<scString, int>("sgn.float", GASM_INSTR_CODE_SGN_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("sgn.double", GASM_INSTR_CODE_SGN_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("sgn.xdouble", GASM_INSTR_CODE_SGN_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("ceil.float", GASM_INSTR_CODE_CEIL_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("ceil.double", GASM_INSTR_CODE_CEIL_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("ceil.xdouble", GASM_INSTR_CODE_CEIL_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("floor.float", GASM_INSTR_CODE_FLOOR_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("floor.double", GASM_INSTR_CODE_FLOOR_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("floor.xdouble", GASM_INSTR_CODE_FLOOR_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("frac.float", GASM_INSTR_CODE_FRAC_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("frac.double", GASM_INSTR_CODE_FRAC_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("frac.xdouble", GASM_INSTR_CODE_FRAC_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("trunc.float", GASM_INSTR_CODE_TRUNC_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("trunc.double", GASM_INSTR_CODE_TRUNC_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("trunc.xdouble", GASM_INSTR_CODE_TRUNC_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("trunc64.float", GASM_INSTR_CODE_TRUNC64_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("trunc64.double", GASM_INSTR_CODE_TRUNC64_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("trunc64.xdouble", GASM_INSTR_CODE_TRUNC64_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("truncf.float", GASM_INSTR_CODE_TRUNCF_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("truncf.double", GASM_INSTR_CODE_TRUNCF_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("truncf.xdouble", GASM_INSTR_CODE_TRUNCF_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("round.float", GASM_INSTR_CODE_ROUND_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("round.double", GASM_INSTR_CODE_ROUND_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("round.xdouble", GASM_INSTR_CODE_ROUND_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("round64.float", GASM_INSTR_CODE_ROUND64_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("round64.double", GASM_INSTR_CODE_ROUND64_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("round64.xdouble", GASM_INSTR_CODE_ROUND64_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("roundf.float", GASM_INSTR_CODE_ROUNDF_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("roundf.double", GASM_INSTR_CODE_ROUNDF_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("roundf.xdouble", GASM_INSTR_CODE_ROUNDF_XDOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("array.init", GASM_INSTR_CODE_ARRAY_INIT));
    m_funcMap.insert(std::make_pair<scString, int>("array.define", GASM_INSTR_CODE_ARRAY_DEFINE));
    m_funcMap.insert(std::make_pair<scString, int>("array.size", GASM_INSTR_CODE_ARRAY_SIZE));
    m_funcMap.insert(std::make_pair<scString, int>("array.get_item", GASM_INSTR_CODE_ARRAY_GET_ITEM));
    m_funcMap.insert(std::make_pair<scString, int>("array.set_item", GASM_INSTR_CODE_ARRAY_SET_ITEM));
    m_funcMap.insert(std::make_pair<scString, int>("array.add_item", GASM_INSTR_CODE_ARRAY_ADD_ITEM));
    m_funcMap.insert(std::make_pair<scString, int>("array.erase_item", GASM_INSTR_CODE_ARRAY_ERASE_ITEM));
    m_funcMap.insert(std::make_pair<scString, int>("array.index_of", GASM_INSTR_CODE_ARRAY_INDEX_OF));
    m_funcMap.insert(std::make_pair<scString, int>("array.merge", GASM_INSTR_CODE_ARRAY_MERGE));
    m_funcMap.insert(std::make_pair<scString, int>("array.range", GASM_INSTR_CODE_ARRAY_RANGE));
    m_funcMap.insert(std::make_pair<scString, int>("struct.init", GASM_INSTR_CODE_STRUCT_INIT));
    m_funcMap.insert(std::make_pair<scString, int>("struct.size", GASM_INSTR_CODE_STRUCT_SIZE));
    m_funcMap.insert(std::make_pair<scString, int>("struct.get_item", GASM_INSTR_CODE_STRUCT_GET_ITEM));
    m_funcMap.insert(std::make_pair<scString, int>("struct.set_item", GASM_INSTR_CODE_STRUCT_SET_ITEM));
    m_funcMap.insert(std::make_pair<scString, int>("struct.add_item", GASM_INSTR_CODE_STRUCT_ADD_ITEM));
    m_funcMap.insert(std::make_pair<scString, int>("struct.erase_item", GASM_INSTR_CODE_STRUCT_ERASE_ITEM));
    m_funcMap.insert(std::make_pair<scString, int>("struct.index_of", GASM_INSTR_CODE_STRUCT_INDEX_OF));
    m_funcMap.insert(std::make_pair<scString, int>("struct.merge", GASM_INSTR_CODE_STRUCT_MERGE));
    m_funcMap.insert(std::make_pair<scString, int>("struct.range", GASM_INSTR_CODE_STRUCT_RANGE));
    m_funcMap.insert(std::make_pair<scString, int>("struct.get_item_name", GASM_INSTR_CODE_STRUCT_GET_ITEM_NAME));
    m_funcMap.insert(std::make_pair<scString, int>("string.size", GASM_INSTR_CODE_STRING_SIZE));
    m_funcMap.insert(std::make_pair<scString, int>("string.merge", GASM_INSTR_CODE_STRING_MERGE));
    m_funcMap.insert(std::make_pair<scString, int>("string.set_char", GASM_INSTR_CODE_STRING_SET_CHAR));
    m_funcMap.insert(std::make_pair<scString, int>("string.get_char", GASM_INSTR_CODE_STRING_GET_CHAR));
    m_funcMap.insert(std::make_pair<scString, int>("string.range", GASM_INSTR_CODE_STRING_RANGE));
    m_funcMap.insert(std::make_pair<scString, int>("string.find", GASM_INSTR_CODE_STRING_FIND));
    m_funcMap.insert(std::make_pair<scString, int>("string.replace", GASM_INSTR_CODE_STRING_REPLACE));
    m_funcMap.insert(std::make_pair<scString, int>("vector.min", GASM_INSTR_CODE_VECTOR_MIN));
    m_funcMap.insert(std::make_pair<scString, int>("vector.max", GASM_INSTR_CODE_VECTOR_MAX));
    m_funcMap.insert(std::make_pair<scString, int>("vector.sum", GASM_INSTR_CODE_VECTOR_SUM));
    m_funcMap.insert(std::make_pair<scString, int>("vector.avg", GASM_INSTR_CODE_VECTOR_AVG));
    m_funcMap.insert(std::make_pair<scString, int>("vector.sort", GASM_INSTR_CODE_VECTOR_SORT));
    m_funcMap.insert(std::make_pair<scString, int>("vector.distinct", GASM_INSTR_CODE_VECTOR_DISTINCT));
    m_funcMap.insert(std::make_pair<scString, int>("vector.norm", GASM_INSTR_CODE_VECTOR_NORM));
    m_funcMap.insert(std::make_pair<scString, int>("vector.dot_product", GASM_INSTR_CODE_VECTOR_DOT_PRODUCT));
    m_funcMap.insert(std::make_pair<scString, int>("vector.std_dev", GASM_INSTR_CODE_VECTOR_STD_DEV));
    m_funcMap.insert(std::make_pair<scString, int>("date.nowf", GASM_INSTR_CODE_DATE_NOWF));
    m_funcMap.insert(std::make_pair<scString, int>("date.nows", GASM_INSTR_CODE_DATE_NOWS));
    m_funcMap.insert(std::make_pair<scString, int>("date.nowi", GASM_INSTR_CODE_DATE_NOWI));
    m_funcMap.insert(std::make_pair<scString, int>("date.datei", GASM_INSTR_CODE_DATE_DATEI));
    m_funcMap.insert(std::make_pair<scString, int>("rand.init", GASM_INSTR_CODE_RAND_INIT));
    m_funcMap.insert(std::make_pair<scString, int>("rand.randomf", GASM_INSTR_CODE_RAND_RANDOMF));
    m_funcMap.insert(std::make_pair<scString, int>("rand.randomi", GASM_INSTR_CODE_RAND_RANDOMI));
    m_funcMap.insert(std::make_pair<scString, int>("block.add", GASM_INSTR_CODE_BLOCK_ADD));
    m_funcMap.insert(std::make_pair<scString, int>("block.init", GASM_INSTR_CODE_BLOCK_INIT));
    m_funcMap.insert(std::make_pair<scString, int>("block.erase", GASM_INSTR_CODE_BLOCK_ERASE));
    m_funcMap.insert(std::make_pair<scString, int>("block.size", GASM_INSTR_CODE_BLOCK_SIZE));
    m_funcMap.insert(std::make_pair<scString, int>("block.count", GASM_INSTR_CODE_BLOCK_COUNT));
    m_funcMap.insert(std::make_pair<scString, int>("block.active_id", GASM_INSTR_CODE_BLOCK_ACTIVE_ID));
  }
  
  virtual sgpFunction *createFunction(const scString &a_name) { 
    sgpFunction *res;
    sgpFFFuncMap::iterator it = m_funcMap.find(a_name);
    if (it != m_funcMap.end()) {
      int code = it->second;   
      switch (code) {
        case GASM_INSTR_CODE_NOOP:         res = new sgpFuncNoop(); break;
        case GASM_INSTR_CODE_MOVE:         res = new sgpFuncMove(); break;
        case GASM_INSTR_CODE_INIT:         res = new sgpFuncInit(); break;
        case GASM_INSTR_CODE_CALL:         res = new sgpFuncCall(); break;
        case GASM_INSTR_CODE_RETURN:       res = new sgpFuncReturn(); break;
        case GASM_INSTR_CODE_SET_NEXT_BLOCK: res = new sgpFuncSetNextBlock(); break;        
        case GASM_INSTR_CODE_SET_NEXT_BLOCK_REL: res = new sgpFuncSetNextBlockRel(); break;        
        case GASM_INSTR_CODE_CLR_NEXT_BLOCK: res = new sgpFuncClrNextBlock(); break;        
        case GASM_INSTR_CODE_PUSH_NEXT_RESULT: res = new sgpFuncPushNextResult(); break;        
        case GASM_INSTR_CODE_PUSH:         res = new sgpFuncPush(); break;        
        case GASM_INSTR_CODE_POP:          res = new sgpFuncPop(); break;        
        case GASM_INSTR_CODE_DATA:         res = new sgpFuncData(); break;
        case GASM_INSTR_CODE_REF_BUILD:    res = new sgpFuncRefBuild(); break;
        case GASM_INSTR_CODE_REF_CLEAR:    res = new sgpFuncRefClear(); break;
        case GASM_INSTR_CODE_REF_IS_REF:    res = new sgpFuncRefIsRef(); break;
        case GASM_INSTR_CODE_DEFINE:       res = new sgpFuncDefine(); break;
        case GASM_INSTR_CODE_CAST:         res = new sgpFuncCast(); break;
        case GASM_INSTR_CODE_SKIPIFN:      res = new sgpFuncSkipIfn(); break;
        case GASM_INSTR_CODE_SKIPIF:       res = new sgpFuncSkipIf(); break;
        case GASM_INSTR_CODE_SKIP:         res = new sgpFuncSkip(); break;
        case GASM_INSTR_CODE_JUMP_BACK:    res = new sgpFuncJumpBack(); break;
        case GASM_INSTR_CODE_REPNZ:    res = new sgpFuncRepnz(); break;
        case GASM_INSTR_CODE_REPNZ_BACK: res = new sgpFuncRepnzBack(); break;        
        case GASM_INSTR_CODE_RTTI_DATA_TYPE:  res = new sgpFuncDataType(); break;
        case GASM_INSTR_CODE_LOGIC_IIF:    res = new sgpFuncIif(); break;
        case GASM_INSTR_CODE_LOGIC_AND:    res = new sgpFuncAnd(); break;
        case GASM_INSTR_CODE_LOGIC_OR:     res = new sgpFuncOr(); break;
        case GASM_INSTR_CODE_LOGIC_XOR:    res = new sgpFuncXor(); break;
        case GASM_INSTR_CODE_LOGIC_NOT:    res = new sgpFuncNot(); break;
        case GASM_INSTR_CODE_EQU_INT:      res = new sgpFuncEquInt(); break;
        case GASM_INSTR_CODE_EQU_INT64:    res = new sgpFuncEquInt64(); break;
        case GASM_INSTR_CODE_EQU_BYTE:     res = new sgpFuncEquByte(); break;
        case GASM_INSTR_CODE_EQU_UINT:     res = new sgpFuncEquUInt(); break;
        case GASM_INSTR_CODE_EQU_UINT64:   res = new sgpFuncEquUInt64(); break;
        case GASM_INSTR_CODE_EQU_FLOAT:    res = new sgpFuncEquFloat(); break;
        case GASM_INSTR_CODE_EQU_DOUBLE:   res = new sgpFuncEquDouble(); break;
        case GASM_INSTR_CODE_EQU_XDOUBLE:  res = new sgpFuncEquXDouble(); break;
        case GASM_INSTR_CODE_EQU_STRING:   res = new sgpFuncEquString(); break;
        case GASM_INSTR_CODE_EQU_BOOL:     res = new sgpFuncEquBool(); break;
        case GASM_INSTR_CODE_EQU_VARIANT:  res = new sgpFuncEquVariant(); break;
        case GASM_INSTR_CODE_GT_INT:       res = new sgpFuncGtInt(); break;
        case GASM_INSTR_CODE_GT_INT64:     res = new sgpFuncGtInt64(); break;
        case GASM_INSTR_CODE_GT_BYTE:      res = new sgpFuncGtByte(); break;
        case GASM_INSTR_CODE_GT_UINT:      res = new sgpFuncGtUInt(); break;
        case GASM_INSTR_CODE_GT_UINT64:    res = new sgpFuncGtUInt64(); break;
        case GASM_INSTR_CODE_GT_FLOAT:     res = new sgpFuncGtFloat(); break;
        case GASM_INSTR_CODE_GT_DOUBLE:    res = new sgpFuncGtDouble(); break;
        case GASM_INSTR_CODE_GT_XDOUBLE:   res = new sgpFuncGtXDouble(); break;
        case GASM_INSTR_CODE_GT_STRING:    res = new sgpFuncGtString(); break;
        case GASM_INSTR_CODE_GT_BOOL:      res = new sgpFuncGtBool(); break;
        case GASM_INSTR_CODE_GT_VARIANT:   res = new sgpFuncGtVariant(); break;
        case GASM_INSTR_CODE_GTE_INT:      res = new sgpFuncGteInt(); break;
        case GASM_INSTR_CODE_GTE_INT64:    res = new sgpFuncGteInt64(); break;
        case GASM_INSTR_CODE_GTE_BYTE:     res = new sgpFuncGteByte(); break;
        case GASM_INSTR_CODE_GTE_UINT:     res = new sgpFuncGteUInt(); break;
        case GASM_INSTR_CODE_GTE_UINT64:   res = new sgpFuncGteUInt64(); break;
        case GASM_INSTR_CODE_GTE_FLOAT:    res = new sgpFuncGteFloat(); break;
        case GASM_INSTR_CODE_GTE_DOUBLE:   res = new sgpFuncGteDouble(); break;
        case GASM_INSTR_CODE_GTE_XDOUBLE:  res = new sgpFuncGteXDouble(); break;
        case GASM_INSTR_CODE_GTE_STRING:   res = new sgpFuncGteString(); break;
        case GASM_INSTR_CODE_GTE_BOOL:     res = new sgpFuncGteBool(); break;
        case GASM_INSTR_CODE_GTE_VARIANT:  res = new sgpFuncGteVariant(); break;
        case GASM_INSTR_CODE_LT_INT:       res = new sgpFuncLtInt(); break;
        case GASM_INSTR_CODE_LT_INT64:     res = new sgpFuncLtInt64(); break;
        case GASM_INSTR_CODE_LT_BYTE:      res = new sgpFuncLtByte(); break;
        case GASM_INSTR_CODE_LT_UINT:      res = new sgpFuncLtUInt(); break;
        case GASM_INSTR_CODE_LT_UINT64:    res = new sgpFuncLtUInt64(); break;
        case GASM_INSTR_CODE_LT_FLOAT:     res = new sgpFuncLtFloat(); break;
        case GASM_INSTR_CODE_LT_DOUBLE:    res = new sgpFuncLtDouble(); break;
        case GASM_INSTR_CODE_LT_XDOUBLE:   res = new sgpFuncLtXDouble(); break;
        case GASM_INSTR_CODE_LT_STRING:    res = new sgpFuncLtString(); break;
        case GASM_INSTR_CODE_LT_BOOL:      res = new sgpFuncLtBool(); break;
        case GASM_INSTR_CODE_LT_VARIANT:   res = new sgpFuncLtVariant(); break;
        case GASM_INSTR_CODE_LTE_INT:      res = new sgpFuncLteInt(); break;
        case GASM_INSTR_CODE_LTE_INT64:    res = new sgpFuncLteInt64(); break;
        case GASM_INSTR_CODE_LTE_BYTE:     res = new sgpFuncLteByte(); break;
        case GASM_INSTR_CODE_LTE_UINT:     res = new sgpFuncLteUInt(); break;
        case GASM_INSTR_CODE_LTE_UINT64:   res = new sgpFuncLteUInt64(); break;
        case GASM_INSTR_CODE_LTE_FLOAT:    res = new sgpFuncLteFloat(); break;
        case GASM_INSTR_CODE_LTE_DOUBLE:   res = new sgpFuncLteDouble(); break;
        case GASM_INSTR_CODE_LTE_XDOUBLE:  res = new sgpFuncLteXDouble(); break;
        case GASM_INSTR_CODE_LTE_STRING:   res = new sgpFuncLteString(); break;
        case GASM_INSTR_CODE_LTE_BOOL:     res = new sgpFuncLteBool(); break;
        case GASM_INSTR_CODE_LTE_VARIANT:  res = new sgpFuncLteVariant(); break;
        case GASM_INSTR_CODE_SAME:         res = new sgpFuncSame(); break;
        case GASM_INSTR_CODE_CMP_INT:      res = new sgpFuncCmpInt(); break;
        case GASM_INSTR_CODE_CMP_INT64:    res = new sgpFuncCmpInt64(); break;
        case GASM_INSTR_CODE_CMP_BYTE:     res = new sgpFuncCmpByte(); break;
        case GASM_INSTR_CODE_CMP_UINT:     res = new sgpFuncCmpUInt(); break;
        case GASM_INSTR_CODE_CMP_UINT64:   res = new sgpFuncCmpUInt64(); break;
        case GASM_INSTR_CODE_CMP_FLOAT:    res = new sgpFuncCmpFloat(); break;
        case GASM_INSTR_CODE_CMP_DOUBLE:   res = new sgpFuncCmpDouble(); break;
        case GASM_INSTR_CODE_CMP_XDOUBLE:  res = new sgpFuncCmpXDouble(); break;
        case GASM_INSTR_CODE_CMP_STRING:   res = new sgpFuncCmpString(); break;
        case GASM_INSTR_CODE_CMP_VARIANT:  res = new sgpFuncCmpVariant(); break;
        case GASM_INSTR_CODE_ADD_INT:      res = new sgpFuncAddInt(); break;
        case GASM_INSTR_CODE_ADD_INT64:    res = new sgpFuncAddInt64(); break;
        case GASM_INSTR_CODE_ADD_BYTE:     res = new sgpFuncAddByte(); break;
        case GASM_INSTR_CODE_ADD_UINT:     res = new sgpFuncAddUInt(); break;
        case GASM_INSTR_CODE_ADD_UINT64:   res = new sgpFuncAddUInt64(); break;
        case GASM_INSTR_CODE_ADD_FLOAT:    res = new sgpFuncAddFloat(); break;
        case GASM_INSTR_CODE_ADD_DOUBLE:   res = new sgpFuncAddDouble(); break;
        case GASM_INSTR_CODE_ADD_XDOUBLE:  res = new sgpFuncAddXDouble(); break;
        case GASM_INSTR_CODE_SUB_INT:      res = new sgpFuncSubInt(); break;
        case GASM_INSTR_CODE_SUB_INT64:    res = new sgpFuncSubInt64(); break;
        case GASM_INSTR_CODE_SUB_BYTE:     res = new sgpFuncSubByte(); break;
        case GASM_INSTR_CODE_SUB_UINT:     res = new sgpFuncSubUInt(); break;
        case GASM_INSTR_CODE_SUB_UINT64:   res = new sgpFuncSubUInt64(); break;
        case GASM_INSTR_CODE_SUB_FLOAT:    res = new sgpFuncSubFloat(); break;
        case GASM_INSTR_CODE_SUB_DOUBLE:   res = new sgpFuncSubDouble(); break;
        case GASM_INSTR_CODE_SUB_XDOUBLE:  res = new sgpFuncSubXDouble(); break;
        case GASM_INSTR_CODE_MULT_INT:     res = new sgpFuncMultInt(); break;
        case GASM_INSTR_CODE_MULT_INT64:   res = new sgpFuncMultInt64(); break;
        case GASM_INSTR_CODE_MULT_BYTE:    res = new sgpFuncMultByte(); break;
        case GASM_INSTR_CODE_MULT_UINT:    res = new sgpFuncMultUInt(); break;
        case GASM_INSTR_CODE_MULT_UINT64:  res = new sgpFuncMultUInt64(); break;
        case GASM_INSTR_CODE_MULT_FLOAT:   res = new sgpFuncMultFloat(); break;
        case GASM_INSTR_CODE_MULT_DOUBLE:  res = new sgpFuncMultDouble(); break;
        case GASM_INSTR_CODE_MULT_XDOUBLE:  res = new sgpFuncMultXDouble(); break;
        case GASM_INSTR_CODE_DIV_INT:      res = new sgpFuncDivInt(); break;
        case GASM_INSTR_CODE_DIV_INT64:    res = new sgpFuncDivInt64(); break;
        case GASM_INSTR_CODE_DIV_BYTE:     res = new sgpFuncDivByte(); break;
        case GASM_INSTR_CODE_DIV_UINT:     res = new sgpFuncDivUInt(); break;
        case GASM_INSTR_CODE_DIV_UINT64:   res = new sgpFuncDivUInt64(); break;
        case GASM_INSTR_CODE_DIV_FLOAT:    res = new sgpFuncDivFloat(); break;
        case GASM_INSTR_CODE_DIV_DOUBLE:   res = new sgpFuncDivDouble(); break;
        case GASM_INSTR_CODE_DIV_XDOUBLE:  res = new sgpFuncDivXDouble(); break;

        case GASM_INSTR_CODE_INVS_FLOAT:    res = new sgpFuncInvsFloat(); break;
        case GASM_INSTR_CODE_INVS_DOUBLE:   res = new sgpFuncInvsDouble(); break;
        case GASM_INSTR_CODE_INVS_XDOUBLE:  res = new sgpFuncInvsXDouble(); break;

        case GASM_INSTR_CODE_MULTDIV_INT:  res = new sgpFuncMultDivInt(); break;
        case GASM_INSTR_CODE_MULTDIV_BYTE: res = new sgpFuncMultDivByte(); break;
        case GASM_INSTR_CODE_MULTDIV_UINT: res = new sgpFuncMultDivUInt(); break;
        case GASM_INSTR_CODE_MULTDIV_FLOAT:  res = new sgpFuncMultDivFloat(); break;
        case GASM_INSTR_CODE_MULTDIV_DOUBLE:  res = new sgpFuncMultDivDouble(); break;
        case GASM_INSTR_CODE_POW_UINT:     res = new sgpFuncPowUInt(); break;
        case GASM_INSTR_CODE_MOD_INT:      res = new sgpFuncModInt(); break;
        case GASM_INSTR_CODE_MOD_INT64:    res = new sgpFuncModInt64(); break;
        case GASM_INSTR_CODE_MOD_BYTE:     res = new sgpFuncModByte(); break;
        case GASM_INSTR_CODE_MOD_UINT:     res = new sgpFuncModUInt(); break;
        case GASM_INSTR_CODE_MOD_UINT64:   res = new sgpFuncModUInt64(); break;
        case GASM_INSTR_CODE_BIT_AND_BYTE: res = new sgpFuncBitAndByte(); break;
        case GASM_INSTR_CODE_BIT_AND_UINT: res = new sgpFuncBitAndUInt(); break;
        case GASM_INSTR_CODE_BIT_AND_UINT64:  res = new sgpFuncBitAndUInt64(); break;
        case GASM_INSTR_CODE_BIT_OR_BYTE:  res = new sgpFuncBitOrByte(); break;
        case GASM_INSTR_CODE_BIT_OR_UINT:  res = new sgpFuncBitOrUInt(); break;
        case GASM_INSTR_CODE_BIT_OR_UINT64:  res = new sgpFuncBitOrUInt64(); break;
        case GASM_INSTR_CODE_BIT_XOR_BYTE: res = new sgpFuncBitXorByte(); break;
        case GASM_INSTR_CODE_BIT_XOR_UINT: res = new sgpFuncBitXorUInt(); break;
        case GASM_INSTR_CODE_BIT_XOR_UINT64:  res = new sgpFuncBitXorUInt64(); break;
        case GASM_INSTR_CODE_BIT_NOT_BYTE: res = new sgpFuncBitNotByte(); break;
        case GASM_INSTR_CODE_BIT_NOT_UINT: res = new sgpFuncBitNotUInt(); break;
        case GASM_INSTR_CODE_BIT_NOT_UINT64:  res = new sgpFuncBitNotUInt64(); break;
        case GASM_INSTR_CODE_BIT_SHR_BYTE: res = new sgpFuncBitShrByte(); break;
        case GASM_INSTR_CODE_BIT_SHR_UINT: res = new sgpFuncBitShrUInt(); break;
        case GASM_INSTR_CODE_BIT_SHR_UINT64:  res = new sgpFuncBitShrUInt64(); break;
        case GASM_INSTR_CODE_BIT_SHL_BYTE: res = new sgpFuncBitShlByte(); break;
        case GASM_INSTR_CODE_BIT_SHL_UINT: res = new sgpFuncBitShlUInt(); break;
        case GASM_INSTR_CODE_BIT_SHL_UINT64:  res = new sgpFuncBitShlUInt64(); break;
        case GASM_INSTR_CODE_ABS_INT:      res = new sgpFuncAbsInt(); break;
        case GASM_INSTR_CODE_ABS_INT64:    res = new sgpFuncAbsInt64(); break;
        case GASM_INSTR_CODE_ABS_FLOAT:    res = new sgpFuncAbsFloat(); break;
        case GASM_INSTR_CODE_ABS_DOUBLE:   res = new sgpFuncAbsDouble(); break;
        case GASM_INSTR_CODE_ABS_XDOUBLE:  res = new sgpFuncAbsXDouble(); break;

        case GASM_INSTR_CODE_NEG_INT:      res = new sgpFuncNegInt(); break;
        case GASM_INSTR_CODE_NEG_INT64:    res = new sgpFuncNegInt64(); break;
        case GASM_INSTR_CODE_NEG_FLOAT:    res = new sgpFuncNegFloat(); break;
        case GASM_INSTR_CODE_NEG_DOUBLE:   res = new sgpFuncNegDouble(); break;
        case GASM_INSTR_CODE_NEG_XDOUBLE:  res = new sgpFuncNegXDouble(); break;

        case GASM_INSTR_CODE_SGN_INT:      res = new sgpFuncSgnInt(); break;
        case GASM_INSTR_CODE_SGN_INT64:    res = new sgpFuncSgnInt64(); break;
        case GASM_INSTR_CODE_SGN_FLOAT:    res = new sgpFuncSgnFloat(); break;
        case GASM_INSTR_CODE_SGN_DOUBLE:   res = new sgpFuncSgnDouble(); break;
        case GASM_INSTR_CODE_SGN_XDOUBLE:  res = new sgpFuncSgnXDouble(); break;
        case GASM_INSTR_CODE_CEIL_FLOAT:   res = new sgpFuncCeilFloat(); break;
        case GASM_INSTR_CODE_CEIL_DOUBLE:  res = new sgpFuncCeilDouble(); break;
        case GASM_INSTR_CODE_CEIL_XDOUBLE: res = new sgpFuncCeilXDouble(); break;
        case GASM_INSTR_CODE_FLOOR_FLOAT:  res = new sgpFuncFloorFloat(); break;
        case GASM_INSTR_CODE_FLOOR_DOUBLE: res = new sgpFuncFloorDouble(); break;
        case GASM_INSTR_CODE_FLOOR_XDOUBLE:  res = new sgpFuncFloorXDouble(); break;
        case GASM_INSTR_CODE_FRAC_FLOAT:     res = new sgpFuncFracFloat(); break;
        case GASM_INSTR_CODE_FRAC_DOUBLE:    res = new sgpFuncFracDouble(); break;
        case GASM_INSTR_CODE_FRAC_XDOUBLE:   res = new sgpFuncFracXDouble(); break;
        case GASM_INSTR_CODE_TRUNC_FLOAT:    res = new sgpFuncTruncFloat(); break;
        case GASM_INSTR_CODE_TRUNC_DOUBLE:   res = new sgpFuncTruncDouble(); break;
        case GASM_INSTR_CODE_TRUNC_XDOUBLE:  res = new sgpFuncTruncXDouble(); break;
        case GASM_INSTR_CODE_TRUNC64_FLOAT:  res = new sgpFuncTrunc64Float(); break;
        case GASM_INSTR_CODE_TRUNC64_DOUBLE: res = new sgpFuncTrunc64Double(); break;
        case GASM_INSTR_CODE_TRUNC64_XDOUBLE:  res = new sgpFuncTrunc64XDouble(); break;
        case GASM_INSTR_CODE_TRUNCF_FLOAT:     res = new sgpFuncTruncfFloat(); break;
        case GASM_INSTR_CODE_TRUNCF_DOUBLE:    res = new sgpFuncTruncfDouble(); break;
        case GASM_INSTR_CODE_TRUNCF_XDOUBLE:   res = new sgpFuncTruncfXDouble(); break;
        case GASM_INSTR_CODE_ROUND_FLOAT:      res = new sgpFuncRoundFloat(); break;
        case GASM_INSTR_CODE_ROUND_DOUBLE:     res = new sgpFuncRoundDouble(); break;
        case GASM_INSTR_CODE_ROUND_XDOUBLE:    res = new sgpFuncRoundXDouble(); break;
        case GASM_INSTR_CODE_ROUND64_FLOAT:    res = new sgpFuncRound64Float(); break;
        case GASM_INSTR_CODE_ROUND64_DOUBLE:   res = new sgpFuncRound64Double(); break;
        case GASM_INSTR_CODE_ROUND64_XDOUBLE:  res = new sgpFuncRound64XDouble(); break;
        case GASM_INSTR_CODE_ROUNDF_FLOAT:     res = new sgpFuncRoundfFloat(); break;
        case GASM_INSTR_CODE_ROUNDF_DOUBLE:    res = new sgpFuncRoundfDouble(); break;
        case GASM_INSTR_CODE_ROUNDF_XDOUBLE:   res = new sgpFuncRoundfXDouble(); break;
        case GASM_INSTR_CODE_ARRAY_INIT:     res = new sgpFuncArrayInit(); break;
        case GASM_INSTR_CODE_ARRAY_DEFINE:     res = new sgpFuncArrayDefine(); break;
        case GASM_INSTR_CODE_ARRAY_SIZE:       res = new sgpFuncArraySize(); break;
        case GASM_INSTR_CODE_ARRAY_GET_ITEM:   res = new sgpFuncArrayGetItem(); break;
        case GASM_INSTR_CODE_ARRAY_SET_ITEM:   res = new sgpFuncArraySetItem(); break;
        case GASM_INSTR_CODE_ARRAY_ADD_ITEM:   res = new sgpFuncArrayAddItem(); break;
        case GASM_INSTR_CODE_ARRAY_ERASE_ITEM:   res = new sgpFuncArrayEraseItem(); break;
        case GASM_INSTR_CODE_ARRAY_INDEX_OF:     res = new sgpFuncArrayIndexOf(); break;
        case GASM_INSTR_CODE_ARRAY_MERGE:     res = new sgpFuncArrayMerge(); break;
        case GASM_INSTR_CODE_ARRAY_RANGE:     res = new sgpFuncArrayRange(); break;
        case GASM_INSTR_CODE_STRUCT_INIT:     res = new sgpFuncStructInit(); break;
        case GASM_INSTR_CODE_STRUCT_SIZE:        res = new sgpFuncStructSize(); break;
        case GASM_INSTR_CODE_STRUCT_GET_ITEM:    res = new sgpFuncStructGetItem(); break;
        case GASM_INSTR_CODE_STRUCT_SET_ITEM:    res = new sgpFuncStructSetItem(); break;
        case GASM_INSTR_CODE_STRUCT_ADD_ITEM:    res = new sgpFuncStructAddItem(); break;
        case GASM_INSTR_CODE_STRUCT_ERASE_ITEM:  res = new sgpFuncStructEraseItem(); break;
        case GASM_INSTR_CODE_STRUCT_INDEX_OF:    res = new sgpFuncStructIndexOf(); break;
        case GASM_INSTR_CODE_STRUCT_MERGE:     res = new sgpFuncStructMerge(); break;
        case GASM_INSTR_CODE_STRUCT_RANGE:     res = new sgpFuncStructRange(); break;
        case GASM_INSTR_CODE_STRUCT_GET_ITEM_NAME:     res = new sgpFuncStructGetItemName(); break;

        case GASM_INSTR_CODE_STRING_SIZE:        res = new sgpFuncStringSize(); break;
        case GASM_INSTR_CODE_STRING_MERGE:       res = new sgpFuncStringMerge(); break;
        case GASM_INSTR_CODE_STRING_SET_CHAR:    res = new sgpFuncStringSetChar(); break;
        case GASM_INSTR_CODE_STRING_GET_CHAR:    res = new sgpFuncStringGetChar(); break;
        case GASM_INSTR_CODE_STRING_RANGE:       res = new sgpFuncStringRange(); break;
        case GASM_INSTR_CODE_STRING_FIND:        res = new sgpFuncStringFind(); break;
        case GASM_INSTR_CODE_STRING_REPLACE:     res = new sgpFuncStringReplace(); break;
        case GASM_INSTR_CODE_VECTOR_MIN:         res = new sgpFuncVectorMinValue(); break;
        case GASM_INSTR_CODE_VECTOR_MAX:         res = new sgpFuncVectorMaxValue(); break;
        case GASM_INSTR_CODE_VECTOR_SUM:         res = new sgpFuncVectorSum(); break;
        case GASM_INSTR_CODE_VECTOR_AVG:         res = new sgpFuncVectorAvg(); break;
        case GASM_INSTR_CODE_VECTOR_SORT:        res = new sgpFuncVectorSort(); break;
        case GASM_INSTR_CODE_VECTOR_DISTINCT:    res = new sgpFuncVectorDistinct(); break;
        case GASM_INSTR_CODE_VECTOR_NORM:        res = new sgpFuncVectorNorm(); break;
        case GASM_INSTR_CODE_VECTOR_DOT_PRODUCT: res = new sgpFuncVectorDotProduct(); break;
        case GASM_INSTR_CODE_VECTOR_STD_DEV    : res = new sgpFuncVectorStdDev(); break;
        case GASM_INSTR_CODE_DATE_NOWF    :      res = new sgpFuncDateNowf(); break;
        case GASM_INSTR_CODE_DATE_NOWS    :      res = new sgpFuncDateNows(); break;
        case GASM_INSTR_CODE_DATE_NOWI    :      res = new sgpFuncDateNowi(); break;
        case GASM_INSTR_CODE_DATE_DATEI   :      res = new sgpFuncDateDatei(); break;
        case GASM_INSTR_CODE_RAND_INIT    :      res = new sgpFuncRandInit(); break;
        case GASM_INSTR_CODE_RAND_RANDOMF   :      res = new sgpFuncRandRandomf(); break;
        case GASM_INSTR_CODE_RAND_RANDOMI   :      res = new sgpFuncRandRandomi(); break;
        case GASM_INSTR_CODE_BLOCK_ADD   :      res = new sgpFuncBlockAdd(); break;
        case GASM_INSTR_CODE_BLOCK_INIT   :      res = new sgpFuncBlockInit(); break;
        case GASM_INSTR_CODE_BLOCK_ERASE   :     res = new sgpFuncBlockErase(); break;
        case GASM_INSTR_CODE_BLOCK_SIZE   :      res = new sgpFuncBlockSize(); break;
        case GASM_INSTR_CODE_BLOCK_COUNT   :     res = new sgpFuncBlockCount(); break;
        case GASM_INSTR_CODE_BLOCK_ACTIVE_ID   : res = new sgpFuncBlockActiveId(); break;
        default:
          res = SC_NULL;
      }
    } else {
      res = SC_NULL;
    }
    return res;
  };
protected:
  sgpFFFuncMap m_funcMap;  
};

// ----------------------------------------------------------------------------
// sgpFunLibCore
// ----------------------------------------------------------------------------
sgpFunLibCore::sgpFunLibCore(): sgpFunLib()
{
}

sgpFunLibCore::~sgpFunLibCore()
{
}

void sgpFunLibCore::registerFuncs()
{
  sgpFunFactory *x = addFactory(new sgpFFCore());
  dynamic_cast<sgpFFCore *>(x)->init();
  
  registerFunction("noop", x);
  registerFunction("move", x);
  registerFunction("init", x);
  registerFunction("call", x);
  registerFunction("return", x);
  registerFunction("set_next_block", x);
  registerFunction("set_next_block_rel", x);
  registerFunction("clr_next_block", x);

  registerFunction("push_next_result", x);
  registerFunction("push", x);
  registerFunction("pop", x);
  
  registerFunction("data", x);

//------
  registerFunction("ref.build", x);
  registerFunction("ref.clear", x);
  registerFunction("ref.is_ref", x);

  registerFunction("define", x);
  registerFunction("cast", x);

//------
  registerFunction("skip.ifn", x); // alias for IF
  registerFunction("skip.if", x);
  registerFunction("skip", x); 
  registerFunction("jump.back", x);
  registerFunction("repnz", x);
  registerFunction("repnz.back", x);

//------
  registerFunction("rtti.data_type", x);

//------
  registerFunction("iif", x);
  registerFunction("logic.iif", x);  
  registerFunction("and", x);
  registerFunction("logic.and", x);
  registerFunction("or", x);
  registerFunction("logic.or", x);
  registerFunction("xor", x);
  registerFunction("logic.xor", x);
  registerFunction("not", x);
  registerFunction("logic.not", x);
  
//------
  registerFunction("equ.int", x);
  registerFunction("equ.int64", x);
  registerFunction("equ.byte", x);
  registerFunction("equ.uint", x);
  registerFunction("equ.uint64", x);

  registerFunction("equ.float", x);
  registerFunction("equ.double", x);
  registerFunction("equ.xdouble", x);

  registerFunction("equ.string", x);
  registerFunction("equ.bool", x);
  registerFunction("equ.variant", x);

//------
  registerFunction("gt.int", x);
  registerFunction("gt.int64", x);
  registerFunction("gt.byte", x);
  registerFunction("gt.uint", x);
  registerFunction("gt.uint64", x);

  registerFunction("gt.float", x);
  registerFunction("gt.double", x);
  registerFunction("gt.xdouble", x);

  registerFunction("gt.string", x);
  registerFunction("gt.bool", x);
  registerFunction("gt.variant", x);

//------
  registerFunction("gte.int", x);
  registerFunction("gte.int64", x);
  registerFunction("gte.byte", x);
  registerFunction("gte.uint", x);
  registerFunction("gte.uint64", x);

  registerFunction("gte.float", x);
  registerFunction("gte.double", x);
  registerFunction("gte.xdouble", x);

  registerFunction("gte.string", x);
  registerFunction("gte.bool", x);
  registerFunction("gte.variant", x);

//------
  registerFunction("lt.int", x);
  registerFunction("lt.int64", x);
  registerFunction("lt.byte", x);
  registerFunction("lt.uint", x);
  registerFunction("lt.uint64", x);

  registerFunction("lt.float", x);
  registerFunction("lt.double", x);
  registerFunction("lt.xdouble", x);

  registerFunction("lt.string", x);
  registerFunction("lt.bool", x);
  registerFunction("lt.variant", x);

//------
  registerFunction("lte.int", x);
  registerFunction("lte.int64", x);
  registerFunction("lte.byte", x);
  registerFunction("lte.uint", x);
  registerFunction("lte.uint64", x);

  registerFunction("lte.float", x);
  registerFunction("lte.double", x);
  registerFunction("lte.xdouble", x);

  registerFunction("lte.string", x);
  registerFunction("lte.bool", x);
  registerFunction("lte.variant", x);

  registerFunction("same", x);

//------
  registerFunction("cmp.int", x);
  registerFunction("cmp.int64", x);
  registerFunction("cmp.byte", x);
  registerFunction("cmp.uint", x);
  registerFunction("cmp.uint64", x);

  registerFunction("cmp.float", x);
  registerFunction("cmp.double", x);
  registerFunction("cmp.xdouble", x);

  registerFunction("cmp.string", x);
  registerFunction("cmp.variant", x);

//------
  registerFunction("add.int", x);
  registerFunction("add.int64", x);
  registerFunction("add.byte", x);
  registerFunction("add.uint", x);
  registerFunction("add.uint64", x);

  registerFunction("add.float", x);
  registerFunction("add.double", x);
  registerFunction("add.xdouble", x);

//------
  registerFunction("sub.int", x);
  registerFunction("sub.int64", x);
  registerFunction("sub.byte", x);
  registerFunction("sub.uint", x);
  registerFunction("sub.uint64", x);

  registerFunction("sub.float", x);
  registerFunction("sub.double", x);
  registerFunction("sub.xdouble", x);

//------
  registerFunction("mult.int", x);
  registerFunction("mult.int64", x);
  registerFunction("mult.byte", x);
  registerFunction("mult.uint", x);
  registerFunction("mult.uint64", x);

  registerFunction("mult.float", x);
  registerFunction("mult.double", x);
  registerFunction("mult.xdouble", x);

//------
  registerFunction("div.int", x);
  registerFunction("div.int64", x);
  registerFunction("div.byte", x);
  registerFunction("div.uint", x);
  registerFunction("div.uint64", x);

  registerFunction("div.float", x);
  registerFunction("div.double", x);
  registerFunction("div.xdouble", x);

//------
  registerFunction("invs.float", x);
  registerFunction("invs.double", x);
  registerFunction("invs.xdouble", x);

//------
  registerFunction("multdiv.int", x);
  registerFunction("multdiv.byte", x);
  registerFunction("multdiv.uint", x);

  registerFunction("multdiv.float", x);
  registerFunction("multdiv.double", x);

//------
  registerFunction("pow.uint", x);

//------
  registerFunction("mod.int", x);
  registerFunction("mod.int64", x);
  registerFunction("mod.byte", x);
  registerFunction("mod.uint", x);
  registerFunction("mod.uint64", x);

//------
  registerFunction("bit.and.byte", x);
  registerFunction("bit.and.uint", x);
  registerFunction("bit.and.uint64", x);

//------
  registerFunction("bit.or.byte", x);
  registerFunction("bit.or.uint", x);
  registerFunction("bit.or.uint64", x);

//------
  registerFunction("bit.xor.byte", x);
  registerFunction("bit.xor.uint", x);
  registerFunction("bit.xor.uint64", x);

//------
  registerFunction("bit.not.byte", x);
  registerFunction("bit.not.uint", x);
  registerFunction("bit.not.uint64", x);

//------
  registerFunction("bit.shr.byte", x);
  registerFunction("bit.shr.uint", x);
  registerFunction("bit.shr.uint64", x);

//------
  registerFunction("bit.shl.byte", x);
  registerFunction("bit.shl.uint", x);
  registerFunction("bit.shl.uint64", x);

//------
  registerFunction("abs.int", x);
  registerFunction("abs.int64", x);

  registerFunction("abs.float", x);
  registerFunction("abs.double", x);
  registerFunction("abs.xdouble", x);

//------
  registerFunction("neg.int", x);
  registerFunction("neg.int64", x);

  registerFunction("neg.float", x);
  registerFunction("neg.double", x);
  registerFunction("neg.xdouble", x);

//------
  registerFunction("sgn.int", x);
  registerFunction("sgn.int64", x);

  registerFunction("sgn.float", x);
  registerFunction("sgn.double", x);
  registerFunction("sgn.xdouble", x);

//------
  registerFunction("ceil.float", x);
  registerFunction("ceil.double", x);
  registerFunction("ceil.xdouble", x);

//------
  registerFunction("floor.float", x);
  registerFunction("floor.double", x);
  registerFunction("floor.xdouble", x);

//------
  registerFunction("frac.float", x);
  registerFunction("frac.double", x);
  registerFunction("frac.xdouble", x);

//------
  registerFunction("trunc.float", x);
  registerFunction("trunc.double", x);
  registerFunction("trunc.xdouble", x);

//------
  registerFunction("trunc64.float", x);
  registerFunction("trunc64.double", x);
  registerFunction("trunc64.xdouble", x);

//------
  registerFunction("truncf.float", x);
  registerFunction("truncf.double", x);
  registerFunction("truncf.xdouble", x);

//------
  registerFunction("round.float", x);
  registerFunction("round.double", x);
  registerFunction("round.xdouble", x);

//------
  registerFunction("round64.float", x);
  registerFunction("round64.double", x);
  registerFunction("round64.xdouble", x);

//------
  registerFunction("roundf.float", x);
  registerFunction("roundf.double", x);
  registerFunction("roundf.xdouble", x);

//------
  registerFunction("array.init", x);
  registerFunction("array.define", x);
  registerFunction("array.size", x); 
  registerFunction("array.get_item", x);
  registerFunction("array.set_item", x);
  registerFunction("array.add_item", x);
  registerFunction("array.erase_item", x);
  registerFunction("array.index_of", x);
  registerFunction("array.merge", x);
  registerFunction("array.range", x);

//------
  registerFunction("struct.init", x); 
  registerFunction("struct.size", x); 
  registerFunction("struct.get_item", x);
  registerFunction("struct.set_item", x);
  registerFunction("struct.add_item", x);
  registerFunction("struct.erase_item", x);
  registerFunction("struct.index_of", x);
  registerFunction("struct.merge", x);
  registerFunction("struct.range", x);
  registerFunction("struct.get_item_name", x);

//------
  registerFunction("string.size", x); 
  registerFunction("string.merge", x); 
  registerFunction("string.set_char", x); 
  registerFunction("string.get_char", x); 
  registerFunction("string.range", x); 
  registerFunction("string.find", x); 
  registerFunction("string.replace", x); 

//------
  registerFunction("vector.min", x); 
  registerFunction("vector.max", x); 
  registerFunction("vector.sum", x); 
  registerFunction("vector.avg", x); 
  registerFunction("vector.sort", x); 
  registerFunction("vector.distinct", x); 
  registerFunction("vector.norm", x); 
  registerFunction("vector.dot_product", x); 
  registerFunction("vector.std_dev", x); 

//------
  registerFunction("date.nowf", x); 
  registerFunction("date.nows", x); 
  registerFunction("date.nowi", x); 
  registerFunction("date.datei", x); 

//------
  registerFunction("rand.init", x); 
  registerFunction("rand.randomi", x); 
  registerFunction("rand.randomf", x); 

//------
  registerFunction("block.add", x); 
  registerFunction("block.init", x); 
  registerFunction("block.erase", x); 
  registerFunction("block.size", x); 
  registerFunction("block.count", x); 
  registerFunction("block.active_id", x); 
}
