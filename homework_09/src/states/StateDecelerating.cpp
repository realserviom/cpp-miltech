#include "states/StateStopped.h"
#include "states/StateDecelerating.h"

std::unique_ptr<IDroneState> StateDecelerating::execute(Drone& curMyDrone)
{
  return std::make_unique<StateStopped>();
}

const std::string StateDecelerating::name() const
{
  return "DECELERATING";
}