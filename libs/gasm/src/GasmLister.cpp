/////////////////////////////////////////////////////////////////////////////
// Name:        GasmLister.cpp
// Project:     sgp
// Purpose:     List Gasm program as assembler code.
// Author:      Piotr Likus
// Modified by:
// Created:     08/02/2009
/////////////////////////////////////////////////////////////////////////////

#include "sc/utils.h"
#include "sgp/GasmLister.h"

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

using namespace dtp;

sgpLister::sgpLister()
{
  setFeatures(slfDefault);
}

sgpLister::~sgpLister()
{
}

uint sgpLister::getFeatures()
{
  return m_features;
}
  
void sgpLister::setFeatures(uint mask)
{
  m_features = mask;
}  

void sgpLister::setFunctionList(const sgpFunctionMapColn &functions)
{  
  m_functions = functions; 
}

void sgpLister::listProgram(const scDataNode &code, scStringList &output)
{
  sgpProgramCode program;
  scDataNode blockCode, inputArgs, outputArgs;

  program.setFullCode(code); 
  output.clear();
  for(uint i=0,cnt=program.getBlockCount(); i<cnt; i++)
  {
    program.getBlockMetaInfo(i, inputArgs, outputArgs);
    program.getBlock(i, blockCode);
    listBlock(i, inputArgs, outputArgs, blockCode, output);
  }
}

void sgpLister::listBlock(uint blockNo, const scDataNode &inputArgs, const scDataNode &outputArgs, const scDataNode &blockCode, scStringList &output)
{
  output.push_back(".block");  
  output.push_back(".input "+descArgs(inputArgs));
  output.push_back(".output "+descArgs(outputArgs));
  listInstructions(blockCode, output);
  output.push_back(".end");    
}

scString sgpLister::descArgs(const scDataNode &args)
{
  scString res;
  scDataNode argList;
  scDataNode nameList;

  if (args.isArray() || args.isParent())
    argList = args;
  // otherwise empty argument list  

  scDataNode element;
  for(int i=0,epos = argList.size(); i!=epos; i++)
  {
    argList.getElement(i, element);
    switch (element.getValueType()) {
      case vt_byte:
        nameList.addItem(scDataNode("byte"));
        break;      
      case vt_int: 
        nameList.addItem(scDataNode("int"));
        break;      
      case vt_uint:
        nameList.addItem(scDataNode("uint"));
        break;      
      case vt_int64:
        nameList.addItem(scDataNode("int64"));
        break;      
      case vt_uint64:
        nameList.addItem(scDataNode("uint64"));
        break;      
      case vt_string:
        nameList.addItem(scDataNode("string"));
        break;      
      case vt_bool:
        nameList.addItem(scDataNode("bool"));
        break;      
      case vt_float:
        nameList.addItem(scDataNode("float"));
        break;      
      case vt_double:
        nameList.addItem(scDataNode("double"));
        break;      
      case vt_xdouble:
        nameList.addItem(scDataNode("xdouble"));
        break;      
      default:
      //case vt_parent: case vt_array: case vt_null:
      //case vt_vptr:
      //case vt_date: case vt_time: case vt_datetime:
        nameList.addItem(scDataNode("variant"));
        break;
    }  
  }   
  res = nameList.implode(",");
  return res;    
}

void sgpLister::listInstructions(const scDataNode &blockCode, scStringList &output)
{
  const scString linePfx = "   ";
  cell_size_t idx = 1;
  cell_size_t cnt = blockCode.size();
  uint instrCode, argCount, dataCnt;
  uint instrCodeCell;
  scDataNode labels, jumpArgs;
  bool bIsJump;
  scString labelName;

  scString name;
  scString line;

  // gen labels
  if ((slfDetectLabels & m_features) != 0) {
    idx = 1;
    while (idx < cnt) {
      if (extractInstr(blockCode, idx, instrCode, argCount, name, bIsJump)) {
        if (bIsJump && (argCount > 0))
          checkJump(blockCode, idx, instrCode, argCount, labels);
      } else {
        instrCodeCell = blockCode.getUInt(idx);
        sgpVMachine::decodeInstr(instrCodeCell, instrCode, argCount);
      }
      idx += (1 + argCount);
    }  
  }
  
  //debug: listUnknownData(blockCode, 1, cnt-1, output);  idx += cnt;
  idx = 1;
  while (idx < cnt) {
    if (checkLabel(idx, labels, labelName)) {
      line = ":"+labelName; 
        output.push_back(line);    
    }  
    if (extractInstr(blockCode, idx, instrCode, argCount, name, bIsJump)) {
      if (argCount > 0) {
        if (bIsJump) {
          checkJump(blockCode, idx, instrCode, argCount, labels, &jumpArgs);          
          line = descArgValues(blockCode, idx + 1, argCount, &jumpArgs);
        } else {
          line = descArgValues(blockCode, idx + 1, argCount);
        } 
        
        line = linePfx+name+" "+line;
        output.push_back(line);    
        if (name == "data") {        
          dataCnt = blockCode.getUInt(idx + 3);
          if (dataCnt > 0) {
            listUnknownData(blockCode, idx + 4, dataCnt, output);
            argCount += dataCnt;
          }  
        }  
      }  
      else {
        line = linePfx+name;
        output.push_back(line);    
      }  
    } else {
      instrCodeCell = blockCode.getUInt(idx);
      sgpVMachine::decodeInstr(instrCodeCell, instrCode, argCount);
      line = ".data "+descUnknownData(blockCode, idx, argCount+1);
      output.push_back(line);
    }
    idx += (1 + argCount);
  }
}

bool sgpLister::extractInstr(const scDataNode &blockCode, cell_size_t cellPos, uint &instrCode, uint &argCount, scString &name, bool &bIsJump)
{
  bool res;
  sgpFunction *functor;

  uint instrCodeCell = blockCode.getUInt(cellPos);
  sgpVMachine::decodeInstr(instrCodeCell, instrCode, argCount);

  uint minArgCount, maxArgCount;
    
  sgpFunctionMapColnIterator p = m_functions.find(instrCode);

  if (p == m_functions.end()) {
    res = false;
  } else {  
    sgpFunctionTransporter transporter = p->second;
    functor = &(*transporter);
    name = functor->getName();
    bIsJump = functor->isJumpAction();
  
    if (argCount == 0) {
      functor->getArgCount(minArgCount, maxArgCount);
      argCount = minArgCount;
    }
    res = true;
  }
  
  return res;
}

scString sgpLister::descArgValues(const scDataNode &blockCode, cell_size_t pos, cell_size_t aSize, const scDataNode *jumpArgs)
{
  scString res;
  scDataNode valueList;
  scDataNode cell;
  
  for(cell_size_t i=pos,epos = pos + aSize,bsize=blockCode.size(); i != epos && i < bsize; i++)
  {
    blockCode.getElement(i, cell);
    if ((jumpArgs != SC_NULL) && (jumpArgs->hasChild(toString(i-pos)))) {
      valueList.addChild(new scDataNode("@"+jumpArgs->getString(toString(i-pos))));
    } else {  
      valueList.addChild(new scDataNode(descArg(cell)));
    }  
  }
  res = valueList.implode(", ");
  return res;
}

// describe data (non-argument values, labels excluded)
scString sgpLister::descUnknownData(const scDataNode &blockCode, cell_size_t pos, cell_size_t aSize)
{
  scString res;
  scDataNode valueList;
  scDataNode cell;
  for(cell_size_t i=pos,epos = pos + aSize,bsize=blockCode.size(); i != epos && i < bsize; i++)
  {
    blockCode.getElement(i, cell);
    valueList.addChild(new scDataNode(descArg(cell)));
  }
  res = valueList.implode(", ");
  return res;
}

void sgpLister::listUnknownData(const scDataNode &blockCode, cell_size_t pos, cell_size_t aSize, scStringList &output)
{
  cell_size_t leftCnt = aSize;
  cell_size_t partCnt;
  cell_size_t currPos = pos;
  while (leftCnt > 0) {
    partCnt = (leftCnt > 8)?8:leftCnt;
    output.push_back(".data "+descUnknownData(blockCode, currPos, partCnt));
    leftCnt -= partCnt;
    currPos += partCnt;
  }
}

scString sgpLister::descArg(const scDataNode &cell)
{
  scString res;
  uint argType = cell.getUInt("arg_type", gatfConst);
  uint dataType; 
  switch (argType) {
    case gatfNull: res = "null"; break;
    case gatfRegister: 
      if (cell.hasChild("value"))
        res = cell.getString("value");
      else
        res = "?";
      res = "#"+res;  
      break;    
    case gatfConst: 
      if (cell.hasChild("value")) {
        res = cell.getString("value");
        dataType = const_cast<scDataNode &>(cell).getChildren().getByName("value").getValueType();
      } else {
        res = cell.getAsString();
        dataType = cell.getValueType();
      }
      // add value suffix, fix boolean
      switch (dataType) {
        case vt_byte:
          res += "B";
          break;      
        case vt_int: 
          break;      
        case vt_uint:
          if (cell.getAsUInt() >= UINT_MAX - SGP_MAX_REG_NO)
            res = "#" + toString((cell.getAsUInt() - (UINT_MAX - SGP_MAX_REG_NO)));
          else  
            res += "U";
          break;      
        case vt_int64:
          res += "L";
          break;      
        case vt_uint64:
          res += "LU";
          break;      
        case vt_bool:
          res = (res == "T")?"true":"false";
          break;      
        case vt_float:
          res += "F";
          break;      
        case vt_double:
          if (res.find(".") == scString::npos)
            res += "D";
          break;      
        case vt_xdouble:
          res += "X";
          break;      
        //case vt_string:
        //  break;      
        default:
          break;      
      } // switch datatype       
      break;
    default:
      res = "?";    
  }  
  return res;
}

// extract label name if exists
bool sgpLister::checkLabel(cell_size_t addr, const scDataNode &labels, scString &labelName)
{
  scString keyName = toString(addr);
  if (labels.hasChild(keyName)) {
    labelName = "L"+toHexString(labels.getUInt64(keyName));
    return true;
  } else {
    return false;
  }    
}

// check if this is a jump instruction, if yes, generate new labels
void sgpLister::checkJump(const scDataNode &blockCode, cell_size_t instrAddr, uint instrCode, uint argCount, scDataNode &labels, scDataNode *outJumpArgs)
{
  sgpFunctionMapColnIterator p = m_functions.find(instrCode);
  if (outJumpArgs != SC_NULL)
    outJumpArgs->clear();
    
  if (p != m_functions.end()) {
    scString keyName;
    sgpFunction *functor;
    sgpFunctionTransporter transporter = p->second;
    functor = &(*transporter);
    scDataNode argCell;
    
    if (functor->isJumpAction()) {
      scDataNode jumpArgs;
      long64 jumpValue;
      int argPos, argDir;
      functor->getJumpArgs(jumpArgs);
      for(uint i=0,epos = jumpArgs.size(); i!=epos; i++) {
        argPos = jumpArgs.getInt(i);

        if (argPos < 0) {
          argDir = -1;
          argPos = (-argPos)-1;
        } else {
          argDir = 1;
          argPos = argPos - 1;
        }

        if (instrAddr + argPos + 1 < blockCode.size()) {
          blockCode.getElement(instrAddr + argPos + 1, argCell); 
          if (!argCell.isArray() && !argCell.isParent() && !argCell.isNull()) {
            jumpValue = argCell.getAsUInt64();
            if (argDir < 0) {
              jumpValue = -jumpValue;
            }              
            jumpValue += (instrAddr + 1 + argCount);
            keyName = toString(jumpValue);
            
            if (outJumpArgs == SC_NULL) {
              if (!labels.hasChild(keyName))
                labels.addChild(keyName, new scDataNode(jumpValue));
            } else {
              if (labels.hasChild(keyName)) {
                outJumpArgs->addChild(toString(i), new scDataNode("L"+toHexString(jumpValue)));
              }                
            }  
          } // if not structure
        } // if argument inside the code 
      } // for 
    } // if jump 
  } // if instruction found 
} // function
