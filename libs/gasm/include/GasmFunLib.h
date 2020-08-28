/////////////////////////////////////////////////////////////////////////////
// Name:        GasmFunLib.h
// Project:     sgp
// Purpose:     Function library support
// Author:      Piotr Likus
// Modified by:
// Created:     05/02/2009
/////////////////////////////////////////////////////////////////////////////

#ifndef _GASMFUNLIB_H__
#define _GASMFUNLIB_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GasmFunLib.h
///
/// File description

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
//boost
#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_list.hpp>

// sgp
#include "sgp/GasmVMachine.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------
class sgpFunLib;
class sgpFunFactory;

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Class definitions
// ---------------------------------------------------------------------------- 
typedef boost::ptr_list<sgpFunLib> sgpFunLibColn;
typedef std::map<scString,sgpFunFactory *> sgpFunFactoryMap;
typedef boost::ptr_list<sgpFunFactory> sgpFunFactoryColn;
typedef std::map<uint,scString> sgpFunNameMap;

class sgpFunFactory {
public:
  sgpFunFactory(int defaultCode = -1) {m_defaultCode = defaultCode;};
  virtual ~sgpFunFactory() {};
  virtual sgpFunction *createFunction(const scString &a_name);
  virtual int getInstrCode(const scString &a_name) {return m_defaultCode;};
protected:
  int m_defaultCode;  
};

class sgpFunLib: public sgpFunFactory {
public:
  sgpFunLib();
  virtual ~sgpFunLib();
  void init();
  static void prepareFuncList( 
    const scStringList &includeList, const scStringList &excludeList,
    sgpFunctionMapColn &functions);
  static void duplicate( 
    const sgpFunctionMapColn &input,
    sgpFunctionMapColn &output);
  static void addLibToMain(sgpFunLib *lib);  
  void addLib(sgpFunLib *lib);  
protected:  
  virtual void registerFuncs();
  static sgpFunLib *checkMainLib();
  void intPrepareFuncList( 
    const scStringList &includeList, const scStringList &excludeList,
    sgpFunctionMapColn &functions);
  void intPrepareFuncBySubLibs( 
    const scStringList &includeList, const scStringList &excludeList,
    sgpFunctionMapColn &functions);
  void intPrepareFuncByFuncMap( 
    const scStringList &includeList, const scStringList &excludeList,
    sgpFunctionMapColn &functions);
  void registerFunction(const scString &name, sgpFunFactory *factory);   
  scString calcFuncNameFilter(const scString &a_name);
  sgpFunFactory *addFactory(sgpFunFactory *factory);
  void intDuplicate(const sgpFunctionMapColn &input, sgpFunctionMapColn &output);
protected:  
  static sgpFunLib *m_mainLib;
  sgpFunLibColn m_subLibs;
  sgpFunFactoryMap m_funcMap;
  sgpFunNameMap m_funcNameMap;
  sgpFunFactoryColn m_factories;
};


#endif // _GASMFUNLIB_H__
