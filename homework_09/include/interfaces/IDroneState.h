#pragma once
#include <memory>
#include "DroneStateId.h"

class Drone;  // ГАРАНТІЯ БЕЗПЕКИ: просто кажемо, що такий клас є

class IDroneState {
public:
    virtual ~IDroneState() = default;

    virtual void execute(Drone& curMyDrone) = 0;

    virtual const std::string name() const = 0;

    virtual DroneStateId id() const = 0;
};
