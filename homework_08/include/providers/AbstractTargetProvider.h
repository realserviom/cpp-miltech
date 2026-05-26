#pragma once
#pragma once
#include "../Types.h"
#include "../interfaces/ITargetProvider.h"


class AbstractTargetProvider : public ITargetProvider {
protected:
    int m_targetCount = 0;
    int m_timeSteps = 0;
    int m_numberCounterInTimeSpot;
    Coord** m_targets = nullptr; 

public:
    ~AbstractTargetProvider() override = default;

    virtual void loadTargets() = 0;

    void init(int &numberCounterInTimeSpot) override;

    int getTargetCount() override;

    Coord** getTargets() override;

    int getTimeIteration(int &counter) override;

    int getNextIteration(int &iteration) override;

    Coord getTargetPositionInCounter(int &index, int &counter) override;

    Coord getTargetPositionInIteration(int &index, int &timeIteration) override;

};