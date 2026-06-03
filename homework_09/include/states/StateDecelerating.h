#pragma once
#include "interfaces/IDroneState.h"
#include "Drone.h"

class StateDecelerating : public IDroneState {
public:
  std::unique_ptr<IDroneState> execute(Drone& curMyDrone, const float& targetAngle) override;
  const std::string name() const override;
  int id() const override;
};