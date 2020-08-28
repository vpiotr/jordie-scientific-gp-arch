/////////////////////////////////////////////////////////////////////////////
// Name:        GasmGeneration.h
// Project:     sgpLib
// Purpose:     Entity container for GP algorithms.
// Author:      Piotr Likus
// Modified by:
// Created:     13/07/2013
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPGASMGENER_H__
#define _SGPGASMGENER_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file GasmGeneration.h
\brief Entity container for GP algorithms.

Long description
*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "sgp\EntityBase.h"
#include "sgp\EntityForGasm.h"

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
// ----------------------------------------------------------------------------
// sgpInfoBlockVarMap
// ----------------------------------------------------------------------------
class sgpInfoBlockVarMap {
public:
  sgpInfoBlockVarMap() {}
  virtual ~sgpInfoBlockVarMap() {}
  void insert(uint varId, uint infoIndex, const scString &aName);
  bool getInfoIndex(uint varId, uint &output) const;
  bool getVarNameByIndex(uint infoIndex, scString &output) const;
  bool isVarDefined(uint varId) const;
  uint size() const {return m_idToIndexMap.size(); }
protected:
  std::map<uint,uint> m_idToIndexMap;
  std::map<uint,scString> m_indexToNameMap;  
};

// ----------------------------------------------------------------------------
// sgpGasmGeneration
// ----------------------------------------------------------------------------
class sgpGasmGeneration: public sgpGaGeneration {
public:
  sgpGasmGeneration() {}
  virtual ~sgpGasmGeneration() {}

  void setInfoMap(sgpInfoBlockVarMap *map) {
    m_infoMap = map;
  }    
  
  virtual sgpEntityBase *cloneItem(int index) const {
    return newItem(
      *(
          dynamic_cast<sgpEntityForGasm *>(
            &(const_cast<sgpGasmGeneration *>(this)->m_items[index])
          )
       )   
    );
  }  

  virtual sgpEntityBase *newItem(const sgpEntityBase &src) const {
    return newItem(
      *(
          dynamic_cast<sgpEntityForGasm *>(
            &const_cast<sgpEntityBase &>(src)
          )
       )
    );
  }    
  
  virtual sgpEntityBase *newItem() const {
    std::auto_ptr<sgpEntityForGasm> infoGuard(new sgpEntityForGasm());
    infoGuard->setInfoMap(m_infoMap);
    return infoGuard.release();
  }  

  virtual sgpEntityBase *newItem(const sgpEntityForGasm &rhs) const {
    std::auto_ptr<sgpEntityForGasm> infoGuard(new sgpEntityForGasm(rhs));
    infoGuard->setInfoMap(m_infoMap);
    return infoGuard.release();
  }
  
  virtual sgpGaGeneration *newEmpty() const { 
    std::auto_ptr<sgpGasmGeneration> res(new sgpGasmGeneration());
    res->setInfoMap(m_infoMap);
    return res.release(); 
  }
  
protected:  
  sgpInfoBlockVarMap *m_infoMap;
};


#endif // _SGPGASMGENER_H__
