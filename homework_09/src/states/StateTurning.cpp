#include "states/StateAccelerating.h"
#include "states/StateTurning.h"
#include "Debug.h"

std::unique_ptr<IDroneState> StateTurning::execute(Drone& curMyDrone, const float& targetAngle)
{
  if (!curMyDrone.updateRotation(targetAngle, curMyDrone.config.turnThreshold)) {
    DEBUG("--- Повернулися! Газуємо! ---");
    return std::make_unique<StateAccelerating>();  // Повернулися? Газуємо!
  }

  return std::make_unique<StateTurning>();
}

const std::string StateTurning::name() const
{
  return "TURNING";
}

int StateTurning::id() const
{
  return 3;
}
