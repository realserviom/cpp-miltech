#include "Drone.h"
#include <cmath>
#include "Types.h"
#include "interfaces/IDroneState.h"
#include "states/StateStopped.h"
#include <iostream>
#include <thread>
#include "Debug.h"
#include "functions.h"
#include <cmath>
#include <algorithm>

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

        DEBUG("----------- Physic ----------------");
        DEBUG("-- Physic.stepCount: " << stepCount << " --");
        DEBUG("-- Physic.angularState: " << angularState << " --");
        DEBUG("-- Physic.pox.x: " << pos.x << " --");
        DEBUG("-- Physic.pox.y: " << pos.y << " --");
        DEBUG("-- Physic.speed: " << speed << " --");
        DEBUG("-- Physic.state.name: " << state->name() << " --");

        // if (stepCount > 1980) {
        //   throw std::runtime_error("[stepCount runtime_error]");
        // }

        this->move();
        // timeSecSinceStart буде повертати одне і те саме число якщо ми:
        // проставимо різний timeScale:
        // 1) якщо physicsTimeStep = 0.001 тоді stepCount буде наприклад 1000 (за період 1 секунду) тобто в 10 раз швидше збільшуватися
        // 2) якщо physicsTimeStep = 0.01 тоді stepCount буде 1 (100 за період 1 секунду)
        timeSecSinceStart = stepCount * config.physicsTimeStep;
      }

      stepCount++;
      // тут ділимо на config.timeScale типу прискорюємо цикл while але фізику рахуємо як для timeScale = 1
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
  bool isStopped = (state->name() == "TURNING" || state->name() == "STOPPED");

  // Обчислюємо різницю кутів та нормалізуємо її в межах [-PI, PI]
  float angleDiff = currentTargetAngle - angularState;
  angleDiff = std::atan2(std::sin(angleDiff), std::cos(angleDiff));

  // Визначаємо поріг для миттєвого вирівнювання залежно від стану
  float maxStepPerTick = config.angularSpeed * config.physicsTimeStep;
  float currentThreshold = isStopped ? maxStepPerTick : turnThreshold;

  // Якщо кут менший за поріг — довертаємо точно на ціль і виходимо
  if (std::abs(angleDiff) <= currentThreshold) {
    angularState = currentTargetAngle;
    return false;
  }

  // Визначаємо крок повороту (для руху беремо мінімум)
  float rotationStep = isStopped ? maxStepPerTick : std::min(maxStepPerTick, turnThreshold);

  // Повертаємо в потрібну сторону
  if (angleDiff > 0) {
    angularState += rotationStep;
  }
  else {
    angularState -= rotationStep;
  }

  // Тримаємо кут в межах [0, 2*PI]
  if (angularState > M_PI * 2)
    angularState -= M_PI * 2;
  if (angularState < 0)
    angularState += M_PI * 2;

  return true;
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

bool Drone::needRotation(float targetAngle, float dir) const
{
  return std::abs(targetAngle - dir) > config.turnThreshold;
}

float Drone::calculateArrivalTime(float targetAngle, float distance, float distFall, float dir, float speed) const
{
  float angleDiff = targetAngle - dir;
  angleDiff = std::atan2(std::sin(angleDiff), std::cos(angleDiff));

  float actualAngleToTurn = std::abs(angleDiff);

  float timeTurned = (actualAngleToTurn > config.turnThreshold) ? ((actualAngleToTurn - config.turnThreshold) / config.angularSpeed) : 0.0f;

  float d = distance - config.accelPath - distFall;

  if (d > 0) {
    return timeTurned + config.timeAcceleration + (d / config.attackSpeed);
  }
  else {
    float smallDistance = (distance - distFall > 0) ? (distance - distFall) : distance;
    return timeTurned + calculateSmallArrivalTime(speed, smallDistance);
  }
}

float Drone::calculateSmallArrivalTime(float s, float distance) const
{
  if (s == config.attackSpeed) {
    return distance / config.attackSpeed;
  }
  float D = s * s + 2.0f * config.acceleration * distance;
  if (D < 0)
    return distance / config.attackSpeed;

  return (-s + std::sqrt(D)) / config.acceleration;
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

std::mutex& Drone::getMutex() const
{
  return stateMutex;
}

std::string Drone::getStateName() const
{
  std::lock_guard<std::mutex> lock(stateMutex);
  return state ? state->name() : "STOPPED";
}
