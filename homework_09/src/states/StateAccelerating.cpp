#include "states/StateMoving.h"
#include "states/StateAccelerating.h"

std::unique_ptr<IDroneState> StateAccelerating::execute(Drone& curMyDrone, const float& targetAngle)
{
  curMyDrone.updateRotation(targetAngle);  // Легке підрулювання
  curMyDrone.updatePosition();
  curMyDrone.speed += (curMyDrone.config.acceleration * curMyDrone.config.simTimeStep);
  if (curMyDrone.speed >= curMyDrone.config.attackSpeed) {
    curMyDrone.speed = curMyDrone.config.attackSpeed;
    return std::make_unique<StateMoving>();
  }

  return std::make_unique<StateAccelerating>();
}

const std::string StateAccelerating::name() const
{
  return "ACCELERATING";
}

int StateAccelerating::id() const
{
  return 1;
}
