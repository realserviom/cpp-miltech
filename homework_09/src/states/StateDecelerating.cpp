#include "states/StateTurning.h"
#include "states/StateDecelerating.h"
#include "Debug.h"

std::unique_ptr<IDroneState> StateDecelerating::execute(Drone& curMyDrone)
{
  std::unique_ptr<IDroneState> newState = nullptr;
  curMyDrone.sendMovementCommand(-1.0f, 0.0f);  // Гальмуємо на повну, не крутимося

  if (curMyDrone.getSpeed() <= 0.0f) {
    newState = std::make_unique<StateTurning>();
  }

  newState = std::make_unique<StateDecelerating>();

  return newState;
}

const std::string StateDecelerating::name() const
{
  return "DECELERATING";
}

DroneStateId StateDecelerating::id() const
{
  return DroneStateId::DECELERATING;
}
