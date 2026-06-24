#include "Drone.h"
#include <cmath>
#include "interfaces/IDroneState.h"
#include "states/StateStopped.h"
#include "states/StateDecelerating.h"
#include "states/StateAccelerating.h"
#include "states/StateTurning.h"
#include "states/StateMoving.h"
#include <cstddef>
#include <iostream>
#include <thread>
#include "Debug.h"
#include "functions.h"

Drone::Drone(const DroneConfig& config)
  : config(config)
{
  pos.x = config.startPos.x;
  pos.y = config.startPos.y;
  speed = 0.0f;
  angularState = config.initialDir;
  state = std::make_unique<StateStopped>();
  currentTargetAngle = config.initialDir;
}

// === БАГАТОПОТОЧНИЙ ІНТЕРФЕЙС ===
void Drone::start()
{
  DEBUG("--- start drone thread! ---");
  running = true;
  physicsThread = std::thread(&Drone::physicsLoop, this);
}

void Drone::stop()
{
  running = false;
  if (physicsThread.joinable()) {
    physicsThread.join();
  }
}

bool Drone::isThreadReady() const
{
  return isReady;
}

void Drone::sendCommand(DroneCommand cmd)
{
  commandQueue.push(std::move(cmd));  // Переміщуємо команду прямо всередину черги
}

DroneTelemetry Drone::getTelemetry() const
{
  std::lock_guard<std::mutex> lock(stateMutex);
  DroneTelemetry tel;
  tel.pos = this->pos;

  // Повертаємо нормалізований вектор швидкості
  tel.normSpeed.x = std::cos(angularState) * speed;
  tel.normSpeed.y = std::sin(angularState) * speed;
  tel.speed = speed;
  tel.angularState = angularState;
  tel.stateId = state->id();
  tel.stateName = state->name();
  tel.timeSecSinceStart = timeSecSinceStart;
  return tel;
}

// === ВНУТРІШНІЙ ЦИКЛ ПОТОКУ ФІЗИКИ ===
void Drone::physicsLoop()
{
  isReady = true;
  int stepCount = 0;

  // Беремо абсолютний час старту симуляції
  auto startTime = std::chrono::high_resolution_clock::now();

  // Загортаємо ВЕСЬ робочий цикл у try-catch
  try {
    while (running) {
      // Перевіряємо чергу команд від MissionProcessor
      DroneCommand cmd;
      if (commandQueue.try_pop(cmd)) {
        std::lock_guard<std::mutex> lock(stateMutex);
        this->currentTargetAngle = cmd.targetAngle;

        if (cmd.state != nullptr) {
          this->state = std::move(cmd.state);
        }
      }

      {
        std::lock_guard<std::mutex> lock(stateMutex);
        this->move();
        timeSecSinceStart = stepCount * config.physicsTimeStep;
      }

      stepCount++;

      auto nextTimePoint = getNextTimePoint(startTime, (config.physicsTimeStep / config.timeScale), stepCount);
      std::this_thread::sleep_until(nextTimePoint);
    }
  }
  catch (const std::exception& e) {
    std::cerr << "[КРИТИЧНА ПОМИЛКА ПОТОКУ ДРОНА]: " << e.what() << '\n';
    running = false;
  }
  catch (...) {
    std::cerr << "[КРИТИЧНА ПОМИЛКА ПОТОКУ ДРОНА]: Невідомий виняток!\n";
    running = false;
  }

  isReady = false;
}

bool Drone::updateRotation(float turnThreshold)
{
  float angleDiff = currentTargetAngle - angularState;

  angleDiff = std::atan2(std::sin(angleDiff), std::cos(angleDiff));

  if (std::abs(angleDiff) < config.angularSpeed * config.physicsTimeStep) {
    angularState = currentTargetAngle;
    return false;
  }

  // Якщо різниця більша за поріг — крутимо туди, куди ближче
  if (std::abs(angleDiff) > turnThreshold) {
    if (angleDiff > 0) {
      // angleDiff додатний -> крутимо проти годинникової
      angularState += config.angularSpeed * config.physicsTimeStep;
    }
    else {
      // angleDiff від'ємний -> крутимо за годинниковою
      angularState -= config.angularSpeed * config.physicsTimeStep;
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

  float dt = config.physicsTimeStep;
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

void Drone::move()
{
  auto nextState = state->execute(*this);

  if (nextState) {
    state = std::move(nextState);
  }
}

void Drone::accelerate()
{
  speed += (config.acceleration * config.physicsTimeStep);
  if (speed > config.attackSpeed) {
    speed = config.attackSpeed;
  }
}

void Drone::decelerate()
{
  speed -= (config.acceleration * config.physicsTimeStep);
  if (speed <= 0) {
    speed = 0;
  }
}

float Drone::getSpeed()
{
  return speed;
}
