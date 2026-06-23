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

Coord **AbstractTargetProvider::getTargets()
{
  return this->m_targets;
}

float AbstractTargetProvider::getArrayTimeStep() const
{
  return arrayTimeStep;
}

void AbstractTargetProvider::setArrayTimeStep(float time)
{
  arrayTimeStep = time;
}

float AbstractTargetProvider::getTargetTimeStep() const
{
  return targetTimeStep;
}

void AbstractTargetProvider::setTargetTimeStep(float time)
{
  targetTimeStep = time;
}

float AbstractTargetProvider::getTimeScale() const
{
  return timeScale;
}

void AbstractTargetProvider::setTimeScale(float time)
{
  timeScale = time;
}
