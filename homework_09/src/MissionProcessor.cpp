#include <iostream>
#include "GPIOController.h"
#include "Types.h"
#include "UARTProcessor.h"
#include "functions.h"
#include "Drone.h"
#include "MissionProcessor.h"
#include "Debug.h"
#include <iomanip>
#include <cmath>
#include <thread>
#include "interfaces/IDroneState.h"
#include "states/StateDecelerating.h"
#include "functions.h"
#include <unistd.h>
#include <chrono>
#include <thread>
#include "RollingTargetStack.h"
#include "MavlinkTelemetry.hpp"

MissionProcessor::MissionProcessor(std::shared_ptr<UARTProcessor> uartProcessor,
                                   std::shared_ptr<IBallisticSolver> solver,
                                   std::shared_ptr<IConfigLoader> configLoader,
                                   int& fd)
  : m_uartProcessor(uartProcessor)
  , m_solver(solver)
  , m_configLoader(configLoader)
  , m_fd(fd)
  , target(0)
  , dropPoint({0, 0})
  , aimPoint({0, 0})
  , predictedTarget({0, 0})
{
}

void MissionProcessor::setNumberOfTarget(uint8_t number)
{
  numberOfTargets = number;
}

void MissionProcessor::setHitRadius(float number)
{
  hitRadius = number;
}

void MissionProcessor::init(DroneConfig& myDrone)
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

  targetTimes.resize(numberOfTargets, 0.0f);
  targetDistances.resize(numberOfTargets, 0.0f);
  targetAngles.resize(numberOfTargets, 0.0f);

  // Дебаг інфо
  DEBUG("Час зупинки або прискорення: " << std::fixed << std::setprecision(2) << myDrone.timeAcceleration << " с");
  DEBUG("Прискорення дрона: " << myDrone.acceleration << " м/с2 ---");
  DEBUG("Швидкість обертання: " << myDrone.angularSpeed << " р/с ---");
  DEBUG("===========================");
}

std::optional<Target> MissionProcessor::fillArrays(bool& canChangeTarget,
                                                   const Drone& curMyDrone,  // Потрібен лише для виклику константних методів конфігурації
                                                   const float distDuringFall,
                                                   const float t_pol)
{
  std::optional<Target> currentTargetOption;
  for (int targetId = 0; targetId < numberOfTargets; targetId++) {
    auto targetOptions = m_uartProcessor->getTargetPosition(targetId);

    if (!targetOptions.has_value()) {
      continue;
    }

    if (target == targetId) {
      currentTargetOption = targetOptions.value();
    }

    targetStack->push(targetId, targetOptions.value());

    Coord targetPos = targetOptions->pos;

    float length = calculateLength(targetPos - curMyDrone.pos);

    float deltaX = targetPos.x - curMyDrone.pos.x;
    float deltaY = targetPos.y - curMyDrone.pos.y;

    float angle_in_rad = atan2(deltaY, deltaX);

    targetAngles[targetId] = angle_in_rad;

    targetTimes[targetId] = curMyDrone.calculateArrivalTime(targetAngles[targetId], length, distDuringFall);
  }

  return currentTargetOption;
}

std::unique_ptr<IDroneState> MissionProcessor::changeTarget(float& targetAngle, const bool& canChangeTarget, Drone& curMyDrone)
{
  const float newTarget = getIndexByMinValue(targetTimes);
  targetAngle = targetAngles[target];

  const std::string currentStateName = curMyDrone.state->name();

  // canChangeTarget - мітка чи є дозвіл міняти ціль (якщо дрон вже летить на ціль і вона не далеко, то міняти ціль не можна)
  // newTarget != target - перевірка чи дійсно нова ціль? Щоб лишній раз не гальмувати і обертати
  // (хоча іноді навіть поточна ціль може критично змінити напрямок руху і тоді треба буде гальмувати і розвертатися але це не враховано)
  // currentStateName != "STOPPED" - перевірка чи дрон не стоїть на місці (якщо стоїть то можна міняти ціль)
  // currentStateName != "DECELERATING" - перевірка чи дрон
  if (canChangeTarget && newTarget != target && currentStateName != "STOPPED" && currentStateName != "DECELERATING") {
    DEBUG("Нова ціль: " << newTarget);
    target = newTarget;
    targetAngle = targetAngles[newTarget];

    if (curMyDrone.needRotation(targetAngle)) {
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

void MissionProcessor::start()
{
  DEBUG("--- start mission thread! ---");
  running = true;
<<<<<<< HEAD
  missionThread = std::thread(&MissionProcessor::missionLoop, this);
}

=======

  m_mavlink = std::make_unique<MavlinkTelemetry>();

  if (!m_mavlink->init(mavlinkIp, mavlinkPort)) {
    std::cerr << "[Drone] Failed to initialize MAVLink telemetry!" << std::endl;
    return false;
  }

  std::cout << "[Drone] MAVLink telemetry started on " << mavlinkIp << ":" << mavlinkPort << std::endl;

  missionThread = std::thread(&MissionProcessor::missionLoop, this);
}

>>>>>>> a99eaf74d126f5fa9e70a0c25d6fb0cc0c686988
void MissionProcessor::missionLoop()
{
  LOG("--- ПОЧАТОК МІСІЇ ---");
  isReady = true;
  bool canChangeTarget = true;  // мітка чи є дозвіл міняти ціль
  float prevFinalDistance = 100000000;
  DroneConfig myDroneConfig;

  // Локальні змінні для потоку балістики/main
  const dlink::AmmoCfg* ammo = nullptr;
  std::unique_ptr<Drone> curMyDrone = nullptr;

  float t_pol;               // час польоту снаряду
  float distDuringFall = 0;  // дистанція, яку проходить снаряд за час t_pol (горизонтальна відстань від точки скидання до цілі)

  dlink::Telemetry droneData;

  bool already_dropped = false;
  auto nextTimePoint = std::chrono::high_resolution_clock::now() + std::chrono::seconds(20);

  // Створюємо GPIO контролер. В контейнері він автоматично увімкне імітацію!
  GPIOController gpio;

  // START — на ніжці підняти вольтаж на 1 одразу на старті й тримати. (сигнал про готовність дрона)
  gpio.set_start(1);

  bool drop = false;

  // // Беремо абсолютний час старту симуляції
  // auto startTime = std::chrono::high_resolution_clock::now();

  // Головний цикл цього потоку (Балістичний калькулятор + Скид)
  while (running) {
    // Перевіряємо, чи отримали конфіг боєприпасу
    if (ammo == nullptr) {
      ammo = m_uartProcessor->getAmmoConfigPtr();
      if (ammo != nullptr) {
        std::cout << "[Ballistics] Отримано конфіг з UART! Name = " << ammo->nTargets << std::endl;
        std::cout << "[Ballistics] Розраховуємо час польоту снаряду і відстань що він проходить!" << std::endl;

        try {
          AmmoParams ammoParams;

          ammoParams.mass = ammo->mass;
          ammoParams.drag = ammo->drag;
          ammoParams.lift = ammo->lift;

          DEBUG("ammo->nTargets: " << ammo->nTargets);

          setNumberOfTarget(1);
          setHitRadius(3);

          // ініціалізація параметрів дрона і початкових параметрів руху
          init(myDroneConfig);

          // Поточний стан дрона
          curMyDrone = std::make_unique<Drone>(myDroneConfig, m_uartProcessor);

          int size = static_cast<int>(std::ceil(2.0 / curMyDrone->config.timeStep)) + 1;

          // Ініціалізуємо/перестворюємо об'єкт із потрібним size!
          targetStack = std::make_unique<RollingTargetStack>(size);

          distDuringFall = m_solver->getDistDuringFall(t_pol, myDroneConfig, &ammoParams);

          curMyDrone->start();  // Запускаємо потік фізики дрона
        }
        catch (const std::runtime_error& e) {
          throw std::runtime_error("[AnalyticalSolver] КРИТИЧНА ПОМИЛКА: " + std::string(e.what()));
        }

        if (distDuringFall <= 0) {
          throw std::runtime_error("Горизонтальна дистанція повинна бути додатня");
        }

        DEBUG("Горизонтальна дистанція яку проходить дрон за час " << t_pol << " сек. рівна " << distDuringFall << " м.");
        DEBUG("-----------------------------------");
      }
    }

    // якщо ми ще не скинули боєприпас і дрон ініціалізований ( тобто ми отримали конфіг дрона)
    // тоді розраховуємо точку скиду
    if (drop == false && curMyDrone != nullptr) {
      DroneTelemetry telemetry;
      if (m_uartProcessor->getTelemetry(telemetry)) {
        std::lock_guard<std::mutex> lock(proccessMutex);
        curMyDrone->setDroneParams(telemetry);
        DEBUG("--- curDrone X, Y = " << std::fixed << std::setprecision(8) << telemetry.pos.x << ", " << std::fixed << std::setprecision(8)
                                     << telemetry.pos.y << " м ---");
        // DEBUG("--- curDroneZ = " << std::fixed << std::setprecision(8) << telemetry.z << " м ---");
        DEBUG("--- curDroneT_ms = " << std::fixed << std::setprecision(8) << telemetry.t_ms << " мc ---");
        // DEBUG("--- curDrone_vx = " << std::fixed << std::setprecision(8) << telemetry.normSpeed.x << " м/c ---");
        // DEBUG("--- curDrone_vy = " << std::fixed << std::setprecision(8) << telemetry.normSpeed.y << " м/c ---");
        DEBUG("--- curDrone_speed = " << std::fixed << std::setprecision(8) << telemetry.speed << " мс/c ---");
        DEBUG("--- curDrone_dir = " << std::fixed << std::setprecision(8) << telemetry.angularState << " р. ---");
      }

      // ВІДПРАВКА ТЕЛЕМЕТРІЇ В MAVLINK

      if (m_mavlink) {
        m_mavlink->processTelemetry(telemetry.pos.x,
                                    telemetry.pos.y,
                                    telemetry.z,
                                    telemetry.normSpeed.x,
                                    telemetry.normSpeed.y,
                                    telemetry.angularState,
                                    static_cast<uint32_t>(telemetry.t_ms));
      }

      // ################## РОЗРАХУНОК ТОЧКИ СКИДУ #############################################
      // -----------  заповнення масивів для пошуку найближчих цілей ---------------------------
      std::optional<Target> targetPosition = fillArrays(canChangeTarget, *curMyDrone, distDuringFall, t_pol);

      if (!targetPosition.has_value()) {
        continue;
      }

      Coord droneDir = {(float)cos(curMyDrone->angularState), (float)sin(curMyDrone->angularState)};

      predictedTarget = predictTargetPosition(*targetStack, t_pol, curMyDrone->config.timeStep, target);

      DEBUG("--- targetPosition = " << targetPosition->pos.x << ", " << targetPosition->pos.y << " ---");
      DEBUG("--- targetVelocity = " << targetPosition->velocity.x << ", " << targetPosition->velocity.y << " ---");

      predictedTarget = targetPosition->pos + targetPosition->velocity * t_pol;

      DEBUG("--- predictedTarget: (" << predictedTarget.x << ", " << predictedTarget.y << ") ---");

      // точка скиду (куди летить дрон)
      // TODO  тут ще можна підкоригувати напрямок дрону маючи dirToDrone
      Coord dirToDrone = normalize(curMyDrone->pos - predictedTarget);
      dropPoint = predictedTarget - dirToDrone * (float)distDuringFall;

      // Знаходимо точку, куди прилетить боєприпас
      aimPoint = curMyDrone->pos + normalize(droneDir) * distDuringFall;
      // DEBUG("--- dropPoint: (" << curMyDrone->pos.x << ", " << curMyDrone->pos.y << ") ---");
      // DEBUG("--- aimPoint: (" << aimPoint.x << ", " << aimPoint.y << ") ---");

      // точний розрахунок коли наближаємся до вже запланованої цілі
      if (!canChangeTarget) {
        targetAngles[target] = atan2(predictedTarget.y - curMyDrone->pos.y, predictedTarget.x - curMyDrone->pos.x);
      }

      double finalDistance = calculateLength(aimPoint - predictedTarget);

      // умова при якій дрон попадає в ціль з точністю "curMyDrone.config.hitRadius / 1.3"
      if (curMyDrone->lastState->name() == "MOVING" && finalDistance <= curMyDrone->config.hitRadius / 1.3) {
        {
          std::lock_guard<std::mutex> lock(curMyDrone->getMutex());
          LOG("--- БОЄПРИПАС СКИНУТИЙ! Ураження : " << std::fixed << std::setprecision(2) << finalDistance << " м від цілі номер " << target
                                                    << " ---");
          DEBUG("--- myDrone.arrayTimeStep: " << std::setprecision(4) << curMyDrone->config.arrayTimeStep << " ---");
          DEBUG("--- dropPoint: (" << dropPoint.x << ", " << dropPoint.y << ") ---");
          DEBUG("--- aimPoint: (" << aimPoint.x << ", " << aimPoint.y << ") ---");
          DEBUG("--- predictedTarget: (" << predictedTarget.x << ", " << predictedTarget.y << ") ---");
        }

        gpio.pulse_drop();
        drop = true;
        std::cout << "[Ballistics] Команду DROP виконано!" << std::endl;
        continue;
      }

      // якщо в нас відстань між дроном і цілю почала збільшуватися
      // тоді запускаємо пошук цілі знову тому що дрон не вийшов на позицію
      if (calculateLength(curMyDrone->pos - predictedTarget) < distDuringFall) {
        LOG("Поточна ціль: " << target);
        LOG("canChangeTarget: true");
        canChangeTarget = true;
      }

      // провіряємо чи в нас є стан бо якщо ми відправили команду на зміну стану дрона то в нас вже немає стану бо ми його віддали в чергу
      // дрона і чекаємо нову телеметрію з новим станом дрона
      if (curMyDrone->state != nullptr) {
        DEBUG("Поточний стан дрона: " << curMyDrone->state->name());

        // зміна цілі і прорахунок руху дрона
        float targetAngle;

        std::unique_ptr<IDroneState> newState = changeTarget(targetAngle, canChangeTarget, *curMyDrone);

        // Створюємо команду
        DroneCommand cmd;
        cmd.targetAngle = targetAngle;  // Кут передаємо завжди, щоб дрон знав, куди тримати курс
        cmd.state = newState != nullptr ? std::move(newState) : nullptr;

        // Безповоротно віддаємо команду в чергу дрона
        curMyDrone->sendCommand(std::move(cmd));
      }
    }

    if (drop == true) {
      dlink::Result outResult;
      if (m_uartProcessor->getResult(outResult)) {
        LOG("--- Результат --- ");
        LOG("hit: " << std::to_string(outResult.hit));
        LOG("targetId: " << std::to_string(outResult.targetId));
        LOG("miss_m: " << std::fixed << std::setprecision(2) << outResult.miss_m);
        LOG("drop_t_ms: " << std::fixed << std::setprecision(2) << outResult.drop_t_ms);
        LOG("----------------- ");
        running = false;
        break;
      }
    }

    std::this_thread::sleep_for(std::chrono::nanoseconds(100000000));
  }

  curMyDrone->stop();  // Зупиняємо потік фізики дрона
  LOG("--- КІНЕЦЬ МІСІЇ ---");
}
