/////////////////////////////////////////////////////////////////////////////
// Name:        GasmScannerForFitBlock.cpp
// Project:     sgpLib
// Purpose:     GASM scanner for fitness calculations. Single block version.
// Author:      Piotr Likus
// Modified by:
// Created:     12/06/2011
/////////////////////////////////////////////////////////////////////////////

#include "sgp/GasmScannerForFitBlock.h"
#include "sgp/GasmScannerForFitUtils.h"
#include "sc/smath.h"

  // -- construct
sgpGasmScannerForFitBlock::sgpGasmScannerForFitBlock(): 
  m_initDone(false), m_instrCodeCount(0), m_functions(SC_NULL)
{
}

sgpGasmScannerForFitBlock::~sgpGasmScannerForFitBlock()
{
}


// -- properties
void sgpGasmScannerForFitBlock::setGenome(const sgpGaGenome &value)
{
  m_genome = value;
}

void sgpGasmScannerForFitBlock::setCodeMeta(const sgpGaGenomeMetaList &value)
{
  m_codeMeta = value; 
}

void sgpGasmScannerForFitBlock::setInstrCodeCount(uint value)
{
  m_instrCodeCount = value;
}

uint sgpGasmScannerForFitBlock::getInstrCodeCount() const
{
  return m_instrCodeCount;
}

void sgpGasmScannerForFitBlock::setFunctions(sgpFunctionMapColn *value)
{
  m_functions = value;
}

// -- execute
void sgpGasmScannerForFitBlock::init()
{
  assert(m_functions != SC_NULL);
  m_instrCodeCount =  m_functions->size();
  m_initDone = true;
}

// read details of first block
bool sgpGasmScannerForFitBlock::scanFirstBlockCode(const sgpEntityForGasm &info, const scDataNode &code) 
{
  uint codeIndex;
  if (info.hasInfoBlock())
    codeIndex = 1;
  else
    codeIndex = 0;

  if (codeIndex >= static_cast<uint>(info.getGenomeCount())) {
    m_genome.clear();
    m_codeMeta.clear();
    return false;
  }
  
  return scanBlockCode(info, code, codeIndex);  
}

// read details of nth genome/block
bool sgpGasmScannerForFitBlock::scanBlockCode(const sgpEntityForGasm &info, const scDataNode &code, uint codeIndex) 
{
  info.getGenome(codeIndex, m_genome);      
  sgpEntityForGasm::buildMetaForCode(m_genome, m_codeMeta);
  return true;
}
  
// calculate how many input arguments has been used
uint sgpGasmScannerForFitBlock::countInputUsed(uint startRegNo, uint endRegNo)
{
  uint res;
  scDataNode value;
  uint regNo;
  std::set<uint> found;
  
  for(sgpGaGenomeMetaList::const_iterator it=m_codeMeta.begin(), epos = m_codeMeta.end(); it != epos; ++it)
  {
    if (it->userType == ggtRegNo) {
      value = m_genome[it - m_codeMeta.begin()];
      regNo = sgpVMachine::getRegisterNo(value);
      if ((regNo >= startRegNo) && (regNo < endRegNo))
        found.insert(regNo);
    }
  }
   
  res = found.size();
  return res;
}

// calculate min(distance to specified regs)
// input: range of required regs + global range scope (using mod), reg type: input,output,any
// output: 
//  0: all specified regs were used
//  0..1: some regs found to be used
//  1..n: minimal distance to specified regs (mod n)
// How to calc:
//  f(x) = 2 + min(distance)/max-distance - used-regs/req-regs
//  if input mode <> any -> verify if correct, otherwise do not use point 
//  for calculation
double sgpGasmScannerForFitBlock::calcRegDistance(uint searchMinRegNo, uint searchMaxRegNo, uint regNoMod, 
  sgpGvmArgIoMode ioMode, const sgpVMachine &vmachine) const
{
  double res;
  bool checkIoEnabled = (ioMode != gatfAny);

  scDataNode value;
  uint regNo;
  std::set<uint> found;
  int minDist = regNoMod; 
  int regDistMin, regDistMax;
  cell_size_t instrOffset = 0;
  cell_size_t readOffset = cell_size_t(0) - 1;
  scDataNode argMeta;
  
  for(sgpGaGenomeMetaList::const_iterator it=m_codeMeta.begin(), epos = m_codeMeta.end(); it != epos; ++it)
  {
    if (it->userType == ggtInstrCode) {
      instrOffset = it - m_codeMeta.begin();
    } else if (it->userType == ggtRegNo) {
      value = m_genome[it - m_codeMeta.begin()];
      regNo = sgpVMachine::getRegisterNo(value);
      if (checkIoEnabled) {
        if (readOffset != instrOffset) {
          getArgMeta(instrOffset, argMeta);
          readOffset = instrOffset;
        }  
        if (!sgpGasmScannerForFitUtils::verifyArgDir(argMeta, (it - m_codeMeta.begin()) - instrOffset - 1, ioMode))
          continue;
        if ((ioMode == gatfInput) && !vmachine.canReadRegister(regNo))
          continue;
        if ((ioMode == gatfOutput) && !vmachine.canWriteRegister(regNo))
          continue;         
      }
      if ((regNo >= searchMinRegNo) && (regNo <= searchMaxRegNo)) {
        found.insert(regNo);
        minDist = 0;
      } else {
        regDistMin = tourus_distance(searchMinRegNo, regNo, regNoMod); 
        regDistMax = tourus_distance(searchMaxRegNo, regNo, regNoMod); 
        if (regDistMin < minDist)
          minDist = regDistMin;
        if (regDistMax < minDist)
          minDist = regDistMax;  
      }          
    }
  }
   
  res = 2.0 + double(minDist)/double(regNoMod) - (found.size()/(1+searchMaxRegNo-searchMinRegNo));
  return res;
}

// Rate for all instructions:
// Calculates how much input is different then output (regs only)
// Protects against unbreakable instructions like "neg #230, #230"
// if input reg found in output arg: instr-rate = 1/2
// otherwise: 1.0 (if no regs, or reg not found)
// result: avg(instr-rate)
double sgpGasmScannerForFitBlock::calcRegIoDist() const
{
  std::set<uint> inputRegs;
  uint foundOutCnt;
  double res;

  scDataNode value;
  uint regNo;
  cell_size_t instrOffset = 0;
  cell_size_t argOffset;
  scDataNode argMeta;
  uint argIoMode;
  double rateSum = 0.0;
  uint instrCount = 0;
  bool instrAccepted;
  
  sgpGaGenomeMetaList::const_iterator argIterator;
  
  for(sgpGaGenomeMetaList::const_iterator it=m_codeMeta.begin(), epos = m_codeMeta.end(); it != epos; ++it)
  {
    if (it->userType == ggtInstrCode) {
      instrOffset = it - m_codeMeta.begin();
      getArgMeta(instrOffset, argMeta);
      
      argIterator = it;
      ++argIterator;
      argOffset = argIterator - m_codeMeta.begin();
      instrAccepted = false;
      while((argIterator != epos) && (argIterator->userType != ggtInstrCode))
      {
        if (argIterator->userType == ggtRegNo) {
          argIoMode = sgpGasmScannerForFitUtils::getArgIoMode(argMeta, argOffset - instrOffset - 1);

          if ((argIoMode & gatfInput) != 0) {
            value = m_genome[argOffset];
            regNo = sgpVMachine::getRegisterNo(value);                  
            inputRegs.insert(regNo);
          } 

          if ((argIoMode & gatfOutput) != 0) {
            instrAccepted = true;
          } 
        }  
        ++argIterator;
        ++argOffset;
      }
      instrAccepted = instrAccepted && (!inputRegs.empty());
      
      // list of input args ready - now find output regs in it

      foundOutCnt = 0;
      argIterator = it;
      ++argIterator;
      argOffset = argIterator - m_codeMeta.begin();
      if (instrAccepted)
      while((argIterator != epos) && (argIterator->userType != ggtInstrCode))
      {
        if (argIterator->userType == ggtRegNo) {
          argIoMode = sgpGasmScannerForFitUtils::getArgIoMode(argMeta, argOffset - instrOffset - 1);

          if (((argIoMode & gatfInput) == 0) && ((argIoMode & gatfOutput) != 0))
          {
            value = m_genome[argOffset];
            regNo = sgpVMachine::getRegisterNo(value);                  

            if (inputRegs.find(regNo) != inputRegs.end())
            {
              foundOutCnt++;
            }            
          } 
        }  
        ++argIterator;
        ++argOffset;
      }      
      // now use result for instr-rating
      if (instrAccepted) {
        instrCount++;
        rateSum += 1.0 / (1.0 + static_cast<double>(foundOutCnt));
      }  
    } // instr-code
  } // for

  if (instrCount > 0)
    res = rateSum / static_cast<double>(instrCount);
  else
    res = 1.0;  

  return res;
}
    
// returns number of writes to regs# that are not used later divided by total # of writes
double sgpGasmScannerForFitBlock::calcWritesNotUsed(uint regNoMod, double minRatio) const
{
  std::multiset<uint> foundWrites;
  double res;

  scDataNode value;
  uint regNo;
  cell_size_t instrOffset = 0;
  cell_size_t readOffset = cell_size_t(0) - 1;
  scDataNode argMeta;
  uint argIoMode;
  uint writeCount = 0;
  
  for(sgpGaGenomeMetaList::const_iterator it=m_codeMeta.begin(), epos = m_codeMeta.end(); it != epos; ++it)
  {
    if (it->userType == ggtInstrCode) {
      instrOffset = it - m_codeMeta.begin();
    } else if (it->userType == ggtRegNo) {
      value = m_genome[it - m_codeMeta.begin()];
      regNo = sgpVMachine::getRegisterNo(value);
      
      if (readOffset != instrOffset) {
        getArgMeta(instrOffset, argMeta);
        readOffset = instrOffset;
      }  
      
      argIoMode = sgpGasmScannerForFitUtils::getArgIoMode(argMeta, (it - m_codeMeta.begin()) - instrOffset - 1);
      if ((argIoMode & gatfInput) != 0) {
      // remove reg from not read writes
        foundWrites.erase(regNo);
      } 
      if ((argIoMode & gatfOutput) != 0) {
      // add reg to writes collection if not block output
        if (regNo != SGP_REGB_OUTPUT) {
          foundWrites.insert(regNo);
          writeCount++;
        } // not block output 
      } // output
    } // regNo
  } // for

  if (writeCount > 0) {  
    res = double(foundWrites.size())/double(writeCount);
    if (res < minRatio)
      res = minRatio;
  }   
  else  
    res = 0.0;
  res += 1.0;  
  return res;
}

// calculate how many writes are performed to input args
double sgpGasmScannerForFitBlock::calcWritesToInput(uint regNoMod, uint minInputRegNo, uint maxInputRegNo) const
{
  std::multiset<uint> foundWrites;
  double res;

  scDataNode value;
  uint regNo;
  cell_size_t instrOffset = 0;
  cell_size_t readOffset = cell_size_t(0) - 1;
  scDataNode argMeta;
  uint argIoMode;
  
  for(sgpGaGenomeMetaList::const_iterator it=m_codeMeta.begin(), epos = m_codeMeta.end(); it != epos; ++it)
  {
    if (it->userType == ggtInstrCode) {
      instrOffset = it - m_codeMeta.begin();
    } else if (it->userType == ggtRegNo) {
      value = m_genome[it - m_codeMeta.begin()];
      regNo = sgpVMachine::getRegisterNo(value);
      
      if (readOffset != instrOffset) {
        getArgMeta(instrOffset, argMeta);
        readOffset = instrOffset;
      }  
      
      argIoMode = sgpGasmScannerForFitUtils::getArgIoMode(argMeta, (it - m_codeMeta.begin()) - instrOffset - 1);
      if ((argIoMode & gatfOutput) != 0) {
      // add reg to writes collection if write to input
        if ((regNo >= minInputRegNo) && (regNo <= maxInputRegNo)) {
          foundWrites.insert(regNo);
        } 
      } // output
    } // regNo
  } // for

  res = double(foundWrites.size());
  res += 1.0;  
  return res;
}


// calculate number of writes to output reg#
// higher value - worse
double sgpGasmScannerForFitBlock::calcWritesToOutput(uint regNoMod, uint minOutputRegNo, uint maxOutputRegNo) const
{
  std::multiset<uint> foundWrites;
  double res;

  scDataNode value;
  uint regNo;
  cell_size_t instrOffset = 0;
  cell_size_t readOffset = cell_size_t(0) - 1;
  scDataNode argMeta;
  uint argIoMode;
  uint writeCnt = 0;
  
  for(sgpGaGenomeMetaList::const_iterator it=m_codeMeta.begin(), epos = m_codeMeta.end(); it != epos; ++it)
  {
    if (it->userType == ggtInstrCode) {
      instrOffset = it - m_codeMeta.begin();
    } else if (it->userType == ggtRegNo) {
      value = m_genome[it - m_codeMeta.begin()];
      regNo = sgpVMachine::getRegisterNo(value);
      
      if (readOffset != instrOffset) {
        getArgMeta(instrOffset, argMeta);
        readOffset = instrOffset;
      }  
      
      argIoMode = sgpGasmScannerForFitUtils::getArgIoMode(argMeta, (it - m_codeMeta.begin()) - instrOffset - 1);
      if ((argIoMode & gatfOutput) != 0) {
        writeCnt++;
      // add reg to writes collection if write to input
        if ((regNo >= minOutputRegNo) && (regNo <= maxOutputRegNo)) {
          foundWrites.insert(regNo);
        } 
      } // output
    } // regNo
  } // for

  res = double(foundWrites.size());
  if (foundWrites.size() >= 1)
  // cnt=1 should be same as cnt=0
    res = double(foundWrites.size() - 1);
  else  
    res = 0.0;
  if (writeCnt > 0)
    res = res / double(writeCnt);  
  res += 1.0;  
  return res;
}


// calculate number of writes after last write to output reg#
// higher value - worse
double sgpGasmScannerForFitBlock::calcOutputDistanceToEnd(uint regNoMod, uint minOutputRegNo, uint maxOutputRegNo, double minRate) const
{
  std::multiset<uint> foundWrites;
  double res;

  scDataNode value;
  uint regNo;
  cell_size_t instrOffset = 0;
  cell_size_t readOffset = cell_size_t(0) - 1;
  scDataNode argMeta;
  uint argIoMode;
  uint writeCnt = 0;
  uint writesAfter = 0;
  
  for(sgpGaGenomeMetaList::const_iterator it=m_codeMeta.begin(), epos = m_codeMeta.end(); it != epos; ++it)
  {
    if (it->userType == ggtInstrCode) {
      instrOffset = it - m_codeMeta.begin();
    } else if (it->userType == ggtRegNo) {
      value = m_genome[it - m_codeMeta.begin()];
      regNo = sgpVMachine::getRegisterNo(value);
      
      if (readOffset != instrOffset) {
        getArgMeta(instrOffset, argMeta);
        readOffset = instrOffset;
      }  
      
      argIoMode = sgpGasmScannerForFitUtils::getArgIoMode(argMeta, (it - m_codeMeta.begin()) - instrOffset - 1);
      if ((argIoMode & gatfOutput) != 0) {
        writeCnt++;
        if ((regNo >= minOutputRegNo) && (regNo <= maxOutputRegNo)) 
          writesAfter = 0;
        else  
          writesAfter++;
      } // output
    } // regNo
  } // for

  if (writeCnt >= 1) { 
    res = double(writesAfter)/double(writeCnt);
    if (res < minRate)
      res = minRate;
  }  
  else  
    res = 0.0;
  res += 1.0;  
  return res;
}


// calculate how many different registers has been used, exclude input args
double sgpGasmScannerForFitBlock::calcUsedRegs(uint regNoMod, uint minInputRegNo, uint maxInputRegNo) const
{
  std::set<uint> foundRegs;
  double res;

  scDataNode value;
  uint regNo;
  scDataNode argMeta;
  
  for(sgpGaGenomeMetaList::const_iterator it=m_codeMeta.begin(), epos = m_codeMeta.end(); it != epos; ++it)
  {
    if (it->userType == ggtRegNo) {
      value = m_genome[it - m_codeMeta.begin()];
      regNo = sgpVMachine::getRegisterNo(value);

      if (
           ((regNo < minInputRegNo) || (regNo > maxInputRegNo))
           &&
           (regNo != SGP_REGB_OUTPUT)
         )
      {
        foundRegs.insert(regNo);
      }         
    } // regNo
  } // for

  res = double(foundRegs.size())/double(regNoMod);
  res += 1.0;  
  return res;
}


// returns number of reads that are using uninitialized regs# divided by total # of reads
double sgpGasmScannerForFitBlock::calcUninitializedReads(uint regNoMod, uint minInputRegNo, uint maxInputRegNo) const
{
  std::set<uint> foundWrites;
  std::multiset<uint> foundReads;
  double res;

  scDataNode value;
  uint regNo;
  cell_size_t instrOffset = 0;
  cell_size_t readOffset = cell_size_t(0) - 1;
  scDataNode argMeta;
  uint argIoMode;
  uint readCount = 0;
  
  for(sgpGaGenomeMetaList::const_iterator it=m_codeMeta.begin(), epos = m_codeMeta.end(); it != epos; ++it)
  {
    if (it->userType == ggtInstrCode) {
      instrOffset = it - m_codeMeta.begin();
    } else if (it->userType == ggtRegNo) {
      value = m_genome[it - m_codeMeta.begin()];
      regNo = sgpVMachine::getRegisterNo(value);
      
      if (readOffset != instrOffset) {
        getArgMeta(instrOffset, argMeta);
        readOffset = instrOffset;
      }  
      
      argIoMode = sgpGasmScannerForFitUtils::getArgIoMode(argMeta, (it - m_codeMeta.begin()) - instrOffset - 1);
      if ((argIoMode & gatfInput) != 0) {
      // if write not found before & not block input - add to failed reads
        if ((regNo < minInputRegNo) || (regNo > maxInputRegNo)) {
          if (foundWrites.find(regNo) == foundWrites.end()) {
            foundReads.insert(regNo);
          } 
          readCount++;  
        }
      } 
      
      if ((argIoMode & gatfOutput) != 0) {
      // add reg to writes collection if not block output
        foundWrites.insert(regNo);
      } // output
    } // regNo
  } // for

  if (readCount > 0)  
    res = double(foundReads.size())/double(readCount);
  else  
    res = 0.0;
  res += 1.0;  
  return res;
}

// calculate how many unique instruction codes are used / total-instr-count
double sgpGasmScannerForFitBlock::calcUniqueInstrCodes(double maxReqRatio) const
{
  std::set<uint> foundCodes;
  double res;

  scDataNode value;
  cell_size_t instrOffset = 0;
  scDataNode argMeta;
  uint instrCodeRaw, argCount;
  
  for(sgpGaGenomeMetaList::const_iterator it=m_codeMeta.begin(), epos = m_codeMeta.end(); it != epos; ++it)
  {
    if (it->userType == ggtInstrCode) {
      instrOffset = it - m_codeMeta.begin();
      getInstrInfo(instrOffset, instrCodeRaw, argCount);
      foundCodes.insert(instrCodeRaw);
    }
  } // for

  uint totalCount = getInstrCodeCount();
  assert(totalCount > 0);

  res = double(foundCodes.size())/double(totalCount);

  // only 50% is required  
  if (res > maxReqRatio)
    res = maxReqRatio;  

  // add 1.0 for safety  
  res += 1.0;  
  return res;
}

// calc constant to all argument count ratio (50% is maximum required)
double sgpGasmScannerForFitBlock::calcConstantToArgRatio(double minReqRatio) const
{
  // -- count arg types
  uint constCount = 0;
  uint argCount = 0;
  double ratio;
  
  for(sgpGaGenomeMetaList::const_iterator it=m_codeMeta.begin(), epos = m_codeMeta.end(); it != epos; ++it)
  { 
    switch (it->userType) {
      case ggtRegNo:
        argCount++;
        break;
      case ggtValue:
        argCount++;
        constCount++;
        break;
      default:
        break; // do nothing  
    }
  }

  // -- calc ratio       
  if (argCount > 0) {
    ratio = double(constCount)/double(argCount);
    if (ratio < minReqRatio)
      ratio =  minReqRatio;
  } else {
    ratio = 0.0;
  }
          
  return 1.0 + ratio;      
}

// calculate instructions with constant-only arguments
double sgpGasmScannerForFitBlock::calcConstOnlyInstr(double minRatio) const
{
  double res;

  scDataNode value;
  uint regNo;
  cell_size_t instrOffset = 0;
  cell_size_t readOffset = cell_size_t(0) - 1;
  scDataNode argMeta;
  uint argIoMode;
  uint instrCount = 0;
  uint constInstrCount = 0;
  uint argCount = 0;
  
  for(sgpGaGenomeMetaList::const_iterator it=m_codeMeta.begin(), epos = m_codeMeta.end(); it != epos; ++it)
  {
    if (it->userType == ggtInstrCode) {
      if (argCount > 0) {
        constInstrCount++;
        argCount = 0;
      }  
      instrOffset = it - m_codeMeta.begin();
      instrCount++;
      if (readOffset != instrOffset) {
        getArgMeta(instrOffset, argMeta);
        readOffset = instrOffset;
        argCount = 0;
        for(uint i=0,epos=argMeta.size(); i!=epos; i++)
        {
          argIoMode = sgpGasmScannerForFitUtils::getArgIoMode(argMeta, i);
          if ((argIoMode & gatfInput) != 0) 
            argCount++;
        }
      }          
    } else if (it->userType == ggtRegNo) {
      value = m_genome[it - m_codeMeta.begin()];
      regNo = sgpVMachine::getRegisterNo(value);
              
      argIoMode = sgpGasmScannerForFitUtils::getArgIoMode(argMeta, (it - m_codeMeta.begin()) - instrOffset - 1);
      if ((argIoMode & gatfInput) != 0) {
      // if read from register - cancel argCount
        argCount = 0;
      }  
    } // regNo
  } // for

  if (argCount > 0) {
    constInstrCount++;
  }      

  if (instrCount > 0) {
    res = double(constInstrCount)/double(instrCount);
    if (res < minRatio)
      res = minRatio;
  }    
  else
    res = 0.0;  
  res += 1.0;  
  return res;
}
  
// calc ratio of too long sequences of the same instructions
// too long means > estimated correct maximum = 3  
// if sequence is shorter it is calculated as len = 1
double sgpGasmScannerForFitBlock::calcSameInstrCodeRatio() const
{
  uint lastInstrCode;
  double res;

  scDataNode value;
  cell_size_t instrOffset = 0;
  scDataNode argMeta;
  uint instrCodeRaw, argCount;
  lastInstrCode = uint(0) - 1;
  uint seqCount = 0;
  uint instrCount = 0;
  double seqLimit, seqMid;
  
  for(sgpGaGenomeMetaList::const_iterator it=m_codeMeta.begin(), epos = m_codeMeta.end(); it != epos; ++it)
  {
    if (it->userType == ggtInstrCode) {
      instrOffset = it - m_codeMeta.begin();
      getInstrInfo(instrOffset, instrCodeRaw, argCount);
      instrCount++;
      if (instrCodeRaw != lastInstrCode) {
        seqCount++;
        lastInstrCode = instrCodeRaw;
      }
    }
  } // for

  if (instrCount > 0) { 
    // max sequence count required is 1/2 * number of intructions
    seqLimit = double(instrCount)/2.0;
    seqMid = seqCount;
    if (seqMid > seqLimit)
      seqMid = seqLimit; 
    res = seqMid/double(instrCount);
  } else  
    res = 0.0;

  res += 1.0;  
  return res;
}
  
void sgpGasmScannerForFitBlock::getArgMeta(cell_size_t instrOffset, scDataNode &argMeta) const
{
  argMeta.clear();
  
  uint instrCode = m_genome[instrOffset].getAsUInt();
  uint instrCodeRaw, argCount;  
    
  sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
  sgpFunction *func = getFunctorForInstrCode(instrCodeRaw);
  if (func != SC_NULL) { 
    func->getArgMeta(argMeta);     
  }                
}

void sgpGasmScannerForFitBlock::getInstrInfo(cell_size_t instrOffset, uint &instrCodeRaw, uint &argCount) const
{
  uint instrCode = m_genome[instrOffset].getAsUInt();
   
  sgpVMachine::decodeInstr(instrCode, instrCodeRaw, argCount);
}

sgpFunction *sgpGasmScannerForFitBlock::getFunctorForInstrCode(uint instrCode) const {
  assert(m_functions != SC_NULL);
  return ::getFunctorForInstrCode(*m_functions, instrCode);
}  
