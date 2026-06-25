#pragma once
#include "./interfaces/IBallisticSolver.h"
#include "./interfaces/IConfigLoader.h"
#include "./interfaces/ITargetProvider.h"
#include <vector>
#include <memory>
#include "Debug.h"
#include "constants.h"
#include "Drone.h"
#include "interfaces/IDroneState.h"

class MissionProcessor {
private:
    // Вказівники на наші стратегії
  std::shared_ptr<ITargetProvider> m_targetProvider = nullptr;
  std::shared_ptr<IBallisticSolver> m_solver = nullptr;
  std::shared_ptr<IConfigLoader> m_configLoader = nullptr;

  int target;

  // масив який містить час підльоту до кожної цілі
  std::vector<float> targetTimes;

  // масив який містить відстань під поточного положення дрону до кожної цілі
  std::vector<float> targetDistances;

  // масив який містить кут напрямку для кожної цілі відносто осі X в радіанах
  std::vector<float> targetAngles;

  void missionLoop(Drone& curMyDrone);

  std::atomic<bool> running{false};
  std::atomic<bool> isReady{false};

public:
  std::vector<SimStep> steps{MAX_STEPS};  // Масив кроків для симуляції

  Coord dropPoint;        // точка скиду
  Coord aimPoint;         // куди впаде бомба
  Coord predictedTarget;  // прогнозована позиція цілі

  int numberOfTargets = 0;
  float distDuringFall = 0.0f;
  float t_pol = 0.0f;
  std::thread missionThread;

  MissionProcessor(std::shared_ptr<ITargetProvider> targetProvider,
                   std::shared_ptr<IBallisticSolver> solver,
                   std::shared_ptr<IConfigLoader> configLoader)
    : m_targetProvider(targetProvider)
    , m_solver(solver)
    , m_configLoader(configLoader)
  {
    target = 0;
    dropPoint = {0, 0};
    aimPoint = {0, 0};
    predictedTarget = {0, 0};
  }

  void init(DroneConfig& myDrone, const AmmoParams*& ammo);

  void fillArrays(bool& canChangeTarget, const int& counter, const Drone& curMyDrone);

  void setTargetProvider(std::shared_ptr<ITargetProvider> targetProvider);
  void setBallisticSolver(std::shared_ptr<IBallisticSolver> solver);
  void setConfigLoader(std::shared_ptr<IConfigLoader> configLoader);
  void addStep(const int counter, DroneTelemetry& telemetry);
  std::unique_ptr<IDroneState> changeTarget(float& targetAngle, const bool& canChangeTarget, Drone& curMyDrone);

  void start(Drone& curMyDrone);
  bool isThreadReady() const;
};