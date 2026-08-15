#include "states/StateMoving.h"
#include "constants.h"

void StateMoving::execute(Drone& curMyDrone)
{
  float accel, turnRate;
  // Рівномірний рух з мінімальним обертанням
  curMyDrone.calculateMoveParams(accel, turnRate);
  accel = MAX_ACCEL;                                // Газуємо на повну
  curMyDrone.sendMovementCommand(accel, turnRate);  // Рухаємося на повну, і крутимося
}

const std::string StateMoving::name() const
{
  return "MOVING";
}

DroneStateId StateMoving::id() const
{
  return DroneStateId::MOVING;
}