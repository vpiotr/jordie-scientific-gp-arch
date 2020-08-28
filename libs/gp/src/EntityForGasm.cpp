/////////////////////////////////////////////////////////////////////////////
// Name:        EntityForGasm.cpp
// Project:     sgpLib
// Purpose:     Single entity storage class for GP algorithms.
// Author:      Piotr Likus
// Modified by:
// Created:     13/07/2013
/////////////////////////////////////////////////////////////////////////////

#include "sgp\EntityForGasm.h"

const ulong64 SGP_GASM_EV_MAGIC_NO1 = 0xa50505a5UL;
const ulong64 SGP_GASM_EV_MAGIC_NO2 = 0xe70303e7UL;

// ----------------------------------------------------------------------------
// sgpEntityForGasm
// ----------------------------------------------------------------------------

void sgpEntityForGasm::getGenomeAsNode(scDataNode &output, int offset, int count) const
{
  sgpProgramCode program;
  getGenomeAsProgram(program, offset, count);
  output = program.getFullCode();
}  

// returns code in form compatible with Gasm vmachine
// info block is returned as-is
void sgpEntityForGasm::getGenomeAsProgram(sgpProgramCode &program, int offset, int count) const
{ 
  std::auto_ptr<scDataNode> blockGuard;
  uint blockNo;
  scDataNode cellNode;
  sgpGaGenome genome;
  scDataNode inputArgs, outputArgs;
 
  blockNo = offset;

  if (count < 0)
    count = m_programGenome.size() - offset;

  program.clear();
    
  for(sgpGasmGenomeList::const_iterator it = m_programGenome.begin() + offset, epos = m_programGenome.begin() + offset + count; it != epos; ++it) {
    blockGuard.reset(new scDataNode());
    blockGuard->setAsList();
    
    genome = const_cast<sgpEntityForGasm *>(this)->m_programGenome[blockNo];
    for(sgpGaGenome::const_iterator it=genome.begin(),epos=genome.end(); it != epos; ++it)
    {
      blockGuard->addChild(new scDataNode(*it));
    }

    if (!m_programMeta[blockNo].isNull())
      program.addBlock(blockGuard.release(), new scDataNode(m_programMeta[blockNo]));
    else {
#ifdef DEBUG_ASM_CODE_STRUCT    
      if (!isInfoBlock(*blockGuard))
        throw scError(scString("No meta found for block: ")+toString(blockNo));
#endif        
      program.addBlock(blockGuard.release());
    }  
      
    blockNo++;
  }
}

void sgpEntityForGasm::setGenomeAsNode(const scDataNode &genome)
{
  sgpProgramCode program;
  program.setFullCode(genome);
  std::auto_ptr<scDataNode> guardMeta;
  
  m_programGenome.clear();
  m_programMeta.clear();
  
  m_programGenome.resize(program.getBlockCount());
  scDataNode blockCode;
  scDataNode rawBlock;
  
  for(uint i=0, epos = program.getBlockCount(); i != epos; i++) {
    program.getBlock(i, rawBlock);
    if (isInfoBlock(rawBlock)) {
      // copy whole block
      rawBlock.copyTo(m_programGenome[i]);
      // add null meta
      m_programMeta.addChild(new scDataNode()); // null value
    } else {      
      // copy block's code without meta   
      blockCode.clear();
      program.getBlockCode(i, blockCode);
      blockCode.copyTo(m_programGenome[i]);
      // copy block's meta
      guardMeta.reset(new scDataNode());
      program.getBlockMetaInfo(i, *guardMeta);
      m_programMeta.addChild(guardMeta.release());
    }    
  }
}

// skip evolving params
void sgpEntityForGasm::getProgramCode(scDataNode &output) const
{
  uint offset;  
  if (hasInfoBlock()) 
    offset = 1;
  else
    offset = 0;
      
  getGenomeAsNode(output, offset);
//  if (hasInfoBlock()) 
//    output.eraseElement(0);
}

void sgpEntityForGasm::getProgramCode(sgpProgramCode &output) const 
{
  uint offset;  
  if (hasInfoBlock()) 
    offset = 1;
  else
    offset = 0;
      
  getGenomeAsProgram(output, offset);
}

void sgpEntityForGasm::setProgramCode(const scDataNode &value)
{
  if (hasInfoBlock()) {
    scDataNode infoBlock;
    getInfoBlock(infoBlock);
    setGenomeAsNode(value);
    setInfoBlock(infoBlock);
  } else { 
    setGenomeAsNode(value);
  }  
}

void sgpEntityForGasm::getGenomeArgMeta(uint genomeNo, scDataNode &output)
{
  m_programMeta.getElement(genomeNo, output);
}

bool sgpEntityForGasm::isInfoBlock(uint genomeNo) const
{
  bool res = false;
  if (genomeNo == SGP_GASM_INFO_BLOCK_IDX)
    if (hasInfoBlock())
      res = true;
  return res;    
}

bool sgpEntityForGasm::hasInfoBlock() const
{
  return hasInfoBlock(m_programGenome);
}

bool sgpEntityForGasm::hasInfoBlock(const scDataNode &code)
{
  bool res = false;
  if (code.size() > 0) {
    scDataNode &block0 = const_cast<scDataNode &>(code)[SGP_GASM_INFO_BLOCK_IDX];
    if (block0.size() > 1) {
      scDataNodeValueType typ1 = block0[0].getValueType();
      if (typ1 == vt_uint64)       
      {       
        cell_size_t siz0 = block0.size();
        scDataNodeValueType typ2 = block0[siz0 - 1].getValueType();
        if (typ2 == vt_uint64) 
        {      
          if (
               (block0.getUInt64(0) == SGP_GASM_EV_MAGIC_NO1)
               &&
               (block0.getUInt64(siz0 - 1) == SGP_GASM_EV_MAGIC_NO2)
             )
          {
            res = true;
          }
        }       
      }  
    }
  }
  return res;
}

bool sgpEntityForGasm::hasInfoBlock(const sgpGasmGenomeList &genome)
{
  bool res = false;
  if (genome.size() > 0) {
    sgpGaGenome &block0 = const_cast<sgpGasmGenomeList &>(genome)[SGP_GASM_INFO_BLOCK_IDX];
    if (block0.size() > 0) {
      cell_size_t siz0 = block0.size();
      scDataNodeValueType typ1 = block0[0].getValueType();
      scDataNodeValueType typ2 = block0[siz0 - 1].getValueType();
      if ((typ1 == vt_uint64) && (typ2 == vt_uint64)) {
        if (
             (block0[0].getAsUInt64() == SGP_GASM_EV_MAGIC_NO1)
             &&
             (block0[siz0 - 1].getAsUInt64() == SGP_GASM_EV_MAGIC_NO2)
           )
        {
          res = true;
        }       
      }  
    }
  }
  return res;
}

bool sgpEntityForGasm::isInfoBlock(const scDataNode &code)
{
  bool res = false;
  if (code.size() > 1) {
    scDataNodeValueType typ1 = code.getElementType(0);
    if (typ1 == vt_uint64)       
    {       
      cell_size_t siz0 = code.size();
      scDataNodeValueType typ2 = code.getElementType(siz0 - 1);
      if (typ2 == vt_uint64) 
      {      
        if (
             (code.getUInt64(0) == SGP_GASM_EV_MAGIC_NO1)
             &&
             (code.getUInt64(siz0 - 1) == SGP_GASM_EV_MAGIC_NO2)
           )
        {
          res = true;
        }
      }       
    }  
  }
  return res;
}

void sgpEntityForGasm::buildMetaForInfoBlock(sgpGaGenomeMetaList &output)
{
  sgpGaGenomeMetaBuilder builder;
  builder.addMetaConst(scDataNode());
  builder.addMetaAlphaString("01", SGP_GASM_INFO_VAR_LEN);
  builder.addMetaAlphaString("01", SGP_GASM_INFO_VAR_LEN);
  builder.addMetaAlphaString("01", SGP_GASM_INFO_VAR_LEN);
  builder.addMetaConst(scDataNode());
  builder.getMeta(output);
  for(uint i=0, epos=output.size(); i != epos; i++)
    output[i].userType = ggtValue;
}

void sgpEntityForGasm::buildMetaForCode(const sgpGaGenome &genome, sgpGaGenomeMetaList &output)
{
  sgpGaGenomeMetaBuilder builder;
  output.clear();
  uint instrBytes = 0;
  uint instrCodeRaw, instrCode, argCount;
  uint userType;
  uint instrOffset;
  scDataNode value;
  uint epos = genome.size();
  bool validInstr;

  instrOffset = 0;
  while(instrOffset < epos) 
  {
    value = genome[instrOffset];
    if (instrBytes == 0) {       
      validInstr = false;
      
      if (value.getValueType() == vt_uint) {
        instrCode = value.getAsUInt();
        if (sgpVMachine::isEncodedInstrCode(instrCode)) 
          validInstr = true;
      } else {
        instrCode = 0; // for warnings
      }   
        
      if (validInstr) {
        sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
        instrBytes = argCount + 1;
        userType = ggtInstrCode;
      } else {
        instrBytes = 1;
        userType = ggtUnknInstrCode;
      }
    } else {
      if (sgpVMachine::getArgType(value) == gatfRegister) {
        userType = ggtRegNo;
      } else {
        userType = ggtValue;
      }    
    }  
    builder.addMetaValue(value);
    if (value.getValueType() == vt_string) {
      scString str = value.getAsString();
      if (!str.empty())
        builder.setLastSize(1 + str.length());
    }    
    builder.setLastUserType(userType);
    instrBytes--;
    instrOffset++;
  }  
  builder.getMeta(output);
  assert(output.size() == genome.size());
}

void sgpEntityForGasm::setInfoBlock(const scDataNode &input) 
{
  if (!hasInfoBlock()) {
    // insert new value at the start of m_programGenome
    sgpGasmGenomeList tmpGenome;
    tmpGenome.push_back(new sgpGaGenome());
    tmpGenome.transfer(tmpGenome.end(),
       m_programGenome.begin(),
       m_programGenome.end(),
       m_programGenome);
    assert(!m_programGenome.size());    
    m_programGenome.transfer(m_programGenome.end(),
//       tmpGenome.begin(),
       tmpGenome);  
    assert(!tmpGenome.size());    
    // insert new value at the start of m_programMeta
    scDataNode tmp;
    tmp.addChild(new scDataNode());
    tmp.transferChildrenFrom(m_programMeta);
    m_programMeta.clear();
    m_programMeta.transferChildrenFrom(tmp);
  }
  sgpGaGenome &genome = m_programGenome[SGP_GASM_INFO_BLOCK_IDX];
  genome.clear();
  genome.reserve(input.size() + 2);
  scDataNodeValue val;
  val.setAsUInt64(SGP_GASM_EV_MAGIC_NO1);
  genome.push_back(val);
  for(uint i=0, epos = input.size(); i != epos; i++) 
    genome.push_back(input[i]);
  val.setAsUInt64(SGP_GASM_EV_MAGIC_NO2);
  genome.push_back(val);
}

bool sgpEntityForGasm::getInfoBlock(scDataNode &output) const
{
  bool res = false;
  if (hasInfoBlock()) {
    output.copyValueFrom(m_programGenome[SGP_GASM_INFO_BLOCK_IDX]);
    output.eraseElement(0);
    output.eraseElement(output.size() - 1);
    res = true;
  }
  return res;
}

bool sgpEntityForGasm::getInfoDouble(uint infoId, double &output) const
{  
  uint targetIdx;
  bool res = getInfoValueIndex(infoId, targetIdx);
  if (res) {
    const scDataNodeValue &ref = m_programGenome[SGP_GASM_INFO_BLOCK_IDX][targetIdx];

    if (ref.getValueType() == vt_string)
      output = sgp::decodeBitDouble(ref.getAsString());
    else {  
      output = ref.getAsDouble();
    }  
  }  
  return res;
}

// returns minimal non-zero value for a given info parameter size
bool sgpEntityForGasm::getInfoDoubleMinNonZero(uint infoId, double &output) const
{
  uint targetIdx;
  bool res = getInfoValueIndex(infoId, targetIdx);
  if (res) {
    const scDataNodeValue &ref = m_programGenome[SGP_GASM_INFO_BLOCK_IDX][targetIdx];
    output = sgp::decodeBitDouble(
      sgp::encodeBitString(1, ref.getAsString().length()));
  }  
  return res;
}

int sgpEntityForGasm::getInfoValuePosInGenome(uint infoId) const
{
  uint resUInt;
  if (getInfoValueIndex(infoId, resUInt))
    return resUInt;
  else
    return 0;  
}

//Returns size of info block.
uint sgpEntityForGasm::getInfoBlockSize() const
{
  return m_programGenome[SGP_GASM_INFO_BLOCK_IDX].size();
}

//Returns pos of value in info block.
bool sgpEntityForGasm::getInfoValueIndex(uint infoId, uint &vindex) const
{
  assert(infoId != 0);
  int targetIdx = -1;
  if (m_infoMap != SC_NULL) {
    uint idx;
    if (m_infoMap->getInfoIndex(infoId, idx))
      targetIdx = idx;
  }  
  uint sizeOfInfo = m_programGenome[SGP_GASM_INFO_BLOCK_IDX].size();
  bool found = (targetIdx >= 0) && (sizeOfInfo > static_cast<uint>(targetIdx + 1));
  if (found) {
    vindex = static_cast<uint>(targetIdx + 1);
  } else { 
#ifdef OPT_LOG_INFO_ACCESS_ERROR  
    scString msg = scString("GE002: Error while accessing info param")+
        ", id: ["+toString(infoId)+"], "+
        ", pos: ["+toString(targetIdx)+"], "+
        ", info size: ["+toString(m_programGenome[SGP_GASM_INFO_BLOCK_IDX].size())+"]"; 
    scLog::addWarning(msg);    
#endif    
    vindex = sizeOfInfo;
  }  
  return found;
}

bool sgpEntityForGasm::setInfoDouble(uint infoId, double value)
{
  uint targetIdx;
  bool res = getInfoValueIndex(infoId, targetIdx);
  if (res) {
    const scDataNodeValue &ref = m_programGenome[SGP_GASM_INFO_BLOCK_IDX][targetIdx];

    if (ref.getValueType() == vt_string) {
      scString sVal;
      sgp::encodeBitDouble(value, ref.getAsString().length(), sVal);
      m_programGenome[SGP_GASM_INFO_BLOCK_IDX][targetIdx].setAsString(sVal);
    }  
    else {  
      m_programGenome[SGP_GASM_INFO_BLOCK_IDX][targetIdx].setAsDouble(value);
    }  
  }  
  return res;
}

bool sgpEntityForGasm::getInfoUInt(uint infoId, uint &output) const
{
  int targetIdx = -1;
  if (m_infoMap != SC_NULL) {
    uint idx;
    if (m_infoMap->getInfoIndex(infoId, idx))
      targetIdx = idx;
  }  
  
  bool res = (targetIdx >= 0);
  if (res) {
    const scDataNodeValue &ref = m_programGenome[SGP_GASM_INFO_BLOCK_IDX][targetIdx + 1];
    if (ref.getValueType() == vt_string)
      output = static_cast<uint>(sgp::decodeBitString(ref.getAsString()));
    else   
      output = ref.getAsUInt();
  }  
  return res;
}  

bool sgpEntityForGasm::setInfoUInt(uint infoId, uint value)
{
  uint targetIdx;
  bool res = getInfoValueIndex(infoId, targetIdx);
  if (res) {
    const scDataNodeValue &ref = m_programGenome[SGP_GASM_INFO_BLOCK_IDX][targetIdx];

    if (ref.getValueType() == vt_string) {
      scString sVal;
      sVal = sgp::encodeBitString(value, ref.getAsString().length());
      m_programGenome[SGP_GASM_INFO_BLOCK_IDX][targetIdx].setAsString(sVal);
    }  
    else {  
      m_programGenome[SGP_GASM_INFO_BLOCK_IDX][targetIdx].setAsUInt(value);
    }  
  }  
  return res;
}

void sgpEntityForGasm::castValueAsUInt(const scDataNodeValue &ref, uint &output) const
{
  if (ref.getValueType() == vt_string)
    output = static_cast<uint>(sgp::decodeBitString(ref.getAsString()));
  else   
    output = ref.getAsUInt();
}

