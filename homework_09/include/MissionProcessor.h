#pragma once
#include "./interfaces/IBallisticSolver.h"
#include "./interfaces/IConfigLoader.h"
#include "./interfaces/ITargetProvider.h"
#include <vector>
#include <memory>
#include "Debug.h"
#include "constants.h"
#include "Drone.h"

class MissionProcessor {
private:
    // Вказівники на наші стратегії
  std::unique_ptr<ITargetProvider> m_targetProvider = nullptr;
  std::unique_ptr<IBallisticSolver> m_solver = nullptr;
  std::unique_ptr<IConfigLoader> m_configLoader = nullptr;

  // масив який містить час підльоту до кожної цілі
  std::vector<float> targetTimes;

  // масив який містить відстань під поточного положення дрону до кожної цілі
  std::vector<float> targetDistances;

  // масив який містить кут напрямку для кожної цілі відносто осі X в радіанах
  std::vector<float> targetAngles;

  Drone init(DroneConfig& myDrone, const AmmoParams*& ammo, int& numberCounterInTimeSpot, int& numberOfTargets);

public:
  std::vector<SimStep> steps{MAX_STEPS};  // Масив кроків для симуляції

  MissionProcessor(std::unique_ptr<ITargetProvider> targetProvider,
                   std::unique_ptr<IBallisticSolver> solver,
                   std::unique_ptr<IConfigLoader> configLoader)
    : m_targetProvider(std::move(targetProvider))
    , m_solver(std::move(solver))
    , m_configLoader(std::move(configLoader))
  {
  }

  void setTargetProvider(std::unique_ptr<ITargetProvider> targetProvider);
  void setBallisticSolver(std::unique_ptr<IBallisticSolver> solver);
  void setConfigLoader(std::unique_ptr<IConfigLoader> configLoader);
  void executeMission();
  void addStep(const int counter, Drone& mDrone);
};