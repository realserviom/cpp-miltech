#include <iostream>
#include "functions.h"
#include "Drone.h"
#include "MissionProcessor.h"
#include "Debug.h"
#include "constants.h"
#include <iomanip>
#include <cmath>
#include "interfaces/IDroneState.h"
#include "states/StateDecelerating.h"

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

void MissionProcessor::executeMission()
{
  if (!m_targetProvider || !m_solver || !m_configLoader) {
    std::cout << "[MissionProcessor] Помилка: Не всі компоненти підключені!\n";
    return;
  }

  LOG("--- ПОЧАТОК МІСІЇ ---");

  DroneConfig myDrone;

  int numberCounterInTimeSpot = 0;
  int numberOfTargets = 0;
  const AmmoParams* ammo = nullptr;

  // ініціалізація параметрів дрона і початкових параметрів руху
  Drone curMyDrone = init(myDrone, ammo, numberCounterInTimeSpot, numberOfTargets);

  int counter = 0;                        // лічильник часу
  bool canChangeTarget = true;            // мітка чи є дозвіл міняти ціль
  std::vector<SimStep> steps(MAX_STEPS);  // Масив кроків для симуляції

  float t_pol;               // час польоту
  float distDuringFall = 0;  // дистанція падіння
  float prevFinalDistance = 10000000;  // відхилення координат падіння боєприпаса від цілі в попередньому кроці

  try {
    distDuringFall = m_solver->getDistDuringFall(t_pol, myDrone, ammo);
  }
  catch (const std::runtime_error& e) {
    throw std::runtime_error("[AnalyticalSolver] КРИТИЧНА ПОМИЛКА: " + std::string(e.what()));
  }

  DEBUG("Горизонтальна дистанція яку проходить дрон за час " << t_pol << " сек. рівна " << distDuringFall << " м.");
  DEBUG("-----------------------------------");

  if (distDuringFall <= 0) {
    throw std::runtime_error("Горизонтальна дистанція повинна бути додатня");
  }

  bool addingStep = false;

  while (true) {
    // запис даних в масиви з яких будемо формувати вихідний файл
    // при першій ітерації не записуємо точку в масив
    if (addingStep) {
      steps[counter - 1] = {
        {curMyDrone.pos.x, curMyDrone.pos.y},
        curMyDrone.angularState,
        curMyDrone.state->id(),
        curMyDrone.target,
        {curMyDrone.dropPoint.x, curMyDrone.dropPoint.y},
        {curMyDrone.aimPoint.x, curMyDrone.aimPoint.y},
        {curMyDrone.predictedTarget.x, curMyDrone.predictedTarget.y},
      };
    }

    // розраховуємо всі дані для визначення поточної найближчої цілі
    const int timeIteration = m_targetProvider->getTimeIteration(counter);

    // DEBUG("--- counter = " << counter << " ---");
    // DEBUG("--- timeIteration = " << timeIteration << " ---");
    // DEBUG("--- curDroneX = " << std::fixed << std::setprecision(8) << curMyDrone.pos.x << " м ---");
    // DEBUG("--- curDroneY = " << std::fixed << std::setprecision(8) << curMyDrone.pos.y << " м ---");
    // DEBUG("--- curMyDrone.angularState = " << std::fixed << std::setprecision(2) << curMyDrone.angularState << " р. ---");
    // DEBUG("--- curDroneSpeed = " << curMyDrone.speed << " ---");
    // DEBUG("--- curDroneState = " << curMyDrone.state->name() << " ---");
    // DEBUG("--- currentTarget = " << curMyDrone.target << " ---");

    if (canChangeTarget) {
      for (int i = 0; i < numberOfTargets; i++) {
        Coord targetPos = m_targetProvider->getTargetPositionInCounter(i, counter);

        // DEBUG("targetPos.x: " << targetPos.x);
        // DEBUG("targetPos.y: " << targetPos.y);

        float length = calculateLength(targetPos.x, targetPos.y, curMyDrone.pos.x, curMyDrone.pos.y);

        float deltaX = targetPos.x - curMyDrone.pos.x;
        float deltaY = targetPos.y - curMyDrone.pos.y;

        // Функція atan2 повертає результат у радіанах
        // це кут цілі відносно положення дрона
        float angle_in_rad = atan2(deltaY, deltaX);

        targetAngles[i] = angle_in_rad;

        // час за який дрон долетить до цілі з вичитанням шляху падіння боєприпасу а також шляхом на розгін
        float t = curMyDrone.calculateArrivalTime(targetAngles[i], length, distDuringFall);

        // якщо ми митєво долітаємо до цілі в межах наступної часової ітерації по координаті a це arrayTimeStep секунд
        // тоді враховуємо відхилення цілі за час дольоту до неї
        // іноді ціль може рухатися до дрону тоді підльот буде тривати ще менше часу
        // час не враховує те що коли боєприпас скинутий ціль ще зміщується
        // в цьому випадку ми вже ціль не можемо поміняти тому що недоцільно зупиняти і набирати знову швидкість
        // Хоча при умові що пороговий кут в межаш похибки тоді можна міняти але з іншої сторони ми можемо перепригувати
        // із цілі на ціль що дасть велику похибку (цікаво як роблять виробники ПЗ для ППО ?)
        if (t < myDrone.arrayTimeStep && i == curMyDrone.target) {
          canChangeTarget = false;

          LOG("canChangeTarget: false");

          int timeIteration = m_targetProvider->getTimeIteration(counter);
          int nextIteration = m_targetProvider->getNextIteration(timeIteration);

          Coord targetNextPos = m_targetProvider->getTargetPositionInIteration(i, nextIteration);

          float Vxtarget = (targetNextPos.x - targetPos.x) / myDrone.arrayTimeStep;
          float Vytarget = (targetNextPos.y - targetPos.y) / myDrone.arrayTimeStep;

          // знайшли зміщення маючи час руху до цілі
          double targetXEndPoint = targetPos.x + (Vxtarget * t);
          double targetYEndPoint = targetPos.y + (Vytarget * t);

          // вирахували нову відстань
          float length = calculateLength(targetXEndPoint, targetYEndPoint, curMyDrone.pos.x, curMyDrone.pos.y);

          targetDistances[i] = length;

          // тут ми маємо визначити час протягом якого дрон досягне цілі
          // враховуючи стан дрона (статус), координати, швидкість дрона, час падіння боєприпасу
          // на даний момент ми нехтуємо кутом повороту будемо повертати під час польоту
          // а з іншої сторони в цей момент дрон вже має бути направлений на ціль
          float t_new = curMyDrone.calculateSmallArrivalTime(length - distDuringFall > 0 ? length - distDuringFall : length);

          if (std::abs(t_new - t) < myDrone.timeHitRadius) {
            // DEBUG("std::abs(t_new - t): " << t_new);
            t = t_new;
          }
          else if (t_new < t) {
            t = (t + t_new) / 2;
            // DEBUG("t = (t + t_new) / 2: " << t);
          }
          else {
            t = t_new;
            // DEBUG("t = t_new: " << t_new);
          }

          // знайшли зміщення маючи час руху до цілі t_new
          targetXEndPoint = targetPos.x + (Vxtarget * t);
          targetYEndPoint = targetPos.y + (Vytarget * t);

          // Кут, під яким дрон повинен летіти, щоб влучити в точку зустрічі
          // Ми врахували зміщення до цілі і тому перераховуємо кут нахилу дрона до цілі
          float targetAngle = atan2(targetYEndPoint - curMyDrone.pos.y, targetXEndPoint - curMyDrone.pos.x);

          //  записуємо тільки один раз кут зміщення це коли вже пряма наводка до цілі
          targetAngles[i] = targetAngle;
          // DEBUG("Перерахували кут напрямку для цілі: " << i);
        }

        if (i == curMyDrone.target) {
          DEBUG("До " << i << " цілі " << std::fixed << std::setprecision(2) << length << "; Кут у радіанах: " << std::setprecision(4)
                      << targetAngles[i] << " rad; Час польоту: " << std::setprecision(2) << t << " с");
        }

        // DEBUG("targetPos.x: " << targetPos.x << " targetPos.y: " << targetPos.y << " curMyDrone.pos.x: " << curMyDrone.pos.x
        //                       << " curMyDrone.pos.y: " << curMyDrone.pos.y);

        targetTimes[i] = t;
      }
    }

    // визначаємо ціль за мінімальним часом підльоту

    int newTarget = getIndexByMinValue(targetTimes);

    // Умова при якій програма находить точку скиду
    // Координа падіння боєприпасу дрона через t_pol секунд
    // Але t_pol дрона в нас досягається при досягані швидкості атаки якщо швидкість менша тоді треба перераховувати
    // В цій задачі ми не будемо перераховувати цей час. Будемо перелітати і йти на друге коло польоту на ціль
    // якщо відстані не співпадуть

    // на скільки я знаю треба працювати без cos і sin тоу що це для процесора важкі операції
    // в майбутньому перепишу
    Coord droneDir = {(float)cos(curMyDrone.angularState), (float)sin(curMyDrone.angularState)};

    // мітка часу коли боєприпас долетить до землі якщо буде випущений в даний момент
    float Tt = counter * myDrone.simTimeStep + t_pol;

    // ітерація координати (кожних myDrone.arrayTimeStep секунд нова координата)
    int futureIteration = std::floor(Tt / myDrone.arrayTimeStep);
    int nextFutureIteration = m_targetProvider->getNextIteration(futureIteration);

    // час що залишився
    float remainderTimeInSpot = std::fmod(Tt, myDrone.arrayTimeStep);

    Coord dataPosFutureIteration = m_targetProvider->getTargetPositionInIteration(newTarget, futureIteration);
    Coord dataPosNextFutureIteration = m_targetProvider->getTargetPositionInIteration(newTarget, nextFutureIteration);

    Coord deltaPos = dataPosNextFutureIteration - dataPosFutureIteration;

    // швидкість Vtarget це швидкість зміни координатів
    Coord Vtarget = deltaPos / myDrone.arrayTimeStep;

    // прогнозована позиція цілі
    curMyDrone.predictedTarget = dataPosFutureIteration + Vtarget * remainderTimeInSpot;

    // точка скиду (куди летить дрон)
    // TODO  тут ще можна підкоригувати напрямок дрону маючи dirToDrone
    Coord dirToDrone = normalize(curMyDrone.pos - curMyDrone.predictedTarget);
    curMyDrone.dropPoint = curMyDrone.predictedTarget - dirToDrone * (float)distDuringFall;

    // Знаходимо точку, куди прилетить боєприпас
    curMyDrone.aimPoint = curMyDrone.pos + normalize(droneDir) * distDuringFall;

    // точний розрахунок коли наближаємся до вже запланованої цілі
    if (!canChangeTarget) {
      targetAngles[newTarget] = atan2(curMyDrone.predictedTarget.y - curMyDrone.pos.y, curMyDrone.predictedTarget.x - curMyDrone.pos.x);
    }

    Coord delta = curMyDrone.aimPoint - curMyDrone.predictedTarget;

    double finalDistance = length(delta);

    if (finalDistance <= 0.2 || (prevFinalDistance < finalDistance && !canChangeTarget)) {
      LOG("--- БОЄПРИПАС СКИНУТИЙ! Ураження : " << std::fixed << std::setprecision(2) << finalDistance << " м від цілі номер " << newTarget
                                                << " ---");
      DEBUG("--- prevFinalDistance: " << std::setprecision(4) << prevFinalDistance << " м.  ---");
      DEBUG("--- Tt: " << std::setprecision(4) << Tt << " ---");
      DEBUG("--- myDrone.arrayTimeStep: " << std::setprecision(4) << myDrone.arrayTimeStep << " ---");
      DEBUG("--- remainderTimeInSpot: " << std::setprecision(4) << remainderTimeInSpot << " ---");
      DEBUG("--- dropPoint: (" << curMyDrone.pos.x << ", " << curMyDrone.pos.y << ") ---");
      DEBUG("--- aimPoint: (" << curMyDrone.aimPoint.x << ", " << curMyDrone.aimPoint.y << ") ---");
      DEBUG("--- predictedTarget: (" << curMyDrone.predictedTarget.x << ", " << curMyDrone.predictedTarget.y << ") ---");

      break;
    }

    prevFinalDistance = finalDistance;

    // якщо в нас відстань між дроном і цілю почала збільшуватися
    // тоді включаємо пошук цілі знову тому що дрон не вийшов на позицію
    if (length(curMyDrone.pos - curMyDrone.predictedTarget) < distDuringFall) {
      // DEBUG("length(curMyDrone.pos - targetEndPoint): " << length(curMyDrone.pos - targetEndPoint));
      // DEBUG("distDuringFall: " << distDuringFall);
      LOG("Поточна ціль: " << curMyDrone.target);
      LOG("canChangeTarget: true");
      canChangeTarget = true;
    }

    curMyDrone.move(newTarget, canChangeTarget, targetAngles[newTarget]);

    addingStep = true;

    counter++;

    if (counter > MAX_STEPS) {
      LOG("============== спрацював ліміт ітерацій  ==============");
      break;
    }
  }

  if (counter <= MAX_STEPS) {
    saveOutputFileByStep(counter - 1, steps);
    LOG("--- МІСІЮ ЗАВЕРШЕНО ---");
  }
}
