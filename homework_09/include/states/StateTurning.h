#pragma once
#include "interfaces/IDroneState.h"


class StateTurning : public IDroneState {
public:
    std::unique_ptr<IDroneState> execute(Drone& curMyDrone) override;
    const std::string name() const override;
};