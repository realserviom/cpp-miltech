#include "Drone.h"
#include <unistd.h>
#include <cmath>
#include "interfaces/IDroneState.h"
#include "states/StateAccelerating.h"
#include "states/StateDecelerating.h"
#include "states/StateMoving.h"
#include "states/StateStopped.h"
#include <iostream>
#include <thread>
#include "Debug.h"
#include "UARTProcessor.h"
#include "states/StateTurning.h"
#include <unistd.h>
#include "constants.h"

Drone::Drone(const DroneConfig& config, std::shared_ptr<UARTProcessor> uart)
  : config(config)
  , m_uartProcessor(uart)
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

void Drone::sendMovementCommand(float accel, float turnRate)
{
  // DEBUG("Send accel, turnRate: " << accel << ", " << turnRate);
  m_uartProcessor->sendControl(accel, turnRate);
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

void Drone::setDroneParams(DroneTelemetry& telemetry)
{
  pos = telemetry.pos;
  speed = telemetry.speed;
  angularState = telemetry.angularState;
  z = telemetry.z;
  t_ms = telemetry.t_ms;
  normSpeed = telemetry.normSpeed;

  switch (telemetry.stateId) {
    case 0: {
      state = std::make_unique<StateStopped>();
      break;
    }
    case 1: {
      state = std::make_unique<StateAccelerating>();
      break;
    }
    case 2: {
      state = std::make_unique<StateDecelerating>();
      break;
    }
    case 3: {
      state = std::make_unique<StateTurning>();
      break;
    }
    case 4: {
      state = std::make_unique<StateMoving>();
      break;
    }
    default: {
      state = std::make_unique<StateStopped>();
      break;
    }
  }
}

// === ВНУТРІШНІЙ ЦИКЛ ПОТОКУ ФІЗИКИ ===
void Drone::physicsLoop()
{
  isReady = true;

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
        if (state != nullptr) {
          state->execute(*this);
          state = nullptr;
        }
      }

      usleep(100000);
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

void Drone::updateRotation(float& accel, float& turnRate, float turnThreshold)
{
  float angleDiff = currentTargetAngle - angularState;

  angleDiff = std::atan2(std::sin(angleDiff), std::cos(angleDiff));

  if (std::abs(angleDiff) - turnThreshold < 0.0001f) {
    accel = MAX_ACCEL;  // Газуємо на повну
    turnRate = 0;       // Не крутимося
  }
  else if (std::abs(angleDiff) < turnThreshold) {
    accel = MAX_ACCEL;                                            // Газуємо на повну
    turnRate = (angleDiff > 0) ? MAX_TURN_RATE : -MAX_TURN_RATE;  // Крутимо в напрямку цілі +1 це вліво
  }
  else if (std::abs(angleDiff) > turnThreshold) {
    accel = 0.0f;                               // Не газуємо
    turnRate = (angleDiff > 0) ? MAX_TURN_RATE : -MAX_TURN_RATE;  // Крутимо в напрямку цілі +1 це вліво
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

float Drone::getSpeed()
{
  return speed;
}
