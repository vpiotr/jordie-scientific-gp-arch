/////////////////////////////////////////////////////////////////////////////
// Name:        GasmOperatorInit.h
// Project:     scLib
// Purpose:     Initialization operator for GP
// Author:      Piotr Likus
// Modified by:
// Created:     29/12/2009
/////////////////////////////////////////////////////////////////////////////


#ifndef _GASMOPERATORINIT_H__
#define _GASMOPERATORINIT_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file GasmOperatorInit.h
///
/// File description

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sc/dtypes.h"

#include "sgp/GasmEvolver.h"
#include "sgp/GaOperatorBasic.h"
#include "sgp/GasmVMachine.h"
#include "sgp/EntityIslandTool.h"

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

class sgpGasmOperatorInitEntity: public sgpGaOperatorInitEntity {
public:
  // construction
  sgpGasmOperatorInitEntity();
  virtual ~sgpGasmOperatorInitEntity();
  // properties
  void setFunctions(const sgpFunctionMapColn &functions);
  void getFunctions(sgpFunctionMapColn &functions);
  void setSizeRange(uint minSize, uint maxSize);
  void getSizeRange(uint &minSize, uint &maxSize);
  void setMaxBlockCount(uint value);
  uint getMaxBlockCount();
  void setDefaultBlockMeta(const scDataNode &value);
  void getDefaultBlockMeta(scDataNode &value);
  void setVMachine(sgpVMachine *machine);
  void setSupportedDataTypes(uint mask);
  void setInfoBlockMeta(const sgpGaGenomeMetaList &metaList);
  uint getActiveIslandId();
  void setActiveIslandId(uint islandId);
  void setIslandTool(sgpEntityIslandToolIntf *value);
  // run
  virtual void buildRandomEntity(sgpEntityBase &output);
protected:
  void buildRandomBlock(uint inputCount, uint blockIndex, scDataNode &output);
  bool verifyArgs(sgpGaGenome &newCode, sgpGasmRegSet &writtenRegs);
  virtual void addInfoVarPart(scDataNode &infoBlock);
private:
  uint m_activeIslandId;
  sgpFunctionMapColn m_functions;
  uint m_minSize;
  uint m_maxSize;
  uint m_maxBlockCount;
  scDataNode m_defaultBlockMeta;
  sgpVMachine *m_machine;  
  uint m_supportedDataTypes;
  sgpGaGenomeMetaList m_infoBlockMeta;
  sgpEntityIslandToolIntf *m_islandTool;
};

// ----------------------------------------------------------------------------
// sgpGasmOperatorInit
// ----------------------------------------------------------------------------
class sgpGasmOperatorInit: public sgpGaOperatorInitGen {
  typedef sgpGaOperatorInitGen inherited;
public:
  // construction
  sgpGasmOperatorInit();
  virtual ~sgpGasmOperatorInit();
  // properties
  void setFunctions(const sgpFunctionMapColn &functions);
  void getFunctions(sgpFunctionMapColn &functions);
  void setSizeRange(uint minSize, uint maxSize);
  void getSizeRange(uint &minSize, uint &maxSize);
  void setInfoVarPartSizeRange(uint minSize, uint maxSize);
  void getInfoVarPartSizeRange(uint &minSize, uint &maxSize);
  void setMaxBlockCount(uint value);
  uint getMaxBlockCount();
  void setDefaultBlockMeta(const scDataNode &value);
  void getDefaultBlockMeta(scDataNode &value);
  void setVMachine(sgpVMachine *machine);
  void setSupportedDataTypes(uint mask);
  void setInfoBlockMeta(const sgpGaGenomeMetaList &metaList);
  // run
  virtual void execute(uint limit, sgpGaGeneration &newGeneration);
  virtual void init();
protected:
  virtual void initOperatorInitEntity();
  void checkSizeLimits();
  sgpGaOperatorInitEntity *getOperatorInitEntity();
private:
  std::auto_ptr<sgpGaOperatorInitEntity> m_operatorInitEntity;
  sgpFunctionMapColn m_functions;
  uint m_minSize;
  uint m_maxSize;
  uint m_infoVarPartMinSize;
  uint m_infoVarPartMaxSize;
  uint m_maxBlockCount;
  scDataNode m_defaultBlockMeta;
  sgpVMachine *m_machine;  
  uint m_supportedDataTypes;
  sgpGaGenomeMetaList m_infoBlockMeta;
};



#endif // _GASMOPERATORINIT_H__
