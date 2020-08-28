/////////////////////////////////////////////////////////////////////////////
// Name:        GpEvalFlgSemiSwarmOccam.cpp
// Project:     sgpLib
// Purpose:     Filter for objectives which makes sure more important 
//              objectives dominate less important - by modifying items which
//              have better values then allowed.
// Author:      Piotr Likus
// Modified by:
// Created:     11/06/2011
/////////////////////////////////////////////////////////////////////////////

#include "sgp/GpEvalFltSemiSwarmOccam.h"

sgpGpEvalFltSemiSwarmOccam::sgpGpEvalFltSemiSwarmOccam(sgpGaOperatorEvaluate *prior): 
  sgpGaOperatorEvaluate(), m_prior(prior), m_objectiveCount(0)
  {
  }

sgpGpEvalFltSemiSwarmOccam::~sgpGpEvalFltSemiSwarmOccam()
{
}

uint sgpGpEvalFltSemiSwarmOccam::getObjectiveCount() const
{
  return m_objectiveCount;
}

bool sgpGpEvalFltSemiSwarmOccam::execute(uint stepNo, bool isNewGen, sgpGaGeneration &generation)
{
  bool res = m_prior->execute(stepNo, isNewGen, generation);
  if (m_objectiveCount == 0)
    updateObjectiveCount(generation);
  runSemiSwarmOccam(generation);
  return res;
}

void sgpGpEvalFltSemiSwarmOccam::updateObjectiveCount(sgpGaGeneration &generation)
{
  if (generation.empty())
    return;

  sgpFitnessValue val; 
  generation.at(0).getFitness(val);
  m_objectiveCount = val.size();
}

void sgpGpEvalFltSemiSwarmOccam::setObjectiveWeights(const sgpWeightVector &value)
{
  m_objectiveWeights = value;
  m_objectiveCount = value.size();
}


//---------------------------------------------------------------------------------
/// Filters objective values basing on objective weights.
/// Eliminates best values from items which are weak on more important objectives.
/// So if we are looking for best value of objective with weight = 2, we use only
/// entities with objectives with weight <= 2. 
/// It forces domination of more important objectives over less important.
///
/// Best objective vector is updated using PSO algorithm.
//---------------------------------------------------------------------------------
void sgpGpEvalFltSemiSwarmOccam::runSemiSwarmOccam(sgpGaGeneration &newGeneration)
{
  sgpFitnessValue newBestValues;
  sgpFitnessValue fitVector;
  sgpUseObjVector useVector;
  double targetValue;
  
  findBestObjectives(newGeneration, newBestValues, useVector);  
    
  for(uint i=1, epos = getObjectiveCount(); i != epos; i++)
  {
    if (!useVector[i])
      continue;
      
    if (m_bestObjectives.size() == 0) {
      targetValue = newBestValues[i];
    } else {
      targetValue = m_objectiveTargetRate*newBestValues[i] + (1.0-m_objectiveTargetRate)*m_bestObjectives[i];
    }  

    for(uint j=0,eposj=newGeneration.size(); j!=eposj; j++) 
    {
      newGeneration.at(j).getFitness(fitVector);
      if (!isBestOnHigherLevel(fitVector, newBestValues, m_objectiveWeights[i])) {
        if (fitVector[i] > targetValue) {
          newGeneration.at(j).setFitness(i, targetValue);
        }  
      }  
    }
  }
  
  m_bestObjectives = newBestValues;
}


// for each objective find best value using weights  
void sgpGpEvalFltSemiSwarmOccam::findBestObjectives(const sgpGaGeneration &aGeneration, 
  sgpFitnessValue &output, sgpUseObjVector &useVector)
{
  double wLevel;
  uint bestIndex;

  uint objCnt = getObjectiveCount();
  output.resize(objCnt);
  useVector.resize(objCnt);
  
  for(uint i=1, epos = objCnt; i != epos; i++)
  {
    wLevel = m_objectiveWeights[i];
    useVector[i] = findBestGenomeForLevel(aGeneration, wLevel, i, bestIndex);
    output[i] = aGeneration.at(bestIndex).getFitness(i);      
  }    
}


// find best genome index using objectives with weight < than specified level
// if several found on best level - return the one with lowest value of specified objective 
bool sgpGpEvalFltSemiSwarmOccam::findBestGenomeForLevel(const sgpGaGeneration &aGeneration, double wLevel, 
  uint objectiveIndex, uint &foundIdx) const
{
  uint bestIdx = 0;
  sgpFitnessValue fit, bestFit;
  
  bool found = false;
  bool match = false;
  bool equal;
  uint objCnt = getObjectiveCount();
  bool topLevel = true;

  for(uint j = 1; j != objCnt; j++)
  {
    if (m_objectiveWeights[j] < wLevel) {
      topLevel = false;
      break;
    }
  }
  
  if (topLevel) {
    findBestGenomeForTopLevel(aGeneration, wLevel, foundIdx);
    return false;
  }  
        
  for(uint i = aGeneration.beginPos(), epos = aGeneration.endPos(); i != epos; i++)
  {
    aGeneration.at(i).getFitness(fit);
    if (found) {
    // compare to best
      match = true;
      equal = true;
      for(uint j = 1; j != objCnt; j++)
      {
        if (m_objectiveWeights[j] < wLevel) {
          // correct level
          if (fit[j] < bestFit[j]) {
            match = false;
            equal = false;
            break;
          } else if (fit[j] != bestFit[j]) {
            equal = false;
          } 
        } // correct level 
      } // for j
      
      if (equal) {
      // ensure that directing objective value is lowest
        if (fit[objectiveIndex] > bestFit[objectiveIndex]) {
          match = false;
        }
      }
    } // if can compare
        
    if (!found || match)
    {
      bestFit = fit;
      bestIdx = i;
      found = true;
    }
  }
  
  foundIdx = bestIdx;
  return found;
}

bool sgpGpEvalFltSemiSwarmOccam::findBestGenomeForTopLevel(const sgpGaGeneration &aGeneration, double wLevel, 
  uint &foundIdx) const
{
  uint bestIdx = 0;
  sgpFitnessValue fit, bestFit;
  bool found = false;
  bool match = false;
  uint objCnt = getObjectiveCount();
        
  for(uint i = aGeneration.beginPos(), epos = aGeneration.endPos(); i != epos; i++)
  {
    aGeneration.at(i).getFitness(fit);
    if (found) {
    // compare to best
      match = true;
      for(uint j = 1; j != objCnt; j++)
      {
        if (m_objectiveWeights[j] <= wLevel) {
          // correct level
          if (fit[j] < bestFit[j]) {
            match = false;
            break;
          } 
        } // correct level 
      } // for j
    } // if can compare
        
    if (!found || match)
    {
      bestFit = fit;
      bestIdx = i;
      found = true;
    }
  }
  
  foundIdx = bestIdx;
  return found;
}

bool sgpGpEvalFltSemiSwarmOccam::isBestOnHigherLevel(const sgpFitnessValue &fitVector, const sgpFitnessValue &bestValues, double wLevel)
{
  bool res = true;
  for(uint i=1, epos = getObjectiveCount(); i != epos; i++)
  {
    if (m_objectiveWeights[i] < wLevel) {
      if (fitVector[i] < bestValues[i]) {
        res = false;
        break;
      }
    }
  }
  return res;
}
