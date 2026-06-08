#include <iostream>
#include "functions.h"
#include "Drone.h"
#include "MissionProcessor.h"
#include "Debug.h"
#include "constants.h"
#include <iomanip>
#include <cmath>
#include "interfaces/IDroneState.h"

Drone MissionProcessor::init(DroneConfig& myDrone, const AmmoParams*& ammo, int& numberCounterInTimeSpot, int& numberOfTargets)
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

  float ratio = myDrone.arrayTimeStep / myDrone.simTimeStep;

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

  // Поточний стан дрона
  Drone curMyDrone(myDrone);

  // Дебаг інфо
  DEBUG("Час зупинки або прискорення: " << std::fixed << std::setprecision(2) << myDrone.timeAcceleration << " с");
  DEBUG("Прискорення дрона: " << myDrone.acceleration << " м/с2 ---");
  DEBUG("Величина оберту дрона за ітерацію: " << myDrone.radInIteration << " р/с ---");
  DEBUG("===========================");

  // Завантаження боєприпасу
  ammo = m_configLoader->getAmmoParameters(myDrone.ammoName);

  if (ammo == nullptr) {
    throw std::runtime_error("Помилка: боєприпас " + std::string(myDrone.ammoName) + " не знайдено в базі!");
  }

  DEBUG("Знайдено боєприпас: " << myDrone.ammoName);
  DEBUG("Параметри: mass: " << std::fixed << std::setprecision(3) << ammo->mass << ", drag: " << ammo->drag << ", lift: " << ammo->lift);

  return curMyDrone;
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

void MissionProcessor::addStep(const int counter, Drone& mDrone)
{
  steps[counter] = {
    {mDrone.pos.x, mDrone.pos.y},
    mDrone.angularState,
    mDrone.state->id(),
    mDrone.target,
    {mDrone.dropPoint.x, mDrone.dropPoint.y},
    {mDrone.aimPoint.x, mDrone.aimPoint.y},
    {mDrone.predictedTarget.x, mDrone.predictedTarget.y},
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
    Coord targetPos = m_targetProvider->getTargetPositionInCounter(targetId, counter);

    // DEBUG("targetPos.x: " << targetPos.x);
    // DEBUG("targetPos.y: " << targetPos.y);

    float length = calculateLength(targetPos.x, targetPos.y, curMyDrone.pos.x, curMyDrone.pos.y);

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
    if (t < myDroneConfig.arrayTimeStep && targetId == curMyDrone.target) {
      canChangeTarget = false;

      LOG("canChangeTarget: false");

      int timeIteration = m_targetProvider->getTimeIterationByCounter(counter);
      int nextIteration = m_targetProvider->getNextIteration(timeIteration);

      Coord targetNextPos = m_targetProvider->getTargetPositionInIteration(targetId, nextIteration);

      float Vxtarget = (targetNextPos.x - targetPos.x) / myDroneConfig.arrayTimeStep;
      float Vytarget = (targetNextPos.y - targetPos.y) / myDroneConfig.arrayTimeStep;

      // знайшли зміщення маючи час руху до цілі
      double targetXEndPoint = targetPos.x + (Vxtarget * t);
      double targetYEndPoint = targetPos.y + (Vytarget * t);

      // вирахували нову відстань
      float length = calculateLength(targetXEndPoint, targetYEndPoint, curMyDrone.pos.x, curMyDrone.pos.y);

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
      targetXEndPoint = targetPos.x + (Vxtarget * t);
      targetYEndPoint = targetPos.y + (Vytarget * t);

      // Кут, під яким дрон повинен летіти, щоб влучити в точку зустрічі
      // Ми врахували зміщення до цілі і тому перераховуємо кут нахилу дрона до цілі
      float targetAngle = atan2(targetYEndPoint - curMyDrone.pos.y, targetXEndPoint - curMyDrone.pos.x);

      //  записуємо тільки один раз кут зміщення це коли вже пряма наводка до цілі
      targetAngles[targetId] = targetAngle;
      // DEBUG("Перерахували кут напрямку для цілі: " << i);
    }

    if (targetId == curMyDrone.target) {
      DEBUG("До " << targetId << " цілі " << std::fixed << std::setprecision(2) << length << "; Кут у радіанах: " << std::setprecision(4)
                  << targetAngles[targetId] << " rad; Час польоту: " << std::setprecision(2) << t << " с");
    }

    targetTimes[targetId] = t;
  }
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
  Drone curMyDrone = init(myDroneConfig, ammo, numberCounterInTimeSpot, numberOfTargets);

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

  while (true) {
    // розраховуємо всі дані для визначення поточної найближчої цілі
    const int timeIteration = m_targetProvider->getTimeIterationByCounter(counter);

    DEBUG("--- counter = " << counter << " ---");
    DEBUG("--- timeIteration = " << timeIteration << " ---");
    DEBUG("--- curDroneX = " << std::fixed << std::setprecision(8) << curMyDrone.pos.x << " м ---");
    DEBUG("--- curDroneY = " << std::fixed << std::setprecision(8) << curMyDrone.pos.y << " м ---");
    DEBUG("--- curMyDrone.angularState = " << std::fixed << std::setprecision(2) << curMyDrone.angularState << " р. ---");
    DEBUG("--- curDroneSpeed = " << curMyDrone.speed << " ---");
    DEBUG("--- curDroneState = " << curMyDrone.state->name() << " ---");
    DEBUG("--- currentTarget = " << curMyDrone.target << " ---");

    // ################## РОЗРАХУНОК ТОЧКИ СКИДУ #############################################
    // -----------  заповнення масивів для пошуку найближчих цілей ---------------------------
    // std::vector<float> targetTimes;
    // std::vector<float> targetDistances;
    // std::vector<float> targetAngles;
    fillArrays(canChangeTarget, numberOfTargets, counter, curMyDrone, myDroneConfig, distDuringFall);

    // -----------  логіка розрахунку точки скиду ---------------------------
    // на скільки я знаю треба працювати без cos і sin тоу що це для процесора важкі операції
    // в майбутньому перепишу
    Coord droneDir = {(float)cos(curMyDrone.angularState), (float)sin(curMyDrone.angularState)};

    // мітка часу коли боєприпас долетить до землі якщо буде випущений в даний момент
    float Tt = counter * myDroneConfig.simTimeStep + t_pol;

    // ітерація координати (кожних myDrone.arrayTimeStep секунд нова координата)
    int futureIterationForTarget = m_targetProvider->getTimeIterationByTime(Tt, curMyDrone.config.arrayTimeStep);

    int nextFutureIterationForTarget = m_targetProvider->getNextIteration(futureIterationForTarget);

    // час що залишився
    float remainderTimeInSpot = std::fmod(Tt, myDroneConfig.arrayTimeStep);

    Coord targetPosIteration = m_targetProvider->getTargetPositionInIteration(curMyDrone.target, futureIterationForTarget);
    Coord targetPosNextIteration = m_targetProvider->getTargetPositionInIteration(curMyDrone.target, nextFutureIterationForTarget);

    Coord deltaTargetPos = targetPosNextIteration - targetPosIteration;

    // швидкість Vtarget це швидкість зміни координатів
    Coord Vtarget = deltaTargetPos / myDroneConfig.arrayTimeStep;

    // прогнозована позиція цілі
    curMyDrone.predictedTarget = targetPosIteration + Vtarget * remainderTimeInSpot;

    // точка скиду (куди летить дрон)
    // TODO  тут ще можна підкоригувати напрямок дрону маючи dirToDrone
    Coord dirToDrone = normalize(curMyDrone.pos - curMyDrone.predictedTarget);
    curMyDrone.dropPoint = curMyDrone.predictedTarget - dirToDrone * (float)distDuringFall;

    // Знаходимо точку, куди прилетить боєприпас
    curMyDrone.aimPoint = curMyDrone.pos + normalize(droneDir) * distDuringFall;

    addStep(counter, curMyDrone);

    // точний розрахунок коли наближаємся до вже запланованої цілі
    if (!canChangeTarget) {
      targetAngles[curMyDrone.target] =
        atan2(curMyDrone.predictedTarget.y - curMyDrone.pos.y, curMyDrone.predictedTarget.x - curMyDrone.pos.x);
    }

    double finalDistance = length(curMyDrone.aimPoint - curMyDrone.predictedTarget);

    // умова при якій дрон попадає в ціль з точністю "curMyDrone.config.hitRadius / 20"
    if (finalDistance <= curMyDrone.config.hitRadius / 10 ||
        (prevFinalDistance < finalDistance && !canChangeTarget && prevFinalDistance <= curMyDrone.config.hitRadius / 10)) {
      LOG("--- БОЄПРИПАС СКИНУТИЙ! Ураження : " << std::fixed << std::setprecision(2) << finalDistance << " м від цілі номер "
                                                << curMyDrone.target << " ---");
      DEBUG("--- prevFinalDistance: " << std::setprecision(4) << prevFinalDistance << " м.  ---");
      DEBUG("--- Tt: " << std::setprecision(4) << Tt << " ---");
      DEBUG("--- myDrone.arrayTimeStep: " << std::setprecision(4) << myDroneConfig.arrayTimeStep << " ---");
      DEBUG("--- remainderTimeInSpot: " << std::setprecision(4) << remainderTimeInSpot << " ---");
      DEBUG("--- dropPoint: (" << curMyDrone.pos.x << ", " << curMyDrone.pos.y << ") ---");
      DEBUG("--- aimPoint: (" << curMyDrone.aimPoint.x << ", " << curMyDrone.aimPoint.y << ") ---");
      DEBUG("--- predictedTarget: (" << curMyDrone.predictedTarget.x << ", " << curMyDrone.predictedTarget.y << ") ---");

      break;
    }

    prevFinalDistance = finalDistance;

    // якщо в нас відстань між дроном і цілю почала збільшуватися
    // тоді запускаємо пошук цілі знову тому що дрон не вийшов на позицію
    if (length(curMyDrone.pos - curMyDrone.predictedTarget) < distDuringFall) {
      LOG("Поточна ціль: " << curMyDrone.target);
      LOG("canChangeTarget: true");
      canChangeTarget = true;
    }

    counter++;

    if (counter > MAX_STEPS) {
      LOG("============== спрацював ліміт ітерацій  ==============");
      break;
    }

    // зміна цілі і прорахунок руху дрона
    curMyDrone.move(targetTimes, canChangeTarget, targetAngles);
  }

  if (counter <= MAX_STEPS) {
    saveOutputFileByStep(counter + 1, steps);
    LOG("--- МІСІЮ ЗАВЕРШЕНО ---");
  }
}
