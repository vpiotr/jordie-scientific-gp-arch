/////////////////////////////////////////////////////////////////////////////
// Name:        GpEvalFlgSemiSwarmOccam.h
// Project:     sgpLib
// Purpose:     Filter for objectives which makes sure more important 
//              objectives dominate less important - by modifying items which
//              have better values then allowed.
// Author:      Piotr Likus
// Modified by:
// Created:     11/06/2011
/////////////////////////////////////////////////////////////////////////////

#ifndef _SGPGPEFSEMISWARMOCC_H__
#define _SGPGPEFSEMISWARMOCC_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/** \file GpEvalFlgSemiSwarmOccam.h
\brief Filter for objectives forcing objective domination by weight.

Filter for objectives which makes sure more important 
objectives dominate less important - by modifying items which
have better values then allowed.

Step 1
Find best values of objectives on level 1 (lowest objective weight).

Step k
Find best values of objectives on level k, among entities with maximim objectives
(in higher objs) found before on levels < k.
*/

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include <vector>

#include "sgp/GaEvolver.h"

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------
typedef std::vector<bool> sgpUseObjVector;
// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------

class sgpGpEvalFltSemiSwarmOccam: public sgpGaOperatorEvaluate {
public:
  // construct
  sgpGpEvalFltSemiSwarmOccam(sgpGaOperatorEvaluate *prior);
  virtual ~sgpGpEvalFltSemiSwarmOccam(); 
  // properties
  uint getObjectiveCount() const;
  void setObjectiveWeights(const sgpWeightVector &value);
  // run
  virtual bool execute(uint stepNo, bool isNewGen, sgpGaGeneration &generation);
protected:
  void updateObjectiveCount(sgpGaGeneration &generation);
  void runSemiSwarmOccam(sgpGaGeneration &newGeneration);
  void findBestObjectives(const sgpGaGeneration &aGeneration, 
    sgpFitnessValue &output, sgpUseObjVector &useVector);
  bool findBestGenomeForLevel(const sgpGaGeneration &aGeneration, double wLevel, 
    uint objectiveIndex, uint &foundIdx) const;
  bool findBestGenomeForTopLevel(const sgpGaGeneration &aGeneration, double wLevel, 
    uint &foundIdx) const;
  bool isBestOnHigherLevel(const sgpFitnessValue &fitVector, const sgpFitnessValue &bestValues, double wLevel);
private:
  sgpGaOperatorEvaluate *m_prior;
  uint m_objectiveCount;
  sgpFitnessValue m_bestObjectives;
  double m_objectiveTargetRate;
  sgpWeightVector m_objectiveWeights;
};

#endif // _SGPGPEFSEMISWARMOCC_H__
