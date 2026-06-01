#include "states/StateStopped.h"
#include "states/StateAccelerating.h"
#include "Drone.h"

std::unique_ptr<IDroneState> StateStopped::execute(Drone& curMyDrone)
{
  return std::make_unique<StateAccelerating>();
}

const std::string StateStopped::name() const
{
  return "STOPPED";
}

int StateStopped::id() const
{
  return 0;
}
