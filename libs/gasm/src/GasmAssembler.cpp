/////////////////////////////////////////////////////////////////////////////
// Name:        GasmAssembler.cpp
// Project:     sgpLib
// Purpose:     Class for converting text to bytecode.
// Author:      Piotr Likus
// Modified by:
// Created:     09/02/2009
/////////////////////////////////////////////////////////////////////////////

#include "sc/utils.h"
#include "sgp/GasmAssembler.h"
#include <climits>

#ifdef DEBUG_MEM
#include "sc/DebugMem.h"
#endif

using namespace dtp;

sgpAssembler::sgpAssembler()
{
  m_typeNames.addChild("int", new scDataNode((uint)gdtfInt));
  m_typeNames.addChild("int64", new scDataNode((uint)gdtfInt64));
  m_typeNames.addChild("byte", new scDataNode((uint)gdtfByte));
  m_typeNames.addChild("uint", new scDataNode((uint)gdtfUInt));
  m_typeNames.addChild("uint64", new scDataNode((uint)gdtfUInt64));
  m_typeNames.addChild("float", new scDataNode((uint)gdtfFloat));
  m_typeNames.addChild("double", new scDataNode((uint)gdtfDouble));
  m_typeNames.addChild("xdouble", new scDataNode((uint)gdtfXDouble));
  m_typeNames.addChild("bool", new scDataNode((uint)gdtfBool));
  m_typeNames.addChild("string", new scDataNode((uint)gdtfString));
  m_typeNames.addChild("variant", new scDataNode((uint)gdtfVariant));
  m_typeNames.addChild("struct", new scDataNode((uint)gdtfStruct));
  m_typeNames.addChild("array", new scDataNode((uint)gdtfArray));
}

sgpAssembler::~sgpAssembler()
{
}

bool sgpAssembler::parseText(const scStringList &lines, scDataNode &outputCode)
{
  bool res = true;
  sgpProgramCode prg;
  scDataNode lexBlock;
  cell_size_t currAddr = 1;
  scDataNode blockLabels;
  scString cmd;
  scDataNode params;
  scDataNode blockInputParams;
  scDataNode blockOutputParams;
  
  for(uint i=0,epos = lines.size(); i != epos; i++) {
    if (!parseLine(lines[i], currAddr, blockLabels, cmd, params))
    {
      res = false;
      break;
    }  
    if (cmd.substr(0, 1) == ".") {
      if (cmd == ".block") {
        if (lexBlock.size() > 0) {
          throw scError("Block not ended before line no. "+toString(i));
        } 
      } else if (cmd == ".end") {      
        scDataNode blockCode;
        processBlockCode(lexBlock, blockLabels, blockCode);
        prg.addBlock(blockInputParams, blockOutputParams, blockCode);
        blockInputParams.clear(); blockOutputParams.clear(); lexBlock.clear(); blockLabels.clear();
        currAddr = 0;
      } else if (cmd == ".input") {      
        processBlockArgs(params, blockInputParams);
      } else if (cmd == ".output") {      
        processBlockArgs(params, blockOutputParams);
      } else {
        throw scError("Unknown macro: "+cmd+" in line no. "+toString(i));
      } 
    } else if (cmd.length() > 0) {// not a macro
      std::auto_ptr<scDataNode> newCmd(new scDataNode());
      newCmd->addChild(new scDataNode(cmd));
      newCmd->addChild(new scDataNode(params));
      lexBlock.addChild(newCmd.release());
      currAddr += (1 + params.size());
    }
  }
  
  outputCode = prg.getFullCode();
  return res;
}

void sgpAssembler::setFunctionList(const sgpFunctionMapColn &functions)
{
  m_functions = functions; 
  m_functionNameMap.clear();
  uint code = 0;

  for(sgpFunctionMapColn::const_iterator it=functions.begin(),
      epos=functions.end(); 
      it!=epos; 
      it++)
  {
    m_functionNameMap.addChild(it->second->getName(), new scDataNode(code));
    code++;
  }  
}

bool sgpAssembler::parseLine(const scString &aLine, cell_size_t currAddr, scDataNode &labels, scString &cmd, scDataNode &paramList)
{
  bool res = true;
  scString scanCmd = aLine;

  scanCmd = strTrim(scanCmd);
  if (scanCmd.empty() || (scanCmd.substr(0,1) == ";")) {
    cmd = "";
    return res;
  }  
    
  int c = 0, cp1 = 0;
  scString::size_type i, m;
  scString::size_type endPos = scanCmd.length();
  int ctx;
  int quoteChar = 0;
  scString cmdName, paramValue;
  scString labelName;

  enum {CTX_START, CTX_LABEL, CTX_CMD, 
    CTX_PARAM_LIST, CTX_PARAM,
    CTX_PARAM_VAL, CTX_PARAM_VAL_QUOTED, 
    CTX_END
  };  

  paramList.clear();      
  ctx = CTX_START;
  i = 0;
  while( (i<(int)endPos) && (ctx != CTX_END))  
  {
    c = scanCmd[i];

    if (i < scanCmd.length() - 1) cp1 = scanCmd[i + 1];
    else cp1 = 0;
        
    if (c < 32) {     
      throwSyntaxError(scanCmd, i, c); 
    }  

    if (ctx == CTX_PARAM_VAL_QUOTED) {
      if (c == quoteChar) {
        if (cp1 == quoteChar) {
          paramValue += char(c);
          i += 2;
        } else {
        // end of quote value
          scDataNode quotedStr;
          quotedStr.addChild("quoted", new scDataNode(true));          
          quotedStr.addChild("value", new scDataNode(paramValue));
          
          paramList.addChild(new scDataNode(quotedStr));
          paramValue = "";
          quoteChar = 0;
          
          m = scanCmd.find_first_not_of(" ", i+1);
          if (m != scString::npos) {
            if (scanCmd[m] == ',') {
              i = m+1;
              ctx = CTX_PARAM;
            } else {
              throwSyntaxError(scanCmd, m, c); 
            } 
          }
          else {
            ctx = CTX_END;
            ++i;
          }            
        }  
      } else {
      // standard char inside quoted string
        paramValue += char(c);
        i++;
      }
    } else {
    // not inside quoted string
      if (isSpecial(c)) { // ":,
        switch (c)
        {
          case ':': 
            if ((ctx == CTX_START) && (isStartOfName(cp1))) {
              ctx = CTX_LABEL;
              i++;
            } else {
              throwSyntaxError(scanCmd, i, c); 
            }
            break;
          case '"': 
            if (ctx == CTX_PARAM) {
              ctx = CTX_PARAM_VAL_QUOTED;
              quoteChar = c;
              i++;
            } else {
              throwSyntaxError(scanCmd, i, c); 
            }
            break;
          case ',': 
            if ((ctx == CTX_PARAM) || (ctx == CTX_PARAM_VAL)) {
              ctx = CTX_PARAM;
              
              paramList.addChild(new scDataNode(paramValue));
              paramValue = "";
              
              m = scanCmd.find_first_not_of(" ", i+1);
              if (m != scString::npos) {
                ctx = CTX_PARAM;
                i = m;
              }
              else {
                ctx = CTX_END;
                i = endPos;
              }            
            } else {
            // incorrect context
              throwSyntaxError(scanCmd, i, c); 
            }
            break;
          default:   
            throwSyntaxError(scanCmd, i, c); 
        } // switch c
      } else {
      // not a special char
        switch (ctx)
        {
         case CTX_START:
           cmdName += char(c);
           ctx = CTX_CMD;
           ++i;
           break;
         case CTX_LABEL:
           if (c == ' ') {
             labels.addChild(labelName, new scDataNode(currAddr));
             labelName = "";

             m = scanCmd.find_first_not_of(" ", i);
             if (m != scString::npos) {
               if (scanCmd[m] == '.' || isStartOfName(scanCmd[m])) {
                 ctx = CTX_CMD; 
                 i = m;
                 cmdName = "";
               } else {
                 throwSyntaxError(scanCmd, m, scanCmd[m]);                
               }
             } else {
               ctx = CTX_END;
               i = endPos;
             }            
           } else { // label continues  
             labelName += char(c);
             ++i;
           }  
           break;
         case CTX_CMD: 
           if (c == ' ') {
             m = scanCmd.find_first_not_of(" ", i);
             if (m != scString::npos) {
               i = m;
               ctx = CTX_PARAM;           
             } else {
               i = endPos;
               ctx = CTX_END;
             }  
           } else {
             cmdName += char(c);
             ++i;
           }
           break;
         case CTX_PARAM: 
           paramValue = "";
           if (c != ' ') {
             paramValue += char(c);
             ctx = CTX_PARAM_VAL;  
           }  
           ++i;
           break;         
         case CTX_PARAM_VAL: 
           if (c != ' ')
             paramValue += char(c);
           ++i;
           break;         
        default:
          throwSyntaxError(scanCmd, i, c); 
          break;
        } // switch context for non-special char  
      }
    }
  } // while

  if (labelName.length() > 0) 
     labels.addChild(labelName, new scDataNode(currAddr));
  
  if (paramValue.length() > 0) {
    if (quoteChar != 0) {
      scDataNode quotedStr;
      quotedStr.addChild("quoted", new scDataNode(true));          
      quotedStr.addChild("value", new scDataNode(paramValue));
      paramList.addChild(new scDataNode(quotedStr));
    } else {
      paramList.addChild(new scDataNode(paramValue));
    }  
  }
  
  cmd = cmdName;
  return res;
}

void sgpAssembler::throwSyntaxError(const scString &text, int a_pos, int a_char) const {
   throw scError("gasm syntax error, wrong character ["+toString(a_char)+"] in command ["+text+"] at pos: "+toString(a_pos));
}

bool sgpAssembler::isStartOfName(int c)
{
  return (
    (
      (c >= 'A') && (c <= 'Z')
    )
      ||
    (
      (c >= 'a') && (c <= 'z')
    )
  );
}

bool sgpAssembler::isSpecial(int c)
{
  bool res;
  switch (c) {
    case ':': case '"': case ',': res = true; break;
    default: res = false;
  }
  return res;      
}

// convert type names to list of nodes with a given type
void sgpAssembler::processBlockArgs(const scDataNode &params, scDataNode &blockParams)
{
  scDataNode element;
  scString typeName;

  blockParams.clear();  
  for(uint i=0,epos=params.size(); i!=epos;i++) {
    params.getElement(i, element);
    typeName = element.getAsString();
    sgpGvmDataTypeFlag aType;
    if (!checkDataTypeName(typeName, aType))
      throw scError("Unknown type: ["+typeName+"]");
    scDataNode typeValue;
    sgpVMachine::initDataNodeAs(aType, typeValue);
    blockParams.addChild(new scDataNode(typeValue));  
  }
}

bool sgpAssembler::checkDataTypeName(const scString &name, sgpGvmDataTypeFlag &output) {
  bool res;
  
  if (!m_typeNames.hasChild(name)) {
    res = false;
  } else {  
    uint typeCode = m_typeNames.getUInt(name);  
    output = sgpGvmDataTypeFlag(typeCode);
    res = true;
  }  
  return res;
}

// - convert command name to integer value
// - encode arguments
// - substitute labels with jump size
void sgpAssembler::processBlockCode(const scDataNode &lexBlock, const scDataNode &blockLabels, scDataNode &blockCode)
{
  scString cmd;
  scDataNode commandWithArgs;
  scDataNode args;
  cell_size_t currAddr = 1;
  int icmd;

  blockCode.setAsArray(vt_datanode);
     
  for(uint i=0,epos=lexBlock.size(); i != epos; i++)
  {
    lexBlock.getElement(i, commandWithArgs);
    cmd = commandWithArgs.getString(0);
    icmd = getCommandCode(cmd);
    if (icmd < 0) {
      throw scError("Unknown instruction: "+cmd);
    } else {  
      decodeArgs(commandWithArgs.getElement(1), blockLabels, currAddr, args);
      blockCode.addItem(scDataNode(sgpVMachine::encodeInstr(icmd, args.size())));
      blockCode.addItemList(args);
      currAddr += (1 + args.size());
    }    
  }
}

int sgpAssembler::getCommandCode(const scString &cmdName)
{
  if (!m_functionNameMap.hasChild(cmdName))
    return -1;
  else 
    return m_functionNameMap.getUInt(cmdName);
}      

void sgpAssembler::decodeArgs(const scDataNode &lexArgs, const scDataNode &blockLabels, cell_size_t currAddr, scDataNode &output)
{
  scDataNode element;
  scString sValue;
  int c, c2, c3;
  cell_size_t jumpSize, jumpAddr;
  scDataNode newArg;
  bool isInt;
  int idx;
  sgpGvmDataTypeFlag argType;
  
  output.clear();
  output.setAsArray(vt_datanode);  
  
  for(uint i=0,epos=lexArgs.size(); i!=epos; i++) {
    lexArgs.getElement(i, element);
    if (element.isParent()) {
    // quoted string
      output.addItem(scDataNode(element.getString("value"))); 
    } else {
      sValue = element.getAsString();
      if (sValue.length() > 0)
        c = sValue[0];
      else
        c = 0;
      switch (c) {
        case '@': // label
          sValue = sValue.substr(1);
          if (!blockLabels.hasChild(sValue))
            throw scError("Unknown label ["+sValue+"] at address: "+toString(currAddr));

          jumpAddr = blockLabels.getUInt(sValue);
          if (jumpAddr < currAddr)
            jumpSize = (currAddr - jumpAddr)+(1+lexArgs.size());
          else  
            jumpSize = jumpAddr - (currAddr+1+lexArgs.size());

          output.addItem(scDataNode(jumpSize)); 
          break;
        case '#': // register
          sValue = sValue.substr(1);
          uint regNo;
          regNo = stringToUInt(sValue);
          regNo = UINT_MAX - SGP_MAX_REG_NO + regNo;
          newArg.setAsUInt(regNo);           
          //@newArg.clear();
          //@newArg.addChild(new scDataNode("arg_type", gatfRegister));
          //@newArg.addChild(new scDataNode("value", ));
          output.addItem(newArg); 
          break;
        default: // other constants
          sgpGvmDataTypeFlag aType;
          if (sValue.empty() || (sValue == "null")) {
            output.addItem(scDataNode()); 
          } else if (checkDataTypeName(sValue, aType)) {
            output.addItem(scDataNode(uint(aType))); 
          } else if (sValue == "true") { 
            output.addItem(scDataNode(true)); 
          } else if (sValue == "false") { 
            output.addItem(scDataNode(false)); 
          } else { // numeric
            isInt = true;
            idx = sValue.length() - 1;
            c2 = sValue[idx];
            if (idx > 0)
              c3 = sValue[idx - 1];
            else
              c3 = 0;
                
            argType = gdtfNull;
            switch (c2) {
             case 'I': 
               argType = gdtfInt;
               output.addItem(scDataNode(int(stringToInt(sValue.substr(0, idx))))); 
               break; 
             case 'B':
               argType = gdtfByte;
               output.addItem(scDataNode(byte(stringToInt(sValue.substr(0, idx))))); 
               break;
             case 'U':
               argType = gdtfUInt;
               output.addItem(scDataNode(uint(stringToUInt(sValue.substr(0, idx))))); 
               break;
             case 'L':
               if (c3 == 'U') {
                 argType = gdtfUInt64;
                 output.addItem(scDataNode(ulong64(stringToUInt64(sValue.substr(0, idx - 1))))); 
               } else { 
                 argType = gdtfInt64;
                 output.addItem(scDataNode(long64(stringToInt64(sValue.substr(0, idx))))); 
               }  
               break;
             case 'F':
               argType = gdtfFloat;
               output.addItem(scDataNode(float(stringToFloat(sValue.substr(0, idx))))); 
               break;
             case 'D':
               argType = gdtfDouble;
               output.addItem(scDataNode(double(stringToDouble(sValue.substr(0, idx))))); 
               break;
             case 'X':
               argType = gdtfXDouble;
               output.addItem(scDataNode(xdouble(stringToXDouble(sValue.substr(0, idx))))); 
               break;
             default:
               if (sValue.find(".") != scString::npos) {
                 argType = gdtfDouble;
                 output.addItem(scDataNode(double(stringToDouble(sValue)))); 
               } else { 
                 argType = gdtfInt;
                 output.addItem(scDataNode(int(stringToInt(sValue)))); 
               }  
            }  
            
          } // numeric 
      } // switch c   
    } // not a parent
  } // for loop
}//function
