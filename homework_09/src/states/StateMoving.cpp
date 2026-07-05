#include "states/StateMoving.h"

std::unique_ptr<IDroneState> StateMoving::execute(Drone& curMyDrone)
{
  // Рівномірний рух з мінімальним обертанням
  curMyDrone.updateRotation(curMyDrone.config.turnThreshold);
  curMyDrone.updatePosition();

  return std::make_unique<StateMoving>();
}

const std::string StateMoving::name() const
{
  return "MOVING";
}

DroneStateId StateMoving::id() const
{
  return DroneStateId::MOVING;
}