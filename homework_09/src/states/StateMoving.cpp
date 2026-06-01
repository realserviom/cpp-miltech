#include "states/StateMoving.h"
#include "states/StateAccelerating.h"

std::unique_ptr<IDroneState> StateMoving::execute(Drone& curMyDrone)
{
  return std::make_unique<StateAccelerating>();
}

const std::string StateMoving::name() const
{
  return "MOVING";
}

int StateMoving::id() const
{
  return 4;
}