#pragma once
#include "Types.h"
#include "../interfaces/IBallisticSolver.h"

class AnalyticalSolver : public IBallisticSolver {
public:
    AnalyticalSolver() = default;

    double getDistDuringFall(float &t_pol, DroneConfig &myDrone, const AmmoParams *ammo) override;
    float getTimePol(const float &d, const float &l, const float &m, const float &zd, const float &attackSpeed);
    ~AnalyticalSolver() override = default;
};
