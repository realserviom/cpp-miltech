#pragma once
#include "interfaces/IDroneState.h"
#include "Drone.h"

class StateDecelerating : public IDroneState {
public:
  void execute(Drone& curMyDrone) override;
  const std::string name() const override;
  DroneStateId id() const override;
};