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

void MissionProcessor::fillArrays(bool& canChangeTarget, const Drone& curMyDrone, const float distDuringFall, const float t_pol)
{
  DEBUG("fillArrays numberOfTargets: " << numberOfTargets);

  for (uint8_t targetId = 0; targetId < numberOfTargets; targetId++) {
    auto targetOptions = m_uartProcessor->getTargetPosition(targetId);

    Coord targetPos = targetOptions->pos;

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

      Coord velocity = targetOptions->velocity;

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
      DEBUG("Перерахували кут напрямку для цілі: " << std::to_string(targetId));
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

  // canChangeTarget - мітка чи є дозвіл міняти ціль (якщо дрон вже летить на ціль і вона не далеко, то міняти ціль не можна)
  // newTarget != target - перевірка чи дійсно нова ціль? Щоб лишній раз не гальмувати і обертати
  // (хоча іноді навіть поточна ціль може критично змінити напрямок руху і тоді треба буде гальмувати і розвертатися але це не враховано)
  // currentStateName != "STOPPED" - перевірка чи дрон не стоїть на місці (якщо стоїть то можна міняти ціль)
  // currentStateName != "DECELERATING" - перевірка чи дрон
  if (canChangeTarget && newTarget != target && currentStateName != "STOPPED" && currentStateName != "DECELERATING") {
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

void MissionProcessor::start()
{
  DEBUG("--- start mission thread! ---");
  running = true;
  missionThread = std::thread(&MissionProcessor::missionLoop, this);
}

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

  // Головний цикл цього потоку (Балістичний калькулятор + Скид)
  while (true) {
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

    DroneTelemetry telemetry;

    // якщо ми ще не скинули боєприпас і дрон існує (отримали конфіг дрона) тоді розраховуємо точку скиду
    if (drop == false && curMyDrone != nullptr) {
      if (m_uartProcessor->getTelemetry(telemetry)) {
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

      // ################## РОЗРАХУНОК ТОЧКИ СКИДУ #############################################
      // -----------  заповнення масивів для пошуку найближчих цілей ---------------------------
      fillArrays(canChangeTarget, *curMyDrone, distDuringFall, t_pol);

      // -----------  логіка розрахунку точки скиду ---------------------------
      // на скільки я знаю треба працювати без cos і sin тоу що це для процесора важкі операції
      // в майбутньому перепишу
      Coord droneDir = {(float)cos(curMyDrone->angularState), (float)sin(curMyDrone->angularState)};

      // мітка часу в таблиці targets для визначення майбутньої позиції цілі
      // ми взяли весь час що пройшов + час коли боєприпас долетить до землі якщо буде випущений в даний момент

      dlink::TargetPos targetPacket;
      m_uartProcessor->getTargetPosition2(targetPacket);  // отримуємо поточну ціль з UART

      DEBUG("targetPacket.id " << std::to_string(targetPacket.id) << " pos: (" << std::to_string(targetPacket.x) << ", "
                               << std::to_string(targetPacket.y) << ")");

      auto targetPosition = m_uartProcessor->getTargetPosition(target);

      if (!targetPosition.has_value()) {
        continue;
      }

      DEBUG("--- targetPosition = " << targetPosition->pos.x << ", " << targetPosition->pos.y << " ---");
      DEBUG("--- targetVelocity = " << targetPosition->velocity.x << ", " << targetPosition->velocity.y << " ---");

      predictedTarget = targetPosition->pos + targetPosition->velocity * t_pol;

      // DEBUG("--- predictedTarget: (" << predictedTarget.x << ", " << predictedTarget.y << ") ---");

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

      // умова при якій дрон попадає в ціль з точністю "hitRadius / 2"
      if (finalDistance <= hitRadius / 2 || (prevFinalDistance < finalDistance && !canChangeTarget && prevFinalDistance <= hitRadius / 4)) {
        LOG("--- БОЄПРИПАС СКИНУТИЙ! Ураження : " << std::fixed << std::setprecision(2) << finalDistance << " м від цілі номер " << target
                                                  << " ---");
        DEBUG("--- prevFinalDistance: " << std::setprecision(4) << prevFinalDistance << " м.  ---");
        DEBUG("--- dropPoint: (" << curMyDrone->pos.x << ", " << curMyDrone->pos.y << ") ---");
        DEBUG("--- aimPoint: (" << aimPoint.x << ", " << aimPoint.y << ") ---");
        DEBUG("--- predictedTarget: (" << predictedTarget.x << ", " << predictedTarget.y << ") ---");

        // Викликаємо GPIO з цього потоку! Потік UART при цьому не блокується на 80 мс
        gpio.pulse_drop();
        drop = true;
        std::cout << "[Ballistics] Команду DROP виконано!" << std::endl;
        continue;
      }

      prevFinalDistance = finalDistance;

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
        break;
      }
    }

    std::this_thread::sleep_for(std::chrono::nanoseconds(100000000));
  }

  curMyDrone->stop();  // Зупиняємо потік фізики дрона
  LOG("--- КІНЕЦЬ МІСІЇ ---");
}
