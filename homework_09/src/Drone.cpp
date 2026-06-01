#include "Drone.h"
#include <cmath>
#include "interfaces/IDroneState.h"
#include "states/StateStopped.h"
#include "states/StateAccelerating.h"
#include "states/StateDecelerating.h"
#include "states/StateTurning.h"
#include "states/StateMoving.h"

Drone::Drone(const DroneConfig& config)
  : config(config)
{
  pos.x = config.startPos.x;
  pos.y = config.startPos.y;
  speed = 0.0f;
  angularState = config.initialDir;
  state = std::make_unique<StateStopped>();
  target = 0;
  dropPoint = {0, 0};
  aimPoint = {0, 0};
  predictedTarget = {0, 0};
}

bool Drone::updateRotation(float targetAngle, float turnThreshold)
{
  if (std::abs(targetAngle - angularState) < config.radInIteration) {
    angularState = targetAngle;
    return false;
  }

  if ((targetAngle - angularState) > turnThreshold) {
    angularState += config.radInIteration;
    return true;
  }
  else if ((angularState - targetAngle) > turnThreshold) {
    angularState -= config.radInIteration;
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
  float timeTurned = (targetAngle - angularState) > config.turnThreshold ? (targetAngle - angularState) / config.angularSpeed : 0;

  return timeTurned + config.timeAcceleration + ((distance - config.accelPath - distFall) / speed);
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

// 7. Реалізація методу move
void Drone::move(int newTarget, float targetAngle, bool& keyChangeTarget)
{
  // State == STOPPED тільки коли стартує
  std::string currentNewStateName;
  std::string currentStateName = state->name();
  if (currentStateName == "STOPPED") {
    // updateRotation повертає true, якщо поворот ще триває
    if (updateRotation(targetAngle, config.turnThreshold)) {
      state = std::make_unique<StateTurning>();
    }
    else {
      state = std::make_unique<StateAccelerating>();
    }
  }
  else if (currentStateName == "DECELERATING") {
    // Зупиняємо дрон до повної зупинки, щоб потім його повернути
    updatePosition();
    speed -= (config.acceleration * config.simTimeStep);

    if (speed <= 0) {
      speed = 0;
      state = std::make_unique<StateTurning>();
    }
  }
  else if (newTarget != target) {
    // Змінюємо ціль
    keyChangeTarget = true;
    target = newTarget;  // Оновлюємо внутрішню ціль дрона, щоб не заходити сюди щоразу

    // Перевіряємо чи кут напрямку в межах нової цілі
    if (needRotation(targetAngle, config.turnThreshold)) {
      if (currentStateName == "TURNING") {
        // Якщо false — ми закінчили поворот і починаємо рух
        if (!updateRotation(targetAngle, config.turnThreshold)) {
          currentNewStateName = "ACCELERATING";
          state = std::make_unique<StateAccelerating>();
        }
      }
      else {
        updatePosition();
        if (speed <= 0) {
          speed = 0;
          state = std::make_unique<StateTurning>();
          currentNewStateName = "TURNING";
        }
        else {
          state = std::make_unique<StateDecelerating>();
          currentNewStateName = "DECELERATING";
        }
      }
    }
    else {
      currentStateName = (speed <= config.attackSpeed) ? "ACCELERATING" : "MOVING";
      updateRotation(targetAngle);  // Обертання без порогу
      updatePosition();

      // Збільшуємо швидкість
      speed += (config.acceleration * config.simTimeStep);
      if (speed >= config.attackSpeed) {
        speed = config.attackSpeed;
        state = std::make_unique<StateMoving>();
        currentNewStateName = "MOVING";
      }
    }
  }
  else {
    // Звичайний рух до поточної цілі
    if (currentStateName == "ACCELERATING") {
      // Обертаємо паралельно руху
      updateRotation(targetAngle);
      updatePosition();

      speed += (config.acceleration * config.simTimeStep);
      if (speed >= config.attackSpeed) {
        speed = config.attackSpeed;
        // currentNewStateName = "MOVING";
        state = std::make_unique<StateMoving>();
      }
    }
    else if (currentStateName == "TURNING") {
      if (!updateRotation(targetAngle, config.turnThreshold)) {
        // currentNewStateName = "ACCELERATING";
        state = std::make_unique<StateAccelerating>();
      }
    }
    else if (currentStateName == "MOVING") {
      // Рівномірний рух з мінімальним обертанням
      updateRotation(targetAngle);
      updatePosition();
    }
  }
}
