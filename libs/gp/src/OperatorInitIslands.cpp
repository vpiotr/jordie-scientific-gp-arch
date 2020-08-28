/////////////////////////////////////////////////////////////////////////////
// Name:        OperatorInitIslands.cpp
// Project:     sgpLib
// Purpose:     Init operator for island-splitted populations
// Author:      Piotr Likus
// Modified by:
// Created:     19/06/2010
/////////////////////////////////////////////////////////////////////////////

#include "sgp/OperatorInitIslands.h"
#include "sc/smath.h"
#include "sc/utils.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

sgpOperatorInitIslands::sgpOperatorInitIslands(): inherited()
{
  m_usedIslandLimits.clear();
  m_usedIslandLimits.setAsParent();
  m_notifier.reset(new scNotifier);
}

sgpOperatorInitIslands::~sgpOperatorInitIslands()
{
}

// properties
void sgpOperatorInitIslands::setIslandLimits(const scDataNode &limits)
{ 
  m_islandLimits = limits;
}

void sgpOperatorInitIslands::getUsedIslandLimits(scDataNode &limits)
{
  limits = m_usedIslandLimits;
}  

uint sgpOperatorInitIslands::getUsedIslandLimit(uint islandId)
{
  scString islandName(toString(islandId));
  if (m_usedIslandLimits.hasChild(islandName))
    return m_usedIslandLimits.getUInt(islandName);
  else
    return 0;  
}

void sgpOperatorInitIslands::addListener(uint eventCode, scListener *listener)
{
  m_notifier->addListener(eventCode, listener);
}

void sgpOperatorInitIslands::removeListener(uint eventCode, scListener *listener)
{
  m_notifier->removeListener(eventCode, listener);
}

// run
void sgpOperatorInitIslands::execute(uint limit, sgpGaGeneration &newGeneration)
{
  initUsedLimits(limit);  
  executeInitAllIslands(newGeneration);
}

void sgpOperatorInitIslands::executeInitAllIslands(sgpGaGeneration &newGeneration)
{
  scString islandName;
  uint islandLimit;
  uint islandId;

  for(uint i=0, epos = m_usedIslandLimits.size(); i != epos; i++)
  {   
    islandName = m_usedIslandLimits.getElementName(i);  
    islandLimit = m_usedIslandLimits.getUInt(i);
    islandId = stringToUInt(islandName); 
    executeInitIsland(islandLimit, newGeneration, islandId);         
  }  
}

void sgpOperatorInitIslands::initUsedLimits(uint limit)
{    
  double sizeSum = 0.0;

  for(uint j=0, eposj = m_islandLimits.size(); j != eposj; j++)
  {   
    sizeSum += m_islandLimits.getDouble(j);
  }

  uint islandLimit, spaceLeft;
  uint islandId;
  
  spaceLeft = limit;

  m_usedIslandLimits.clear();
  m_usedIslandLimits.setAsParent();
  scString islandName;
  
  for(uint j=0, eposj = m_islandLimits.size(); j != eposj; j++)
  {   
    if (j < eposj - 1)
      islandLimit = round<uint>((m_islandLimits.getDouble(j)/sizeSum)*static_cast<double>(limit)); 
    else
      islandLimit = spaceLeft;
    islandName = m_islandLimits.getElementName(j);  
    islandId = stringToUInt(islandName); 
    m_usedIslandLimits.addChild(islandName, new scDataNode(islandLimit));
    spaceLeft -= islandLimit;
  }
}

void sgpOperatorInitIslands::executeInitIsland(uint limit, sgpGaGeneration &newGeneration, uint islandId)
{
  invokeIslandSelected(islandId);
  inherited::execute(limit, newGeneration);
}

void sgpOperatorInitIslands::invokeIslandSelected(uint islandId)
{ 
  scDataNode param(islandId);
  m_notifier->notify(SOII_ISLAND_SELECTED, &param);
}

