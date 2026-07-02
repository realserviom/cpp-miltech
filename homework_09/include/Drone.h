#pragma once
#include "Types.h"
#include "ThreadSafeQueue.h"  // Твоя шаблонна черга команд
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include "UARTProcessor.h"

class IDroneState;

class Drone {
public:
  Coord pos;           // поточна позиція (x, y)
  Coord normSpeed;     // нормалізований вектор швидкості (vx, vy)
  float speed;         // поточна швидкість
  float angularState;  // поточне положення дрона відносно осі x

  DroneConfig config;
  float t_ms;  // поточний час польоту дрона в мілісекундах
  float z;     // поточна висота польоту дрона в метрах

  std::shared_ptr<UARTProcessor> m_uartProcessor;

  explicit Drone(const DroneConfig& config, std::shared_ptr<UARTProcessor> uart);

  std::unique_ptr<IDroneState> state;
  void updateRotation(float& accel, float& turnRate, float turnThreshold = 0.0f);

  float getSpeed();

  void accelerate();
  void decelerate();

  bool needRotation(float targetAngle, float turnThreshold = 0.0f) const;

  float calculateArrivalTime(float targetAngle, float distance, float distFall) const;

  float calculateSmallArrivalTime(float distance) const;

  // =========================================================================
  // КЕРУВАННЯ ПОТОКОМ ФІЗИКИ
  // =========================================================================
  void start();
  void stop();
  bool isThreadReady() const;

  void sendControl(float accel, float turnRate);

  void sendMovementCommand(float accel, float turnRate);

  // Потокобезпечний інтерфейс для MissionProcessor
  void sendCommand(DroneCommand cmd);

  // Проставляємо параметри дрона що прийшли по телеметрії
  void setDroneParams(DroneTelemetry& telemetry);

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