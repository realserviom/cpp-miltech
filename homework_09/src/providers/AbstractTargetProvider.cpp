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

int AbstractTargetProvider::getTimeIterationByCounter(const int &counter)
{
  const int wholeRangeCounters = this->m_numberCounterInTimeSpot * this->m_timeSteps;

  // якщо в нас поточна ітерація більша чи рівна wholeRangeCounters тоді берем остачу від цілочисленого
  // ділення на wholeRangeCounters
  const int new_counter = counter >= wholeRangeCounters ? static_cast<int>(counter % wholeRangeCounters) : counter;

  return static_cast<int>(std::floor(new_counter / this->m_numberCounterInTimeSpot));
}

int AbstractTargetProvider::getTimeIterationByTime(float time, const float &arrayTimeStep)
{
  const int wholeRangeTime = arrayTimeStep * this->m_timeSteps;

  if (time > wholeRangeTime) {
    time = std::fmod(time, wholeRangeTime);
  }
  return std::floor(time / arrayTimeStep);
}

int AbstractTargetProvider::getNextIteration(int &iteration)
{
  if (iteration == (this->m_timeSteps - 1)) {
    return 0;
  }

  return iteration + 1;
}

Coord AbstractTargetProvider::getTargetPositionInCounter(int &targetId, const int &counter)
{
  int timeIteration = this->getTimeIterationByCounter(counter);

  if (this->m_targets && targetId >= 0 && targetId < m_targetCount) {
    return this->m_targets[targetId][timeIteration];
  }
  return Coord{0.0, 0.0};
}

Coord AbstractTargetProvider::getTargetPositionInIteration(int &index, int &timeIteration)
{
  if (this->m_targets && index >= 0 && index < m_targetCount) {
    return this->m_targets[index][timeIteration];
  }
  return Coord{0.0, 0.0};
}
