#include "states/StateTurning.h"
#include "states/StateDecelerating.h"
#include "Debug.h"

std::unique_ptr<IDroneState> StateDecelerating::execute(Drone& curMyDrone)
{
  curMyDrone.updatePosition();

  if (curMyDrone.getSpeed() <= 0.0f) {
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
