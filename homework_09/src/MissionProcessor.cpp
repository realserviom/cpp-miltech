#include <iostream>
#include "GPIOController.h"
#include "Types.h"
#include "UARTProcessor.h"
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

void MissionProcessor::init(DroneConfig& myDrone, const int numberOfTargets)
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

void MissionProcessor::addStep(const int counter, DroneTelemetry& telemetry)
{
  steps.push_back({{telemetry.pos.x, telemetry.pos.y},
                   telemetry.angularState,
                   telemetry.stateId,
                   target,
                   {dropPoint.x, dropPoint.y},
                   {aimPoint.x, aimPoint.y},
                   {predictedTarget.x, predictedTarget.y},
                   telemetry.timeSecSinceStart});
}

void MissionProcessor::fillArrays(
  bool& canChangeTarget, const int& counter, const Drone& curMyDrone, const float distDuringFall, const float t_pol)
{
  for (int targetId = 0; targetId < numberOfTargets; targetId++) {
    Target targetOptions = m_targetProvider->getTargetPosition(targetId);

    Coord targetPos = targetOptions.pos;

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
    if (t < (t_pol + 1) && targetId == target) {
      canChangeTarget = false;
      // LOG("canChangeTarget: false");

      Coord velocity = targetOptions.velocity;

      // прогнозована позиція цілі
      Coord targetEndPoint = targetPos + velocity * t;

      // вирахували нову відстань
      float length = calculateLength(targetEndPoint - curMyDrone.pos);
      targetDistances[targetId] = length;

      // тут ми маємо визначити час протягом якого дрон досягне цілі
      // враховуючи стан дрона (статус), координати, швидкість дрона, час падіння боєприпасу
      // на даний момент ми нехтуємо кутом повороту будемо повертати під час польоту
      // а з іншої сторони в цей момент дрон вже має бути направлений на ціль
      float t_new = curMyDrone.calculateSmallArrivalTime(length - distDuringFall > 0 ? length - distDuringFall : length);

      if (std::abs(t_new - t) < curMyDrone.config.timeHitRadius) {
        t = t_new;
      }
      else if (t_new < t) {
        t = (t + t_new) / 2;
      }
      else {
        t = t_new;
      }

      // знайшли зміщення маючи новий час руху до цілі t
      Coord targetEndPoint2 = targetPos + velocity * t;

      // Кут, під яким дрон повинен летіти, щоб влучити в точку зустрічі
      // Ми врахували зміщення до цілі і тому перераховуємо кут нахилу дрона до цілі
      float targetAngle = atan2(targetEndPoint2.y - curMyDrone.pos.y, targetEndPoint2.x - curMyDrone.pos.x);

      //  записуємо тільки один раз кут зміщення це коли вже пряма наводка до цілі
      targetAngles[targetId] = targetAngle;
      DEBUG("Перерахували кут напрямку для цілі: " << targetId);
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

bool MissionProcessor::isThreadReady() const
{
  return isReady;
}

void MissionProcessor::start(Drone& curMyDrone)
{
  DEBUG("--- start mission thread! ---");
  running = true;
  missionThread = std::thread(&MissionProcessor::missionLoop, this, std::ref(curMyDrone));
}

void MissionProcessor::missionLoop(Drone& curMyDrone)
{
  DroneConfig myDroneConfig;

  // ініціалізація параметрів дрона і початкових параметрів руху

  // Створюємо GPIO контролер. В контейнері він автоматично увімкне імітацію!
  GPIOController gpio;

  std::cout << "Запуск головного циклу опитування..." << std::endl;

  // START — на ніжці підняти вольтаж на 1 одразу на старті й тримати. (сигнал про готовність дрона)
  gpio.set_start(1);

  // Локальні змінні для потоку балістики/main
  const dlink::AmmoCfg* ammo = nullptr;
  std::unique_ptr<Drone> curMyDrone = nullptr;

  float t_pol;               // час польоту снаряду
  float distDuringFall = 0;  // дистанція, яку проходить снаряд за час t_pol (горизонтальна відстань від точки скидання до цілі)

  dlink::Telemetry droneData;

  bool already_dropped = false;
  auto nextTimePoint = std::chrono::high_resolution_clock::now() + std::chrono::seconds(20);

  std::cout << "[Main/Ballistics] Потік балістики запущено!" << std::endl;

  // Головний цикл цього потоку (Балістичний калькулятор + Скид)
  while (true) {
    // Перевіряємо, чи отримали конфіг боєприпасу
    if (ammo == nullptr) {
      ammo = m_uartProcessor->getAmmoConfigPtr();
      if (ammo != nullptr) {
        std::cout << "[Ballistics] Отримано конфіг з UART! Name = " << ammo->name << std::endl;
        std::cout << "[Ballistics] Розраховуємо час польоту снаряду і відстань що він проходить!" << std::endl;

        try {
          AmmoParams ammoParams;

          ammoParams.mass = ammo->mass;
          ammoParams.drag = ammo->drag;
          ammoParams.lift = ammo->lift;

          init(myDroneConfig, ammo->nTargets);

          // Поточний стан дрона

          curMyDrone = std::make_unique<Drone>(myDroneConfig);

          distDuringFall = m_solver->getDistDuringFall(t_pol, myDroneConfig, &ammoParams);
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

    if (curMyDrone != nullptr) {
    }

    //  Забираємо свіжу телеметрію та координати цілі, якщо вони є
    bool hasFreshData = uartProcessor->getTelemetry(droneData);

    if (hasFreshData && ammo != nullptr) {
      // ТУТ ПРАЦЮЄ ВАШ БАЛІСТИЧНИЙ КАЛЬКУЛЯТОР
      // Він використовує: droneData (де зараз дрон), targetData (де ціль) та ammo (вага/опір)

      // Наприклад, калькулятор вирахував, що пора міняти nextTimePoint або скидати прямо зараз
    }

    // КРОК C: Логіка виконання скиду
    auto currentTime = std::chrono::high_resolution_clock::now();
    if (!already_dropped && currentTime >= nextTimePoint) {
      // Викликаємо GPIO з цього потоку! Потік UART при цьому не блокується на 80 мс
      gpio.pulse_drop();
      already_dropped = true;
      std::cout << "[Ballistics] Команду DROP виконано!" << std::endl;
    }

    // Крок балістичного циклу (наприклад, 100 Гц або 1000 Гц)
    usleep(10000);  // 10 мс
  }

  uartProcessor->start();
  return 0;

  processor.start(curMyDrone, distDuringFall, t_pol);

  while (!curMyDrone.isThreadReady() || !uartProcessor->isThreadReady() || !processor.isThreadReady()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  processor.missionThread.join();

  curMyDrone.stop();
  uartProcessor->stop();
}
catch (const std::runtime_error& e)
{
  std::cout << e.what() << std::endl;
  return -1;
}

  if (!m_targetProvider || !m_solver || !m_configLoader) {
    std::cout << "[MissionProcessor] Помилка: Не всі компоненти підключені!\n";
    return;
  }
  LOG("--- ПОЧАТОК МІСІЇ ---");

  isReady = true;

  int counter = 0;              // лічильник часу
  bool canChangeTarget = true;  // мітка чи є дозвіл міняти ціль
  float prevFinalDistance = 100000000;
  // Беремо абсолютний час старту симуляції
  auto startTime = std::chrono::high_resolution_clock::now();

  while (true) {
    // розраховуємо всі дані для визначення поточної найближчої цілі
    DroneTelemetry telemetry = curMyDrone.getTelemetry();

    DEBUG("--- counter = " << counter << " ---");
    DEBUG("--- curDroneX = " << std::fixed << std::setprecision(8) << telemetry.pos.x << " м ---");
    DEBUG("--- curDroneY = " << std::fixed << std::setprecision(8) << telemetry.pos.y << " м ---");
    // DEBUG("--- curMyDrone.angularState = " << std::fixed << std::setprecision(2) << telemetry.angularState << " р. ---");
    DEBUG("--- curDroneSpeed = " << telemetry.speed << " ---");
    // DEBUG("--- curDroneStateName = " << telemetry.stateName << " ---");
    DEBUG("--- currentTarget = " << target << " ---");

    // ################## РОЗРАХУНОК ТОЧКИ СКИДУ #############################################
    // -----------  заповнення масивів для пошуку найближчих цілей ---------------------------
    fillArrays(canChangeTarget, counter, curMyDrone, distDuringFall, t_pol);

    // -----------  логіка розрахунку точки скиду ---------------------------
    // на скільки я знаю треба працювати без cos і sin тоу що це для процесора важкі операції
    // в майбутньому перепишу
    Coord droneDir = {(float)cos(telemetry.angularState), (float)sin(telemetry.angularState)};

    // мітка часу в таблиці targets для визначення майбутньої позиції цілі
    // ми взяли весь час що пройшов + час коли боєприпас долетить до землі якщо буде випущений в даний момент

    Target targetPosition = UARTProcessor->getTargetPosition(target);

    DEBUG("--- targetPosition = " << targetPosition.pos.x << ", " << targetPosition.pos.y << " ---");
    DEBUG("--- targetVelocity = " << targetPosition.velocity.x << ", " << targetPosition.velocity.y << " ---");

    predictedTarget = targetPosition.pos + targetPosition.velocity * t_pol;

    DEBUG("--- predictedTarget: (" << predictedTarget.x << ", " << predictedTarget.y << ") ---");

    // точка скиду (куди летить дрон)
    // TODO  тут ще можна підкоригувати напрямок дрону маючи dirToDrone
    Coord dirToDrone = normalize(telemetry.pos - predictedTarget);
    dropPoint = predictedTarget - dirToDrone * (float)distDuringFall;

    // Знаходимо точку, куди прилетить боєприпас
    aimPoint = telemetry.pos + normalize(droneDir) * distDuringFall;
    DEBUG("--- dropPoint: (" << telemetry.pos.x << ", " << telemetry.pos.y << ") ---");
    DEBUG("--- aimPoint: (" << aimPoint.x << ", " << aimPoint.y << ") ---");

    addStep(counter, telemetry);

    // точний розрахунок коли наближаємся до вже запланованої цілі
    if (!canChangeTarget) {
      targetAngles[target] = atan2(predictedTarget.y - telemetry.pos.y, predictedTarget.x - telemetry.pos.x);
    }

    double finalDistance = calculateLength(aimPoint - predictedTarget);

    // умова при якій дрон попадає в ціль з точністю "curMyDrone.config.hitRadius / 20"
    if (finalDistance <= curMyDrone.config.hitRadius / 2 ||
        (prevFinalDistance < finalDistance && !canChangeTarget && prevFinalDistance <= curMyDrone.config.hitRadius / 4)) {
      LOG("--- БОЄПРИПАС СКИНУТИЙ! Ураження : " << std::fixed << std::setprecision(2) << finalDistance << " м від цілі номер " << target
                                                << " ---");
      DEBUG("--- prevFinalDistance: " << std::setprecision(4) << prevFinalDistance << " м.  ---");
      DEBUG("--- myDrone.arrayTimeStep: " << std::setprecision(4) << curMyDrone.config.arrayTimeStep << " ---");
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

  if (counter <= MAX_STEPS) {
    saveOutputFileByStep(counter + 1, steps);
    LOG("--- МІСІЮ ЗАВЕРШЕНО!!! ---");
  }
}
