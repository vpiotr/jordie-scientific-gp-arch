/////////////////////////////////////////////////////////////////////////////
// Name:        GasmGeneration.cpp
// Project:     sgpLib
// Purpose:     Container for GP entities
// Author:      Piotr Likus
// Modified by:
// Created:     13/07/2013
/////////////////////////////////////////////////////////////////////////////

#include "sgp/GasmGeneration.h"

// ----------------------------------------------------------------------------
// sgpInfoBlockVarMap
// ----------------------------------------------------------------------------
void sgpInfoBlockVarMap::insert(uint varId, uint infoIndex, const scString &aName)
{
  m_idToIndexMap.insert(std::make_pair<uint,uint>(varId, infoIndex));
  m_indexToNameMap.insert(std::make_pair<uint,scString>(infoIndex, aName));
}

bool sgpInfoBlockVarMap::getInfoIndex(uint varId, uint &output) const
{
  bool res = false;
  
  std::map<uint,uint>::const_iterator it = m_idToIndexMap.find(varId);
  
  if (it != m_idToIndexMap.end()) {
    res = true;
    output = it->second;
  }
  return res;
}

bool sgpInfoBlockVarMap::getVarNameByIndex(uint infoIndex, scString &output) const
{
  bool res = false;
  
  std::map<uint,scString>::const_iterator it = m_indexToNameMap.find(infoIndex);
  
  if (it != m_indexToNameMap.end()) {
    res = true;
    output = it->second;
  }
  return res;
}

bool sgpInfoBlockVarMap::isVarDefined(uint varId) const
{
  uint idx;
  return getInfoIndex(varId, idx);
}

