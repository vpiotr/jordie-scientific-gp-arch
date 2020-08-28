/////////////////////////////////////////////////////////////////////////////
// Name:        GasmFunLibAnn.cpp
// Project:     sgp
// Purpose:     Artificial Neural Networks support.
// Author:      Piotr Likus
// Modified by:
// Created:     14/02/2009
/////////////////////////////////////////////////////////////////////////////

#include <cmath>

#include "sgp/GasmFunLibAnn.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

class sgpFuncAnnBase: public sgpFunction {
public:
  virtual uint getLastCost() {return 3;}; 
};

class sgpFuncAnnBaseCalc: public sgpFuncAnnBase {
public:
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 3; a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfRegister, gdtfArray, output);
    addArgMeta(gatfInput, gatfRegister, gdtfArray, output);
    return true;
  }

  virtual bool execute(const scDataNode &args) const {
    uint regNo1 = m_machine->getRegisterNo(args[1]);
    uint regNo2 = m_machine->getRegisterNo(args[2]);
    
    scDataNode *ptr1 = m_machine->getRegisterPtr(regNo1);
    scDataNode *ptr2 = m_machine->getRegisterPtr(regNo2);
    
    bool res;
    if ((ptr1 != SC_NULL) && (ptr2 != SC_NULL))
    {
      scDataNode outValue;
      calcValue(*ptr1, *ptr2, outValue);
      m_machine->setLValue(args[0], outValue);    
      res = true;
    } else {
      res = false;
    }
    
    return res;
  }  
  
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const = 0;
};

class sgpFuncAnnOutDoubleFunc: public sgpFuncAnnBaseCalc {
public:
  virtual scString getName() const { return "ann.sigm"; };  
  virtual void calcValue(const scDataNode &arg1, const scDataNode &arg2, scDataNode &output) const {
    double netSum = 0.0;
    int epos = (arg1.size() < arg2.size())?arg1.size()+1:arg2.size();
    int i;
    double outValue;

    if (epos > 0) {
      for(i = 0; i != epos - 1; i++) {
        netSum += (arg1.getDouble(i)*arg2.getDouble(i));
      }
      netSum += arg2.getDouble(epos-1);
      outValue = calcOut(netSum);
    } else {
      outValue = 0.0;
    }    
    
    output.setAsDouble(outValue);
  }
  virtual double calcOut(double inValue) const = 0;
};

// ann.sigm #out, #input_vect, #weight_vect : calculate standard BP neuron output, 
//   nety = sum(xi*wi)
//   y = 1/(1+exp(-nety))
//   weight vector can be shorter or longer than input, 
//   - if shorter - some inputs will not be used
//   - if longer, only n+1 weights are used, where n is length of input
class sgpFuncAnnSigm: public sgpFuncAnnOutDoubleFunc {
public:
  virtual scString getName() const { return "ann.sigm"; };  
  virtual double calcOut(double inValue) const {
    return (1 / (1+exp(-inValue)));
  }
};

// ann.rad #out, #input_vect, #weight_vect : calculate standard BP neuron output, 
//   nety = sum(xi*wi)
//   y = exp(-nety^2)
//   weight vector can be shorter or longer than input, 
//   - if shorter - some inputs will not be used
//   - if longer, only n+1 weights are used, where n is length of input
class sgpFuncAnnRad: public sgpFuncAnnOutDoubleFunc {
public:
  virtual scString getName() const { return "ann.rad"; };  
  virtual double calcOut(double inValue) const {
    return (exp(-(inValue*inValue)));
  }
};

// ----------------------------------------------------------------------------
// functions - rest
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// function factories
// ----------------------------------------------------------------------------
typedef std::map<scString,int> sgpFFFuncMap; 

const int GASM_INSTR_CODE_ANN_SIGM = 10;     
const int GASM_INSTR_CODE_ANN_RAD = 20;     

class sgpFAnn: public sgpFunFactory {
public:
  void init() {
    m_funcMap.insert(std::make_pair<scString, int>("ann.sigm", GASM_INSTR_CODE_ANN_SIGM));
    m_funcMap.insert(std::make_pair<scString, int>("ann.rad", GASM_INSTR_CODE_ANN_RAD));
  }
  
  virtual sgpFunction *createFunction(const scString &a_name) { 
    sgpFunction *res;
    sgpFFFuncMap::iterator it = m_funcMap.find(a_name);
    if (it != m_funcMap.end()) {
      int code = it->second;   
      switch (code) {
        case GASM_INSTR_CODE_ANN_SIGM:    res = new sgpFuncAnnSigm(); break;
        case GASM_INSTR_CODE_ANN_RAD:    res = new sgpFuncAnnRad(); break;

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
// sgpFunLibAnn
// ----------------------------------------------------------------------------
sgpFunLibAnn::sgpFunLibAnn(): sgpFunLib()
{
}

sgpFunLibAnn::~sgpFunLibAnn()
{
}

void sgpFunLibAnn::registerFuncs()
{
  sgpFunFactory *x = addFactory(new sgpFAnn());
  dynamic_cast<sgpFAnn *>(x)->init();
  
//------
  registerFunction("ann.sigm", x);
  registerFunction("ann.rad", x);
//------
}
