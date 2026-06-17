#include "states/StateTurning.h"
#include "states/StateDecelerating.h"

std::unique_ptr<IDroneState> StateDecelerating::execute(Drone& curMyDrone)
{
  curMyDrone.updatePosition();
  curMyDrone.decelerate();

  if (curMyDrone.getSpeed() <= 0) {
    return std::make_unique<StateTurning>();
  }

  return nullptr;
}

const std::string StateDecelerating::name() const
{
  return "DECELERATING";
}

DroneStateId StateDecelerating::id() const
{
  return DroneStateId::DECELERATING;
}
