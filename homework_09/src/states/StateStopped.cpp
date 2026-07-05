#include "states/StateStopped.h"
#include "states/StateAccelerating.h"
#include "states/StateTurning.h"
#include "Drone.h"
#include "Debug.h"

std::unique_ptr<IDroneState> StateStopped::execute(Drone& curMyDrone, const float& targetAngle)
{
  if (curMyDrone.updateRotation(targetAngle, curMyDrone.config.turnThreshold)) {
    DEBUG("--- Стояли. Починаємо повертатися! ---");
    return std::make_unique<StateTurning>();
  }
  else {
    curMyDrone.updateRotation(targetAngle);  // Легке підрулювання
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

int StateStopped::id() const
{
  return 0;
}
