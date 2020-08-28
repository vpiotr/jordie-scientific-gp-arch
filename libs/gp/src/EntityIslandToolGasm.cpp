/////////////////////////////////////////////////////////////////////////////
// Name:        EntityIslandToolGasm.cpp
// Project:     sgpLib
// Purpose:     Tool for island support - GASM-specific.
// Author:      Piotr Likus
// Modified by:
// Created:     23/07/2011
/////////////////////////////////////////////////////////////////////////////

#include "sgp\EntityIslandToolGasm.h"
#include "sgp\GasmEvolver.h"
#include "sgp\GasmOperator.h"

using namespace dtp;

void sgpEntityIslandToolGasm::setIslandLimit(uint value)
{
  m_islandLimit = value;
}

void sgpEntityIslandToolGasm::prepareIslandMap(const sgpGaGeneration &newGeneration, scDataNode &output)
{
  uint islandId;
  const sgpEntityForGasm *workInfo;
  scString islandName;

  assert(m_islandLimit > 0);
  for(uint i = newGeneration.beginPos(), epos = newGeneration.endPos(); i != epos; i++)
  {
    workInfo = checked_cast<const sgpEntityForGasm *>(newGeneration.atPtr(i));
    
    if (workInfo->getInfoUInt(SGP_INFOBLOCK_IDX_OTHER_PARAM_BASE + SGP_INFOPARAM_IDX_OTHER_PARAM_ISLAND_ID, islandId))
    {
      islandId = calcIslandId(islandId, m_islandLimit);                       
    } else {
      islandId = 0;
    }
    islandName = toString(islandId);
    if (!output.hasChild(islandName)) {
      output.addChild(new scDataNode(islandName));
      output[islandName].setAsArray(vt_uint);
    }  
    output[islandName].addItemAsUInt(i);  
  }    
}

bool sgpEntityIslandToolGasm::getIslandId(const sgpEntityBase &entity, uint &output)
{
  const sgpEntityForGasm *workInfo = checked_cast<const sgpEntityForGasm *>(&entity);
  bool res = true;
  int islandIdx = workInfo->getInfoValuePosInGenome(SGP_INFOBLOCK_IDX_OTHER_PARAM_BASE + SGP_INFOPARAM_IDX_OTHER_PARAM_ISLAND_ID);

  if ((islandIdx < 0) || (m_islandLimit == 0))
    res = false;
  
  if (res)  
  { 
    workInfo->getInfoUInt(SGP_INFOBLOCK_IDX_OTHER_PARAM_BASE + SGP_INFOPARAM_IDX_OTHER_PARAM_ISLAND_ID, output);
    output = ::calcIslandId(output, m_islandLimit);
  }
  return res;
}

bool sgpEntityIslandToolGasm::setIslandId(sgpEntityBase &entity, uint value)
{
  bool res = true;
  sgpEntityForGasm *workInfo = checked_cast<sgpEntityForGasm *>(&entity);
  int islandIdx = workInfo->getInfoValuePosInGenome(SGP_INFOBLOCK_IDX_OTHER_PARAM_BASE + SGP_INFOPARAM_IDX_OTHER_PARAM_ISLAND_ID);

  if (islandIdx < 0)
    res = false;
  
  if (res)  
  { 
    workInfo->setInfoUInt(SGP_INFOBLOCK_IDX_OTHER_PARAM_BASE + SGP_INFOPARAM_IDX_OTHER_PARAM_ISLAND_ID, value);
  }
  return res;
}
