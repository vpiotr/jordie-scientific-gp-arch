/////////////////////////////////////////////////////////////////////////////
// Name:        EntityForGasm.h
// Project:     sgpLib
// Purpose:     Single entity storage class for GP algorithms.
// Author:      Piotr Likus
// Modified by:
// Created:     13/07/2013
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPENTFORGASM_H__
#define _SGPENTFORGASM_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file EntityForGasm.h
\brief Single entity storage class for GP algorithms.


*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sgp\EntityBase.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------
typedef boost::ptr_vector<sgpGaGenome> sgpGasmGenomeList;

// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
const ulong64 SGP_GASM_INFO_BLOCK_IDX = 0;
const uint SGP_GASM_INFO_VAR_LEN = 32;

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// sgpEntityForGasm
// ----------------------------------------------------------------------------
class sgpEntityForGasm: public sgpEntityBase {
public:
  sgpEntityForGasm() {}
  
  sgpEntityForGasm(const sgpEntityForGasm &src) {
    m_programGenome = src.m_programGenome;
    m_programMeta = src.m_programMeta;
    src.getFitness(m_fitness);
  }

  virtual sgpEntityBase &operator=(const sgpEntityBase &src) {
    if (&src != this) {
      m_programGenome = 
        dynamic_cast<sgpEntityForGasm &>(
          const_cast<sgpEntityBase &>(
            src
        )).m_programGenome;
        
      m_programMeta = 
        dynamic_cast<sgpEntityForGasm &>(
          const_cast<sgpEntityBase &>(
            src
        )).m_programMeta;
        
      src.getFitness(m_fitness);
    } 
    return *this;
  }
  // properties
  virtual void getGenome(int genomeNo, sgpGaGenome &output) const { 
    output = (m_programGenome[genomeNo]);
  }  
  virtual void getGenome(sgpGaGenome &output) const { throw scError("Do not use!"); }
  virtual const sgpGaGenome &getGenome() const { throw scError("Do not use!"); }
  virtual void setGenome(const sgpGaGenome &genome) { throw scError("Do not use!"); }
  virtual void setGenome(int genomeNo, const sgpGaGenome &genome) { 
    m_programGenome[genomeNo] = genome;
  }
  virtual uint getGenomeCount() const { return m_programGenome.size(); }

  virtual void getGenomeAsNode(scDataNode &output, int offset = 0, int count = -1) const;
  virtual void setGenomeAsNode(const scDataNode &genome);

  virtual void getGenomeAsProgram(sgpProgramCode &program, int offset = 0, int count = -1) const;
  
  //--- code support
  virtual void getProgramCode(scDataNode &output) const; // skip evolving params
  virtual void getProgramCode(sgpProgramCode &output) const; // skip evolving params
  virtual void setProgramCode(const scDataNode &value);
  virtual void getGenomeArgMeta(uint genomeNo, scDataNode &output);
  
  //--- info block support
  bool isInfoBlock(uint genomeNo) const;
  virtual bool hasInfoBlock() const;
  static bool hasInfoBlock(const scDataNode &input);
  static bool hasInfoBlock(const sgpGasmGenomeList &genome);
  static bool isInfoBlock(const scDataNode &code);
  static void buildMetaForInfoBlock(sgpGaGenomeMetaList &output);
  static void buildMetaForCode(const sgpGaGenome &genome, sgpGaGenomeMetaList &output);
  virtual bool getInfoBlock(scDataNode &output) const;
  virtual void setInfoBlock(const scDataNode &input);
  bool getInfoDouble(uint infoId, double &output) const;
  bool getInfoDoubleMinNonZero(uint infoId, double &output) const;
  bool getInfoUInt(uint infoId, uint &output) const;
  bool setInfoDouble(uint infoId, double value);
  bool setInfoUInt(uint infoId, uint value);
  virtual sgpInfoBlockVarMap *getInfoMap() {return m_infoMap;}
  void setInfoMap(sgpInfoBlockVarMap *value) {m_infoMap = value;}
  int getInfoValuePosInGenome(uint infoId) const;  
  void castValueAsUInt(const scDataNodeValue &ref, uint &output) const;
protected:  
  bool getInfoValueIndex(uint infoId, uint &vindex) const;
  uint getInfoBlockSize() const;
protected:
  sgpGasmGenomeList m_programGenome; // evolved part - code
  scDataNode m_programMeta; // meta information - not evolved
  sgpInfoBlockVarMap *m_infoMap;
};



#endif // _SGPENTFORGASM_H__
