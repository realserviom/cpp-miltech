#include "states/StateMoving.h"

std::unique_ptr<IDroneState> StateMoving::execute(Drone& curMyDrone)
{
  float accel, turnRate;

  // Рівномірний рух з мінімальним обертанням
  curMyDrone.updateRotation(accel, turnRate);
  accel = 1.0f;                                     // Газуємо на повну
  curMyDrone.sendMovementCommand(accel, turnRate);  // Рухаємося на повну, і крутимося

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