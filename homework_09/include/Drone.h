#pragma once
#include "Types.h"
#include "ThreadSafeQueue.h"  // Твоя шаблонна черга команд
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

class IDroneState;

class Drone {
public:
  Coord pos;           // поточна позиція (x, y)
  float speed;         // поточна швидкість
  float angularState;  // поточне положення дрона відносно осі x
  float timeSecSinceStart;  // час послідньої генерації фізики

  DroneConfig config;

  explicit Drone(const DroneConfig& config);

  std::unique_ptr<IDroneState> state;  // поточний стан
  bool updateRotation(float turnThreshold = 0.0f);

  void updatePosition();

  float getSpeed();

  void accelerate();
  void decelerate();

  bool needRotation(float targetAngle, float turnThreshold = 0.0f) const;

  float calculateArrivalTime(float targetAngle, float distance, float distFall) const;

  float calculateSmallArrivalTime(float distance) const;

  void move();

  // =========================================================================
  // КЕРУВАННЯ ПОТОКОМ ФІЗИКИ
  // =========================================================================
  void start();
  void stop();
  bool isThreadReady() const;

  // Потокобезпечний інтерфейс для MissionProcessor
  void sendCommand(DroneCommand cmd);
  DroneTelemetry getTelemetry() const;

private:
  void physicsLoop();  // Головний цикл фонового потоку

  // Засоби синхронізації
  std::atomic<bool> running{false};
  std::atomic<bool> isReady{false};
  std::thread physicsThread;

  mutable std::mutex stateMutex;               // Захищає фізичні параметри дрона
  ThreadSafeQueue<DroneCommand> commandQueue;  // Черга команд
  float currentTargetAngle{0.0f};              // Поточний кут, який виконує фізика
};