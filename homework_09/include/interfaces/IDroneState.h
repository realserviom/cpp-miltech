#pragma once
#include <memory>
#include "Drone.h"

class IDroneState {
public:
    virtual ~IDroneState() = default;
 
    virtual std::unique_ptr<IDroneState>
        execute(Drone& curMyDrone) = 0;
 
    virtual const std::string name() const = 0;
};
