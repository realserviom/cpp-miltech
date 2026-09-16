#include "states/StateDecelerating.h"
#include "Debug.h"
#include "constants.h"

void StateDecelerating::execute(Drone& curMyDrone)
{
  curMyDrone.sendMovementCommand(-MAX_ACCEL, 0.0f);  // Гальмуємо на повну, не крутимося
}

const std::string StateDecelerating::name() const
{
  return "DECELERATING";
}

DroneStateId StateDecelerating::id() const
{
  return DroneStateId::DECELERATING;
}
