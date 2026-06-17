#include "states/StateAccelerating.h"
#include "states/StateTurning.h"
#include "Debug.h"
#include "Drone.h"
#include "interfaces/IDroneState.h"

std::unique_ptr<IDroneState> StateTurning::execute(Drone& curMyDrone)
{
  if (!curMyDrone.updateRotation(curMyDrone.config.turnThreshold)) {
    DEBUG("--- Повернулися! Газуємо! ---");
    return std::make_unique<StateAccelerating>();  // Повернулися? Газуємо!
  }

  return std::make_unique<StateTurning>();
}

const std::string StateTurning::name() const
{
  return "TURNING";
}

DroneStateId StateTurning::id() const
{
  return DroneStateId::TURNING;
}