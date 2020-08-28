/////////////////////////////////////////////////////////////////////////////
// Name:        GasmFunLibMacros.cpp
// Project:     sgp
// Purpose:     Functions for macro support
// Author:      Piotr Likus
// Modified by:
// Created:     11/12/2009
/////////////////////////////////////////////////////////////////////////////

#include "sgp/GasmFunLibMacros.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

class sgpFuncMacrosBase: public sgpFunctionForExpand {
public:
  virtual uint getLastCost() {
    return 1;
  }; 

  virtual bool execute(const scDataNode &args) const 
  {
    throw scError(scString("Function not exacutable: ") + getName());
  }
};

// ----------------------------------------------------------------------------
// sgpFuncMacro
// ----------------------------------------------------------------------------
// macro <macro-no>, #reg-out [, macro-param-inp1 [, macro-param-inp2...]]]: expand macro in place
class sgpFuncMacro: public sgpFuncMacrosBase {
public:
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 1; a_max = SGP_MAX_INSTR_ARG_COUNT; }
  virtual scString getName() const { return "macro"; };

  virtual bool hasDynamicArgs() const
  {
    return true;
  }

  virtual uint getId() const
  {
    return FUNCID_MACROS_MACRO;
  }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfInput, gatfConst, gdtfAllBaseXInts, output);
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    return true;
  }

  virtual bool expand(const scDataNode &args, sgpProgramCode &programCode, uint blockNo, cell_size_t instrOffset, bool validateOrder)
  {
    if (args.empty())
      return false;
    else {  
      scDataNode inputArgs = args;        
      inputArgs.eraseElement(0);        
      scDataNode arg0;
      uint macroNo;
      
      m_machine->evaluateArg(args[0], arg0);    
      if ((sgpVMachine::castDataNodeToGasmType(arg0.getValueType()) & gdtfAllBaseXInts) != 0)
        macroNo = arg0.getAsUInt();
      else
        macroNo = blockNo; // make macroNo invalid so it will be simply deleted 
      return m_machine->expandMacro(macroNo, inputArgs, programCode, blockNo, instrOffset, validateOrder);
    }
  }   
};

// ----------------------------------------------------------------------------
// sgpFuncMacroRel
// ----------------------------------------------------------------------------
// macro_rel <macro-no>, #reg-out [, macro-param-inp1 [, macro-param-inp2...]]]: expand macro in place, 
//   macro-no is a relative macro no
class sgpFuncMacroRel: public sgpFuncMacrosBase {
public:
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 1; a_max = SGP_MAX_INSTR_ARG_COUNT; }
  virtual scString getName() const { return "macro_rel"; };

  virtual bool hasDynamicArgs() const
  {
    return true;
  }

  virtual uint getId() const
  {
    return FUNCID_MACROS_MACRO_REL;
  }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfInput, gatfConst, gdtfAllBaseXInts, output);
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    return true;
  }

  virtual bool expand(const scDataNode &args, sgpProgramCode &programCode, uint blockNo, cell_size_t instrOffset, bool validateOrder)
  {
    if (args.empty())
      return false;
    else {  
      scDataNode inputArgs = args;        
      inputArgs.eraseElement(0);        
      scDataNode arg0;
      uint macroNo;
      
      m_machine->evaluateArg(args[0], arg0);    
      if ((sgpVMachine::castDataNodeToGasmType(arg0.getValueType()) & gdtfAllBaseXInts) != 0)
        macroNo = blockNo + arg0.getAsUInt();
      else
        macroNo = blockNo; // make macroNo invalid so it will be simply deleted 
      return m_machine->expandMacro(macroNo, inputArgs, programCode, blockNo, instrOffset, validateOrder);
    }
  }   
};

// ----------------------------------------------------------------------------
// function factories
// ----------------------------------------------------------------------------
typedef std::map<scString,int> sgpFFFuncMap; 

const int GASM_INSTR_CODE_MACROS_MACRO     = 10;     
const int GASM_INSTR_CODE_MACROS_MACRO_REL = 20;     

class sgpFMacros: public sgpFunFactory {
public:
  void init() {
    m_funcMap.insert(std::make_pair<scString, int>("macro", GASM_INSTR_CODE_MACROS_MACRO));
    m_funcMap.insert(std::make_pair<scString, int>("macro_rel", GASM_INSTR_CODE_MACROS_MACRO_REL));
  }
  
  virtual sgpFunction *createFunction(const scString &a_name) { 
    sgpFunction *res;
    sgpFFFuncMap::iterator it = m_funcMap.find(a_name);
    if (it != m_funcMap.end()) {
      int code = it->second;   
      switch (code) {
        case GASM_INSTR_CODE_MACROS_MACRO:    res = new sgpFuncMacro(); break;
        case GASM_INSTR_CODE_MACROS_MACRO_REL:    res = new sgpFuncMacroRel(); break;
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
// sgpFunLibMacros
// ----------------------------------------------------------------------------
sgpFunLibMacros::sgpFunLibMacros(): sgpFunLib()
{
}

sgpFunLibMacros::~sgpFunLibMacros()
{
}

void sgpFunLibMacros::registerFuncs()
{
  sgpFunFactory *x = addFactory(new sgpFMacros());
  dynamic_cast<sgpFMacros *>(x)->init();
  
//------
  registerFunction("macro", x);
  registerFunction("macro_rel", x);
//------
}
