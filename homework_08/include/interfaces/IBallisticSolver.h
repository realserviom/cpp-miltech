#pragma once
#include "Types.h"

class IBallisticSolver {
public:
    virtual double getDistDuringFall(float &t_pol, DroneConfig &myDrone, const AmmoParams *ammo) = 0;
    virtual ~IBallisticSolver() {}
};