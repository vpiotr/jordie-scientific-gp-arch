/////////////////////////////////////////////////////////////////////////////
// Name:        GasmFunLibFMath.cpp
// Project:     sgp
// Purpose:     Mathematical functions
// Author:      Piotr Likus
// Modified by:
// Created:     13/02/2009
/////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <limits>

#include "sc/utils.h"
#include "sc/smath.h"

#include "sgp/GasmFunLibFMath.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

// define to handle incorrect values as some constants (like sqrt(-1)) = 0.0
#define FMATH_SAFE

#ifdef FMATH_SAFE
#define CHECK_NAN(a) checkNan(a)
#else
#define CHECK_NAN(a) (a)
#endif
  
// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

class sgpFuncFMathBase: public sgpFunction {
public:
  virtual uint getLastCost() {return 3;}; 
};

class sgpFuncFMathConstant: public sgpFuncFMathBase {
public:
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 1; a_max = 1; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAllBaseFloats+gdtfVariant, output);
    return true;
  }
  virtual uint getLastCost() {return 1;}; 
};

class sgpFuncFMathConstantFloat: public sgpFuncFMathConstant {
public:
};

class sgpFuncFMathConstantDouble: public sgpFuncFMathConstant {
public:
};

class sgpFuncFMathConstantXDouble: public sgpFuncFMathConstant {
public:
};

class sgpFuncFMathBaseFloat: public sgpFunction {
public:
  virtual uint getLastCost() {return 3;}; 
  float checkNan(float value) const {
#ifdef FMATH_SAFE
    if (isnan(value))
      return 0.0;
    else  
#endif       
    return value;
  }       
};

class sgpFuncFMathBaseDouble: public sgpFunction {
public:
  virtual uint getLastCost() {return 6;}; 

  double checkNan(double value) const {
#ifdef FMATH_SAFE
    if (isnan(value))
      return 0.0;
    else  
#endif       
    return value;
  }       
};

class sgpFuncFMathBaseXDouble: public sgpFunction {
public:
  virtual uint getLastCost() {return 7;}; 
  
  xdouble checkNan(xdouble value) const {
#ifdef FMATH_SAFE
    if (isnan(value))
      return 0.0;
    else  
#endif       
    return value;
  }         
};

class sgpFuncFMathBaseFloatArg1: public sgpFuncFMathBaseFloat {
public:
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 2; a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAllBaseFloats+gdtfVariant, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseFloats+gdtfVariant, output);
    return true;
  }

  virtual float calcValue(float inValue) const = 0;
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1;
    m_machine->evaluateArg(args[1], arg1);

    float input = arg1.getAsFloat();
    scDataNode outValue(float(calcValue(input)));
    m_machine->setLValue(args[0], outValue);    
    return true;
  }  
};

class sgpFuncFMathBaseDoubleArg1: public sgpFuncFMathBaseDouble {
public:
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 2; a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAllBaseFloats+gdtfVariant, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseFloats+gdtfVariant, output);
    return true;
  }
  
  virtual double calcValue(double inValue) const = 0;

  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1;
    m_machine->evaluateArg(args[1], arg1);

    double input = arg1.getAsFloat();
    scDataNode outValue(double(calcValue(input)));
    m_machine->setLValue(args[0], outValue);    
    return true;
  }  
};

class sgpFuncFMathBaseXDoubleArg1: public sgpFuncFMathBaseDouble {
public:
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 2; a_max = 2; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAllBaseFloats+gdtfVariant, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseFloats+gdtfVariant, output);
    return true;
  }

  virtual xdouble calcValue(xdouble inValue) const = 0;

  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1;
    m_machine->evaluateArg(args[1], arg1);

    xdouble input = arg1.getAsFloat();
    scDataNode outValue(xdouble(calcValue(input)));
    m_machine->setLValue(args[0], outValue);    
    return true;
  }  
};

class sgpFuncFMathBaseFloatArg2: public sgpFuncFMathBaseFloat {
public:
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 3; a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAllBaseFloats+gdtfVariant, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseFloats+gdtfVariant, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseFloats+gdtfVariant, output);
    return true;
  }

  virtual float calcValue(float inValue1, float inValue2) const = 0;
  
  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;
    
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);

    float input1 = arg1.getAsFloat();
    float input2 = arg2.getAsFloat();
    float out1 = calcValue(input1, input2);
    scDataNode outValue(out1);
    
    m_machine->setLValue(args[0], outValue);    
    return true;
  }  
};

class sgpFuncFMathBaseDoubleArg2: public sgpFuncFMathBaseDouble {
public:
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 3; a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAllBaseFloats+gdtfVariant, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseFloats+gdtfVariant, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseFloats+gdtfVariant, output);
    return true;
  }
  
  virtual double calcValue(double inValue1, double inValue2) const = 0;

  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;
    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);

    double input1 = arg1.getAsFloat();
    double input2 = arg2.getAsFloat();

    scDataNode outValue(double(calcValue(input1, input2)));
    
    m_machine->setLValue(args[0], outValue);    
    return true;
  }  
};

class sgpFuncFMathBaseXDoubleArg2: public sgpFuncFMathBaseDouble {
public:
  virtual void getArgCount(uint &a_min, uint &a_max) const { a_min = 3; a_max = 3; }

  virtual bool getArgMeta(scDataNode &output) const
  {
    addArgMeta(gatfOutput, gatfLValue, gdtfAllBaseFloats+gdtfVariant, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseFloats+gdtfVariant, output);
    addArgMeta(gatfInput, gatfAny, gdtfAllBaseFloats+gdtfVariant, output);
    return true;
  }

  virtual xdouble calcValue(xdouble inValue1, xdouble inValue2) const = 0;

  virtual bool execute(const scDataNode &args) const {
    scDataNode arg1, arg2;

    m_machine->evaluateArg(args[1], arg1);
    m_machine->evaluateArg(args[2], arg2);

    xdouble input1 = arg1.getAsFloat();
    xdouble input2 = arg2.getAsFloat();
    
    scDataNode outValue(xdouble(calcValue(input1, input2)));
    
    m_machine->setLValue(args[0], outValue);    
    return true;
  }  
};

// ----------------------------------------------------------------------------
// sin(x)
// ----------------------------------------------------------------------------

// sinf #out, value: execute f(x) = sin(x), float version
class sgpFuncFMathSinf: public sgpFuncFMathBaseFloatArg1 {
public:
  virtual scString getName() const { return "fmath.sin.float"; };
  virtual float calcValue(float inValue) const { return sin(inValue); }
};

// sind #out, value: execute f(x) = sin(x), double version
class sgpFuncFMathSind: public sgpFuncFMathBaseDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.sin.double"; };  
  virtual double calcValue(double inValue) const { return sin(inValue); }
};

// sinx #out, value: execute f(x) = sin(x), xdouble version
class sgpFuncFMathSinx: public sgpFuncFMathBaseXDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.sin.xdouble"; };
  virtual xdouble calcValue(xdouble inValue) const { return sin(inValue); }
};

// ----------------------------------------------------------------------------
// cos(x)
// ----------------------------------------------------------------------------
// cosf #out, value: execute f(x) = cos(x), float version
class sgpFuncFMathCosf: public sgpFuncFMathBaseFloatArg1 {
public:
  virtual scString getName() const { return "fmath.cos.float"; };
  virtual float calcValue(float inValue) const { return cos(inValue); }
};

// cosd #out, value: execute f(x) = cos(x), double version
class sgpFuncFMathCosd: public sgpFuncFMathBaseDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.cos.double"; };  
  virtual double calcValue(double inValue) const { return cos(inValue); }
};

// cosx #out, value: execute f(x) = cos(x), xdouble version
class sgpFuncFMathCosx: public sgpFuncFMathBaseXDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.cos.xdouble"; };
  virtual xdouble calcValue(xdouble inValue) const { return cos(inValue); }
};

// ----------------------------------------------------------------------------
// tan(x)
// ----------------------------------------------------------------------------
// tanf #out, value: execute f(x) = tan(x), float version
class sgpFuncFMathTanf: public sgpFuncFMathBaseFloatArg1 {
public:
  virtual scString getName() const { return "fmath.tan.float"; };
  virtual float calcValue(float inValue) const { 
    return CHECK_NAN(tan(inValue));
  }
};

// tand #out, value: execute f(x) = cos(x), double version
class sgpFuncFMathTand: public sgpFuncFMathBaseDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.tan.double"; };  
  virtual double calcValue(double inValue) const { 
    return CHECK_NAN(tan(inValue));
  }
};

// tanx #out, value: execute f(x) = tan(x), xdouble version
class sgpFuncFMathTanx: public sgpFuncFMathBaseXDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.tan.xdouble"; };
  virtual xdouble calcValue(xdouble inValue) const { 
    return CHECK_NAN(tan(inValue));
  }
};

// ----------------------------------------------------------------------------
// asin(x)
// ----------------------------------------------------------------------------
// asinf #out, value: execute f(x) = asin(x), float version
class sgpFuncFMathAsinf: public sgpFuncFMathBaseFloatArg1 {
public:
  virtual scString getName() const { return "fmath.asin.float"; };
  virtual float calcValue(float inValue) const { 
    return CHECK_NAN(asin(inValue));
  }
};

// asind #out, value: execute f(x) = asin(x), double version
class sgpFuncFMathAsind: public sgpFuncFMathBaseDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.asin.double"; };  
  virtual double calcValue(double inValue) const { 
    return CHECK_NAN(asin(inValue));
  }
};

// asinx #out, value: execute f(x) = asin(x), xdouble version
class sgpFuncFMathAsinx: public sgpFuncFMathBaseXDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.asin.xdouble"; };
  virtual xdouble calcValue(xdouble inValue) const { 
    return CHECK_NAN(asin(inValue));
  }
};

// ----------------------------------------------------------------------------
// acos(x)
// ----------------------------------------------------------------------------
// acosf #out, value: execute f(x) = acos(x), float version
class sgpFuncFMathAcosf: public sgpFuncFMathBaseFloatArg1 {
public:
  virtual scString getName() const { return "fmath.acos.float"; };
  virtual float calcValue(float inValue) const { 
    return CHECK_NAN(acos(inValue));
  }
};

// acosd #out, value: execute f(x) = acos(x), double version
class sgpFuncFMathAcosd: public sgpFuncFMathBaseDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.acos.double"; };  
  virtual double calcValue(double inValue) const { 
    return CHECK_NAN(acos(inValue));
  }
};

// acosx #out, value: execute f(x) = acos(x), xdouble version
class sgpFuncFMathAcosx: public sgpFuncFMathBaseXDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.acos.xdouble"; };
  virtual xdouble calcValue(xdouble inValue) const { 
    return CHECK_NAN(acos(inValue));
  }
};

// ----------------------------------------------------------------------------
// atan(x)
// ----------------------------------------------------------------------------
// atanf #out, value: execute f(x) = atan(x), float version
class sgpFuncFMathAtanf: public sgpFuncFMathBaseFloatArg1 {
public:
  virtual scString getName() const { return "fmath.atan.float"; };
  virtual float calcValue(float inValue) const { 
    return CHECK_NAN(atan(inValue));
  }
};

// atand #out, value: execute f(x) = atan(x), double version
class sgpFuncFMathAtand: public sgpFuncFMathBaseDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.atan.double"; };  
  virtual double calcValue(double inValue) const { 
    return CHECK_NAN(atan(inValue));
  }
};

// atanx #out, value: execute f(x) = atan(x), xdouble version
class sgpFuncFMathAtanx: public sgpFuncFMathBaseXDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.atan.xdouble"; };
  virtual xdouble calcValue(xdouble inValue) const { 
    return CHECK_NAN(atan(inValue));
  }
};

// ----------------------------------------------------------------------------
// sinh(x)
// ----------------------------------------------------------------------------
// sinhf #out, value: execute f(x) = sinh(x), float version
class sgpFuncFMathSinhf: public sgpFuncFMathBaseFloatArg1 {
public:
  virtual scString getName() const { return "fmath.sinh.float"; };
  virtual float calcValue(float inValue) const { 
    return CHECK_NAN(sinh(inValue));
  }
};

// sinhd #out, value: execute f(x) = sinh(x), double version
class sgpFuncFMathSinhd: public sgpFuncFMathBaseDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.sinh.double"; };  
  virtual double calcValue(double inValue) const { 
    return CHECK_NAN(sinh(inValue));
  }
};

// sinhx #out, value: execute f(x) = sinh(x), xdouble version
class sgpFuncFMathSinhx: public sgpFuncFMathBaseXDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.sinh.xdouble"; };
  virtual xdouble calcValue(xdouble inValue) const { 
    return CHECK_NAN(sinh(inValue));
  }
};

// ----------------------------------------------------------------------------
// cosh(x)
// ----------------------------------------------------------------------------
// coshf #out, value: execute f(x) = cosh(x), float version
class sgpFuncFMathCoshf: public sgpFuncFMathBaseFloatArg1 {
public:
  virtual scString getName() const { return "fmath.cosh.float"; };
  virtual float calcValue(float inValue) const { 
    return CHECK_NAN(cosh(inValue));
  }
};

// coshd #out, value: execute f(x) = cosh(x), double version
class sgpFuncFMathCoshd: public sgpFuncFMathBaseDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.cosh.double"; };  
  virtual double calcValue(double inValue) const { 
    return CHECK_NAN(cosh(inValue));
  }
};

// coshx #out, value: execute f(x) = cosh(x), xdouble version
class sgpFuncFMathCoshx: public sgpFuncFMathBaseXDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.cosh.xdouble"; };
  virtual xdouble calcValue(xdouble inValue) const { 
    return CHECK_NAN(cosh(inValue));
  }
};

// ----------------------------------------------------------------------------
// tanh(x)
// ----------------------------------------------------------------------------
// tanhf #out, value: execute f(x) = tanh(x), float version
class sgpFuncFMathTanhf: public sgpFuncFMathBaseFloatArg1 {
public:
  virtual scString getName() const { return "fmath.tanh.float"; };
  virtual float calcValue(float inValue) const { 
    return CHECK_NAN(tanh(inValue));
  }
};

// tanhd #out, value: execute f(x) = tanh(x), double version
class sgpFuncFMathTanhd: public sgpFuncFMathBaseDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.tanh.double"; };  
  virtual double calcValue(double inValue) const { 
    return CHECK_NAN(tanh(inValue));
  }
};

// tanhx #out, value: execute f(x) = tanh(x), xdouble version
class sgpFuncFMathTanhx: public sgpFuncFMathBaseXDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.tanh.xdouble"; };
  virtual xdouble calcValue(xdouble inValue) const { 
    return CHECK_NAN(tanh(inValue));
  }
};

// ----------------------------------------------------------------------------
// exp(x)
// ----------------------------------------------------------------------------
// expf #out, value: execute f(x) = e^x, float version
class sgpFuncFMathExpf: public sgpFuncFMathBaseFloatArg1 {
public:
  virtual scString getName() const { return "fmath.exp.float"; };
  virtual float calcValue(float inValue) const { return exp(inValue); }
};

// expd #out, value: execute f(x) = e^x, double version
class sgpFuncFMathExpd: public sgpFuncFMathBaseDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.exp.double"; };  
  virtual double calcValue(double inValue) const { return exp(inValue); }
};

// expx #out, value: execute f(x) = e^x, xdouble version
class sgpFuncFMathExpx: public sgpFuncFMathBaseXDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.exp.xdouble"; };
  virtual xdouble calcValue(xdouble inValue) const { return exp(inValue); }
};

// ----------------------------------------------------------------------------
// pow(x)
// ----------------------------------------------------------------------------
// powf #out, value: execute f(x,y) = x^y, float version
class sgpFuncFMathPowf: public sgpFuncFMathBaseFloatArg2 {
public:
  virtual scString getName() const { return "fmath.pow.float"; };
  virtual float calcValue(float inValue1, float inValue2) const { 
    return CHECK_NAN(pow(inValue1, inValue2));
  }
};

// powd #out, value: execute f(x,y) = x^y, double version
class sgpFuncFMathPowd: public sgpFuncFMathBaseDoubleArg2 {
public:
  virtual scString getName() const { return "fmath.pow.double"; };  
  virtual double calcValue(double inValue1, double inValue2) const { 
    return CHECK_NAN(pow(inValue1, inValue2));
  }
};

// powx #out, value: execute f(x,y) = x^y, xdouble version
class sgpFuncFMathPowx: public sgpFuncFMathBaseXDoubleArg2 {
public:
  virtual scString getName() const { return "fmath.pow.xdouble"; };
  virtual xdouble calcValue(xdouble inValue1, xdouble inValue2) const { 
    return CHECK_NAN(pow(inValue1, inValue2));
  }
};

// ----------------------------------------------------------------------------
// pow2(x)
// ----------------------------------------------------------------------------
// fmath.pow2.float #out, value: execute f(x) = x^2, float version
class sgpFuncFMathPow2f: public sgpFuncFMathBaseFloatArg1 {
public:
  virtual scString getName() const { return "fmath.pow2.float"; };
  virtual float calcValue(float inValue) const { return fsqr(inValue); }
};

// fmath.pow2.double #out, value: execute f(x) = x^2, float version
class sgpFuncFMathPow2d: public sgpFuncFMathBaseDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.pow2.double"; };  
  virtual double calcValue(double inValue) const { return fsqr(inValue); }
};

// fmath.pow2.xdouble #out, value: execute f(x) = x^2, xdouble version
class sgpFuncFMathPow2x: public sgpFuncFMathBaseXDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.pow2.xdouble"; };
  virtual xdouble calcValue(xdouble inValue) const { return fsqr(inValue); }
};

// ----------------------------------------------------------------------------
// log(x)
// ----------------------------------------------------------------------------
// logf #out, value: find y for x where x = e^y, float version
class sgpFuncFMathLogf: public sgpFuncFMathBaseFloatArg1 {
public:
  virtual scString getName() const { return "fmath.log.float"; };
  virtual float calcValue(float inValue) const { 
    return CHECK_NAN(log(inValue));
  }
};

// logd #out, value: find y for x where x = e^y, double version
class sgpFuncFMathLogd: public sgpFuncFMathBaseDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.log.double"; };  
  virtual double calcValue(double inValue) const { 
    return CHECK_NAN(log(inValue));
  }
};

// logx #out, value: find y for x where x = e^y, xdouble version
class sgpFuncFMathLogx: public sgpFuncFMathBaseXDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.log.xdouble"; };
  virtual xdouble calcValue(xdouble inValue) const { 
    return CHECK_NAN(log(inValue));
  }
};

// ----------------------------------------------------------------------------
// logn(x)
// ----------------------------------------------------------------------------
// lognf #out, value_b, value_x: f(b,x) - find y for x where x = b^y, float ver
class sgpFuncFMathLognf: public sgpFuncFMathBaseFloatArg2 {
public:
  virtual scString getName() const { return "fmath.logn.float"; };
  virtual float calcValue(float inValue1, float inValue2) const { 
    return CHECK_NAN(logn(inValue1, inValue2));
  }
};

// lognd #out, value_b, value_x: f(b,x) - find y for x where x = b^y, double ver
class sgpFuncFMathLognd: public sgpFuncFMathBaseDoubleArg2 {
public:
  virtual scString getName() const { return "fmath.logn.double"; };  
  virtual double calcValue(double inValue1, double inValue2) const { 
    return CHECK_NAN(logn(inValue1, inValue2));
  }
};

// lognx #out, value_b, value_x: f(b,x) - find y for x where x = b^y, xdouble ver
class sgpFuncFMathLognx: public sgpFuncFMathBaseXDoubleArg2 {
public:
  virtual scString getName() const { return "fmath.logn.xdouble"; };
  virtual xdouble calcValue(xdouble inValue1, xdouble inValue2) const { 
    return CHECK_NAN(logn(inValue1, inValue2));
  }
};

// ----------------------------------------------------------------------------
// sqrt(x)
// ----------------------------------------------------------------------------
// sqrtf #out, value: f(x) - find y for x where x = y * y, float version
class sgpFuncFMathSqrtf: public sgpFuncFMathBaseFloatArg1 {
public:
  virtual scString getName() const { return "fmath.sqrt.float"; };
  virtual float calcValue(float inValue) const { 
    return CHECK_NAN(sqrt(inValue));
  }
};

// sqrtd #out, value: f(x) - find y for x where x = y * y, double version
class sgpFuncFMathSqrtd: public sgpFuncFMathBaseDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.sqrt.double"; };  
  virtual double calcValue(double inValue) const { 
    return CHECK_NAN(sqrt(inValue));
  }
};

// sqrtx #out, value: f(x) - find y for x where x = y * y, xdouble version
class sgpFuncFMathSqrtx: public sgpFuncFMathBaseXDoubleArg1 {
public:
  virtual scString getName() const { return "fmath.sqrt.xdouble"; };
  virtual xdouble calcValue(xdouble inValue) const { 
    return CHECK_NAN(sqrt(inValue));
  }
};

// ----------------------------------------------------------------------------
// math constants
// ----------------------------------------------------------------------------

// fmath.e_float #out: returns "e" constant with float precision
class sgpFuncFMathEFloat: public sgpFuncFMathConstantFloat {
public:
  virtual scString getName() const { return "fmath.e.float"; };
  
  virtual bool execute(const scDataNode &args) const {
    m_machine->setLValue(args[0], scDataNode(float(smath_const_e)));    
    return true;
  }
};

// fmath.e_double #out: returns "e" constant with double precision
class sgpFuncFMathEDouble: public sgpFuncFMathConstantDouble {
public:
  virtual scString getName() const { return "fmath.e.double"; };
  
  virtual bool execute(const scDataNode &args) const {
    m_machine->setLValue(args[0], scDataNode(double(smath_const_e)));    
    return true;
  }
};

// fmath.e_xdouble #out: returns "e" constant with xdouble precision
class sgpFuncFMathEXDouble: public sgpFuncFMathConstantXDouble {
public:
  virtual scString getName() const { return "fmath.e.xdouble"; };
  
  virtual bool execute(const scDataNode &args) const {
    m_machine->setLValue(args[0], scDataNode(xdouble(smath_const_e)));    
    return true;
  }
};

// fmath.pi_float #out: returns "e" constant with float precision
class sgpFuncFMathPiFloat: public sgpFuncFMathConstantFloat {
public:
  virtual scString getName() const { return "fmath.pi.float"; };
  
  virtual bool execute(const scDataNode &args) const {
    m_machine->setLValue(args[0], scDataNode(float(smath_const_pi)));    
    return true;
  }
};

// fmath.pi_double #out: returns "e" constant with double precision
class sgpFuncFMathPiDouble: public sgpFuncFMathConstantDouble {
public:
  virtual scString getName() const { return "fmath.pi.double"; };
  
  virtual bool execute(const scDataNode &args) const {
    m_machine->setLValue(args[0], scDataNode(double(smath_const_pi)));    
    return true;
  }
};

// fmath.pi_xdouble #out: returns "e" constant with xdouble precision
class sgpFuncFMathPiXDouble: public sgpFuncFMathConstantXDouble {
public:
  virtual scString getName() const { return "fmath.pi.xdouble"; };
  
  virtual bool execute(const scDataNode &args) const {
    m_machine->setLValue(args[0], scDataNode(xdouble(smath_const_pi)));    
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

const int GASM_INSTR_CODE_FMATH_SINF = 10;     
const int GASM_INSTR_CODE_FMATH_SIND = 20;     
const int GASM_INSTR_CODE_FMATH_SINX = 30;     

const int GASM_INSTR_CODE_FMATH_COSF = 40;     
const int GASM_INSTR_CODE_FMATH_COSD = 50;     
const int GASM_INSTR_CODE_FMATH_COSX = 60;     

const int GASM_INSTR_CODE_FMATH_TANF = 70;     
const int GASM_INSTR_CODE_FMATH_TAND = 80;     
const int GASM_INSTR_CODE_FMATH_TANX = 90;     

const int GASM_INSTR_CODE_FMATH_ASINF = 100;     
const int GASM_INSTR_CODE_FMATH_ASIND = 110;     
const int GASM_INSTR_CODE_FMATH_ASINX = 120;     

const int GASM_INSTR_CODE_FMATH_ACOSF = 130;     
const int GASM_INSTR_CODE_FMATH_ACOSD = 140;     
const int GASM_INSTR_CODE_FMATH_ACOSX = 150;     

const int GASM_INSTR_CODE_FMATH_ATANF = 160;     
const int GASM_INSTR_CODE_FMATH_ATAND = 170;     
const int GASM_INSTR_CODE_FMATH_ATANX = 180;     

const int GASM_INSTR_CODE_FMATH_SINHF = 190;     
const int GASM_INSTR_CODE_FMATH_SINHD = 200;     
const int GASM_INSTR_CODE_FMATH_SINHX = 210;     

const int GASM_INSTR_CODE_FMATH_COSHF = 220;     
const int GASM_INSTR_CODE_FMATH_COSHD = 230;     
const int GASM_INSTR_CODE_FMATH_COSHX = 240;     

const int GASM_INSTR_CODE_FMATH_TANHF = 250;     
const int GASM_INSTR_CODE_FMATH_TANHD = 260;     
const int GASM_INSTR_CODE_FMATH_TANHX = 270;     

const int GASM_INSTR_CODE_FMATH_EXPF = 280;     
const int GASM_INSTR_CODE_FMATH_EXPD = 290;     
const int GASM_INSTR_CODE_FMATH_EXPX = 300;     

const int GASM_INSTR_CODE_FMATH_POWF = 310;     
const int GASM_INSTR_CODE_FMATH_POWD = 320;     
const int GASM_INSTR_CODE_FMATH_POWX = 330;     

const int GASM_INSTR_CODE_FMATH_POW2F = 331;     
const int GASM_INSTR_CODE_FMATH_POW2D = 332;     
const int GASM_INSTR_CODE_FMATH_POW2X = 333;     

const int GASM_INSTR_CODE_FMATH_LOGF = 340;     
const int GASM_INSTR_CODE_FMATH_LOGD = 350;     
const int GASM_INSTR_CODE_FMATH_LOGX = 360;     

const int GASM_INSTR_CODE_FMATH_LOGNF = 370;     
const int GASM_INSTR_CODE_FMATH_LOGND = 380;     
const int GASM_INSTR_CODE_FMATH_LOGNX = 390;     

const int GASM_INSTR_CODE_FMATH_SQRTF = 400;     
const int GASM_INSTR_CODE_FMATH_SQRTD = 410;     
const int GASM_INSTR_CODE_FMATH_SQRTX = 420;     

const int GASM_INSTR_CODE_FMATH_PI_FLOAT = 3000;     
const int GASM_INSTR_CODE_FMATH_PI_DOUBLE = 3010;     
const int GASM_INSTR_CODE_FMATH_PI_XDOUBLE = 3020;     

const int GASM_INSTR_CODE_FMATH_E_FLOAT = 3030;     
const int GASM_INSTR_CODE_FMATH_E_DOUBLE = 3040;     
const int GASM_INSTR_CODE_FMATH_E_XDOUBLE = 3050;     

class sgpFFMath: public sgpFunFactory {
public:
  void init() {
    m_funcMap.insert(std::make_pair<scString, int>("fmath.sin.float", GASM_INSTR_CODE_FMATH_SINF));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.sin.double", GASM_INSTR_CODE_FMATH_SIND));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.sin.xdouble", GASM_INSTR_CODE_FMATH_SINX));

    m_funcMap.insert(std::make_pair<scString, int>("fmath.cos.float", GASM_INSTR_CODE_FMATH_COSF));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.cos.double", GASM_INSTR_CODE_FMATH_COSD));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.cos.xdouble", GASM_INSTR_CODE_FMATH_COSX));

    m_funcMap.insert(std::make_pair<scString, int>("fmath.tan.float", GASM_INSTR_CODE_FMATH_TANF));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.tan.double", GASM_INSTR_CODE_FMATH_TAND));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.tan.xdouble", GASM_INSTR_CODE_FMATH_TANX));

    m_funcMap.insert(std::make_pair<scString, int>("fmath.asin.float", GASM_INSTR_CODE_FMATH_ASINF));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.asin.double", GASM_INSTR_CODE_FMATH_ASIND));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.asin.xdouble", GASM_INSTR_CODE_FMATH_ASINX));

    m_funcMap.insert(std::make_pair<scString, int>("fmath.acos.float", GASM_INSTR_CODE_FMATH_ACOSF));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.acos.double", GASM_INSTR_CODE_FMATH_ACOSD));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.acos.xdouble", GASM_INSTR_CODE_FMATH_ACOSX));

    m_funcMap.insert(std::make_pair<scString, int>("fmath.atan.float", GASM_INSTR_CODE_FMATH_ATANF));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.atan.double", GASM_INSTR_CODE_FMATH_ATAND));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.atan.xdouble", GASM_INSTR_CODE_FMATH_ATANX));

    m_funcMap.insert(std::make_pair<scString, int>("fmath.sinh.float", GASM_INSTR_CODE_FMATH_SINHF));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.sinh.double", GASM_INSTR_CODE_FMATH_SINHD));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.sinh.xdouble", GASM_INSTR_CODE_FMATH_SINHX));

    m_funcMap.insert(std::make_pair<scString, int>("fmath.cosh.float", GASM_INSTR_CODE_FMATH_COSHF));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.cosh.double", GASM_INSTR_CODE_FMATH_COSHD));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.cosh.xdouble", GASM_INSTR_CODE_FMATH_COSHX));

    m_funcMap.insert(std::make_pair<scString, int>("fmath.tanh.float", GASM_INSTR_CODE_FMATH_TANHF));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.tanh.double", GASM_INSTR_CODE_FMATH_TANHD));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.tanh.xdouble", GASM_INSTR_CODE_FMATH_TANHX));

    m_funcMap.insert(std::make_pair<scString, int>("fmath.exp.float", GASM_INSTR_CODE_FMATH_EXPF));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.exp.double", GASM_INSTR_CODE_FMATH_EXPD));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.exp.xdouble", GASM_INSTR_CODE_FMATH_EXPX));

    m_funcMap.insert(std::make_pair<scString, int>("fmath.pow.float", GASM_INSTR_CODE_FMATH_POWF));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.pow.double", GASM_INSTR_CODE_FMATH_POWD));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.pow.xdouble", GASM_INSTR_CODE_FMATH_POWX));

    m_funcMap.insert(std::make_pair<scString, int>("fmath.pow2.float", GASM_INSTR_CODE_FMATH_POW2F));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.pow2.double", GASM_INSTR_CODE_FMATH_POW2D));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.pow2.xdouble", GASM_INSTR_CODE_FMATH_POW2X));

    m_funcMap.insert(std::make_pair<scString, int>("fmath.log.float", GASM_INSTR_CODE_FMATH_LOGF));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.log.double", GASM_INSTR_CODE_FMATH_LOGD));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.log.xdouble", GASM_INSTR_CODE_FMATH_LOGX));

    m_funcMap.insert(std::make_pair<scString, int>("fmath.logn.float", GASM_INSTR_CODE_FMATH_LOGNF));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.logn.double", GASM_INSTR_CODE_FMATH_LOGND));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.logn.xdouble", GASM_INSTR_CODE_FMATH_LOGNX));

    m_funcMap.insert(std::make_pair<scString, int>("fmath.sqrt.float", GASM_INSTR_CODE_FMATH_SQRTF));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.sqrt.double", GASM_INSTR_CODE_FMATH_SQRTD));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.sqrt.xdouble", GASM_INSTR_CODE_FMATH_SQRTX));

    m_funcMap.insert(std::make_pair<scString, int>("fmath.pi.float", GASM_INSTR_CODE_FMATH_PI_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.pi.double", GASM_INSTR_CODE_FMATH_PI_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.pi.xdouble", GASM_INSTR_CODE_FMATH_PI_XDOUBLE));

    m_funcMap.insert(std::make_pair<scString, int>("fmath.e.float", GASM_INSTR_CODE_FMATH_E_FLOAT));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.e.double", GASM_INSTR_CODE_FMATH_E_DOUBLE));
    m_funcMap.insert(std::make_pair<scString, int>("fmath.e.xdouble", GASM_INSTR_CODE_FMATH_E_XDOUBLE));
  }
  
  virtual sgpFunction *createFunction(const scString &a_name) { 
    sgpFunction *res;
    sgpFFFuncMap::iterator it = m_funcMap.find(a_name);
    if (it != m_funcMap.end()) {
      int code = it->second;   
      switch (code) {
        case GASM_INSTR_CODE_FMATH_SINF:    res = new sgpFuncFMathSinf(); break;
        case GASM_INSTR_CODE_FMATH_SIND:    res = new sgpFuncFMathSind(); break;
        case GASM_INSTR_CODE_FMATH_SINX:    res = new sgpFuncFMathSinx(); break;

        case GASM_INSTR_CODE_FMATH_COSF:    res = new sgpFuncFMathCosf(); break;
        case GASM_INSTR_CODE_FMATH_COSD:    res = new sgpFuncFMathCosd(); break;
        case GASM_INSTR_CODE_FMATH_COSX:    res = new sgpFuncFMathCosx(); break;

        case GASM_INSTR_CODE_FMATH_TANF:    res = new sgpFuncFMathTanf(); break;
        case GASM_INSTR_CODE_FMATH_TAND:    res = new sgpFuncFMathTand(); break;
        case GASM_INSTR_CODE_FMATH_TANX:    res = new sgpFuncFMathTanx(); break;

        case GASM_INSTR_CODE_FMATH_ASINF:    res = new sgpFuncFMathAsinf(); break;
        case GASM_INSTR_CODE_FMATH_ASIND:    res = new sgpFuncFMathAsind(); break;
        case GASM_INSTR_CODE_FMATH_ASINX:    res = new sgpFuncFMathAsinx(); break;

        case GASM_INSTR_CODE_FMATH_ACOSF:    res = new sgpFuncFMathAcosf(); break;
        case GASM_INSTR_CODE_FMATH_ACOSD:    res = new sgpFuncFMathAcosd(); break;
        case GASM_INSTR_CODE_FMATH_ACOSX:    res = new sgpFuncFMathAcosx(); break;

        case GASM_INSTR_CODE_FMATH_ATANF:    res = new sgpFuncFMathAtanf(); break;
        case GASM_INSTR_CODE_FMATH_ATAND:    res = new sgpFuncFMathAtand(); break;
        case GASM_INSTR_CODE_FMATH_ATANX:    res = new sgpFuncFMathAtanx(); break;

        case GASM_INSTR_CODE_FMATH_SINHF:    res = new sgpFuncFMathSinhf(); break;
        case GASM_INSTR_CODE_FMATH_SINHD:    res = new sgpFuncFMathSinhd(); break;
        case GASM_INSTR_CODE_FMATH_SINHX:    res = new sgpFuncFMathSinhx(); break;

        case GASM_INSTR_CODE_FMATH_COSHF:    res = new sgpFuncFMathCoshf(); break;
        case GASM_INSTR_CODE_FMATH_COSHD:    res = new sgpFuncFMathCoshd(); break;
        case GASM_INSTR_CODE_FMATH_COSHX:    res = new sgpFuncFMathCoshx(); break;

        case GASM_INSTR_CODE_FMATH_TANHF:    res = new sgpFuncFMathTanhf(); break;
        case GASM_INSTR_CODE_FMATH_TANHD:    res = new sgpFuncFMathTanhd(); break;
        case GASM_INSTR_CODE_FMATH_TANHX:    res = new sgpFuncFMathTanhx(); break;

        case GASM_INSTR_CODE_FMATH_EXPF:    res = new sgpFuncFMathExpf(); break;
        case GASM_INSTR_CODE_FMATH_EXPD:    res = new sgpFuncFMathExpd(); break;
        case GASM_INSTR_CODE_FMATH_EXPX:    res = new sgpFuncFMathExpx(); break;

        case GASM_INSTR_CODE_FMATH_POWF:    res = new sgpFuncFMathPowf(); break;
        case GASM_INSTR_CODE_FMATH_POWD:    res = new sgpFuncFMathPowd(); break;
        case GASM_INSTR_CODE_FMATH_POWX:    res = new sgpFuncFMathPowx(); break;

        case GASM_INSTR_CODE_FMATH_POW2F:    res = new sgpFuncFMathPow2f(); break;
        case GASM_INSTR_CODE_FMATH_POW2D:    res = new sgpFuncFMathPow2d(); break;
        case GASM_INSTR_CODE_FMATH_POW2X:    res = new sgpFuncFMathPow2x(); break;

        case GASM_INSTR_CODE_FMATH_LOGF:    res = new sgpFuncFMathLogf(); break;
        case GASM_INSTR_CODE_FMATH_LOGD:    res = new sgpFuncFMathLogd(); break;
        case GASM_INSTR_CODE_FMATH_LOGX:    res = new sgpFuncFMathLogx(); break;

        case GASM_INSTR_CODE_FMATH_LOGNF:    res = new sgpFuncFMathLognf(); break;
        case GASM_INSTR_CODE_FMATH_LOGND:    res = new sgpFuncFMathLognd(); break;
        case GASM_INSTR_CODE_FMATH_LOGNX:    res = new sgpFuncFMathLognx(); break;

        case GASM_INSTR_CODE_FMATH_SQRTF:    res = new sgpFuncFMathSqrtf(); break;
        case GASM_INSTR_CODE_FMATH_SQRTD:    res = new sgpFuncFMathSqrtd(); break;
        case GASM_INSTR_CODE_FMATH_SQRTX:    res = new sgpFuncFMathSqrtx(); break;

        case GASM_INSTR_CODE_FMATH_E_FLOAT:    res = new sgpFuncFMathEFloat(); break;
        case GASM_INSTR_CODE_FMATH_E_DOUBLE:    res = new sgpFuncFMathEDouble(); break;
        case GASM_INSTR_CODE_FMATH_E_XDOUBLE:    res = new sgpFuncFMathEXDouble(); break;

        case GASM_INSTR_CODE_FMATH_PI_FLOAT:    res = new sgpFuncFMathPiFloat(); break;
        case GASM_INSTR_CODE_FMATH_PI_DOUBLE:    res = new sgpFuncFMathPiDouble(); break;
        case GASM_INSTR_CODE_FMATH_PI_XDOUBLE:    res = new sgpFuncFMathPiXDouble(); break;

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
// sgpFunLibFMath
// ----------------------------------------------------------------------------
sgpFunLibFMath::sgpFunLibFMath(): sgpFunLib()
{
}

sgpFunLibFMath::~sgpFunLibFMath()
{
}

void sgpFunLibFMath::registerFuncs()
{
  sgpFunFactory *x = addFactory(new sgpFFMath());
  dynamic_cast<sgpFFMath *>(x)->init();
  
//------
  registerFunction("fmath.sin.float", x);
  registerFunction("fmath.sin.double", x);
  registerFunction("fmath.sin.xdouble", x);
//------
  registerFunction("fmath.cos.float", x);
  registerFunction("fmath.cos.double", x);
  registerFunction("fmath.cos.xdouble", x);
//------
  registerFunction("fmath.tan.float", x);
  registerFunction("fmath.tan.double", x);
  registerFunction("fmath.tan.xdouble", x);
//------
  registerFunction("fmath.asin.float", x);
  registerFunction("fmath.asin.double", x);
  registerFunction("fmath.asin.xdouble", x);
//------
  registerFunction("fmath.acos.float", x);
  registerFunction("fmath.acos.double", x);
  registerFunction("fmath.acos.xdouble", x);
//------
  registerFunction("fmath.atan.float", x);
  registerFunction("fmath.atan.double", x);
  registerFunction("fmath.atan.xdouble", x);
//------
  registerFunction("fmath.sinh.float", x);
  registerFunction("fmath.sinh.double", x);
  registerFunction("fmath.sinh.xdouble", x);
//------
  registerFunction("fmath.cosh.float", x);
  registerFunction("fmath.cosh.double", x);
  registerFunction("fmath.cosh.xdouble", x);
//------
  registerFunction("fmath.tanh.float", x);
  registerFunction("fmath.tanh.double", x);
  registerFunction("fmath.tanh.xdouble", x);
//------
  registerFunction("fmath.exp.float", x);
  registerFunction("fmath.exp.double", x);
  registerFunction("fmath.exp.xdouble", x);
//------
  registerFunction("fmath.pow.float", x);
  registerFunction("fmath.pow.double", x);
  registerFunction("fmath.pow.xdouble", x);
//------
  registerFunction("fmath.pow2.float", x);
  registerFunction("fmath.pow2.double", x);
  registerFunction("fmath.pow2.xdouble", x);
//------
  registerFunction("fmath.log.float", x);
  registerFunction("fmath.log.double", x);
  registerFunction("fmath.log.xdouble", x);
//------
  registerFunction("fmath.logn.float", x);
  registerFunction("fmath.logn.double", x);
  registerFunction("fmath.logn.xdouble", x);
//------
  registerFunction("fmath.sqrt.float", x);
  registerFunction("fmath.sqrt.double", x);
  registerFunction("fmath.sqrt.xdouble", x);
//------
  registerFunction("fmath.e.float", x);
  registerFunction("fmath.e.double", x);
  registerFunction("fmath.e.xdouble", x);

  registerFunction("fmath.pi.float", x);
  registerFunction("fmath.pi.double", x);
  registerFunction("fmath.pi.xdouble", x);
//------
}
