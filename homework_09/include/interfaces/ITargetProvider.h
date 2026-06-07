#pragma once
#include "Types.h"

class ITargetProvider {
public:

    virtual int getNextIteration(int &iteration) = 0;
    virtual int getTimeIterationByCounter(int &counter) = 0;
    virtual int getTimeIterationByTime(float time, const float &arrayTimeStep) = 0;
    virtual int getTargetCount() = 0;
    virtual Coord getTargetPositionInCounter(int &index, int &counter) = 0;
    virtual Coord getTargetPositionInIteration(int &index, int &timeIteration) = 0;
    virtual Coord** getTargets() = 0;
    virtual ~ITargetProvider() {}
    virtual void init(int &numberCounterInTimeSpot) = 0;
};