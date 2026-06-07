#pragma once
#include "Types.h"
#include "interfaces/ITargetProvider.h"

class AbstractTargetProvider : public ITargetProvider {
protected:
  // кількість цілей
  int m_targetCount = 0;
  // кількість часових кроків для кожної цілі
  int m_timeSteps = 0;
  // кількість ітерацій в одному часовому кроці
  int m_numberCounterInTimeSpot;
  Coord **m_targets = nullptr;

public:
    ~AbstractTargetProvider() override = default;

    virtual void loadTargets() = 0;

    void init(int &numberCounterInTimeSpot) override;

    int getTargetCount() override;

    Coord** getTargets() override;

    int getTimeIterationByCounter(int &counter) override;

    int getTimeIterationByTime(float time, const float &arrayTimeStep) override;

    int getNextIteration(int &iteration) override;

    Coord getTargetPositionInCounter(int &index, int &counter) override;

    Coord getTargetPositionInIteration(int &index, int &timeIteration) override;

};