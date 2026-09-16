#include "states/StateTurning.h"
#include "Debug.h"
#include "Drone.h"
#include "interfaces/IDroneState.h"

void StateTurning::execute(Drone& curMyDrone)
{
  float accel, turnRate;
  curMyDrone.calculateMoveParams(accel, turnRate, curMyDrone.config.turnThreshold);
  curMyDrone.sendMovementCommand(accel, turnRate);
}

const std::string StateTurning::name() const
{
  return "TURNING";
}

DroneStateId StateTurning::id() const
{
  return DroneStateId::TURNING;
}
