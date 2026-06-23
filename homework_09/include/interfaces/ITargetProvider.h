#pragma once
#include "Types.h"

class ITargetProvider {
public:
  float arrayTimeStep;

  virtual int getTargetCount() = 0;

  virtual Coord **getTargets() = 0;

  virtual void setArrayTimeStep(float time) = 0;
  virtual float getArrayTimeStep() const = 0;

  virtual void setTargetTimeStep(float time) = 0;
  virtual float getTargetTimeStep() const = 0;

  virtual void setTimeScale(float time) = 0;
  virtual float getTimeScale() const = 0;

  virtual Coord getTargetPosition(const int target, const float time) = 0;
  virtual Target getTargetPosition(const int target) = 0;

  virtual void init(int &numberCounterInTimeSpot) = 0;

  virtual void start() = 0;
  virtual void stop() = 0;
  virtual bool isThreadReady() const = 0;

  virtual ~ITargetProvider() = default;

private:
  virtual void physicsLoop() = 0;  // Головний цикл фонового потоку
};