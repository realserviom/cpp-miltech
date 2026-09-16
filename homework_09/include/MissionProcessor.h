#pragma once
#include "./interfaces/IBallisticSolver.h"
#include "./interfaces/IConfigLoader.h"
#include <vector>
#include <memory>
#include "Debug.h"
#include "Drone.h"
#include "interfaces/IDroneState.h"
#include "UARTProcessor.h"
#include "RollingTargetStack.h"
#include "MavlinkTelemetry.hpp"

class MissionProcessor {
private:
    // Вказівники на наші стратегії
  std::shared_ptr<UARTProcessor> m_uartProcessor = nullptr;
  std::shared_ptr<IBallisticSolver> m_solver = nullptr;
  std::shared_ptr<IConfigLoader> m_configLoader = nullptr;
  int m_fd;

  uint8_t target;
  mutable std::mutex proccessMutex;

  // масив який містить час підльоту до кожної цілі
  std::vector<float> targetTimes;

  // масив який містить відстань під поточного положення дрону до кожної цілі
  std::vector<float> targetDistances;

  // масив який містить кут напрямку для кожної цілі відносто осі X в радіанах
  std::vector<float> targetAngles;

  void missionLoop();

  std::atomic<bool> running{false};
  std::atomic<bool> isReady{false};
  std::unique_ptr<RollingTargetStack> targetStack;

  std::unique_ptr<MavlinkTelemetry> m_mavlink;
  // std::string m_mavlinkIp = "127.0.0.1";
  std::string m_mavlinkIp = "172.19.192.1";  // for wsl
  int m_mavlinkPort = 14550;

public:
  Coord dropPoint;        // точка скиду
  Coord aimPoint;         // куди впаде бомба
  Coord predictedTarget;  // прогнозована позиція цілі

  uint8_t numberOfTargets = 0;
  float hitRadius = 0.0f;
  std::thread missionThread;

  bool already_dropped = false;

  explicit MissionProcessor(std::shared_ptr<UARTProcessor> uartProcessor,
                            std::shared_ptr<IBallisticSolver> solver,
                            std::shared_ptr<IConfigLoader> configLoader,
                            int& fd);

  void init(DroneConfig& myDrone);

  std::optional<Target> fillArrays(bool& canChangeTarget, const Drone& curMyDrone, const float distDuringFall, const float t_pol);

  std::unique_ptr<IDroneState> changeTarget(float& targetAngle, const bool& canChangeTarget, Drone& curMyDrone);

  void start();
  bool isThreadReady() const;
  void setNumberOfTarget(uint8_t number);
  void setHitRadius(float number);

};