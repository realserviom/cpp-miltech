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

void MissionProcessor::init(DroneConfig& myDrone, const AmmoParams*& ammo, int& numberCounterInTimeSpot, int& numberOfTargets)
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

  numberCounterInTimeSpot = static_cast<int>(ratio);

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
  DEBUG("Величина оберту дрона за ітерацію: " << myDrone.radInIteration << " р/с ---");
  DEBUG("===========================");
}

void MissionProcessor::setTargetProvider(std::unique_ptr<ITargetProvider> targetProvider)
{
  m_targetProvider = std::move(targetProvider);
}

void MissionProcessor::setBallisticSolver(std::unique_ptr<IBallisticSolver> solver)
{
  m_solver = std::move(solver);
}

void MissionProcessor::setConfigLoader(std::unique_ptr<IConfigLoader> configLoader)
{
  m_configLoader = std::move(configLoader);
}

void MissionProcessor::addStep(const int counter, DroneTelemetry& telemetry)
{
  steps[counter] = {
    {telemetry.pos.x, telemetry.pos.y},
    telemetry.angularState,
    telemetry.stateId,
    target,
    {dropPoint.x, dropPoint.y},
    {aimPoint.x, aimPoint.y},
    {predictedTarget.x, predictedTarget.y},
  };
}

void MissionProcessor::fillArrays(bool& canChangeTarget,
                                  const int& numberOfTargets,
                                  const int& counter,
                                  const Drone& curMyDrone,
                                  const DroneConfig& myDroneConfig,
                                  const float& distDuringFall)
{
  for (int targetId = 0; targetId < numberOfTargets; targetId++) {
    Coord targetPos = m_targetProvider->getTargetPosition(targetId, counter * curMyDrone.config.timeStep);

    float length = calculateLength(targetPos - curMyDrone.pos);

    float deltaX = targetPos.x - curMyDrone.pos.x;
    float deltaY = targetPos.y - curMyDrone.pos.y;

    // Функція atan2 повертає результат у радіанах
    // це кут цілі відносно положення дрона
    float angle_in_rad = atan2(deltaY, deltaX);

    targetAngles[targetId] = angle_in_rad;

    // час за який дрон долетить до цілі з вичитанням шляху падіння боєприпасу а також шляхом на розгін
    float t = curMyDrone.calculateArrivalTime(targetAngles[targetId], length, distDuringFall);

    // якщо ми митєво долітаємо до цілі в межах наступної часової ітерації по координаті a це arrayTimeStep секунд
    // тоді враховуємо відхилення цілі за час дольоту до неї
    // іноді ціль може рухатися до дрону тоді підльот буде тривати ще менше часу
    // час не враховує те що коли боєприпас скинутий ціль ще зміщується
    // в цьому випадку ми вже ціль не можемо поміняти тому що недоцільно зупиняти і набирати знову швидкість
    // Хоча при умові що пороговий кут в межаш похибки тоді можна міняти але з іншої сторони ми можемо перепригувати
    // із цілі на ціль що дасть велику похибку (цікаво як роблять виробники ПЗ для ППО ?)
    if (t < myDroneConfig.arrayTimeStep && targetId == target) {
      canChangeTarget = false;
      // LOG("canChangeTarget: false");
      Coord targetEndPoint = m_targetProvider->getTargetPosition(targetId, t + counter * myDroneConfig.arrayTimeStep);

      // вирахували нову відстань
      float length = calculateLength(targetEndPoint - curMyDrone.pos);
      targetDistances[targetId] = length;

      // тут ми маємо визначити час протягом якого дрон досягне цілі
      // враховуючи стан дрона (статус), координати, швидкість дрона, час падіння боєприпасу
      // на даний момент ми нехтуємо кутом повороту будемо повертати під час польоту
      // а з іншої сторони в цей момент дрон вже має бути направлений на ціль
      float t_new = curMyDrone.calculateSmallArrivalTime(length - distDuringFall > 0 ? length - distDuringFall : length);

      if (std::abs(t_new - t) < myDroneConfig.timeHitRadius) {
        t = t_new;
      }
      else if (t_new < t) {
        t = (t + t_new) / 2;
      }
      else {
        t = t_new;
      }

      // знайшли зміщення маючи час руху до цілі t_new

      Coord targetEndPoint2 = m_targetProvider->getTargetPosition(targetId, t + counter * myDroneConfig.arrayTimeStep);

      // Кут, під яким дрон повинен летіти, щоб влучити в точку зустрічі
      // Ми врахували зміщення до цілі і тому перераховуємо кут нахилу дрона до цілі
      float targetAngle = atan2(targetEndPoint2.y - curMyDrone.pos.y, targetEndPoint2.x - curMyDrone.pos.x);

      //  записуємо тільки один раз кут зміщення це коли вже пряма наводка до цілі
      targetAngles[targetId] = targetAngle;
      // DEBUG("Перерахували кут напрямку для цілі: " << i);
    }

    if (targetId == target) {
      // DEBUG("До " << targetId << " цілі " << std::fixed << std::setprecision(2) << length << "; Кут у радіанах: " << std::setprecision(4)
      //             << targetAngles[targetId] << " rad; Час польоту: " << std::setprecision(2) << t << " с");
    }

    targetTimes[targetId] = t;
  }
}

std::unique_ptr<IDroneState> MissionProcessor::changeTarget(float& targetAngle, const bool& canChangeTarget, Drone& curMyDrone)
{
  std::string currentStateName = curMyDrone.state->name();

  const float newTarget = getIndexByMinValue(targetTimes);

  targetAngle = targetAngles[target];

  if (currentStateName != "STOPPED" && currentStateName != "DECELERATING" && canChangeTarget && newTarget != target) {
    DEBUG("Нова ціль: " << newTarget);
    target = newTarget;
    targetAngle = targetAngles[newTarget];

    // Якщо для нової цілі треба сильно розвернутися, а ми летимо на всіх парах або прискорюємося - треба гальмувати
    if (curMyDrone.needRotation(targetAngle, curMyDrone.config.turnThreshold)) {
      if (currentStateName == "MOVING" || currentStateName == "ACCELERATING") {
        DEBUG("--- Сповільнюємося!!!! Треба повертатися! ---");
        return std::make_unique<StateDecelerating>();
      }
    }
  }

  return nullptr;
}

void MissionProcessor::executeMission()
{
  if (!m_targetProvider || !m_solver || !m_configLoader) {
    std::cout << "[MissionProcessor] Помилка: Не всі компоненти підключені!\n";
    return;
  }

  LOG("--- ПОЧАТОК МІСІЇ ---");

  DroneConfig myDroneConfig;

  int numberCounterInTimeSpot = 0;
  int numberOfTargets = 0;
  const AmmoParams* ammo = nullptr;

  // ініціалізація параметрів дрона і початкових параметрів руху
  init(myDroneConfig, ammo, numberCounterInTimeSpot, numberOfTargets);

  // Поточний стан дрона
  Drone curMyDrone(myDroneConfig);
  // Завантаження боєприпасу
  ammo = m_configLoader->getAmmoParameters(myDroneConfig.ammoName);

  if (ammo == nullptr) {
    throw std::runtime_error("Помилка: боєприпас " + std::string(myDroneConfig.ammoName) + " не знайдено в базі!");
  }

  DEBUG("Знайдено боєприпас: " << myDroneConfig.ammoName);
  DEBUG("Параметри: mass: " << std::fixed << std::setprecision(3) << ammo->mass << ", drag: " << ammo->drag << ", lift: " << ammo->lift);

  curMyDrone.start();

  while (!curMyDrone.isThreadReady()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  m_targetProvider->setArrayTimeStep(myDroneConfig.arrayTimeStep);

  int counter = 0;              // лічильник часу
  bool canChangeTarget = true;  // мітка чи є дозвіл міняти ціль
  // std::vector<SimStep> steps(MAX_STEPS);  // Масив кроків для симуляції

  float t_pol;               // час польоту
  float distDuringFall = 0;  // дистанція падіння
  float prevFinalDistance = 10000000;  // відхилення координат падіння боєприпаса від цілі в попередньому кроці

  try {
    distDuringFall = m_solver->getDistDuringFall(t_pol, myDroneConfig, ammo);
  }
  catch (const std::runtime_error& e) {
    throw std::runtime_error("[AnalyticalSolver] КРИТИЧНА ПОМИЛКА: " + std::string(e.what()));
  }

  DEBUG("Горизонтальна дистанція яку проходить дрон за час " << t_pol << " сек. рівна " << distDuringFall << " м.");
  DEBUG("-----------------------------------");

  if (distDuringFall <= 0) {
    throw std::runtime_error("Горизонтальна дистанція повинна бути додатня");
  }

  // Беремо абсолютний час старту симуляції
  auto startTime = std::chrono::high_resolution_clock::now();

  while (true) {
    // розраховуємо всі дані для визначення поточної найближчої цілі
    DroneTelemetry telemetry = curMyDrone.getTelemetry();

    DEBUG("--- counter = " << counter << " ---");
    DEBUG("--- curDroneX = " << std::fixed << std::setprecision(8) << telemetry.pos.x << " м ---");
    DEBUG("--- curDroneY = " << std::fixed << std::setprecision(8) << telemetry.pos.y << " м ---");
    DEBUG("--- curMyDrone.angularState = " << std::fixed << std::setprecision(2) << telemetry.angularState << " р. ---");
    DEBUG("--- curDroneSpeed = " << telemetry.speed << " ---");
    DEBUG("--- curDroneStateName = " << telemetry.stateName << " ---");
    DEBUG("--- currentTarget = " << target << " --cd -");

    // ################## РОЗРАХУНОК ТОЧКИ СКИДУ #############################################
    // -----------  заповнення масивів для пошуку найближчих цілей ---------------------------
    fillArrays(canChangeTarget, numberOfTargets, counter, curMyDrone, myDroneConfig, distDuringFall);

    // -----------  логіка розрахунку точки скиду ---------------------------
    // на скільки я знаю треба працювати без cos і sin тоу що це для процесора важкі операції
    // в майбутньому перепишу
    Coord droneDir = {(float)cos(telemetry.angularState), (float)sin(telemetry.angularState)};

    // мітка часу в таблиці targets для визначення майбутньої позиції цілі
    // ми взяли весь час що пройшов + час коли боєприпас долетить до землі якщо буде випущений в даний момент
    float Tt = counter * myDroneConfig.timeStep + t_pol;
    predictedTarget = m_targetProvider->getTargetPosition(target, Tt);

    // точка скиду (куди летить дрон)
    // TODO  тут ще можна підкоригувати напрямок дрону маючи dirToDrone
    Coord dirToDrone = normalize(telemetry.pos - predictedTarget);
    dropPoint = predictedTarget - dirToDrone * (float)distDuringFall;

    // Знаходимо точку, куди прилетить боєприпас
    aimPoint = telemetry.pos + normalize(droneDir) * distDuringFall;

    addStep(counter, telemetry);

    // точний розрахунок коли наближаємся до вже запланованої цілі
    if (!canChangeTarget) {
      targetAngles[target] = atan2(predictedTarget.y - telemetry.pos.y, predictedTarget.x - telemetry.pos.x);
    }

    double finalDistance = calculateLength(aimPoint - predictedTarget);

    // умова при якій дрон попадає в ціль з точністю "curMyDrone.config.hitRadius / 20"
    if (finalDistance <= myDroneConfig.hitRadius / 10 ||
        (prevFinalDistance < finalDistance && !canChangeTarget && prevFinalDistance <= myDroneConfig.hitRadius / 20)) {
      LOG("--- БОЄПРИПАС СКИНУТИЙ! Ураження : " << std::fixed << std::setprecision(2) << finalDistance << " м від цілі номер " << target
                                                << " ---");
      DEBUG("--- prevFinalDistance: " << std::setprecision(4) << prevFinalDistance << " м.  ---");
      DEBUG("--- Tt: " << std::setprecision(4) << Tt << " ---");
      DEBUG("--- myDrone.arrayTimeStep: " << std::setprecision(4) << myDroneConfig.arrayTimeStep << " ---");
      DEBUG("--- dropPoint: (" << telemetry.pos.x << ", " << telemetry.pos.y << ") ---");
      DEBUG("--- aimPoint: (" << aimPoint.x << ", " << aimPoint.y << ") ---");
      DEBUG("--- predictedTarget: (" << predictedTarget.x << ", " << predictedTarget.y << ") ---");

      break;
    }

    prevFinalDistance = finalDistance;

    // якщо в нас відстань між дроном і цілю почала збільшуватися
    // тоді запускаємо пошук цілі знову тому що дрон не вийшов на позицію
    if (calculateLength(telemetry.pos - predictedTarget) < distDuringFall) {
      LOG("Поточна ціль: " << target);
      LOG("canChangeTarget: true");
      canChangeTarget = true;
    }

    counter++;

    if (counter > MAX_STEPS) {
      LOG("============== спрацював ліміт ітерацій  ==============");
      break;
    }

    // зміна цілі і прорахунок руху дрона
    float targetAngle;

    std::unique_ptr<IDroneState> newState = changeTarget(targetAngle, canChangeTarget, curMyDrone);

    // Створюємо команду
    DroneCommand cmd;
    cmd.targetAngle = targetAngle;  // Кут передаємо завжди, щоб дрон знав, куди тримати курс

    cmd.state = newState != nullptr ? std::move(newState) : nullptr;

    // Безповоротно віддаємо команду в чергу дрона
    curMyDrone.sendCommand(std::move(cmd));

    auto nextTimePoint = getNextTimePoint(startTime, (curMyDrone.config.timeStep / curMyDrone.config.timeScale), counter);
    std::this_thread::sleep_until(nextTimePoint);
  }

  curMyDrone.stop();  // Зупиняє physicsThread через join()

  if (counter <= MAX_STEPS) {
    saveOutputFileByStep(counter + 1, steps);
    LOG("--- МІСІЮ ЗАВЕРШЕНО!!! ---");
  }
}
