#include "states/StateTurning.h"
#include "states/StateDecelerating.h"
#include "Debug.h"

std::unique_ptr<IDroneState> StateDecelerating::execute(Drone& curMyDrone)
{
  DEBUG("--- StateDecelerating!!! ---");

  curMyDrone.updatePosition();
  curMyDrone.decelerate();

  DEBUG("--- curMyDrone.getSpeed(): " << curMyDrone.getSpeed());

  if (curMyDrone.getSpeed() <= 0) {
    return std::make_unique<StateTurning>();
  }

  return std::make_unique<StateDecelerating>();
}

const std::string StateDecelerating::name() const
{
  return "DECELERATING";
}

DroneStateId StateDecelerating::id() const
{
  return DroneStateId::DECELERATING;
}
