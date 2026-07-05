#pragma once
#include "Types.h"

class ITargetProvider {
public:
  float arrayTimeStep;

  virtual int getTargetCount() = 0;

  virtual std::vector<std::vector<Coord>> getTargets() = 0;

  virtual void setArrayTimeStep(float time) = 0;
  virtual float getArrayTimeStep() const = 0;

  virtual Coord getTargetPosition(const int target, const float time) = 0;

  virtual void init(int &numberCounterInTimeSpot) = 0;

  virtual ~ITargetProvider() {}
};