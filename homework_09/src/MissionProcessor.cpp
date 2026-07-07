#include <iostream>
#include "Types.h"
#include "functions.h"
#include "Drone.h"
#include "MissionProcessor.h"
#include "Debug.h"
#include "constants.h"
#include <iomanip>
#include <cmath>
#include "interfaces/IDroneState.h"
#include "states/StateDecelerating.h"
#include "functions.h"
#include "RollingTargetStack.h"

void MissionProcessor::init(DroneConfig& myDrone, const AmmoParams*& ammo)
{
  // Ініціалізація
  try {
    m_configLoader->init(myDrone);
    myDrone.updateCalculatedParams();
    LOG("Дрон успішно налаштований та готовий до польоту!");
  }
  catch (const std::runtime_error& e) {
    throw std::runtime_error("[FileConfigLoader] КРИТИЧНА ПОМИЛКА: " + std::string(e.what()));
  }

  float ratio = myDrone.arrayTimeStep / myDrone.timeStep;

  if (std::abs(ratio - std::floor(ratio)) > 0.0001f) {
    throw std::runtime_error("Помилка: Крок масиву не ділиться націло на крок симуляції!");
  }

  int numberCounterInTimeSpot = static_cast<int>(ratio);

  // Отримуємо цілі
  try {
    m_targetProvider->init(numberCounterInTimeSpot);
    LOG("Цілі успішно загруженні в систему!");
  }
  catch (const std::runtime_error& e) {
    throw std::runtime_error("[JsonTargetProvider] КРИТИЧНА ПОМИЛКА: " + std::string(e.what()));
  }

  numberOfTargets = m_targetProvider->getTargetCount();

  targetTimes.resize(numberOfTargets, 0.0f);
  targetDistances.resize(numberOfTargets, 0.0f);
  targetAngles.resize(numberOfTargets, 0.0f);

  // Дебаг інфо
  DEBUG("Час зупинки або прискорення: " << std::fixed << std::setprecision(2) << myDrone.timeAcceleration << " с");
  DEBUG("Прискорення дрона: " << myDrone.acceleration << " м/с2 ---");
  DEBUG("Швидкість обертання: " << myDrone.angularSpeed << " р/с ---");
  DEBUG("===========================");
}

void MissionProcessor::setTargetProvider(std::shared_ptr<ITargetProvider> targetProvider)
{
  m_targetProvider = targetProvider;
}

void MissionProcessor::setBallisticSolver(std::shared_ptr<IBallisticSolver> solver)
{
  m_solver = solver;
}

void MissionProcessor::setConfigLoader(std::shared_ptr<IConfigLoader> configLoader)
{
  m_configLoader = configLoader;
}

void MissionProcessor::addStep(const int counter, DroneTelemetry& telemetry)
{
  steps.push_back({{telemetry.pos.x, telemetry.pos.y},
                   telemetry.angularState,
                   telemetry.stateId,
                   target,
                   {dropPoint.x, dropPoint.y},
                   {aimPoint.x, aimPoint.y},
                   {predictedTarget.x, predictedTarget.y},
                   telemetry.timeSecSinceStart,
                   counter});
}

void MissionProcessor::fillArrays(bool& canChangeTarget,
                                  const int& counter,
                                  const DroneTelemetry& telemetry,  // Передаємо безпечний знімок стану
                                  const Drone& curMyDrone,  // Потрібен лише для виклику константних методів конфігурації
                                  const float distDuringFall,
                                  const float t_pol,
                                  RollingTargetStack& targetStack)
{
  for (int targetId = 0; targetId < numberOfTargets; targetId++) {
    Target targetOptions = m_targetProvider->getTargetPosition(targetId);

    targetStack.push(targetId, targetOptions);

    Coord targetPos = targetOptions.pos;

    float length = calculateLength(targetPos - telemetry.pos);

    float deltaX = targetPos.x - telemetry.pos.x;
    float deltaY = targetPos.y - telemetry.pos.y;

    float angle_in_rad = atan2(deltaY, deltaX);

    targetAngles[targetId] = angle_in_rad;

    targetTimes[targetId] =
      curMyDrone.calculateArrivalTime(targetAngles[targetId], length, distDuringFall, telemetry.angularState, telemetry.speed);
  }
}

std::unique_ptr<IDroneState> MissionProcessor::changeTarget(
  float& targetAngle, const bool& canChangeTarget, const std::string& currentStateName, float dir, Drone& curMyDrone)
{
  const float newTarget = getIndexByMinValue(targetTimes);
  targetAngle = targetAngles[target];

  if (currentStateName != "STOPPED" && currentStateName != "DECELERATING" && canChangeTarget && newTarget != target) {
    DEBUG("Нова ціль: " << newTarget);
    target = newTarget;
    targetAngle = targetAngles[newTarget];

    if (curMyDrone.needRotation(targetAngle, dir)) {
      if (currentStateName == "MOVING" || currentStateName == "ACCELERATING") {
        DEBUG("--- Сповільнюємося!!!! Треба повертатися! ---");
        return std::make_unique<StateDecelerating>();
      }
    }
  }
  return nullptr;
}

bool MissionProcessor::isThreadReady() const
{
  return isReady;
}

void MissionProcessor::start(Drone& curMyDrone, const float distDuringFall, const float t_pol)
{
  DEBUG("--- start mission thread! ---");
  running = true;
  missionThread = std::thread(&MissionProcessor::missionLoop, this, std::ref(curMyDrone), distDuringFall, t_pol);
}

void MissionProcessor::setRunningTrue()
{
  running = true;
}

void MissionProcessor::missionLoop(Drone& curMyDrone, const float distDuringFall, const float t_pol)
{
  if (!m_targetProvider || !m_solver || !m_configLoader) {
    std::cout << "[MissionProcessor] Помилка: Не всі компоненти підключені!\n";
    return;
  }
  LOG("--- ПОЧАТОК МІСІЇ ---");

  isReady = true;

  int counter = 0;              // лічильник часу
  bool canChangeTarget = true;  // мітка чи є дозвіл міняти ціль
  // Беремо абсолютний час старту симуляції
  auto startTime = std::chrono::high_resolution_clock::now();

  int size = static_cast<int>(std::ceil(2.0 / curMyDrone.config.timeStep)) + 1;

  RollingTargetStack targetStack(size);

  while (running) {
    // розраховуємо всі дані для визначення поточної найближчої цілі
    DroneTelemetry telemetry = curMyDrone.getTelemetry();

    {
      std::lock_guard<std::mutex> lock(proccessMutex);
      DEBUG("----------  Drone Point -----------");
      DEBUG("--- counter = " << counter << " ---");
      DEBUG("--- curDroneX = " << std::fixed << std::setprecision(8) << telemetry.pos.x << " м ---");
      DEBUG("--- curDroneY = " << std::fixed << std::setprecision(8) << telemetry.pos.y << " м ---");
      DEBUG("--- curMyDrone.angularState = " << std::fixed << std::setprecision(2) << telemetry.angularState << " р. ---");
      DEBUG("--- curDroneSpeed = " << telemetry.speed << " ---");
      DEBUG("--- curDroneStateName = " << telemetry.stateName << " ---");
      DEBUG("--- currentTarget = " << target << " ---");
      DEBUG("--- timeSecSinceStart = " << telemetry.timeSecSinceStart << " ---");
    }

    // ################## РОЗРАХУНОК ТОЧКИ СКИДУ #############################################
    // -----------  заповнення масивів для пошуку найближчих цілей ---------------------------
    fillArrays(canChangeTarget, counter, telemetry, curMyDrone, distDuringFall, t_pol, targetStack);

    Coord droneDir = {(float)cos(telemetry.angularState), (float)sin(telemetry.angularState)};
    Target targetPosition = m_targetProvider->getTargetPosition(target);
    predictedTarget = predictTargetPosition(targetStack, t_pol, curMyDrone.config.timeStep, target);

    // DEBUG("--- predictedTarget: (" << predictedTarget.x << ", " << predictedTarget.y << ") ---");

    // точка скиду (куди летить дрон)
    // TODO  тут ще можна підкоригувати напрямок дрону маючи dirToDrone
    Coord dirToDrone = normalize(telemetry.pos - predictedTarget);
    dropPoint = predictedTarget - dirToDrone * (float)distDuringFall;

    // Знаходимо точку, куди прилетить боєприпас
    aimPoint = telemetry.pos + normalize(droneDir) * distDuringFall;
    // DEBUG("--- dropPoint: (" << telemetry.pos.x << ", " << telemetry.pos.y << ") ---");
    // DEBUG("--- aimPoint: (" << aimPoint.x << ", " << aimPoint.y << ") ---");

    addStep(counter, telemetry);

    // точний розрахунок коли наближаємся до вже запланованої цілі
    if (!canChangeTarget) {
      float targetAngle2 = atan2(predictedTarget.y - telemetry.pos.y, predictedTarget.x - telemetry.pos.x);
      targetAngles[target] = targetAngle2;
    }

    double finalDistance = calculateLength(aimPoint - predictedTarget);

    // умова при якій дрон попадає в ціль з точністю "curMyDrone.config.hitRadius / 6"
    // тут можна дописати якщо ми вже пройшли половину MAX_STEPS і точку скиду не найшло тоді ми зменшуємо точність
    if (curMyDrone.state->name() == "MOVING" && finalDistance <= curMyDrone.config.hitRadius / 6) {
      {
        std::lock_guard<std::mutex> lock(curMyDrone.getMutex());
        LOG("--- БОЄПРИПАС СКИНУТИЙ! Ураження : " << std::fixed << std::setprecision(2) << finalDistance << " м від цілі номер " << target
                                                  << " ---");
        DEBUG("--- myDrone.arrayTimeStep: " << std::setprecision(4) << curMyDrone.config.arrayTimeStep << " ---");
        DEBUG("--- dropPoint: (" << telemetry.pos.x << ", " << telemetry.pos.y << ") ---");
        DEBUG("--- aimPoint: (" << aimPoint.x << ", " << aimPoint.y << ") ---");
        DEBUG("--- predictedTarget: (" << predictedTarget.x << ", " << predictedTarget.y << ") ---");
      }
      running = false;
      break;
    }

    // якщо в нас відстань між дроном і цілю почала збільшуватися
    // тоді запускаємо пошук цілі знову тому що дрон не вийшов на позицію
    if (calculateLength(telemetry.pos - targetPosition.pos) > distDuringFall + curMyDrone.config.hitRadius) {
      LOG("Поточна ціль: " << target);
      LOG("canChangeTarget: true");
      canChangeTarget = true;
    }

    counter++;

    if (counter > MAX_STEPS) {
      running = false;
      break;
      LOG("============== спрацював ліміт ітерацій  ==============");
    }

    // зміна цілі і прорахунок руху дрона
    float targetAngle;
    {
      std::lock_guard<std::mutex> lock(curMyDrone.getMutex());
      std::unique_ptr<IDroneState> newState =
        changeTarget(targetAngle, canChangeTarget, telemetry.stateName, telemetry.angularState, curMyDrone);
      DroneCommand cmd;
      cmd.targetAngle = targetAngle;
      cmd.state = newState != nullptr ? std::move(newState) : nullptr;
      curMyDrone.sendCommand(std::move(cmd));
    }

    auto nextTimePoint = getNextTimePoint(startTime, (curMyDrone.config.timeStep / curMyDrone.config.timeScale), counter);
    std::this_thread::sleep_until(nextTimePoint);
  }

  if (counter <= MAX_STEPS) {
    saveOutputFileByStep(counter + 1, steps);
    LOG("--- МІСІЮ ЗАВЕРШЕНО!!! ---");
  }
}
