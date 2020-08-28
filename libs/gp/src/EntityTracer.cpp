/////////////////////////////////////////////////////////////////////////////
// Name:        EntityTracer.cpp
// Project:     scLib
// Purpose:     Trace what happens to entities
// Author:      Piotr Likus
// Modified by:
// Created:     08/11/2009
/////////////////////////////////////////////////////////////////////////////

// dtp
#include "base/date.h"

//perf
#include "perf/Log.h"

//sc
#include "sc/utils.h"

//sgp
#include "sgp/EntityTracer.h"

using namespace dtp;
using namespace perf;

sgpEntityTracer* sgpEntityTracer::m_activeTracer = SC_NULL;

#ifdef _DEBUG
#define TRACE_ET_TRACER
#endif

void prepareOutDir(const scString &outFilePath)
{
  scString dirName = extractDir(outFilePath);
  preparePath(dirName);
}

void saveStringToFile(const scString &text, const scString &outFilePath)
{
  prepareOutDir(outFilePath);
  scChar *cptr = const_cast<scChar *>(text.c_str());
  size_t len = text.length();
  saveBufferToFile(cptr, len, outFilePath);  
}

typedef std::pair<uint, uint> UIntUIntPair;

bool sort_second_pred_lt(const UIntUIntPair& left, const UIntUIntPair& right)
{
  return left.second < right.second;
}

bool sort_second_pred_gt(const UIntUIntPair& left, const UIntUIntPair& right)
{
  return left.second > right.second;
}

// ----------------------------------------------------------------------------
// sgpEntityTracer
// ----------------------------------------------------------------------------
sgpEntityTracer::sgpEntityTracer(const scString &dir, const scString &coreName)
{
  if (m_activeTracer == SC_NULL)
    m_activeTracer = this;
  m_coreDirName = dir;
  m_coreLogName = coreName;  
  m_entityLister = SC_NULL;
}

sgpEntityTracer::~sgpEntityTracer()
{
  if (m_activeTracer == this)
    m_activeTracer = SC_NULL;
}

// properties
void sgpEntityTracer::setCoreDirName(const scString &coreName)
{
  m_coreDirName = coreName;
}

void sgpEntityTracer::setCoreLogName(const scString &coreName)
{
  m_coreLogName = coreName;
}

void sgpEntityTracer::setFunctionId(uint value)
{
  m_functionId = value;
}

void sgpEntityTracer::setExperimentId(const scString &value)
{
  m_experimentId = value;
}  

void sgpEntityTracer::setShapeObjIndex(uint value)
{
  m_shapeObjIndex = value;
}

void sgpEntityTracer::setEntityLister(sgpEntityTracerListerIntf *lister)
{
  m_entityLister = lister;
}

sgpEntityTracer *sgpEntityTracer::checkTracer()
{
  if (m_activeTracer == SC_NULL)
    throw scError("Entity tracer not ready");
  return m_activeTracer;  
}

scString sgpEntityTracer::generateLogName(const scString &varPart, const scString &fileExt, bool fsPath)
{
  scString res(m_coreLogName);

    res = "{exp_id}\\" + res;
    res = m_coreDirName + res;

  strReplaceThis(res, "{exp_id}", m_experimentId.c_str(), true);
  strReplaceThis(res, "{var_part}", varPart.c_str(), true);
  strReplaceThis(res, "{ext}", fileExt.c_str(), true);
    
  if (!fsPath) 
    res = "file:///"+res;  
  return res;
}

// execution
void sgpEntityTracer::performDump()
{
  checkTracer()->intPerformDump();
}

void sgpEntityTracer::handleStart()
{
  checkTracer()->intHandleStart();
}

void sgpEntityTracer::handleStop()
{
  checkTracer()->intHandleStop();
}

void sgpEntityTracer::handleStepBegin(const sgpGaGeneration &newGeneration)
{
  checkTracer()->intHandleStepBegin(newGeneration);
}

void sgpEntityTracer::handleStepEnd()
{ 
  checkTracer()->intHandleStepEnd();
}

void sgpEntityTracer::verifyCodeOK(const sgpGaGeneration &newGeneration, const scString &context)
{
  checkTracer()->intVerifyCodeOK(newGeneration, context);
}

void sgpEntityTracer::handleSamplesChanged()
{
  checkTracer()->intHandleSamplesChanged();
}

void sgpEntityTracer::handleEntityBorn(uint entityIndex, const sgpEntityBase &entity, const scString &context)
{
  checkTracer()->intHandleEntityBorn(entityIndex, entity, context);
}

void sgpEntityTracer::handleEntityMoved(uint oldEntityIndex, uint newEntityIndex, const scString &context)
{
  checkTracer()->intHandleEntityMoved(oldEntityIndex, newEntityIndex, context);
}

// buffered move
void sgpEntityTracer::handleEntityMovedBuf(uint oldEntityIndex, uint newEntityIndex, const scString &context)
{
  checkTracer()->intHandleEntityMovedBuf(oldEntityIndex, newEntityIndex, context);
}

void sgpEntityTracer::handleEntityMovedByHandle(uint entityHandle, uint newEntityIndex, const scString &context)
{
  checkTracer()->intHandleEntityMovedByHandle(entityHandle, newEntityIndex, context);
}

void sgpEntityTracer::handleEntityMovedByHandleBuf(uint entityHandle, uint newEntityIndex, const scString &context)
{
  checkTracer()->intHandleEntityMovedByHandleBuf(entityHandle, newEntityIndex, context);
}

void sgpEntityTracer::handleEntityCopiedByHandle(uint entityHandle, uint newEntityIndex, const scString &context)
{
  checkTracer()->intHandleEntityCopiedByHandle(entityHandle, newEntityIndex, context);
}

void sgpEntityTracer::handleEntityCopiedByHandleBuf(uint entityHandle, uint newEntityIndex, const scString &context)
{
  checkTracer()->intHandleEntityCopiedByHandleBuf(entityHandle, newEntityIndex, context);
}

void sgpEntityTracer::handleEntityDeath(uint entityIndex, sgpEtEventCode eventCode, const scString &context)
{
  checkTracer()->intHandleEntityDeath(entityIndex, eventCode, context);
}

void sgpEntityTracer::handleEntityDeathBuf(uint entityIndex, sgpEtEventCode eventCode, const scString &context)
{
  checkTracer()->intHandleEntityDeathBuf(entityIndex, eventCode, context);
}

void sgpEntityTracer::handleEntityDeathByHandle(uint entityHandle, sgpEtEventCode eventCode, const scString &context)
{
  checkTracer()->intHandleEntityDeathByHandle(entityHandle, eventCode, context);
}

void sgpEntityTracer::handleXOver(const scDataNode &parentList, const scDataNode &childList)
{
  checkTracer()->intHandleXOver(parentList, childList);
}

void sgpEntityTracer::handleEntityChangedInPoint(uint entityIndex, const scString &changeCode, uint genomeIndex, uint changeOffset)
{
  checkTracer()->intHandleEntityChangedInPoint(entityIndex, changeCode, genomeIndex, changeOffset);
}

void sgpEntityTracer::handleEntityChanged(uint entityIndex, const sgpEntityBase &entity)
{
  checkTracer()->intHandleEntityChanged(entityIndex, entity);
}

void sgpEntityTracer::handleEntityEvaluated(uint entityIndex, const sgpFitnessValue &fitVector, const sgpEntityBase &entity)
{
  checkTracer()->intHandleEntityEvaluated(entityIndex, fitVector, entity);
}

void sgpEntityTracer::handleEntityEvent(uint entityIndex, const scString &eventCode)
{
  checkTracer()->intHandleEntityEvent(entityIndex, eventCode);
}

void sgpEntityTracer::handleSetEntityClass(uint entityIndex, uint classCode)
{
  checkTracer()->intHandleSetEntityClass(entityIndex, classCode);
}

void sgpEntityTracer::handleMatchResult(const scDataNode &winners, const scDataNode &loosers)
{
  checkTracer()->intHandleMatchResult(winners, loosers);
}

uint sgpEntityTracer::getEntityHandle(uint entityIndex)
{
  return checkTracer()->intGetEntityHandle(entityIndex);
}

void sgpEntityTracer::flushMoveBuffer()
{
  checkTracer()->intFlushMoveBuffer();
}

bool sgpEntityTracer::hasBufferedActionFor(uint entityIndex)
{
  return checkTracer()->intHasBufferedActionFor(entityIndex);
}

bool sgpEntityTracer::hasBufferedActionForHandle(uint entityHandle)
{
  return checkTracer()->intHasBufferedActionForHandle(entityHandle);
}

void sgpEntityTracer::intPerformDump()
{  
#ifdef TRACE_ET_TRACER
  Log::addInfo("Entity tracer: full dump - start");
#endif  
  sgpEtIndexSet activeEntities;
  getActiveEntities(activeEntities);
  dumpOverview(activeEntities);
  dumpEntityIndex(activeEntities);
  dumpEvents();
#ifdef TRACE_ET_TRACER
  Log::addInfo("Entity tracer: full dump - stop");
#endif  
  flushDeadEntities();
  resetStepStats();
}

void sgpEntityTracer::resetStepStats()
{
  m_matchCount = 0;
  m_matchWinnerCount = 0;
  m_matchLooserCount = 0;
  m_mutEntityChangedCount = 0;
  m_mutChangeCount = 0;
}

void sgpEntityTracer::intHandleStart()
{
#ifdef TRACE_ET_TRACER
  Log::addInfo("Entity tracer: start");
#endif
  m_stepNo = 0;
  m_entityCount = 0;
  m_destroyStatsPerClassAndReason.clear();
  m_destroyStatsPerClassAndReason.addChild("muts", new scDataNode());
  m_entities.clear();
  m_activeIndexMap.clear();
  m_activeIndexMap.setAsParent();
  m_unfairMatchPerObj.clear();
  m_moveBuffer.clear();
  resetStepStats();
}

void sgpEntityTracer::intHandleStop()
{
#ifdef TRACE_ET_TRACER
  Log::addInfo("Entity tracer: stop");
#endif
}

void sgpEntityTracer::intHandleStepBegin(const sgpGaGeneration &newGeneration)
{
#ifdef TRACE_ET_TRACER
  Log::addInfo(scString("Entity tracer: begin of step #")+toString(m_stepNo));
  intVerifyCodeOK(newGeneration, "step-begin");
#endif  
}

void sgpEntityTracer::intHandleStepEnd()
{
  m_stepNo++;
}

void sgpEntityTracer::intVerifyCodeOK(const sgpGaGeneration &newGeneration, const scString &context)
{
  for(uint i = 0, epos = newGeneration.size(); i != epos; i++)
  {
    logEntityDetsDiff(i, *(dynamic_cast<const sgpEntityBase *>(newGeneration.atPtr(i))), context);
  }  
}

void sgpEntityTracer::intHandleSamplesChanged()
{
  sgpEtIndexSet activeEntities;
  getActiveEntities(activeEntities);

  for(sgpEtIndexSet::const_iterator it = activeEntities.begin(), epos = activeEntities.end(); it != epos; ++it) {  
    addInfoEventReal(*it, ET_UNKNOWN_ENTITY_IDX, eecSamplesChanged, "", "", false);
  }
}

void sgpEntityTracer::intHandleEntityBorn(uint entityIndex, const sgpEntityBase &entity, const scString &context)
{
  uint realIndex = m_entities.size(); 
#ifdef TRACE_ET_TRACER
  Log::addInfo("Born, hnd = "+toString(realIndex)+", idx:"+toString(entityIndex));
#endif

  std::auto_ptr<scDataNode> newEntityGuard(new scDataNode());
  newEntityGuard->addChild(ET_ENTITY_FLD_BORN_STEP, new scDataNode(m_stepNo));
  scDataNode code, info, objs;
  extractEntityDets(entity, code, info, objs);
  newEntityGuard->addChild(ET_ENTITY_FLD_ACTIVE_DETS, new scDataNode());

  (*newEntityGuard)[ET_ENTITY_FLD_ACTIVE_DETS].addChild(ET_ENTITY_DETS_CODE, new scDataNode(code));
  (*newEntityGuard)[ET_ENTITY_FLD_ACTIVE_DETS].addChild(ET_ENTITY_DETS_INFO, new scDataNode(info));
  (*newEntityGuard)[ET_ENTITY_FLD_ACTIVE_DETS].addChild(ET_ENTITY_DETS_OBJS, new scDataNode(objs));

  newEntityGuard->addChild(ET_ENTITY_FLD_EVENTS, new scDataNode());
  newEntityGuard->addChild(ET_ENTITY_FLD_EVENT_COUNT, new scDataNode(0));
  newEntityGuard->addChild(ET_ENTITY_FLD_CLASS, new scDataNode(MAX_CLASS));
  
  m_activeIndexMap.setElementSafe(toString(entityIndex), scDataNode(realIndex));
  m_entities.addChild(newEntityGuard.release());
  
  addInfoEventReal(realIndex, entityIndex, eecBorn, "", context, true);
}

void sgpEntityTracer::intHandleEntityMoved(uint oldEntityIndex, uint newEntityIndex, const scString &context)
{
  uint realIndex = getEntityRealIndex(oldEntityIndex);

#ifdef TRACE_ET_TRACER
  Log::addInfo("Move, hnd = "+toString(realIndex)+", from:"+toString(oldEntityIndex)+"->"+toString(newEntityIndex));
#endif

  if (realIndex < m_entities.size()) {
    m_activeIndexMap.setElement(toString(newEntityIndex), scDataNode(realIndex));    
  }
  addInfoEventReal(realIndex, newEntityIndex, eecMoved, "moved, old:["+toString(oldEntityIndex)+"], new:["+toString(newEntityIndex)+"]", context, false);
}


void sgpEntityTracer::intHandleEntityMovedBuf(uint oldEntityIndex, uint newEntityIndex, const scString &context)
{
  uint realIndex = getEntityRealIndex(oldEntityIndex);  
#ifdef TRACE_ET_TRACER
  Log::addInfo("Move-buf, hnd = "+toString(realIndex)+", from:"+toString(oldEntityIndex)+"->"+toString(newEntityIndex));
#endif
  addBufferedAction(eecMoved, realIndex, newEntityIndex, context);
}

void sgpEntityTracer::intHandleEntityMovedByHandle(uint entityHandle, uint newEntityIndex, const scString &context)
{
#ifdef TRACE_ET_TRACER
  Log::addInfo("Move-hnd:"+toString(entityHandle)+"->"+toString(newEntityIndex));
#endif

  if (entityHandle < m_entities.size()) {
    m_activeIndexMap.setElement(toString(newEntityIndex), scDataNode(entityHandle));    
  }
  addInfoEventReal(entityHandle, newEntityIndex, eecMoved, "moved, hnd:["+toString(entityHandle)+"], new:["+toString(newEntityIndex)+"]", context, false);
}

void sgpEntityTracer::intHandleEntityMovedByHandleBuf(uint entityHandle, uint newEntityIndex, const scString &context)
{
#ifdef TRACE_ET_TRACER
  Log::addInfo("Move-hnd-buf:"+toString(entityHandle)+"->"+toString(newEntityIndex));
#endif
  addBufferedAction(eecMoved, entityHandle, newEntityIndex, context);
}

void sgpEntityTracer::intHandleEntityCopiedByHandle(uint entityHandle, uint newEntityIndex, const scString &context)
{
#ifdef TRACE_ET_TRACER
  Log::addInfo("Copy-hnd:"+toString(entityHandle)+"->"+toString(newEntityIndex));
#endif

  if (entityHandle < m_entities.size()) {
    std::auto_ptr<scDataNode> newEntityGuard(new scDataNode(m_entities[entityHandle]));

    (*newEntityGuard).setUInt(ET_ENTITY_FLD_BORN_STEP, m_stepNo);
    (*newEntityGuard)[ET_ENTITY_FLD_EVENTS].clear();
    (*newEntityGuard).setUInt(ET_ENTITY_FLD_EVENT_COUNT, 0);
    uint newEntityHandle = m_entities.size();
    m_entities.addChild(newEntityGuard.release());
    
    m_activeIndexMap.setElement(toString(newEntityIndex), scDataNode(newEntityHandle));    
    
    std::auto_ptr<scDataNode> newReferencesGuard(new scDataNode());
    std::auto_ptr<scDataNode> newReferenceItemGuard;

    newReferenceItemGuard.reset(new scDataNode());
    newReferenceItemGuard->addChild(ET_REF_FLD_NAME, new scDataNode("parent"));
    newReferenceItemGuard->addChild(ET_REF_FLD_LINK, new scDataNode());
    (*newReferenceItemGuard)[ET_REF_FLD_LINK].addChild(ET_ENT_LINK_FLD_REAL_INDEX, new scDataNode(entityHandle));
    (*newReferenceItemGuard)[ET_REF_FLD_LINK].addChild(ET_ENT_LINK_FLD_EVENT_NO, new scDataNode(getNextEventNoReal(entityHandle)));
    newReferencesGuard->addChild(newReferenceItemGuard.release());
    
    addEventReal(newEntityHandle, newEntityIndex, eecCopied, 
      "copied-f, new-idx:["+toString(newEntityIndex)+"]", 
      context, 
      *newReferencesGuard,
      true);

    newReferencesGuard->clear();
    newReferenceItemGuard.reset(new scDataNode());
    newReferenceItemGuard->addChild(ET_REF_FLD_NAME, new scDataNode("child"));
    newReferenceItemGuard->addChild(ET_REF_FLD_LINK, new scDataNode());
    (*newReferenceItemGuard)[ET_REF_FLD_LINK].addChild(ET_ENT_LINK_FLD_REAL_INDEX, new scDataNode(newEntityHandle));
    (*newReferenceItemGuard)[ET_REF_FLD_LINK].addChild(ET_ENT_LINK_FLD_EVENT_NO, new scDataNode(0));
    newReferencesGuard->addChild(newReferenceItemGuard.release());
    
    addEventReal(entityHandle, ET_UNKNOWN_ENTITY_IDX, eecCopied, 
      "copied-t, new-idx:["+toString(newEntityIndex)+"]", 
      context, 
      *newReferencesGuard,
      false);
  }
}

void sgpEntityTracer::intHandleEntityCopiedByHandleBuf(uint entityHandle, uint newEntityIndex, const scString &context)
{
#ifdef TRACE_ET_TRACER
  Log::addInfo("Copy-hnd-buf:"+toString(entityHandle)+"->"+toString(newEntityIndex));
#endif
  addBufferedAction(eecCopied, entityHandle, newEntityIndex, context);
}

void sgpEntityTracer::intHandleEntityDeath(uint entityIndex, sgpEtEventCode eventCode, const scString &context)
{
#ifdef TRACE_ET_TRACER
  Log::addInfo("Death:"+toString(entityIndex)+"-"+toString(eventCode));
#endif

  uint realIndex = getEntityRealIndex(entityIndex);
  updateSolutionDestroyStatsReal(realIndex, eventCode);
  addInfoEventReal(realIndex, entityIndex, eecDeath, "death", context, false);
  if (realIndex < m_entities.size()) {
    m_activeIndexMap.setElement(toString(entityIndex), scDataNode(static_cast<uint>(-1)));    
  }
}

void sgpEntityTracer::intHandleEntityDeathByHandle(uint entityHandle, sgpEtEventCode eventCode, const scString &context)
{  
#ifdef TRACE_ET_TRACER
  Log::addInfo("Death-hnd:"+toString(entityHandle)+"-"+toString(eventCode));
#endif

  updateSolutionDestroyStatsReal(entityHandle, eventCode);
  uint entityIndex = getEntityIndexForHandle(entityHandle);
  uint realIndex = getEntityRealIndexNoCheck(entityIndex);
  addInfoEventReal(entityHandle, entityIndex, eecDeath, "death", context, false);
  if ((realIndex == entityHandle) && (entityHandle < m_entities.size())) {
    m_activeIndexMap.setElement(toString(entityIndex), scDataNode(static_cast<uint>(-1)));    
  }
}

void sgpEntityTracer::intHandleEntityDeathBuf(uint entityIndex, sgpEtEventCode eventCode, const scString &context)
{
  uint realIndex = getEntityRealIndex(entityIndex);
#ifdef TRACE_ET_TRACER
  Log::addInfo("Death-buf: hnd="+toString(realIndex)+", idx="+toString(entityIndex)+", event= "+toString(eventCode));
#endif
  addBufferedAction(eecDeath, realIndex, entityIndex, context, eventCode);
}

// parentList and childList contain real entity indexes
void sgpEntityTracer::intHandleXOver(const scDataNode &parentList, const scDataNode &childList)
{
  std::auto_ptr<scDataNode> newReferencesGuard(new scDataNode());
  std::auto_ptr<scDataNode> newReferenceItemGuard;

  for(uint i = 0, epos = parentList.size(); i != epos; i++) {
    newReferenceItemGuard.reset(new scDataNode());
    newReferenceItemGuard->addChild(ET_REF_FLD_NAME, new scDataNode("parent"));
    newReferenceItemGuard->addChild(ET_REF_FLD_LINK, new scDataNode());
    (*newReferenceItemGuard)[ET_REF_FLD_LINK].addChild(ET_ENT_LINK_FLD_REAL_INDEX, new scDataNode(parentList.getUInt(i)));
    (*newReferenceItemGuard)[ET_REF_FLD_LINK].addChild(ET_ENT_LINK_FLD_EVENT_NO, new scDataNode(getNextEventNoReal(parentList.getUInt(i))));
    newReferencesGuard->addChild(newReferenceItemGuard.release());
  }  

  for(uint i = 0, epos = childList.size(); i != epos; i++) {
    newReferenceItemGuard.reset(new scDataNode());
    newReferenceItemGuard->addChild(ET_REF_FLD_NAME, new scDataNode("child"));
    newReferenceItemGuard->addChild(ET_REF_FLD_LINK, new scDataNode());
    (*newReferenceItemGuard)[ET_REF_FLD_LINK].addChild(ET_ENT_LINK_FLD_REAL_INDEX, new scDataNode(childList.getUInt(i)));
    (*newReferenceItemGuard)[ET_REF_FLD_LINK].addChild(ET_ENT_LINK_FLD_EVENT_NO, new scDataNode(getNextEventNoReal(childList.getUInt(i))));
    newReferencesGuard->addChild(newReferenceItemGuard.release());
  }  

  for(uint i = 0, epos = newReferencesGuard->size(); i != epos; i++) {
    addEventReal((*newReferencesGuard)[i][ET_REF_FLD_LINK].getUInt(ET_ENT_LINK_FLD_REAL_INDEX), 
      ET_UNKNOWN_ENTITY_IDX,
      eecXOver, 
      scString("xover-")+(*newReferencesGuard)[i].getString(ET_REF_FLD_NAME),
      "",
      *newReferencesGuard
    );  
  }
}

void sgpEntityTracer::intHandleEntityChangedInPoint(uint entityIndex, const scString &changeCode, uint genomeIndex, uint changeOffset)
{
  uint realIndex = getEntityRealIndex(entityIndex);
  addEventReal(realIndex, 
    entityIndex,
    eecMutated, 
    changeCode,
    toString(genomeIndex)+scString(":")+toString(changeOffset),
    scDataNode(),
    false // no details about new entity contents yet - wait for "final" event
  );  
  
  m_mutChangeCount++;
}

void sgpEntityTracer::intHandleEntityChanged(uint entityIndex, const sgpEntityBase &entity)
{
  uint realIndex = getEntityRealIndex(entityIndex);
  std::auto_ptr<scDataNode> newReferencesGuard(new scDataNode());
  std::auto_ptr<scDataNode> newReferenceItemGuard;
  scDataNode dets;
  
  newReferenceItemGuard.reset(new scDataNode());
  newReferenceItemGuard->addChild(ET_REF_FLD_NAME, new scDataNode("old_dets"));
  newReferenceItemGuard->addChild(ET_REF_FLD_DETS, new scDataNode());
  getActiveEntityDetsReal(realIndex, dets, true);
  (*newReferenceItemGuard)[ET_REF_FLD_DETS].copyValueFrom(dets);
  newReferencesGuard->addChild(newReferenceItemGuard.release());
  
  newReferenceItemGuard.reset(new scDataNode());
  newReferenceItemGuard->addChild(ET_REF_FLD_NAME, new scDataNode("new_dets"));
  newReferenceItemGuard->addChild(ET_REF_FLD_DETS, new scDataNode());
  
  scDataNode code, info, objs;
  extractEntityDets(entity, code, info, objs);

  dets.clear();
  dets.addChild(ET_ENTITY_DETS_CODE, new scDataNode(code));
  dets.addChild(ET_ENTITY_DETS_INFO, new scDataNode(info));
  dets.addChild(ET_ENTITY_DETS_OBJS, new scDataNode(objs));
  
  setActiveEntityDetsReal(realIndex, dets);
  getActiveEntityDetsReal(realIndex, dets, true);
  
  (*newReferenceItemGuard)[ET_REF_FLD_DETS].copyValueFrom(dets);
  
  newReferencesGuard->addChild(newReferenceItemGuard.release());  
  
  addEventReal(realIndex, 
    entityIndex,
    eecMutatedFinal, 
    "mut-final",
    "",
    *newReferencesGuard,
    false
  );    

  m_mutEntityChangedCount++;
}

void sgpEntityTracer::logEntityDetsDiff(uint entityIndex, const sgpEntityBase &entity, const scString &context)
{
  uint realIndex = getEntityRealIndex(entityIndex);
  std::auto_ptr<scDataNode> newReferencesGuard(new scDataNode());
  std::auto_ptr<scDataNode> newReferenceItemGuard;
  scDataNode dets;
  
  newReferenceItemGuard.reset(new scDataNode());
  newReferenceItemGuard->addChild(ET_REF_FLD_NAME, new scDataNode("old_dets"));
  newReferenceItemGuard->addChild(ET_REF_FLD_DETS, new scDataNode());
  getActiveEntityDetsReal(realIndex, dets, true);
  (*newReferenceItemGuard)[ET_REF_FLD_DETS].copyValueFrom(dets);
  newReferencesGuard->addChild(newReferenceItemGuard.release());
  
  newReferenceItemGuard.reset(new scDataNode());
  newReferenceItemGuard->addChild(ET_REF_FLD_NAME, new scDataNode("new_dets"));
  newReferenceItemGuard->addChild(ET_REF_FLD_DETS, new scDataNode());
  
  scDataNode code, info, objs;
  extractEntityDets(entity, code, info, objs);

  scString oldCode, newCode;
  oldCode = formatCode(dets[ET_EVENT_DETS_CODE]);
  newCode = formatCode(code);
  if (oldCode != newCode) {
    dets.clear();
    dets.addChild(ET_EVENT_DETS_CODE, new scDataNode(code));
    dets.addChild(ET_EVENT_DETS_INFO, new scDataNode(info));
    dets.addChild(ET_EVENT_DETS_OBJS, new scDataNode(objs));
    
    (*newReferenceItemGuard)[ET_REF_FLD_DETS].copyValueFrom(dets);
    
    newReferencesGuard->addChild(newReferenceItemGuard.release());  
    
    addEventReal(realIndex, 
      entityIndex,
      eecOther, 
      "dets-diff",
      context,
      *newReferencesGuard
    );    
  }
}

void sgpEntityTracer::intHandleEntityEvaluated(uint entityIndex, const sgpFitnessValue &fitVector, const sgpEntityBase &entity)
{
  uint realIndex = getEntityRealIndex(entityIndex);
  
  scDataNode objectives;
  for(uint i=0, epos = fitVector.size(); i != epos; i++)
    objectives.addItemAsDouble(fitVector[i]);
    
  m_entities[realIndex][ET_ENTITY_FLD_ACTIVE_DETS][ET_ENTITY_DETS_OBJS].copyValueFrom(objectives);
}

void sgpEntityTracer::intHandleEntityEvent(uint entityIndex, const scString &eventCode)
{
  uint realIndex = getEntityRealIndex(entityIndex);
  addInfoEventReal(realIndex, entityIndex, eecOther, eventCode, "", false);
}

void sgpEntityTracer::intHandleSetEntityClass(uint entityIndex, uint classCode)
{
  uint realIndex = getEntityRealIndex(entityIndex);
  uint oldClass = m_entities[realIndex].getUInt(ET_ENTITY_FLD_CLASS, MAX_CLASS);
  
  assert(classCode < MAX_CLASS);

  if (classCode != oldClass) {
    m_entities[realIndex].setUInt(ET_ENTITY_FLD_CLASS, classCode);
  
    if (classCode > oldClass) {
      sgpEtEventCode code;
      scString mutName;
      findDestroyReasonsReal(realIndex, code, mutName);
      if (code != eecMutated) {
        updateSolutionDestroyStatsReal(realIndex, code);
      } else {
        updateSolutionDestroyStatsByMutReal(realIndex, mutName);
      } 
      logSolutionDestroyedReal(realIndex, oldClass, classCode, code);
    }  
    else {
      logSolutionImprovedReal(realIndex, oldClass, classCode);
    }  
  }      
}

void sgpEntityTracer::intHandleMatchResult(const scDataNode &winners, const scDataNode &loosers)
{
  std::auto_ptr<scDataNode> newReferencesGuard(new scDataNode());
  std::auto_ptr<scDataNode> newReferenceItemGuard;
  uint entityIndex, realIndex;

  for(uint i = 0, epos = winners.size(); i != epos; i++) {
    newReferenceItemGuard.reset(new scDataNode());
    newReferenceItemGuard->addChild(ET_REF_FLD_NAME, new scDataNode("winner"));
    newReferenceItemGuard->addChild(ET_REF_FLD_LINK, new scDataNode());
    entityIndex = winners.getUInt(i);
    realIndex = getEntityRealIndex(entityIndex);
    (*newReferenceItemGuard)[ET_REF_FLD_LINK].addChild(ET_ENT_LINK_FLD_ENT_INDEX, new scDataNode(entityIndex));
    (*newReferenceItemGuard)[ET_REF_FLD_LINK].addChild(ET_ENT_LINK_FLD_REAL_INDEX, new scDataNode(realIndex));
    (*newReferenceItemGuard)[ET_REF_FLD_LINK].addChild(ET_ENT_LINK_FLD_EVENT_NO, new scDataNode(getNextEventNoReal(realIndex)));
    newReferencesGuard->addChild(newReferenceItemGuard.release());
  }  

  for(uint i = 0, epos = loosers.size(); i != epos; i++) {
    newReferenceItemGuard.reset(new scDataNode());
    newReferenceItemGuard->addChild(ET_REF_FLD_NAME, new scDataNode("looser"));
    newReferenceItemGuard->addChild(ET_REF_FLD_LINK, new scDataNode());
    entityIndex = loosers.getUInt(i);
    realIndex = getEntityRealIndex(entityIndex);
    (*newReferenceItemGuard)[ET_REF_FLD_LINK].addChild(ET_ENT_LINK_FLD_ENT_INDEX, new scDataNode(entityIndex));
    (*newReferenceItemGuard)[ET_REF_FLD_LINK].addChild(ET_ENT_LINK_FLD_REAL_INDEX, new scDataNode(realIndex));
    (*newReferenceItemGuard)[ET_REF_FLD_LINK].addChild(ET_ENT_LINK_FLD_EVENT_NO, new scDataNode(getNextEventNoReal(realIndex)));
    newReferencesGuard->addChild(newReferenceItemGuard.release());
  }  

  // generate events "match"
  for(uint i = 0, epos = newReferencesGuard->size(); i != epos; i++) {
    addEventReal((*newReferencesGuard)[i][ET_REF_FLD_LINK].getUInt(ET_ENT_LINK_FLD_REAL_INDEX), 
      (*newReferencesGuard)[i][ET_REF_FLD_LINK].getUInt(ET_ENT_LINK_FLD_ENT_INDEX), 
      eecMatch, 
      scString("match-")+(*newReferencesGuard)[i].getString(ET_REF_FLD_NAME),
      "",
      *newReferencesGuard
    );  
  }
  // update heat maps for objectives for destroyed solutions
  uint looserClass, winnerClass;
  uint winnerIndex;
  for(uint i = 0, epos = loosers.size(); i != epos; i++) {
    entityIndex = loosers.getUInt(i);
    realIndex = getEntityRealIndex(entityIndex);
    looserClass = getEntityClassReal(realIndex);
    for(uint j = 0, eposj = winners.size(); j != eposj; j++) {
      winnerIndex = winners.getUInt(j);
      winnerIndex = getEntityRealIndex(winnerIndex); 
      winnerClass = getEntityClassReal(winnerIndex);
      if (winnerClass > looserClass) {
      // worse entity won 
      //- log
        addInfoEventReal(realIndex, entityIndex, eecTourBetterLost, "match-better-lost-"+toString(looserClass), "");
      //- update stats  
        updateHeatMapForUnfairMatchPerObj(realIndex, winnerIndex);
      }       
    }
  }  
  
  m_matchCount++;
  m_matchWinnerCount += winners.size();
  m_matchLooserCount += loosers.size();  
}

void sgpEntityTracer::logSolutionDestroyedReal(uint realIndex, uint oldClass, uint newClass, sgpEtEventCode reasonCode)
{
  addInfoEventReal(realIndex, ET_UNKNOWN_ENTITY_IDX, eecClassDestroyed, "class-destroyed-"+toString(oldClass)+"-"+toString(newClass), "reason:"+toString(reasonCode));
  assert((oldClass == MAX_CLASS) || (oldClass < 12));
}

void sgpEntityTracer::logSolutionImprovedReal(uint realIndex, uint oldClass, uint newClass)
{
  addInfoEventReal(realIndex, ET_UNKNOWN_ENTITY_IDX, eecClassImproved, "class-improved-"+toString(oldClass)+"-"+toString(newClass), "");
}

uint sgpEntityTracer::intGetEntityHandle(uint entityIndex)
{
  return getEntityRealIndex(entityIndex);
}

void sgpEntityTracer::intFlushMoveBuffer()
{
  for(uint i=0, epos = m_moveBuffer.size(); i != epos; i++) 
  {
    switch (m_moveBuffer[i].getUInt(ET_BUF_ACTION_FLD_OPER)) {
      case eecMoved:
        intHandleEntityMovedByHandle(m_moveBuffer[i].getUInt(ET_BUF_ACTION_FLD_REAL_INDEX),
          m_moveBuffer[i].getUInt(ET_BUF_ACTION_FLD_NEW_INDEX),
          m_moveBuffer[i].getString(ET_BUF_ACTION_FLD_CTX)
        );
        break;  
      case eecCopied:
        intHandleEntityCopiedByHandle(m_moveBuffer[i].getUInt(ET_BUF_ACTION_FLD_REAL_INDEX),
          m_moveBuffer[i].getUInt(ET_BUF_ACTION_FLD_NEW_INDEX),
          m_moveBuffer[i].getString(ET_BUF_ACTION_FLD_CTX)
        );
        break;  
      case eecDeath:
        intHandleEntityDeathByHandle(m_moveBuffer[i].getUInt(ET_BUF_ACTION_FLD_REAL_INDEX),
          static_cast<sgpEtEventCode>(m_moveBuffer[i].getUInt(ET_BUF_ACTION_FLD_CTX_OPER)),
          m_moveBuffer[i].getString(ET_BUF_ACTION_FLD_CTX)
        );
        break;          
      default:
        throw scError("Unknown move buffered action: "+toString(m_moveBuffer[i].getUInt(ET_BUF_ACTION_FLD_OPER)));          
    }
  }  

  m_moveBuffer.clear();
  m_moveBufferMarks.clear();
}

bool sgpEntityTracer::intHasBufferedActionFor(uint entityIndex)
{
  uint realIndex = getEntityRealIndex(entityIndex);
  return (m_moveBufferMarks.find(realIndex) != m_moveBufferMarks.end());
}

bool sgpEntityTracer::intHasBufferedActionForHandle(uint entityHandle)
{
  return (m_moveBufferMarks.find(entityHandle) != m_moveBufferMarks.end());
}

void sgpEntityTracer::extractEntityDets(const sgpEntityBase &entity, scDataNode &code, scDataNode &info, scDataNode &objectives)
{
  m_entityLister->extractDetails(&entity, code, info, objectives);
}

uint sgpEntityTracer::getEntityRealIndex(uint entityIndex)
{
  uint realIndex = m_activeIndexMap.getUInt(toString(entityIndex), m_entities.size());
  if (realIndex >= m_entities.size())
    throw scError(scString("realIndex to high: ")+toString(realIndex));
  return realIndex;
}

uint sgpEntityTracer::getEntityRealIndexNoCheck(uint entityIndex)
{
  uint realIndex = m_activeIndexMap.getUInt(toString(entityIndex), m_entities.size());
  return realIndex;
}

uint sgpEntityTracer::getEntityIndexForHandle(uint entityHandle)
{
  uint res = m_entities.size();
  for(uint i=0, epos = m_activeIndexMap.size(); i != epos; i++) {
    if (m_activeIndexMap[i].getAsUInt() == entityHandle) {
      res = stringToUInt(m_activeIndexMap.getElementName(i));
      break;
    }
  }  
  return res;    
}

void sgpEntityTracer::addInfoEvent(uint entityIndex, sgpEtEventCode eventCode, const scString &eventName, const scString &context)
{
  uint realIndex = getEntityRealIndex(entityIndex);
  addInfoEventReal(realIndex, entityIndex, eventCode, eventName, context);
}

void sgpEntityTracer::addInfoEventReal(uint realIndex, uint entityIndex, sgpEtEventCode eventCode, const scString &eventName, const scString &context, bool addActiveDets)
{
  if (realIndex < m_entities.size()) {
    std::auto_ptr<scDataNode> newEventGuard(new scDataNode());
    newEventGuard->addChild(ET_EVENT_FLD_NAME, new scDataNode(eventName));
    newEventGuard->addChild(ET_EVENT_FLD_CODE, new scDataNode(static_cast<uint>(eventCode)));
    newEventGuard->addChild(ET_EVENT_FLD_STEP, new scDataNode(m_stepNo));
    newEventGuard->addChild(ET_EVENT_FLD_ENTITY_INDEX, new scDataNode(entityIndex));
    newEventGuard->addChild(ET_EVENT_FLD_CTX, new scDataNode(context));
    if (addActiveDets) {
      scDataNode dets;
      getActiveEntityDetsReal(realIndex, dets, true); 
      newEventGuard->addChild(ET_EVENT_FLD_DETS, new scDataNode(dets));
    }
    m_entities[realIndex][ET_ENTITY_FLD_EVENTS].addChild(newEventGuard.release());
    m_entities[realIndex].setUInt(ET_ENTITY_FLD_EVENT_COUNT, m_entities[realIndex].getUInt(ET_ENTITY_FLD_EVENT_COUNT) + 1);
  }
}

void sgpEntityTracer::addEventReal(uint realIndex, uint entityIndex, sgpEtEventCode eventCode, const scString &eventName, const scString &context, const scDataNode &references, bool addActiveDets)
{
  if (realIndex < m_entities.size()) {
    std::auto_ptr<scDataNode> newEventGuard(new scDataNode());
    newEventGuard->addChild(ET_EVENT_FLD_NAME, new scDataNode(eventName));
    newEventGuard->addChild(ET_EVENT_FLD_CODE, new scDataNode(static_cast<uint>(eventCode)));
    newEventGuard->addChild(ET_EVENT_FLD_STEP, new scDataNode(m_stepNo));
    newEventGuard->addChild(ET_EVENT_FLD_ENTITY_INDEX, new scDataNode(entityIndex));
    newEventGuard->addChild(ET_EVENT_FLD_CTX, new scDataNode(context));
    newEventGuard->addChild(ET_EVENT_FLD_REFERENCES, new scDataNode(references));
    if (addActiveDets) {
      scDataNode dets;
      getActiveEntityDetsReal(realIndex, dets, true); 
      newEventGuard->addChild(ET_EVENT_FLD_DETS, new scDataNode(dets));
    }
    m_entities[realIndex][ET_ENTITY_FLD_EVENTS].addChild(newEventGuard.release());
    m_entities[realIndex].setUInt(ET_ENTITY_FLD_EVENT_COUNT, m_entities[realIndex].getUInt(ET_ENTITY_FLD_EVENT_COUNT) + 1);
  }
}

void sgpEntityTracer::updateSolutionDestroyStatsReal(uint realIndex, sgpEtEventCode eventCode)
{
  uint classCode = m_entities[realIndex].getUInt(ET_ENTITY_FLD_CLASS, MAX_CLASS);
  scString classCodeName = toString(classCode);
  if (!m_destroyStatsPerClassAndReason.hasChild(classCodeName)) {
    m_destroyStatsPerClassAndReason.addChild(classCodeName, new scDataNode(ict_parent));
  }  
    
  uint oldCount = m_destroyStatsPerClassAndReason[classCodeName].getUInt(toString(static_cast<uint>(eventCode)), 0);
  m_destroyStatsPerClassAndReason[classCodeName].setElementSafe(toString(static_cast<uint>(eventCode)), scDataNode(oldCount+1));
}

void sgpEntityTracer::updateSolutionDestroyStatsByMutReal(uint realIndex, const scString &mutCode)
{
  updateSolutionDestroyStatsReal(realIndex, eecMutated);

  uint classCode = m_entities[realIndex].getUInt(ET_ENTITY_FLD_CLASS, MAX_CLASS);
  scString classCodeName = toString(classCode);
  if (!m_destroyStatsPerClassAndReason["muts"].hasChild(classCodeName)) {
    m_destroyStatsPerClassAndReason["muts"].addChild(classCodeName, new scDataNode(ict_parent));
  }  

  uint oldCount = m_destroyStatsPerClassAndReason["muts"][classCodeName].getUInt(mutCode, 0);
  m_destroyStatsPerClassAndReason["muts"][classCodeName].setElementSafe(mutCode, scDataNode(oldCount+1));
}

uint sgpEntityTracer::getNextEventNoReal(uint realIndex)
{
  uint res = m_entities[realIndex].getUInt(ET_ENTITY_FLD_EVENT_COUNT);
  return res;
}

uint sgpEntityTracer::getLastEventNoReal(uint realIndex)
{
  uint res = m_entities[realIndex].getUInt(ET_ENTITY_FLD_EVENT_COUNT);
  if (res > 0)
    res--;
  return res;
}

void sgpEntityTracer::setActiveEntityDetsReal(uint realIndex, const scDataNode &dets)
{
  m_entities[realIndex][ET_ENTITY_FLD_ACTIVE_DETS].copyValueFrom(dets);
}

void sgpEntityTracer::getActiveEntityDetsReal(uint realIndex, scDataNode &dets, bool eventNames)
{
  if (eventNames) {  
    dets.clear();
    try { 
    if (!m_entities[realIndex][ET_ENTITY_FLD_ACTIVE_DETS].hasChild(ET_ENTITY_DETS_CODE)) {
      scString entityDets;
      entityDets = m_entities[realIndex].dump();
      Log::addError("Code not found - error inside: "+entityDets);
    }
    dets.addChild(ET_EVENT_DETS_CODE, new scDataNode(m_entities[realIndex][ET_ENTITY_FLD_ACTIVE_DETS][ET_ENTITY_DETS_CODE]));
    dets.addChild(ET_EVENT_DETS_INFO, new scDataNode(m_entities[realIndex][ET_ENTITY_FLD_ACTIVE_DETS][ET_ENTITY_DETS_INFO]));
    dets.addChild(ET_EVENT_DETS_OBJS, new scDataNode(m_entities[realIndex][ET_ENTITY_FLD_ACTIVE_DETS][ET_ENTITY_DETS_OBJS]));  
    } catch(...) {
      scString entityDets;
      entityDets = m_entities[realIndex].dump();
      Log::addError("Access error inside: "+entityDets);
      throw;
    }
  } else {
    dets = m_entities[realIndex][ET_ENTITY_FLD_ACTIVE_DETS];
  }  
}

uint sgpEntityTracer::getEntityClassReal(uint realIndex)
{
  return m_entities[realIndex].getUInt(ET_ENTITY_FLD_CLASS);
}

void sgpEntityTracer::findDestroyReasonsReal(uint realIndex, sgpEtEventCode &resCode, scString &mutName)
{
  const scDataNode& eventList = m_entities[realIndex][ET_ENTITY_FLD_EVENTS];
  sgpEtEventCode eventCode;

  resCode = eecUnknown;
  mutName = "";
  
  if (eventList.size()) {
    uint idx = eventList.size();
    do {
      idx--;
      eventCode = sgpEtEventCode(eventList[idx].getUInt(ET_EVENT_FLD_CODE));
      switch (eventCode) {
        case eecMutated:
          mutName = eventList[idx].getString(ET_EVENT_FLD_NAME);
        case eecSamplesChanged:
          resCode = eventCode;
          break;
        default:
          ; 
      }
      
    } while((idx > 0) && (resCode == eecUnknown));  
  }
}

// count which objectives were worse in looser when "looser" is of better class but lost match
// in other words - which objectives are responsible for unfair lost match in most cases
void sgpEntityTracer::updateHeatMapForUnfairMatchPerObj(uint looserIndex, uint winnerIndex)
{
  scDataNode objectivesForLooser, objectivesForWinner;
    
  objectivesForLooser = m_entities[looserIndex][ET_ENTITY_FLD_ACTIVE_DETS][ET_ENTITY_DETS_OBJS];
  objectivesForWinner = m_entities[winnerIndex][ET_ENTITY_FLD_ACTIVE_DETS][ET_ENTITY_DETS_OBJS];
  uint maxCnt = std::min<uint>(objectivesForLooser.size(), objectivesForWinner.size());

  while(m_unfairMatchPerObj.size() < maxCnt)
    m_unfairMatchPerObj.addItemAsUInt(0);

  for(uint i=0, epos = maxCnt; i != epos; i++) 
  {
    if (objectivesForLooser.getDouble(i) < objectivesForWinner.getDouble(i))
      m_unfairMatchPerObj.setUInt(i, m_unfairMatchPerObj.getUInt(i) + 1);
  }
}

// dump step details, statistics + most important entities (top 50)
// step details:
// - step number
// - date & time
// - function ID 
// - total number of entities
// statistics:
// - destroy stats per type
// - mutation destroy stats per mut type
// - obj heat map for unfair tournaments
// top - each entity is presented as:
//   ID, final class, best class, number of events
void sgpEntityTracer::dumpOverview(const sgpEtIndexSet &activeEntities)
{
  const scString bodyTpl = 
  scString()+
  "<html><body>"+
  "<a name=\"top\"></a>\n"+
  "<h1>Step: {step-no}</h1>\n"+
  "<a href=\"{first-link}\">First</a>&nbsp;|&nbsp;"+
  "<a href=\"{prior-link}\">Prior</a>&nbsp;|&nbsp"+
  "<a href=\"{next-link}\">Next</a>"+
  "<br />"+
  "<b>When:</b>{when}<br />\n"+
  "<b>Function ID:</b>{fun-id}<br />\n"+
  "<b>Total entities:</b>{total-ent-cnt}<br />\n"+
  "<hr />\n"+
  "<h2>Class totals</h2>\n"+
  "{class-stats}"+
  "<hr />\n"+
  "<h2>Statistics</h2>\n"+
  "{stats}"+
  "<hr />\n"+
  "<h2>Top - active only</h2>\n"+
  "{top}"+
  "<br /><hr /><a href=\"{index-link}\">All entities</a><br />\n"+
  "</body></html>";
  
  scDataNode entitiesByClass;
  getEntitiesByClass(entitiesByClass, 50, true, activeEntities);
  scDataNode values;

  scString output;

  uint stepNo = m_stepNo - 1;
  values.addChild("step-no", new scDataNode(stepNo));
  values.addChild("when", new scDataNode(dateTimeToIsoStr(currentDateTime())));
  values.addChild("fun-id", new scDataNode(m_functionId));
  values.addChild("total-ent-cnt", new scDataNode(m_entities.size()));
  values.addChild("stats", new scDataNode(formatStatisticsForDump(activeEntities)));
  values.addChild("class-stats", new scDataNode(formatClassStatisticsForDump(stepNo, activeEntities)));  
  values.addChild("top", new scDataNode(formatTopForDump(entitiesByClass)));

  values.addChild("first-link", new scDataNode(generateOverviewLogName(0, false)));
  if (stepNo > 0)
    values.addChild("prior-link", new scDataNode(generateOverviewLogName(stepNo - 1, false)));
  else  
    values.addChild("prior-link", new scDataNode(generateOverviewLogName(0, false)));
  values.addChild("next-link", new scDataNode(generateOverviewLogName(stepNo + 1, false)));

  values.addChild("index-link", new scDataNode(generateIndexLogName(stepNo, false)));
  
  output = formatTemplate(bodyTpl, values);
  
  saveStringToFile(output, generateOverviewLogName(stepNo, true));
}

scString sgpEntityTracer::formatTemplate(const scString &tpl, const scDataNode &values)
{
  scString res(tpl);
  scString varPattern;
  
  for(uint i = 0, epos = values.size(); i != epos; i++) {
    varPattern = scString("{")+values.getElementName(i)+scString("}"); 
    strReplaceThis(res, varPattern.c_str(), values[i].getAsString().c_str(), true);
  }  
  return res;  
}


void sgpEntityTracer::getEntitiesByClass(scDataNode &output, uint limit)
{
  sgpEtIndexSet nullParam;
  getEntitiesByClass(output, limit, false, nullParam);
}

void sgpEntityTracer::getEntitiesByClass(scDataNode &output, uint limit, bool aliveOnly, const sgpEtIndexSet &activeEntities)
{
  uint classCode;
  uint addedCnt = 0;
  scString className;

  std::set<uint> classSet;

  classSet.clear();
  for(uint i = 0, epos = m_entities.size(); i != epos; i++)
    classSet.insert(m_entities[i].getUInt(ET_ENTITY_FLD_CLASS));

  uint classValue;  
  for(std::set<uint>::const_iterator it = classSet.begin(), epos = classSet.end(); (it != epos) && (addedCnt < limit); ++it)
  {
    classValue = *it;
  
    for(uint i = 0, epos = m_entities.size(); (i != epos) && (addedCnt < limit); i++)
    {
      if (aliveOnly && (activeEntities.find(i) == activeEntities.end()))
        continue;
      classCode = m_entities[i].getUInt(ET_ENTITY_FLD_CLASS);
      if (classCode == classValue) {
        className = toString(classCode);
        if (!output.hasChild(className))
          output.addChild(className, new scDataNode());

        output[className].addItemAsUInt(i);  
        addedCnt++;
      } // if class OK 
    } // for i
  } // for it 
} // function

scString sgpEntityTracer::formatStatisticsForDump(const sgpEtIndexSet &activeEntities)
{
  const scString subHeadTpl1 = 
  "<h3>Destroy reasons: destroyed class split by reasons code</h3>";

  const scString subHeadTpl2 = 
  "<h3>Destroy reasons: destroyed class split by mutation type</h3>";

  const scString subHeadTpl3 = 
  "<h3>Heat map for unfair tournaments split by objective function</h3>";

  const scString subHeadTpl4 = 
  "<h3>Other global statistics</h3>";

  const scString reasonHeadTpl = 
  "<tr><th>Class</th><th>Reason</th><th>Count</th><th>% (in class)</th></tr>";

  const scString reasonHeadForMutTpl = 
  "<tr><th>Class</th><th>Mutation type</th><th>Count</th><th>% (in class)</th></tr>";

  const scString reasonTpl = 
  "<tr><th>{class}</th><th align=\"right\">{reason}</th><td align=\"right\">{value}</td><td align=\"right\">{prc}</td></tr>";

  const scString unfairHeadTpl = 
  "<tr><th>Objective</th><th>Count</th><th>%</th></tr>";

  const scString unfairRowTpl = 
  "<tr><th>{obj}</th><td align=\"right\">{value}</td><td align=\"right\">{prc}</td></tr>";

  const scString tableTpl = 
  scString()+
  "<table border=\"1\">"+
  "<caption>{caption}</caption>"+  
  "{coldefs}"+
  "{rows}"+
  "</table>";
  
  const scString colDefs1 = 
  scString()+
  "<COL align=\"right\"></COL>"+
  "<COL align=\"right\"></COL>"+
  "<COL align=\"right\"></COL>"+
  "<COL align=\"right\"></COL>";

  const scString colDefs2 = 
  scString()+
  "<COL align=\"right\" />"+
  "<COL align=\"right\" />";
  
  scString output;
  scString tableOut;
  scString rowString;
  scDataNode values;

  uint classValue;
  sgpEtEventCode reasonCode;
  scDataNode rowValues;

  std::set<uint> classSet;

  // --- destroy reasons split by event    
  output += subHeadTpl1;  
  tableOut = reasonHeadTpl;
  uint totalCnt, cnt;
    
  totalCnt = 0;
  classSet.clear();
  for(uint classIdx = 0, epos = m_destroyStatsPerClassAndReason.size(); classIdx != epos; classIdx++)
  { 
    if (stringToIntDef(m_destroyStatsPerClassAndReason.getElementName(classIdx), -1) < 0)
      continue;
    classSet.insert(stringToUInt(m_destroyStatsPerClassAndReason.getElementName(classIdx)));
  }  
  
  scDataNode classTotals;
  uint posValue;
  scString className;

  for(uint classIdx = 0, epos = m_destroyStatsPerClassAndReason.size(); classIdx != epos; classIdx++) 
  {
    className = m_destroyStatsPerClassAndReason.getElementName(classIdx);
  
    for(uint reasonIdx = 0, eposr = m_destroyStatsPerClassAndReason[classIdx].size(); reasonIdx != eposr; reasonIdx++)
    {
      if (stringToIntDef(className, -1) < 0)
        continue;
      posValue = stringToUInt(m_destroyStatsPerClassAndReason[classIdx].getElement(reasonIdx).getAsString());    
      totalCnt += posValue;
      classTotals.setUIntDef(className, classTotals.getUInt(className, 0) + posValue);
    }
  }
    
  const scDataNode *classNode; 
  uint classTotalCnt; 
  std::vector<UIntUIntPair> reasonSort; 
  uint reasonIdx; 
  
  for(std::set<uint>::const_iterator it = classSet.begin(), epos = classSet.end(); it != epos; ++it)
  {
    className = toString(*it);
    classNode = &(m_destroyStatsPerClassAndReason[className]);
    reasonSort.clear();
    for(uint reasonIdxRaw = 0, eposr = (*classNode).size(); reasonIdxRaw != eposr; reasonIdxRaw++)
    {
      cnt = stringToUInt((*classNode).getElement(reasonIdxRaw).getAsString());  
      reasonSort.push_back(std::make_pair(reasonIdxRaw, cnt));
    }
           
    std::sort(reasonSort.begin(), reasonSort.end(), sort_second_pred_gt);

    for(uint reasonIdxInSort = 0, eposr = (*classNode).size(); reasonIdxInSort != eposr; reasonIdxInSort++)
    {
      reasonIdx = reasonSort[reasonIdxInSort].first;
      
      rowValues.clear();
      classValue = stringToUInt(className);
      reasonCode = static_cast<sgpEtEventCode>(stringToUInt((*classNode).getElementName(reasonIdx)));

      rowValues.addChild("class", new scDataNode(classValue));
      rowValues.addChild("reason", new scDataNode(getEventName(reasonCode)+"/"+toString(static_cast<uint>(reasonCode))));
      cnt = stringToUInt((*classNode).getElement(reasonIdx).getAsString());  
      classTotalCnt = classTotals.getUInt(className);
      rowValues.addChild("value", new scDataNode(cnt));      
      rowValues.addChild("prc", new scDataNode(toStringPrec(static_cast<double>(cnt)/static_cast<double>(classTotalCnt)*100.0, 2)));      
      rowString = formatTemplate(reasonTpl, rowValues);
      tableOut += rowString;
    }  
  }

  rowValues.clear();
  rowValues.addChild("class", new scDataNode("Total"));
  rowValues.addChild("reason", new scDataNode("&nbsp;"));
  rowValues.addChild("value", new scDataNode(totalCnt));      
  rowValues.addChild("prc", new scDataNode("100.00"));      
  rowString = formatTemplate(reasonTpl, rowValues);
  tableOut += rowString;
  
  values.clear();
  values.addChild("rows", new scDataNode(tableOut));
  values.addChild("caption", new scDataNode(""));
  values.addChild("coldefs", new scDataNode(colDefs1));
  
  output += formatTemplate(tableTpl, values);

  // --- destroy reasons split by mut type  
  if (m_destroyStatsPerClassAndReason.hasChild("muts")) 
  {
    const scDataNode &mutNode = m_destroyStatsPerClassAndReason["muts"];

    classTotals.clear();
    
    totalCnt = 0;
    for(uint classIdx = 0, epos = mutNode.size(); classIdx != epos; classIdx++) 
    {
      className = mutNode.getElementName(classIdx);
      for(uint reasonIdx = 0, eposr = mutNode[classIdx].size(); reasonIdx != eposr; reasonIdx++) {
        posValue = stringToUInt(mutNode[classIdx].getElement(reasonIdx).getAsString());;
        totalCnt += posValue;
        classTotals.setUIntDef(className, classTotals.getUInt(className, 0) + posValue);
      }  
    }
          
    output += subHeadTpl2;  
    tableOut = reasonHeadForMutTpl;
    scString mutName;

    classSet.clear();    
    for(uint classIdx = 0, epos = mutNode.size(); classIdx != epos; classIdx++) 
    { 
      classValue = stringToUInt(mutNode.getElementName(classIdx));
      classSet.insert(classValue);
    }  
      
    for(std::set<uint>::const_iterator it = classSet.begin(), epos = classSet.end(); it != epos; ++it)
    {
      className = toString(*it);
      classNode = &(mutNode[className]);
      
      reasonSort.clear();
      for(uint reasonIdxRaw = 0, eposr = (*classNode).size(); reasonIdxRaw != eposr; reasonIdxRaw++)
      {
        cnt = stringToUInt((*classNode).getElement(reasonIdxRaw).getAsString());
        reasonSort.push_back(std::make_pair(reasonIdxRaw, cnt));
      }
             
      std::sort(reasonSort.begin(), reasonSort.end(), sort_second_pred_gt);

      for(uint reasonIdxInSort = 0, eposr = (*classNode).size(); reasonIdxInSort != eposr; reasonIdxInSort++)
      {
        reasonIdx = reasonSort[reasonIdxInSort].first;
        
        rowValues.clear();
        classValue = stringToUInt(className);
        
        mutName = (*classNode).getElementName(reasonIdx);

        rowValues.addChild("class", new scDataNode(classValue));
        rowValues.addChild("reason", new scDataNode(mutName));
        cnt = stringToUInt((*classNode).getElement(reasonIdx).getAsString());
        classTotalCnt = classTotals.getUInt(className);        
        rowValues.addChild("value", new scDataNode(cnt));      
        rowValues.addChild("prc", new scDataNode(toStringPrec(static_cast<double>(cnt)/static_cast<double>(classTotalCnt)*100.0, 2)));      
        rowString = formatTemplate(reasonTpl, rowValues);
        tableOut += rowString;
      }  
    }

    rowValues.clear();
    rowValues.addChild("class", new scDataNode("Total"));
    rowValues.addChild("reason", new scDataNode("&nbsp;"));
    rowValues.addChild("value", new scDataNode(totalCnt));      
    rowValues.addChild("prc", new scDataNode("100.00"));      
    rowString = formatTemplate(reasonTpl, rowValues);
    tableOut += rowString;
    
    values.clear();
    values.addChild("rows", new scDataNode(tableOut));
    values.addChild("caption", new scDataNode(""));
    values.addChild("coldefs", new scDataNode(colDefs1));
    
    output += formatTemplate(tableTpl, values);
  }

  // -- unfair tournaments by objectives - heat map
  totalCnt = 0;
  for(uint objIdx = 0, epos = m_unfairMatchPerObj.size(); objIdx != epos; objIdx++)
    totalCnt += m_unfairMatchPerObj.getUInt(objIdx);
  
  output += subHeadTpl3;  
  tableOut = unfairHeadTpl;
  for(uint objIdx = 0, epos = m_unfairMatchPerObj.size(); objIdx != epos; objIdx++)
  {
    rowValues.clear();
    rowValues.addChild("obj", new scDataNode(objIdx));
    cnt = m_unfairMatchPerObj.getUInt(objIdx);
    rowValues.addChild("value", new scDataNode(cnt));      
    rowValues.addChild("prc", new scDataNode(toStringPrec(static_cast<double>(cnt)/static_cast<double>(totalCnt)*100.0, 2)));      
    rowString = formatTemplate(unfairRowTpl, rowValues);
    tableOut += rowString;
  }  

  rowValues.clear();
  rowValues.addChild("obj", new scDataNode("Total"));
  rowValues.addChild("value", new scDataNode(totalCnt));      
  rowValues.addChild("prc", new scDataNode("100.00"));      
  rowString = formatTemplate(unfairRowTpl, rowValues);
  tableOut += rowString;
  
  values.clear();
  values.addChild("rows", new scDataNode(tableOut));
  values.addChild("caption", new scDataNode(""));
  values.addChild("coldefs", new scDataNode(colDefs2));
  
  output += formatTemplate(tableTpl, values);
  
  output += subHeadTpl4;
  
  output += formatOtherStats(activeEntities);
  
  output += formatBackToTop();
  return output;
}

scString sgpEntityTracer::formatBackToTop()
{
  const scString linkTpl = 
  "<br /><a href=\"#top\">Back to top</a>";
  return linkTpl;
}

scString sgpEntityTracer::formatClassStatisticsForDump(uint stepNo, const sgpEtIndexSet &activeEntities)
{
  const scString subHeadTpl1 = 
  "<h3>Total class summary</h3>";

  const scString classHeadTpl = 
  "<tr><th>Class</th><th>Count</th><th>%</th><th>Alive</th><th>% of alive</th><th>Diversity</th></tr>";

  const scString rowTpl = 
  scString()+
  "<tr><th><a href=\"#top-{raw-class}\">{class}</a></th><td align=\"right\">{value}</td><td align=\"right\">{prc}</td>"+
  "<td align=\"right\">{value-alive}</td><td align=\"right\">{prc-alive}</td><td align=\"right\">{divers}</td>"+
  "</tr>";

  const scString rowStatTpl = 
  scString()+
  "<tr><th><a href=\"{index-link}\">{class}</a></th><td align=\"right\">{value}</td><td align=\"right\">{prc}</td>"+
  "<td align=\"right\">{value-alive}</td><td align=\"right\">{prc-alive}</td><td align=\"right\">{divers}</td>"+
  "</tr>";

  const scString tableTpl = 
  scString()+
  "<table border=\"1\">"+
  "<caption>{caption}</caption>"+  
  "{coldefs}"+
  "{rows}"+
  "</table>";

  const scString subFooterTpl1 = 
  scString()+
  "<br /><strong>(*) Note:</strong>Items with class=["+toString(MAX_CLASS)+
  "] are xover chilren that was lost after second xover on them. That's why they are not evaluated.";
  
  scString output;
  scString tableOut;
  scString rowString;
  scDataNode values;
  scDataNode entitiesByClass, entitiesByClassAlive;
  
  getEntitiesByClass(entitiesByClass, m_entities.size());
  getEntitiesByClass(entitiesByClassAlive, m_entities.size(), true, activeEntities);

  uint classValue;
  scDataNode rowValues;

  std::set<uint> classSet;

  output += subHeadTpl1;  
  tableOut = classHeadTpl;
  uint totalCnt, cnt;
    
  classSet.clear();
  totalCnt = 0;
  for(uint classIdx = 0, epos = entitiesByClass.size(); classIdx != epos; classIdx++)
  { 
    classSet.insert(stringToUInt(entitiesByClass.getElementName(classIdx)));
    totalCnt += entitiesByClass[classIdx].size();
  }  

  uint totalCntAlive = 0;
  for(uint classIdx = 0, epos = entitiesByClassAlive.size(); classIdx != epos; classIdx++)
  { 
    totalCntAlive += entitiesByClassAlive[classIdx].size();
  }  
  
  const scDataNode *classNode;
  scString className;
  uint cntAlive, shapeCntAlive, totalShapeCntAlive;
    
  totalShapeCntAlive = 0;
    
  for(std::set<uint>::const_iterator it = classSet.begin(), epos = classSet.end(); it != epos; ++it)
  {
    rowValues.clear();
    className = toString(*it);
    classNode = &(entitiesByClass[className]);
    cnt = classNode->size();
    cntAlive = 0;
    shapeCntAlive = 0;
    if (entitiesByClassAlive.hasChild(className)) {
      cntAlive = entitiesByClassAlive[className].size();
      shapeCntAlive = countShapesForClass(entitiesByClassAlive[className], m_shapeObjIndex);
      totalShapeCntAlive += shapeCntAlive;
    }  
    
    classValue = stringToUInt(className);

    if (classValue != MAX_CLASS) 
      rowValues.addChild("class", new scDataNode(classValue));
    else  
      rowValues.addChild("class", new scDataNode(toString(classValue)+" *"));
    rowValues.addChild("raw-class", new scDataNode(classValue));
    rowValues.addChild("value", new scDataNode(cnt));      
    rowValues.addChild("prc", new scDataNode(toStringPrec(static_cast<double>(cnt)/static_cast<double>(totalCnt)*100.0, 2)));      
    rowValues.addChild("value-alive", new scDataNode(cntAlive));      
    rowValues.addChild("prc-alive", new scDataNode(toStringPrec(static_cast<double>(cntAlive)/static_cast<double>(totalCntAlive)*100.0, 2)));      
    rowValues.addChild("divers", new scDataNode(shapeCntAlive));      
    rowString = formatTemplate(rowTpl, rowValues);
    tableOut += rowString;
  }
    
  rowValues.clear();
  rowValues.addChild("class", new scDataNode("Total"));
  rowValues.addChild("value", new scDataNode(totalCnt));      
  rowValues.addChild("prc", new scDataNode("100.00"));      
  rowValues.addChild("value-alive", new scDataNode(totalCntAlive));      
  rowValues.addChild("prc-alive", new scDataNode("100.00"));      
  rowValues.addChild("index-link", new scDataNode(generateIndexLogName(stepNo, false)));  
  uint globalDivers = countShapesForActive(activeEntities, m_shapeObjIndex);
  scString divers;
  if (globalDivers != totalShapeCntAlive) {
    divers = "Global :"+toString(globalDivers)+"/<br />Sum :"+toString(totalShapeCntAlive);
  } else {
    divers = "Global :"+toString(globalDivers);
  }
  
  rowValues.addChild("divers", new scDataNode(divers));      
  
  rowString = formatTemplate(rowStatTpl, rowValues);
  tableOut += rowString;
  
  values.clear();
  values.addChild("rows", new scDataNode(tableOut));
  values.addChild("caption", new scDataNode(""));
  values.addChild("coldefs", new scDataNode(""));
  
  output += formatTemplate(tableTpl, values);
  
  output += subFooterTpl1;
  
  return output;
}

scString sgpEntityTracer::getEventName(sgpEtEventCode code)
{
  scString res;
  switch (code) {
    case eecOther:
      res = "other"; break;      
    case eecMoved:
      res = "moved"; break;
    case eecDeath:
      res = "death"; break;
    case eecMutated:
      res = "mutated"; break;
    case eecMutatedFinal:
      res = "mut-final"; break;
    case eecXOver:
      res = "xover"; break;
    case eecMatch:
      res = "match"; break;
    case eecTourNotSel: 
      res = "tour-not-sel"; break;
    case eecTourLost:
      res = "tour-lost"; break;
    case eecClassDestroyed:
      res = "class-destroy"; break;
    case eecClassImproved:
      res = "class-improved"; break;
    case eecTourBetterLost:
      res = "unfair-tour"; break;
    case eecBorn:
      res = "born"; break;
    case eecCopied:
      res = "copied"; break;
    case eecSamplesChanged:
      res = "samples-chg"; break;
    default: 
    //eecUnknown
      res = "event-unknown-"+toString(code);
  }
  return res;
}

// log list of top entities split by class
// Class: k
// Items: 
// ID, event count 
scString sgpEntityTracer::formatTopForDump(const scDataNode &entitiesByClass)
{
  const scString subHeadTpl1 = 
  scString()+
  "<a name=\"top-{class}\"></a>\n"+
  "<h2>Items with class: {class}</h2>";

  const scString tableTpl = 
  scString()+
  "<table border=\"1\">"+
  "<caption>{caption}</caption>"+  
  "{rows}"+
  "</table>";
  
  const scString groupHead = 
  "<tr><th>ID</th><th>Total events</th></tr>";

  const scString rowTpl = 
  "<tr><td><a href=\"{first-event-file}\">{id}</a></td><td>{event-cnt}</td></tr>";

  scString output;
  scString tableOut;
  scString rowString;
  scDataNode values, rowValues;

  std::set<uint> classSet;
  for(uint i=0, epos = entitiesByClass.size(); i != epos; i++)
    classSet.insert(stringToUInt(entitiesByClass.getElementName(i)));
  
  scString className;
  const scDataNode *classNode;
  const scDataNode *itemNode;
  scString entityId;
  scString firstEventId;
  
  for(std::set<uint>::const_iterator it = classSet.begin(), epos = classSet.end(); it != epos; ++it)
  {
    className = toString(*it);
    
    rowValues.clear();
    rowValues.addChild("class", new scDataNode(className));
    rowString = formatTemplate(subHeadTpl1, rowValues);
    output += rowString;
     
    classNode = &(entitiesByClass[className]); 
    tableOut = groupHead;        
    for(uint j=0, eposj = classNode->size(); j != eposj; j++) 
    {
      itemNode = &(m_entities[(*classNode).getUInt(j)]);
      rowValues.clear();
      entityId = 
        formatEntityId(
          (*itemNode)[ET_ENTITY_FLD_BORN_STEP].getAsUInt(),
          (*classNode).getUInt(j)
          //@(*itemNode)[ET_ENTITY_FLD_BORN_INDEX].getAsUInt()
        );
        
      rowValues.addChild("id", new scDataNode(entityId));
      rowValues.addChild("event-cnt", new scDataNode((*itemNode).getUInt(ET_ENTITY_FLD_EVENT_COUNT)));      
      firstEventId = entityId+"_0";
      rowValues.addChild("first-event-file",new scDataNode( 
        generateEventLogName((*classNode).getUInt(j), 0, false)
      ));      
      rowString = formatTemplate(rowTpl, rowValues);
      tableOut += rowString;      
    }
    
    values.clear();
    values.addChild("rows", new scDataNode(tableOut));
    values.addChild("caption", new scDataNode(""));
    
    output += formatTemplate(tableTpl, values);
    output += formatBackToTop();
  }
  return output;
}

scString sgpEntityTracer::formatEntityId(uint bornStep, uint bornIndex)
{
  return toString(bornStep)+"_"+toString(bornIndex);
}

// dump full entity index - ID, final class, best class, number of events
void sgpEntityTracer::dumpEntityIndex(const sgpEtIndexSet &activeEntities)
{
  const scString bodyTpl = 
  scString()+
  "<html><body>"+
  "<h1>Entity index</h1>\n"+
  "<b>Step:</b>{step}<br />\n"+
  "<b>Total count:</b>{total-entity-cnt}<br />\n"+
  "<a href=\"{overview-link}\">Back to overview</a>"+
  "<hr />\n"+
  "<h2>Class totals</h2>\n"+
  "{class-stats}"+
  "<hr />\n"+
  "<h2>Entities:</h2>\n"+
  "{entity-index}\n"+
  "<br />"+
  "<a href=\"{overview-link}\">Back to overview</a>"+
  "</body></html>";

  scString output;
  scString tableOut;
  scString rowString;
  scDataNode values, rowValues;

  uint stepNo = m_stepNo - 1;
  scDataNode entitiesByClass;
  getEntitiesByClass(entitiesByClass, m_entities.size());
  
  values.clear();
  values.addChild("step", new scDataNode(stepNo));
  values.addChild("total-entity-cnt", new scDataNode(m_entities.size()));
  values.addChild("class-stats", new scDataNode(formatClassStatisticsForDump(stepNo, activeEntities)));  
  values.addChild("entity-index", new scDataNode(
    formatTopForDump(entitiesByClass)
  ));

  values.addChild("overview-link", new scDataNode(generateOverviewLogName(stepNo, false)));
  
  output = formatTemplate(bodyTpl, values);
  
  saveStringToFile(output, generateIndexLogName(stepNo, true));
}

// dump all events for all entities
void sgpEntityTracer::dumpEvents()
{
  for(uint i=0, epos = m_entities.size(); i != epos; i++)
  {
    if (!isEntityFlushed(i))
      dumpEntityEventList(i);  
  }
}

void sgpEntityTracer::dumpEntityEventList(uint realIndex)
{
  for(uint i=0, epos = m_entities[realIndex][ET_ENTITY_FLD_EVENTS].size(); i != epos; i++)
    dumpEntityEvent(realIndex, i);
}

void sgpEntityTracer::dumpEntityEvent(uint realIndex, uint eventNo)
{
  const scString bodyTpl = 
  scString()+
  "<html><body>"+
  "<h1>Entity event</h1>\n"+
  "<b>Step:</b>{step} (<a href=\"{overview-link}\">overview</a>)<br />\n"+
  "<b>Entity:</b>{entity-id}<br />\n"+
  "<b>Event #:</b>{event-no}<br />\n"+
  "<a href=\"{first-link}\">First</a>&nbsp;|&nbsp;"+
  "<a href=\"{prior-link}\">Prior</a>&nbsp;|&nbsp"+
  "<a href=\"{next-link}\">Next</a><br />\n"+
  "<b>Event code:</b>{event-code}<br />\n"+
  "<b>Event name:</b>{event-name}<br />\n"+
  "<b>Context:</b>{context}<br />\n"+
  "<b>Total events:</b>{total-event-cnt}<br />\n"+
  "<hr />\n"+
  "<h2>Event details</h2>\n"+
  "{dets}"+
  "<hr />\n"+
  "<h2>Whole event list</h2>\n"+
  "{event-list}"+
  "<br />"+
  "<a href=\"{overview-link}\">Back to overview</a>"+
  "</body></html>";

  scString output;
  scString tableOut;
  scString rowString;
  scDataNode values, rowValues;
  
  const scDataNode &eventNode = m_entities[realIndex][ET_ENTITY_FLD_EVENTS][eventNo];
  
  values.clear();
  values.addChild("step", new scDataNode(eventNode.getUInt(ET_EVENT_FLD_STEP)));
  values.addChild("entity-id", new scDataNode(formatEntityId(
          m_entities[realIndex].getUInt(ET_ENTITY_FLD_BORN_STEP),
          realIndex
  )));
  values.addChild("event-no", new scDataNode(eventNo));
  
  values.addChild("first-link", new scDataNode(generateEventLogName(realIndex, 0, false)));
  if (eventNo > 0)
    values.addChild("prior-link", new scDataNode(generateEventLogName(realIndex, eventNo - 1, false)));
  else  
    values.addChild("prior-link", new scDataNode(generateEventLogName(realIndex, 0, false)));
  values.addChild("next-link", new scDataNode(generateEventLogName(realIndex, eventNo + 1, false)));
    
  uint eventCode = eventNode.getUInt(ET_EVENT_FLD_CODE);
  values.addChild("event-code", new scDataNode(
    getEventName(static_cast<sgpEtEventCode>(eventCode))+"/"+toString(eventCode)    
  ));
  values.addChild("event-name", new scDataNode(eventNode.getString(ET_EVENT_FLD_NAME)));    
  values.addChild("context", new scDataNode(eventNode.getString(ET_EVENT_FLD_CTX)));    
  values.addChild("total-event-cnt", new scDataNode(m_entities[realIndex][ET_ENTITY_FLD_EVENTS].size()));
  values.addChild("dets", new scDataNode(formatEventDetails(realIndex, eventNo)));  
  values.addChild("event-list", new scDataNode(formatEventList(realIndex, eventNo)));
  values.addChild("overview-link", new scDataNode(generateOverviewLogName(eventNode.getUInt(ET_EVENT_FLD_STEP), false)));
  
  output = formatTemplate(bodyTpl, values);
  
  saveStringToFile(output, generateEventLogName(realIndex, eventNo, true));
}

scString sgpEntityTracer::generateOverviewLogName(uint stepNo, bool addDir)
{
  return generateLogName(toString(stepNo)+scString("_a_overview"), "html", addDir);
}

scString sgpEntityTracer::generateIndexLogName(uint stepNo, bool addDir)
{
  return generateLogName(toString(stepNo)+scString("_a_index"), "html", addDir);
}

scString sgpEntityTracer::generateEventLogName(uint realIndex, uint eventNo, bool addDir)
{
  uint bornIndex = realIndex;
  uint bornStep = m_entities[realIndex].getUInt(ET_ENTITY_FLD_BORN_STEP);  
  scString entityId = formatEntityId(bornStep, bornIndex);
  
  return generateLogName(toString(bornStep)+"\\"+toString(bornIndex % 10)+"\\"+toString(bornIndex)+"\\"+
    ET_FILE_EVENT_PFX+entityId+"_"+toString(eventNo), "html", addDir);
}
  
scString sgpEntityTracer::formatEventList(uint realIndex, uint eventNo)
{
  const scString tableBodyTpl =
  "<ul>{body}</ul>";
   
  const scString rowTpl =
  "<li><a href=\"{event-file}\">{event-no}. {event-name}</a></li>";
  const scString rowThisTpl =
  "<li>{event-no}. {event-name}</li>";
  
  scString output;
  scString tableOut;
  scString rowString;
  scDataNode values, rowValues;  
  
  const scDataNode eventListNode = m_entities[realIndex][ET_ENTITY_FLD_EVENTS];
  
  tableOut = "";
  scString eventName;
  
  for(uint i=0, epos = eventListNode.size(); i != epos; i++)
  {
    rowValues.clear();
    rowValues.addChild("event-file", new scDataNode(
      generateEventLogName(realIndex, i, false)
    ));  
    rowValues.addChild("event-no", new scDataNode(i));
    eventName = eventListNode[i].getString(ET_EVENT_FLD_NAME) + " - " + 
      getEventName(static_cast<sgpEtEventCode>(eventListNode[i].getUInt(ET_EVENT_FLD_CODE))) + " - " +
      toString(eventListNode[i].getUInt(ET_EVENT_FLD_CODE));
    rowValues.addChild("event-name", new scDataNode(eventName));
    if (i == eventNo)
      rowString = formatTemplate(rowThisTpl, rowValues);
    else  
      rowString = formatTemplate(rowTpl, rowValues);
    tableOut += rowString;
  }
  
  values.clear();
  values.addChild("body", new scDataNode(tableOut));
  
  output += formatTemplate(tableBodyTpl, values);
  return output;
}

scString sgpEntityTracer::formatEventDetails(uint realIndex, uint eventNo)
{
  const scString subHeadTpl =
  scString()+
  "<h3>Active details</h3>\n";

  scString output;

  const scDataNode &eventsNode = m_entities[realIndex][ET_ENTITY_FLD_EVENTS][eventNo];

  if (eventsNode.hasChild(ET_EVENT_FLD_REFERENCES))
    for(uint i=0, epos = eventsNode[ET_EVENT_FLD_REFERENCES].size(); i != epos; i++)
      output += formatReference(realIndex, i, eventsNode[ET_EVENT_FLD_REFERENCES][i]);  

  if (eventsNode.hasChild(ET_EVENT_FLD_DETS)) {
    output += subHeadTpl;
    output += formatEntityDetails(eventsNode[ET_EVENT_FLD_DETS]);
  }
  
  return output;
}
  
scString sgpEntityTracer::formatReference(uint srcRealIndex, uint refNo, const scDataNode &refNode)
{
  const scString subHeadTpl =
  scString()+
  "<h3>Reference no. {ref-no}</h3>\n"+
  "<strong>Type:</strong>{ref-type}<br />";
  
  const scString entityLinkTpl =
  scString()+
  "<strong>Link:</strong><a href=\"{event-link}\">{entity-id} # {event-no} (hnd = {entity-hnd})</a><br />\n";

  const scString entityThisLinkTpl =
  scString()+
  "<strong>Link:</strong><a href=\"{event-link}\">[This entity] {entity-id} # {event-no} (hnd = {entity-hnd})</a><br />\n";
  
  scString output;
  scString tableOut;
  scString rowString;
  scDataNode values, rowValues;  
  
  values.clear();
  values.addChild("ref-no", new scDataNode(refNo));
  values.addChild("ref-type", new scDataNode(refNode.getString(ET_REF_FLD_NAME)));

  output += formatTemplate(subHeadTpl, values);

  if (refNode.hasChild(ET_REF_FLD_LINK))
  {
  // entity link
    values.clear();
    values.addChild("event-link", new scDataNode(
      generateEventLogName(
        refNode[ET_REF_FLD_LINK].getUInt(ET_ENT_LINK_FLD_REAL_INDEX), 
        refNode[ET_REF_FLD_LINK].getUInt(ET_ENT_LINK_FLD_EVENT_NO),
        false
      )
    ));    
    uint realIndex = refNode[ET_REF_FLD_LINK].getUInt(ET_ENT_LINK_FLD_REAL_INDEX);
    uint bornIndex = realIndex;
    uint bornStep = m_entities[realIndex].getUInt(ET_ENTITY_FLD_BORN_STEP);
    
    values.addChild("entity-id", new scDataNode(formatEntityId(bornStep, bornIndex)));
    values.addChild("event-no", new scDataNode(refNode[ET_REF_FLD_LINK].getUInt(ET_ENT_LINK_FLD_EVENT_NO)));
    values.addChild("entity-hnd", new scDataNode(refNode[ET_REF_FLD_LINK].getUInt(ET_ENT_LINK_FLD_REAL_INDEX)));

    if (realIndex != srcRealIndex)    
      output += formatTemplate(entityLinkTpl, values);
    else  
      output += formatTemplate(entityThisLinkTpl, values);      
  } else {
  // entity details
      output += formatEntityDetails(refNode[ET_REF_FLD_DETS]);      
  }
  
  return output;  
}

bool sgpEntityTracer::isEntityFlushed(uint realIndex)
{
  return (m_entities[realIndex][ET_ENTITY_FLD_ACTIVE_DETS].isNull());  
}

void sgpEntityTracer::addBufferedAction(sgpEtEventCode eventCode, uint realIndex, uint newEntityIndex, const scString &context, sgpEtEventCode contextEvent)
{
  std::auto_ptr<scDataNode> newEntryGuard(new scDataNode());
  newEntryGuard->addChild(ET_BUF_ACTION_FLD_OPER, new scDataNode(static_cast<uint>(eventCode)));
  newEntryGuard->addChild(ET_BUF_ACTION_FLD_REAL_INDEX, new scDataNode(realIndex));
  newEntryGuard->addChild(ET_BUF_ACTION_FLD_NEW_INDEX, new scDataNode(newEntityIndex));
  newEntryGuard->addChild(ET_BUF_ACTION_FLD_CTX, new scDataNode(context));
  if (contextEvent != eecUnknown) {
    newEntryGuard->addChild(ET_BUF_ACTION_FLD_CTX_OPER, new scDataNode(static_cast<uint>(contextEvent)));
  }  
  m_moveBuffer.addChild(newEntryGuard.release());
  m_moveBufferMarks.insert(realIndex);
}

void sgpEntityTracer::getActiveEntities(sgpEtIndexSet &output)
{  
  output.clear();
  uint realIndex, indexEpos;
  indexEpos = m_entities.size();
  for(uint i=0, epos = m_activeIndexMap.size(); i != epos; i++) {
    realIndex = m_activeIndexMap.getUInt(i);
    if (realIndex < indexEpos)
      output.insert(realIndex);
  }    
}

void sgpEntityTracer::flushDeadEntities()
{
  sgpEtIndexSet activeEntities;
  getActiveEntities(activeEntities);
      
  if (!m_entities.empty()) {  
    uint idx = m_entities.size();
    while(idx > 0) {
      idx--;
      if (!isEntityFlushed(idx)) {
        if (activeEntities.find(idx) == activeEntities.end()) {
          flushDeadEntityItem(idx);
        }  
      }  
    }  
  }
}

void sgpEntityTracer::flushDeadEntityItem(uint realIndex)
{
  m_entities[realIndex][ET_ENTITY_FLD_EVENTS].clear(); 
  m_entities[realIndex][ET_ENTITY_FLD_ACTIVE_DETS].clear(); 
}

scString sgpEntityTracer::formatEntityDetails(const scDataNode &detsNode)
{
  const scString detsTpl =
  scString()+
  "<h4>Code</h4>\n"+
  "{code}\n"+
  "<h4>Info block</h4>\n"+
  "{info}\n"+
  "<h4>Objective values</h4>\n"+
  "{objs}\n";
  
  scString output;
  scDataNode values;
  
  values.clear();
  values.addChild("code", new scDataNode(formatCode(detsNode[ET_EVENT_DETS_CODE])));
  values.addChild("info", new scDataNode(formatInfo(detsNode[ET_EVENT_DETS_INFO])));
  values.addChild("objs", new scDataNode(formatObjs(detsNode[ET_EVENT_DETS_OBJS])));
  
  output = formatTemplate(detsTpl, values);
  return output;
}

scString sgpEntityTracer::formatCode(const scDataNode &data)
{
  const scString listingTpl = 
  scString()+
  "<code>\n"+
  "{listing}\n"+
  "</code>\n";

  const scString lineTpl = 
  "{line}<br />\n";
  
  scStringList lines;
  m_entityLister->listEntityCode(data, lines);
  
  scString output;
  scDataNode values;
  scString listingOut;
  
  listingOut = "";
  for(scStringList::iterator it = lines.begin(), epos = lines.end(); it != epos; ++it)
  {
    values.clear(); 
    values.addChild("line", new scDataNode(*it));
    listingOut += formatTemplate(lineTpl, values);
  }  
  
  values.clear();
  values.addChild("listing", new scDataNode(listingOut));
  output += formatTemplate(listingTpl, values);
  
  return output;
}

scString sgpEntityTracer::formatInfo(const scDataNode &data)
{
  const scString listingTpl = 
  scString()+
  "<code>\n"+
  "{listing}\n"+
  "</code>\n";

  const scString lineTpl = 
  "{line}<br />\n";
  
  scStringList lines;
  if (!data.empty())
    listInfoBlock(data, lines);
  
  scString output;
  scDataNode values;
  scString listingOut;
  
  listingOut = "";
  for(scStringList::iterator it = lines.begin(), epos = lines.end(); it != epos; ++it)
  {
    values.clear(); 
    values.addChild("line", new scDataNode(*it));
    listingOut += formatTemplate(lineTpl, values);
  }  
  
  values.clear();
  values.addChild("listing", new scDataNode(listingOut));
  output += formatTemplate(listingTpl, values);
  
  return output;
}

scString sgpEntityTracer::formatObjs(const scDataNode &data)
{
  const scString listingTpl = 
  scString()+
  "<code>\n"+
  "{listing}\n"+
  "</code>\n";

  const scString lineTpl = 
  "fitness[{index}] = {value}<br />\n";
  
  scString output;
  scDataNode values;
  scString listingOut;
  
  listingOut = "";
  for(uint i=0, epos = data.size(); i != epos; i++) {
    values.clear(); 
    values.addChild("index", new scDataNode(i));
    values.addChild("value", new scDataNode(data.getDouble(i)));
    listingOut += formatTemplate(lineTpl, values);
  }  
  
  values.clear();
  values.addChild("listing", new scDataNode(listingOut));
  output += formatTemplate(listingTpl, values);
  
  return output;
}

uint sgpEntityTracer::countShapesForActive(const sgpEtIndexSet &activeEntities, uint shapeObjIndex)
{
  std::set<double> shapes;
  const scDataNode *detsNode;
  
  for(sgpEtIndexSet::const_iterator it = activeEntities.begin(), epos = activeEntities.end(); it != epos; ++it)
  {
    detsNode = &(m_entities[*it][ET_ENTITY_FLD_ACTIVE_DETS]);    
      shapes.insert((*detsNode)[ET_ENTITY_DETS_OBJS].getDouble(shapeObjIndex));
  }
  
  return shapes.size();
}

uint sgpEntityTracer::countShapesForClass(const scDataNode &activeIndexSet, uint shapeObjIndex)
{
  std::set<double> shapes;
  const scDataNode *detsNode;
  
  for(uint i = 0, epos = activeIndexSet.size(); i != epos; i++)
  {
    detsNode = &(m_entities[activeIndexSet.getUInt(i)][ET_ENTITY_FLD_ACTIVE_DETS]);    
      shapes.insert((*detsNode)[ET_ENTITY_DETS_OBJS].getDouble(shapeObjIndex));
  }
  
  return shapes.size();
}

scString sgpEntityTracer::formatOtherStats(const sgpEtIndexSet &activeEntities)
{
  const scString classHeadTpl = 
  "<tr><th>Parameter</th><th>Value</th></tr>";

  const scString staticHdr =
  scString()+
"<style type=\"text/css\">"+
"table.stats { border-width: 1px  1px  1px  1px; "+
"border-spacing:  2px;  border-style:  outset   outset   outset   outset;"+
"border-color:  gray   gray   gray   gray;   border-collapse:   separate;"+
"background-color: white; } table.sample th { border-width: 1px  1px  1px"+
"1px; padding: 5px 5px 5px 5px; border-style: inset  inset  inset  inset;"+
"border-color:   gray   gray   gray   gray;   background-color:    white;"+
"-moz-border-radius: 0px 0px 0px 0px; } "+
"table.stats td  {  border-width:"+
"1px 1px 1px 1px; padding: 5px 5px 5px  5px;  border-style:  inset  inset"+
"inset  inset;  border-color:  gray  gray  gray  gray;  background-color:"+
"white; -moz-border-radius: 0px 0px 0px 0px; } "+
"</style>";
  
  const scString rowTpl = 
  scString()+
  "<tr><th align=\"right\">{param-name}</th><td align=\"right\">{value}</td>"+
  "</tr>";

  const scString rowStatTpl = 
  scString()+
  "<tr><th align=\"right\">{param-name}</th><td align=\"right\">{value}</td>"+
  "</tr>";

  const scString tableTpl = 
  scString()+
  "<table class=\"stats\" border=\"1\">"+
  "<caption>{caption}</caption>"+  
  "{rows}"+
  "</table>";

  const scString subFooterTpl1 = 
  scString()+
  "<br />";

  const scString caption1 = 
  "Step statistics";
  
  scString output;
  scString tableOut;
  scString rowString;
  scDataNode values;
  scDataNode entitiesByClass, entitiesByClassAlive;
  
  getEntitiesByClass(entitiesByClass, m_entities.size());
  getEntitiesByClass(entitiesByClassAlive, m_entities.size(), true, activeEntities);

  scDataNode rowValues;

  std::set<uint> classSet;

  output += staticHdr;
  tableOut = classHeadTpl;
  
  scDataNode params;
  double value;
  
  params.addChild("Match: Number of matches", new scDataNode(m_matchCount));

  if (m_matchLooserCount > 0)
    value = static_cast<double>(m_matchWinnerCount)/static_cast<double>(m_matchLooserCount);
  else
    value = 0.0;  
  params.addChild("Match: winner / looser ratio", new scDataNode(value));

  if (m_matchCount > 0)
    value = static_cast<double>(m_matchWinnerCount)/static_cast<double>(m_matchCount);
  else
    value = 0.0;  
  params.addChild("Match: avg winner count per match", new scDataNode(value));

  if (!activeEntities.empty())
    value = static_cast<double>(m_mutEntityChangedCount)/static_cast<double>(activeEntities.size());
  else
    value = 0.0;      
  params.addChild("Mutation: entities changed", new scDataNode(
    toString(m_mutEntityChangedCount) + " ( " +
    toStringPrec(100.0 * value, 2) + 
    " % )"
  ));

  if (m_mutEntityChangedCount > 0)
    value = static_cast<double>(m_mutChangeCount)/static_cast<double>(m_mutEntityChangedCount);
  else
    value = 0.0;      

  params.addChild("Mutation: avg number of muts per changed entity", new scDataNode(value));
  
  for(uint i=0, epos = params.size(); i != epos; i++)
  {
    rowValues.clear();
    rowValues.addChild("param-name", new scDataNode(params.getElementName(i)));
    rowValues.addChild("value", new scDataNode(params.getString(i)));      
    rowString = formatTemplate(rowTpl, rowValues);
    tableOut += rowString;
  }
    
  values.clear();
  values.addChild("rows", new scDataNode(tableOut));
  values.addChild("caption", new scDataNode(caption1));
  
  output += formatTemplate(tableTpl, values);
  
  output += subFooterTpl1;
  
  return output;
}

void sgpEntityTracer::listInfoBlock(const scDataNode &data, scStringList &output)
{
  m_entityLister->listInfoBlock(data, output);
}

