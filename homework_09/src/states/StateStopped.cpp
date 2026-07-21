#include "states/StateStopped.h"
#include "Drone.h"
#include "Debug.h"
#include "interfaces/IDroneState.h"

void StateStopped::execute(Drone& curMyDrone)
{
  float accel, turnRate;
  curMyDrone.calculateMoveParams(accel, turnRate, curMyDrone.config.turnThreshold);
  curMyDrone.sendMovementCommand(accel, turnRate);
}

const std::string StateStopped::name() const
{
  return "STOPPED";
}

DroneStateId StateStopped::id() const
{
  return DroneStateId::STOPPED;
}
