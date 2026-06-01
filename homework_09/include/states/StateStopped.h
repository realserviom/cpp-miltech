#pragma once
#include "interfaces/IDroneState.h"
#include "Drone.h"

class StateStopped : public IDroneState {
public:
    std::unique_ptr<IDroneState> execute(Drone& curMyDrone) override;
    const std::string name() const override;
    int id() const override;
};