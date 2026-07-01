#include "states/StateAccelerating.h"
#include "states/StateTurning.h"
#include "Debug.h"
#include "Drone.h"
#include "interfaces/IDroneState.h"

std::unique_ptr<IDroneState> StateTurning::execute(Drone& curMyDrone)
{
  std::unique_ptr<IDroneState> newState = nullptr;

  float accel, turnRate;

  if (!curMyDrone.updateRotation(accel, turnRate, curMyDrone.config.turnThreshold)) {
    DEBUG("--- Повернулися! Газуємо! ---");
    newState = std::make_unique<StateAccelerating>();  // Повернулися? Газуємо!
  }
  curMyDrone.sendMovementCommand(accel, turnRate);
  newState = std::make_unique<StateTurning>();

  return newState;
}

const std::string StateTurning::name() const
{
  return "TURNING";
}

DroneStateId StateTurning::id() const
{
  return DroneStateId::TURNING;
}