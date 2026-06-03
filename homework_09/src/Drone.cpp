#include "Drone.h"
#include <cmath>
#include "interfaces/IDroneState.h"
#include "states/StateStopped.h"
#include "states/StateDecelerating.h"
#include <iostream>
#include "Debug.h"

Drone::Drone(const DroneConfig& config)
  : config(config)
{
  pos.x = config.startPos.x;
  pos.y = config.startPos.y;
  speed = 0.0f;
  angularState = config.initialDir;
  state = std::make_unique<StateStopped>();
  target = -1;
  dropPoint = {0, 0};
  aimPoint = {0, 0};
  predictedTarget = {0, 0};
}

bool Drone::updateRotation(float targetAngle, float turnThreshold)
{
  float angleDiff = targetAngle - angularState;

  angleDiff = std::atan2(std::sin(angleDiff), std::cos(angleDiff));

  if (std::abs(angleDiff) < config.radInIteration) {
    angularState = targetAngle;
    return false;
  }

  // Якщо різниця більша за поріг — крутимо туди, куди ближче
  if (std::abs(angleDiff) > turnThreshold) {
    if (angleDiff > 0) {
      // angleDiff додатний -> крутимо проти годинникової
      angularState += config.radInIteration;
    }
    else {
      // angleDiff від'ємний -> крутимо за годинниковою
      angularState -= config.radInIteration;
    }

    if (angularState > M_PI * 2) {
      angularState -= M_PI * 2;
    }

    if (angularState < 0) {
      angularState += M_PI * 2;
    }

    return true;
  }

  return false;
}

void Drone::updatePosition()
{
  Coord direction = {(float)cos(angularState), (float)sin(angularState)};
  Coord velocity = direction * speed;
  Coord acceleration = direction * config.acceleration;

  float dt = config.simTimeStep;
  float stepSq = (dt * dt) / 2.0f;

  std::string currentStateName = state->name();

  if (currentStateName == "MOVING") {
    pos = pos + (velocity * dt);
  }
  else if (currentStateName == "ACCELERATING") {
    pos = pos + (velocity * dt) + (acceleration * stepSq);
  }
  else if (currentStateName == "DECELERATING") {
    pos = pos + (velocity * dt) - (acceleration * stepSq);
  }
}

bool Drone::needRotation(float targetAngle, float turnThreshold) const
{
  return std::abs(targetAngle - angularState) > turnThreshold;
}

float Drone::calculateArrivalTime(float targetAngle, float distance, float distFall) const
{
  float angleDiff = targetAngle - angularState;
  angleDiff = std::atan2(std::sin(angleDiff), std::cos(angleDiff));

  float actualAngleToTurn = std::abs(angleDiff);

  float timeTurned = (actualAngleToTurn > config.turnThreshold) ? ((actualAngleToTurn - config.turnThreshold) / config.angularSpeed) : 0.0f;

  float d = distance - config.accelPath - distFall;

  if (d > 0) {
    return timeTurned + config.timeAcceleration + (d / config.attackSpeed);
  }
  else {
    float smallDistance = (distance - distFall > 0) ? (distance - distFall) : distance;
    return timeTurned + calculateSmallArrivalTime(smallDistance);
  }
}

float Drone::calculateSmallArrivalTime(float distance) const
{
  if (speed == config.attackSpeed) {
    return distance / config.attackSpeed;
  }
  float D = speed * speed + 2.0f * config.acceleration * distance;
  if (D < 0)
    return distance / config.attackSpeed;

  return (-speed + std::sqrt(D)) / config.acceleration;
}

void Drone::changeTarget(const int& newTarget, const bool& canChangeTarget, const float& targetAngle)
{
  std::string currentStateName = state->name();

  if (currentStateName != "STOPPED" && currentStateName != "DECELERATING" && canChangeTarget && newTarget != target) {
    target = newTarget;
    DEBUG("Нова ціль: " << newTarget);

    // Якщо для нової цілі треба сильно розвернутися, а ми летимо на всіх парах або прискорюємося - треба гальмувати
    if (needRotation(targetAngle, config.turnThreshold)) {
      if (currentStateName == "MOVING" || currentStateName == "ACCELERATING") {
        // auto nextState = std::make_unique<StateDecelerating>();
        state = std::make_unique<StateDecelerating>();
        DEBUG("--- Сповільнюємося!!!! Треба повертатися! ---");
      }
    }
  }
}

// Реалізація методу move
void Drone::move(int& newTarget, const bool canChangeTarget, const float& targetAngle)
{
  // тут змінюємо ціль за певних умов
  changeTarget(newTarget, canChangeTarget, targetAngle);

  std::string currentStateName = state->name();

  auto nextState = state->execute(*this, targetAngle);
  if (nextState) {
    state = std::move(nextState);
  }
}
