/////////////////////////////////////////////////////////////////////////////
// Name:        GasmFunLib.cpp
// Project:     sgp
// Purpose:     Function library support
// Author:      Piotr Likus
// Modified by:
// Created:     05/02/2009
/////////////////////////////////////////////////////////////////////////////

//std
#include <set>

//sc
#include "sc/dtypes.h"
#include "sgp/GasmFunLib.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

sgpFunLib *sgpFunLib::m_mainLib = SC_NULL;

sgpFunction *sgpFunFactory::createFunction(const scString &a_name)
{
  return SC_NULL;
}  

sgpFunLib::sgpFunLib(): sgpFunFactory()
{
  if (m_mainLib == SC_NULL)
    m_mainLib = this;
}

sgpFunLib::~sgpFunLib()
{
  if (m_mainLib == this)
    m_mainLib = SC_NULL;
}

sgpFunLib *sgpFunLib::checkMainLib()
{
  if (m_mainLib == SC_NULL)
    throw scError("Sgp main function library not available");  
  return m_mainLib;  
}

void sgpFunLib::prepareFuncList( 
  const scStringList &includeList, const scStringList &excludeList,
  sgpFunctionMapColn &functions)
{
  checkMainLib()->intPrepareFuncList(includeList, excludeList, functions);
}  

void sgpFunLib::intPrepareFuncList( 
  const scStringList &includeList, const scStringList &excludeList,
  sgpFunctionMapColn &functions)
{
  intPrepareFuncBySubLibs(includeList, excludeList, functions);
  intPrepareFuncByFuncMap(includeList, excludeList, functions);
}  

void sgpFunLib::intPrepareFuncBySubLibs( 
  const scStringList &includeList, const scStringList &excludeList,
  sgpFunctionMapColn &functions)
{
  for(sgpFunLibColn::iterator p = m_subLibs.begin(); p != m_subLibs.end(); p++)
  {
    p->intPrepareFuncList(includeList, excludeList, functions);
  }
}  

void sgpFunLib::intPrepareFuncByFuncMap( 
  const scStringList &includeList, const scStringList &excludeList,
  sgpFunctionMapColn &functions)
{  
  std::set<scString> includeSet, excludeSet;
  scString funcNameMask;
  std::auto_ptr<sgpFunction> funcGuard;
  uint instrCode;
  int defCode;
  scString name;
  sgpFunFactoryMap::iterator itf;

  for(scStringList::const_iterator p = includeList.begin(); p != includeList.end(); p++)
    includeSet.insert(*p);

  for(scStringList::const_iterator p = excludeList.begin(); p != excludeList.end(); p++)
    excludeSet.insert(*p);
      
  for(sgpFunNameMap::iterator p = m_funcNameMap.begin(); p != m_funcNameMap.end(); p++)
  {
    name = p->second;
    funcNameMask = calcFuncNameFilter(name);
    itf = m_funcMap.find(name);
    if (itf == m_funcMap.end())
      continue;
    
    if (
      (
        (includeSet.size() == 0) 
          || 
        (includeSet.find(name) != includeSet.end())
          ||
        (includeSet.find(funcNameMask) != includeSet.end())
      )
      &&
      (
        (excludeSet.size() == 0) 
          || 
        (excludeSet.find(name) == includeSet.end())
          ||
        (excludeSet.find(funcNameMask) == includeSet.end())
      )  
    )
    {         
      defCode = itf->second->getInstrCode(name);
      if (defCode >= 0)
        instrCode = (uint)defCode;
      else
        instrCode = functions.size();
          
      funcGuard.reset(itf->second->createFunction(name));
      if (funcGuard.get() != SC_NULL)
        functions.insert(std::make_pair<uint,sgpFunctionTransporter>(instrCode, 
          sgpFunctionTransporter(funcGuard.release())));
    }  
  }
}  

void sgpFunLib::duplicate( 
  const sgpFunctionMapColn &input,
  sgpFunctionMapColn &output)
{
  checkMainLib()->intDuplicate(input, output);
}

void sgpFunLib::intDuplicate( 
  const sgpFunctionMapColn &input,
  sgpFunctionMapColn &output)
{
  scStringList includeList, emptyList;

  for(sgpFunctionMapColn::const_iterator it=input.begin(),epos = input.end(); it != epos; ++it)
  {
    includeList.push_back(it->second->getName());      
  }  
  intPrepareFuncList(includeList, emptyList, output);
}    

void sgpFunLib::addLib(sgpFunLib *lib)
{
  m_subLibs.push_back(lib);
  lib->init();
}

void sgpFunLib::addLibToMain(sgpFunLib *lib)
{  
  checkMainLib()->m_subLibs.push_back(lib);
  lib->init();
}

void sgpFunLib::init()
{
  registerFuncs();
}

void sgpFunLib::registerFuncs()
{ // empty here
}

void sgpFunLib::registerFunction(const scString &name, sgpFunFactory *factory)
{
  m_funcMap.insert(std::make_pair<scString, sgpFunFactory *>(name, factory));
  m_funcNameMap.insert(std::make_pair<uint, scString>(m_funcNameMap.size(), name));
}


// if name contains dot (.), replace everything after it with '*'
scString sgpFunLib::calcFuncNameFilter(const scString &a_name)
{
  scString res = a_name;
  size_t pos = res.find(".");
  if (pos != scString::npos)
    res.replace(pos+1, res.length() - pos - 1, "*");
  return res;  
}

sgpFunFactory *sgpFunLib::addFactory(sgpFunFactory *factory)
{
  m_factories.push_back(factory);
  return factory;
}
