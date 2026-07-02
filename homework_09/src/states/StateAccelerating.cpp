#include "states/StateAccelerating.h"
#include "constants.h"

void StateAccelerating::execute(Drone& curMyDrone)
{
  float accel, turnRate;

  curMyDrone.updateRotation(accel, turnRate);

  accel = MAX_ACCEL;  // Газуємо на повну

  curMyDrone.sendMovementCommand(accel, turnRate);
}

const std::string StateAccelerating::name() const
{
  return "ACCELERATING";
}

DroneStateId StateAccelerating::id() const
{
  return DroneStateId::ACCELERATING;
}
