#pragma once
#include "interfaces/IDroneState.h"
#include "Drone.h"
#include "Types.h"

class StateStopped : public IDroneState {
public:
  void execute(Drone& curMyDrone) override;
  const std::string name() const override;
  DroneStateId id() const override;

};