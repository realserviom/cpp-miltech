#include "states/StateMoving.h"
#include "states/StateAccelerating.h"
#include <memory>

std::unique_ptr<IDroneState> StateAccelerating::execute(Drone& curMyDrone)
{
  curMyDrone.updateRotation();
  curMyDrone.updatePosition();
  curMyDrone.accelerate();

  if (curMyDrone.getSpeed() >= curMyDrone.config.attackSpeed) {
    return std::make_unique<StateMoving>();
  }

  return nullptr;
}

const std::string StateAccelerating::name() const
{
  return "ACCELERATING";
}

DroneStateId StateAccelerating::id() const
{
  return DroneStateId::ACCELERATING;
}
