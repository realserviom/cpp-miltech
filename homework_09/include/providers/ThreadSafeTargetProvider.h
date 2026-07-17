#pragma once
#include <mutex>
#include "AbstractTargetProvider.h"
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>

class ThreadSafeTargetProvider : public AbstractTargetProvider {
public:
  int stepCount = 0;
  std::vector<Coord> currentPositions;
  std::vector<Coord> prevPositions;

  void init(int& numberCounterInTimeSpot) override;

  ThreadSafeTargetProvider(const std::string& jsonFilePath);

  Target getTargetPosition(const int target) override;

  Coord getTargetPosition(const int target, const float time) override;

  void setTargetPosition();

  void loadTargets() override;

  ~ThreadSafeTargetProvider() override = default;

  int getIterationByCounter(const int& counter);

  int getIterationByTime(float time, const float& arrayTimeStep);

  Coord getTargetPositionInIteration(const int& index, int& timeIteration);

  int getNextIteration(int& iteration);

  Coord getTargetPositionInCounter(int& targetId, const int& counter);

  // =========================================================================
  // КЕРУВАННЯ ПОТОКОМ ФІЗИКИ
  // =========================================================================
  void start() override;
  void stop() override;
  bool isThreadReady() const override;

private:
  void physicsLoop() override;  // Головний цикл фонового потоку

  // Засоби синхронізації
  std::atomic<bool> running{false};
  std::atomic<bool> isReady{false};
  std::thread targetsThread;

  mutable std::mutex targetMutex;  // Захищає положення цілі
};