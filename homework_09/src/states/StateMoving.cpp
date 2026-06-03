#include "states/StateMoving.h"

std::unique_ptr<IDroneState> StateMoving::execute(Drone& curMyDrone, const float& targetAngle)
{
  // Рівномірний рух з мінімальним обертанням
  curMyDrone.updateRotation(targetAngle);
  curMyDrone.updatePosition();

  return std::make_unique<StateMoving>();
}

const std::string StateMoving::name() const
{
  return "MOVING";
}

int StateMoving::id() const
{
  return 4;
}