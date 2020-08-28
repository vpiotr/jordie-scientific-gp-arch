/////////////////////////////////////////////////////////////////////////////
// Name:        GpFitnessFun4Gasm.cpp
// Project:     sgpLib
// Purpose:     Fitness function which executes Gasm program in order to 
//              calculate evolved function.
// Author:      Piotr Likus
// Modified by:
// Created:     12/06/2011
/////////////////////////////////////////////////////////////////////////////

#include "sc/defs.h"
#include "sc/timer.h"
#include "sc/log.h"

#include "sgp/GpFitnessFun4Gasm.h"

sgpFitnessFun4Gasm::sgpFitnessFun4Gasm(): inherited(), m_prepared(false)
{
  m_mainLib.reset(new sgpFunLib());
  m_supportedDataTypes = gdtfAll;
}

sgpFitnessFun4Gasm::~sgpFitnessFun4Gasm()
{
}

uint sgpFitnessFun4Gasm::getProgramStepLimit() const
{
  return 10000;
}

sgpFunctionMapColn &sgpFitnessFun4Gasm::getFunctions()
{
  return m_functions;
}

void sgpFitnessFun4Gasm::setSupportedDataTypes(uint mask)
{
  m_supportedDataTypes = mask;
}

void sgpFitnessFun4Gasm::initProcess(sgpGaGeneration &newGeneration)
{
  prepare();
  m_vmachine.reset(new sgpVMachine());
  initVMachine(*m_vmachine);  
}

void sgpFitnessFun4Gasm::initVMachine(sgpVMachine &vmachine) const 
{
  // do not log errors - it will be ignored anyway
  if ((vmachine.getFeatures() & ggfLogErrors) != 0)
    vmachine.setFeatures((vmachine.getFeatures() ^ ggfLogErrors));

  vmachine.setFunctionList(m_functions);
  //cannot disable variant
  //@ vmachine.setSupportedDataTypes(m_supportedDataTypes);

  // switch off references - for speed (if possible)
  if (!isFunctionSupported("ref.build"))
    if ((vmachine.getSupportedArgTypes() & gatfRef) != 0)
      vmachine.setSupportedArgTypes((vmachine.getSupportedArgTypes() ^ gatfRef));
      
  vmachine.setMaxStackDepth(10);
}  

bool sgpFitnessFun4Gasm::isFunctionSupported(const scString &aName) const 
{
  bool res = false;
  
  for(sgpFunctionMapColn::const_iterator it=m_functions.begin(),
      epos=m_functions.end(); 
      it!=epos; 
      it++)
  {
    if (aName == it->second->getName()) {
      res = true;
      break;
    }   
  }    

  return res;
}

void sgpFitnessFun4Gasm::setVMachineProgram(sgpVMachine &vmachine, const scDataNode &code) const
{
  _TRCSTEP_;
  sgpProgramCode prg;    
  prg.setFullCode(code);
#ifdef TRACE_TIME  
  scTimer::start(TIMER_PREPARE_CODE);  
#endif  
  vmachine.expandCode(prg);  
  vmachine.prepareCode(prg);
#ifdef TRACE_TIME  
  scTimer::stop(TIMER_PREPARE_CODE);  
#endif  
  _TRCSTEP_;
  vmachine.setProgramCode(prg);
  vmachine.forceInstrCache();
  _TRCSTEP_;
}  

sgpFunction *sgpFitnessFun4Gasm::getFunctorForInstrCode(uint instrCode) const {
  return ::getFunctorForInstrCode(m_functions, instrCode);
} 

void sgpFitnessFun4Gasm::initFunctionList()
{
  if (!m_functions.empty())
    return;

  scStringList incList;
  getFunctionList(incList);
  sgpFunLib::prepareFuncList(incList, scStringList(), m_functions);
}

void sgpFitnessFun4Gasm::prepare()
{
  if (m_prepared)
    return;
    
  intPrepare();
  m_prepared = true;
}

void sgpFitnessFun4Gasm::intPrepare()
{
  initLibs(*m_mainLib);
  initFunctionList();
  prepareFunctions();
}

void sgpFitnessFun4Gasm::prepareFunctions()
{
  for(sgpFunctionMapColn::iterator p = m_functions.begin(); p != m_functions.end(); p++)
  {
    p->second->prepare();
  }
}

void sgpFitnessFun4Gasm::runProgram(const scDataNode &input, scDataNode &output, uint startBlockNo) const
{      
#ifdef TRACE_TIME
  scTimer::start(TIMER_RUNPRG_CORE);
#endif
  try {              
    m_vmachine->resetWarmWay();
    m_vmachine->setInput(input);
    m_vmachine->setStartBlockNo(startBlockNo);
    m_vmachine->run(getProgramStepLimit());
  } 
  catch(scError &excp) {
    scLog::addError(scString("Exception (scError): ") + excp.what()+", details: "+excp.getDetails());
  }
  catch(const std::exception& e) {
    scLog::addError(scString("Exception (std): ") + e.what());
  }
  output = m_vmachine->getOutput();
#ifdef TRACE_TIME
  scTimer::stop(TIMER_RUNPRG_CORE);
#endif
}
