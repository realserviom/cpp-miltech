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

  float arrayTimeStep = 0.0f;

public:
    ~AbstractTargetProvider() override = default;

    virtual void loadTargets() = 0;

    int getTargetCount() override;

    virtual void init(int &numberCounterInTimeSpot) override;

    Coord **getTargets() override;

    float getArrayTimeStep() const override;
};