#pragma once
#include "interfaces/ITargetProvider.h"
#include <memory>
#include <mutex>
#include "AbstractTargetProvider.h"
#include <string>
#include <thread>
#include <mutex>
#include <atomic>

class ThreadSafeTargetProvider : public AbstractTargetProvider {
public:
  int stepCount = 0;
  ThreadSafeTargetProvider(const std::string& jsonFilePath);

  Coord getTargetPosition(const int target) override;

  void loadTargets() override;

  ~ThreadSafeTargetProvider();

  int getIterationByCounter(const int& counter);

  int getIterationByTime(float time, const float& arrayTimeStep);

  Coord getTargetPositionInIteration(const int& index, int& timeIteration);

  int getNextIteration(int& iteration);

  Coord getTargetPositionInCounter(int& targetId, const int& counter);

  // =========================================================================
  // КЕРУВАННЯ ПОТОКОМ ФІЗИКИ
  // =========================================================================
  void start();
  void stop();
  bool isThreadReady() const;

private:
  void physicsLoop();  // Головний цикл фонового потоку

  // Засоби синхронізації
  std::atomic<bool> running{false};
  std::atomic<bool> isReady{false};
  std::thread physicsThread;

  mutable std::mutex stateMutex;  // Захищає фізичні параметри дрона
};