/////////////////////////////////////////////////////////////////////////////
// Name:        GpEntityListing.cpp
// Project:     sgpLib
// Purpose:     Functions for listing entities for export to file / reports.
// Author:      Piotr Likus
// Modified by:
// Created:     08/06/2011
/////////////////////////////////////////////////////////////////////////////

#include "sgp/GpEntityListing.h"
#include "sgp/GasmLister.h"

void listProgramToLines(const scDataNode &prg, sgpFunctionMapColn &functions, scStringList &lines)
{
  sgpLister lister;
  scStringList locLines;
  lister.setFunctionList(functions);
  lister.listProgram(prg, locLines);
  for(int i=0, epos = locLines.size(); i != epos; i++)
    lines.push_back(locLines[i]);
}

void listInfoBlockToLines(const scDataNode &infoBlock, const sgpInfoBlockVarMap *infoMap, 
  scStringList &lines)
{
  scString varName, varLabel;
  if (infoMap != SC_NULL) {
    for(uint i=0,epos=infoBlock.size();i!=epos;i++)
    {
      varName = toString(i)+".";
      if (infoMap->getVarNameByIndex(i, varLabel))
        varName += varLabel;
      lines.push_back(varName + ": " + infoBlock.getString(i));  
    }
  }
}

