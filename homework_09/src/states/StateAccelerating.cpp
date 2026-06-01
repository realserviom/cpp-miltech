#include "states/StateStopped.h"
#include "states/StateAccelerating.h"

std::unique_ptr<IDroneState> StateAccelerating::execute(Drone& curMyDrone)
{
  return std::make_unique<StateStopped>();
}

const std::string StateAccelerating::name() const
{
  return "ACCELERATING";
}
