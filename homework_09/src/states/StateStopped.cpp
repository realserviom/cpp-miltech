#include "states/StateStopped.h"
#include "states/StateAccelerating.h"
#include "states/StateTurning.h"
#include "Drone.h"
#include "Debug.h"
#include "interfaces/IDroneState.h"

std::unique_ptr<IDroneState> StateStopped::execute(Drone& curMyDrone)
{
  if (curMyDrone.updateRotation(curMyDrone.config.turnThreshold)) {
    DEBUG("--- Стояли. Починаємо повертатися! ---");
    return std::make_unique<StateTurning>();
  }
  else {
    curMyDrone.updateRotation();  // Легке підрулювання
    curMyDrone.updatePosition();
    DEBUG("--- Стояли. Повертатися не треба! Почали рух! ---");
    return std::make_unique<StateAccelerating>();
  }

  return std::make_unique<StateAccelerating>();
}

const std::string StateStopped::name() const
{
  return "STOPPED";
}

DroneStateId StateStopped::id() const
{
  return DroneStateId::STOPPED;
}
