#include <iostream>
#include "Types.h"
#include "providers/AbstractTargetProvider.h"
#include <cmath>
#include "Debug.h"

void AbstractTargetProvider::init(int &numberCounterInTimeSpot)
{
  m_numberCounterInTimeSpot = numberCounterInTimeSpot;
  LOG("Preparing targets...");
  loadTargets();
  LOG("Targets ready");
}

int AbstractTargetProvider::getTargetCount()
{
  return m_targetCount;
}

std::vector<std::vector<Coord>> AbstractTargetProvider::getTargets()
{
  return this->m_targets;
}

float AbstractTargetProvider::getArrayTimeStep() const
{
  return arrayTimeStep;
}
