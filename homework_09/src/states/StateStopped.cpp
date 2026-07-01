#include "states/StateStopped.h"
#include "states/StateAccelerating.h"
#include "states/StateTurning.h"
#include "Drone.h"
#include "Debug.h"
#include "interfaces/IDroneState.h"

std::unique_ptr<IDroneState> StateStopped::execute(Drone& curMyDrone)
{
  std::unique_ptr<IDroneState> newState = nullptr;

  float accel, turnRate;

  if (curMyDrone.updateRotation(accel, turnRate, curMyDrone.config.turnThreshold)) {
    DEBUG("--- Стояли. Починаємо повертатися! ---");
    newState = std::make_unique<StateTurning>();
  }
  else {
    DEBUG("--- Стояли. Повертатися не треба! Почали рух! ---");
    newState = std::make_unique<StateAccelerating>();
  }

  curMyDrone.sendMovementCommand(accel, turnRate);

  return newState;
}

const std::string StateStopped::name() const
{
  return "STOPPED";
}

DroneStateId StateStopped::id() const
{
  return DroneStateId::STOPPED;
}
