#include "states/StateTurning.h"
#include "states/StateDecelerating.h"

std::unique_ptr<IDroneState> StateDecelerating::execute(Drone& curMyDrone, const float& targetAngle)
{
  curMyDrone.updatePosition();
  curMyDrone.speed -= (curMyDrone.config.acceleration * curMyDrone.config.simTimeStep);
  if (curMyDrone.speed <= 0) {
    curMyDrone.speed = 0;
    return std::make_unique<StateTurning>();
  }

  return std::make_unique<StateDecelerating>();
}

const std::string StateDecelerating::name() const
{
  return "DECELERATING";
}

int StateDecelerating::id() const
{
  return 2;
}
