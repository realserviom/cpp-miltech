#include "states/StateMoving.h"
#include "states/StateAccelerating.h"
#include <memory>

std::unique_ptr<IDroneState> StateAccelerating::execute(Drone& curMyDrone)
{
  std::unique_ptr<IDroneState> newState = nullptr;

  float accel, turnRate;

  curMyDrone.updateRotation(accel, turnRate);

  accel = 1.0f;  // Газуємо на повну

  curMyDrone.sendMovementCommand(accel, turnRate);

  if (curMyDrone.getSpeed() >= curMyDrone.config.attackSpeed) {
    newState = std::make_unique<StateMoving>();
  }

  newState = std::make_unique<StateAccelerating>();

  return newState;
}

const std::string StateAccelerating::name() const
{
  return "ACCELERATING";
}

DroneStateId StateAccelerating::id() const
{
  return DroneStateId::ACCELERATING;
}
