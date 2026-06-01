#include "states/StateStopped.h"
#include "states/StateTurning.h"

std::unique_ptr<IDroneState> StateTurning::execute(Drone& curMyDrone)
{
  return std::make_unique<StateStopped>();
}

const std::string StateTurning::name() const
{
  // default:           return "UNKNOWN";
  return "TURNING";
}

int StateTurning::id() const
{
  return 3;
}
