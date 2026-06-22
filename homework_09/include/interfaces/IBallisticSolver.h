#pragma once
#include "Types.h"

class IBallisticSolver {
public:
    virtual double getDistDuringFall(float &t_pol, DroneConfig &myDrone, const AmmoParams *ammo) = 0;
    virtual float getTimePol(const float &d, const float &l, const float &m, const float &zd, const float &attackSpeed) = 0;

    virtual ~IBallisticSolver() {}
};