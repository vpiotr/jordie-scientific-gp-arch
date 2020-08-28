/////////////////////////////////////////////////////////////////////////////
// Name:        GasmFunLibComplex.cpp
// Project:     sgp
// Purpose:     Complex numbers support library form sgp vmachine.
// Author:      Piotr Likus
// Modified by:
// Created:     14/02/2009
/////////////////////////////////////////////////////////////////////////////

#include <complex>

#include "sgp/GasmFunLibComplex.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

using namespace dtp;

typedef std::complex<double> sgpComplex;

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

class sgpFuncComplexBase: public sgpFunction {
public:
  virtual uint getLastCost() {return 3;}; 
};

class sgpFuncComplexBaseDoubleArg2: public sgpFuncComplexBase {
public:
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 3; a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfArray, output);
    addArgMeta(gatfInput, gatfAny, gdtfArray, output);
    return true;
  }

  virtual sgpComplex calcValue(sgpComplex inValue1, sgpComplex inValue2) const = 0;
  
  virtual bool execute(const scDataNode &args) const {
    bool res;
    
    scDataNode arg1, arg2;

    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);
    
    if ((arg1.size() < 2) || (arg2.size() < 2)) {
      res = false;
    } else {
      res = true;  
  
      sgpComplex cmpx1(arg1.getDouble(0), arg1.getDouble(1));
      sgpComplex cmpx2(arg2.getDouble(0), arg2.getDouble(1));

      sgpComplex outValue = calcValue(cmpx1, cmpx2);
      scDataNode outNode;
      outNode.setAsArray(vt_double);
      outNode.addItem(scDataNode(real(outValue)));
      outNode.addItem(scDataNode(imag(outValue)));

      m_machine->setLValue(args[0], outNode);    
    }
      
    return res;
  }  
};

class sgpFuncComplexAdd: public sgpFuncComplexBaseDoubleArg2 {
public:
  virtual scString getName() const { return "complex.add"; };  
  virtual sgpComplex calcValue(sgpComplex inValue1, sgpComplex inValue2) const {
    return inValue1 + inValue2;
  }
};

class sgpFuncComplexSub: public sgpFuncComplexBaseDoubleArg2 {
public:
  virtual scString getName() const { return "complex.sub"; };  
  virtual sgpComplex calcValue(sgpComplex inValue1, sgpComplex inValue2) const {
    return inValue1 - inValue2;
  }
};

class sgpFuncComplexMult: public sgpFuncComplexBaseDoubleArg2 {
public:
  virtual scString getName() const { return "complex.mult"; };  
  virtual sgpComplex calcValue(sgpComplex inValue1, sgpComplex inValue2) const {
    return inValue1 * inValue2;
  }
};

class sgpFuncComplexDiv: public sgpFuncComplexBaseDoubleArg2 {
public:
  virtual scString getName() const { return "complex.div"; };  
  virtual sgpComplex calcValue(sgpComplex inValue1, sgpComplex inValue2) const {
    return inValue1 / inValue2;
  }
};

class sgpFuncComplexPolar: public sgpFuncComplexBase {
public:
  virtual scString getName() const { return "complex.polar"; };  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 3; a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseFloats, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseFloats, output);
    return true;
  }

  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;

    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);
    
    sgpComplex outValue = std::polar(arg1.getAsDouble(), arg2.getAsDouble());
    
    scDataNode outNode;
    outNode.setAsArray(vt_double);
    outNode.addItem(scDataNode(real(outValue)));
    outNode.addItem(scDataNode(imag(outValue)));

    m_machine->setLValue(args[0], outNode);    
      
    return true;
  }  
};

// complex.sqr : f(x) = x*x where x is complex number
class sgpFuncComplexSqr: public sgpFuncComplexBase {
public:
  virtual scString getName() const { return "complex.sqr"; };  
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 3; a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAll, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseFloats, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseFloats, output);
    return true;
  }

  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;

    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);
  
    double a,b;
    a = arg1.getAsDouble();
    b = arg2.getAsDouble();
    double outValueR = a*a - b*b;
    double outValueI = 2*a*b;
    
    scDataNode outNode;
    outNode.setAsArray(vt_double);
    outNode.addItem(scDataNode(outValueR));
    outNode.addItem(scDataNode(outValueI));

    m_machine->setLValue(args[0], outNode);    
      
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

const int GASM_INSTR_CODE_COMPLEX_ADD = 10;     
const int GASM_INSTR_CODE_COMPLEX_SUB = 20;     
const int GASM_INSTR_CODE_COMPLEX_MULT = 30;     
const int GASM_INSTR_CODE_COMPLEX_DIV = 40;     
const int GASM_INSTR_CODE_COMPLEX_POLAR = 50;     
const int GASM_INSTR_CODE_COMPLEX_SQR = 60;     

class sgpFComplex: public sgpFunFactory {
public:
  void init() {
    m_funcMap.insert(std::make_pair<scString, int>("complex.add", GASM_INSTR_CODE_COMPLEX_ADD));
    m_funcMap.insert(std::make_pair<scString, int>("complex.sub", GASM_INSTR_CODE_COMPLEX_SUB));
    m_funcMap.insert(std::make_pair<scString, int>("complex.mult", GASM_INSTR_CODE_COMPLEX_MULT));
    m_funcMap.insert(std::make_pair<scString, int>("complex.div", GASM_INSTR_CODE_COMPLEX_DIV));
    m_funcMap.insert(std::make_pair<scString, int>("complex.polar", GASM_INSTR_CODE_COMPLEX_POLAR));
    m_funcMap.insert(std::make_pair<scString, int>("complex.sqr", GASM_INSTR_CODE_COMPLEX_SQR));
  }
  
  virtual sgpFunction *createFunction(const scString &a_name) { 
    sgpFunction *res;
    sgpFFFuncMap::iterator it = m_funcMap.find(a_name);
    if (it != m_funcMap.end()) {
      int code = it->second;   
      switch (code) {
        case GASM_INSTR_CODE_COMPLEX_ADD:    res = new sgpFuncComplexAdd(); break;
        case GASM_INSTR_CODE_COMPLEX_SUB:    res = new sgpFuncComplexSub(); break;
        case GASM_INSTR_CODE_COMPLEX_MULT:    res = new sgpFuncComplexMult(); break;
        case GASM_INSTR_CODE_COMPLEX_DIV:    res = new sgpFuncComplexDiv(); break;
        case GASM_INSTR_CODE_COMPLEX_POLAR:    res = new sgpFuncComplexPolar(); break;
        case GASM_INSTR_CODE_COMPLEX_SQR:    res = new sgpFuncComplexSqr(); break;

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
// sgpFunLibComplex
// ----------------------------------------------------------------------------
sgpFunLibComplex::sgpFunLibComplex(): sgpFunLib()
{
}

sgpFunLibComplex::~sgpFunLibComplex()
{
}

void sgpFunLibComplex::registerFuncs()
{
  sgpFunFactory *x = addFactory(new sgpFComplex());
  dynamic_cast<sgpFComplex *>(x)->init();
  
//------
  registerFunction("complex.add", x);
  registerFunction("complex.sub", x);
  registerFunction("complex.mult", x);
  registerFunction("complex.div", x);
  registerFunction("complex.polar", x);
  registerFunction("complex.sqr", x);
//------
}
