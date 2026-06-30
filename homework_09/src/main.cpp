/******************************************************************************

Welcome to GDB Online.
GDB online is an online compiler and debugger tool for C, C++, Python, Java, PHP, Ruby, Perl,
C#, OCaml, VB, Swift, Pascal, Fortran, Haskell, Objective-C, Assembly, HTML, CSS, JS, SQLite, Prolog.
Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/
#include <iostream>
#include <memory>
#include <stdlib.h>
#include <cmath>
#include "Types.h"
#include "config/ComponentFactory.h"
#include "MissionProcessor.h"
#include <iomanip>
#include "GPIOController.h"
#include "functions.h"
#include "UARTProcessor.h"
#include "drone_link.h"

// Визначення константи Пі, якщо її немає в cmath
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main()
{
  try {
    // Налаштування socat для тестування:
    // sudo apt update && sudo apt install -y socat - встановлення
    // socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB - налаштування

    // const char* uartDevice = "/dev/ttyAMA1"; - це для Raspberry Pi
    const char* uartDevice = "/tmp/ttyA";  // це socat
    int uartFd = openUart(uartDevice);

    if (uartFd < 0) {
      return 1;  // Виходимо, якщо порт не відкрився
    }

    std::cout << "UART порт " << uartDevice << " успішно налаштовано на 115200 бод." << std::endl;

    // Створюємо uartProcessor який буде працювати в окремому потоці і обробляти дані з UART
    UARTProcessor uartProcessor(uartFd);

    // Створюємо аналитичний балістичний калькулятор табличного типу
    std::shared_ptr<IBallisticSolver> analyticalSolver = createSolver(SolverType::TABLE);

    std::shared_ptr<IConfigLoader> configLoader = createLoader(LoaderType::FILE, "../data/config.json", "../data/ammo.json");

    std::shared_ptr<ITargetProvider> targetProvider = createProvider(ProviderType::TIME, "../data/targets.json");

    MissionProcessor processor(targetProvider, analyticalSolver, configLoader);

    DroneConfig myDroneConfig;

    // MissionProcessor processor(targetProvider, analyticalSolver, configLoader);

    // Створюємо GPIO контролер. В контейнері він автоматично увімкне імітацію!
    GPIOController gpio;

    std::cout << "Запуск головного циклу опитування..." << std::endl;
    uartProcessor.start();

    // START — на ніжці підняти вольтаж на 1 одразу на старті й тримати. (сигнал про готовність дрона)
    gpio.set_start(1);

    // Локальні змінні для потоку балістики/main
    const dlink::AmmoCfg* ammo = nullptr;
    float t_pol;               // час польоту снаряду
    float distDuringFall = 0;  // дистанція, яку проходить снаряд за час t_pol (горизонтальна відстань від точки скидання до цілі)

    dlink::Telemetry droneData;
    dlink::TargetPos targetData;

    bool already_dropped = false;
    auto nextTimePoint = std::chrono::high_resolution_clock::now() + std::chrono::seconds(20);

    std::cout << "[Main/Ballistics] Потік балістики запущено!" << std::endl;

    // Головний цикл цього потоку (Балістичний калькулятор + Скид)
    while (true) {
      // Перевіряємо, чи отримали конфіг боєприпасу
      if (ammo == nullptr) {
        ammo = uartProcessor.getAmmoConfigPtr();
        if (ammo != nullptr) {
          std::cout << "[Ballistics] Отримано конфіг з UART! Name = " << ammo->name << std::endl;
          std::cout << "[Ballistics] Розраховуємо час польоту снаряду і відстань що він проходить!" << std::endl;

          try {
            AmmoParams ammoParams;

            ammoParams.mass = ammo->mass;
            ammoParams.drag = ammo->drag;
            ammoParams.lift = ammo->lift;

            distDuringFall = analyticalSolver->getDistDuringFall(t_pol, myDroneConfig, &ammoParams);
          }
          catch (const std::runtime_error& e) {
            throw std::runtime_error("[AnalyticalSolver] КРИТИЧНА ПОМИЛКА: " + std::string(e.what()));
          }

          DEBUG("Горизонтальна дистанція яку проходить дрон за час " << t_pol << " сек. рівна " << distDuringFall << " м.");
          DEBUG("-----------------------------------");
        }
      }

      //  Забираємо свіжу телеметрію та координати цілі, якщо вони є
      bool hasFreshData = uartProcessor.getTelemetry(droneData) && uartProcessor.getTarget(targetData);

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

    uartProcessor.stop();
    return 0;

    std::shared_ptr<IConfigLoader> configLoader = createLoader(LoaderType::FILE, "../data/config.json", "../data/ammo.json");

    std::shared_ptr<ITargetProvider> targetProvider = createProvider(ProviderType::TIME, "../data/targets.json");

    std::shared_ptr<IBallisticSolver> analyticalSolver = createSolver(SolverType::TABLE);

    MissionProcessor processor(targetProvider, analyticalSolver, configLoader);

    DroneConfig myDroneConfig;

    // ініціалізація параметрів дрона і початкових параметрів руху
    processor.init(myDroneConfig, ammo);

    // Поточний стан дрона
    Drone curMyDrone(myDroneConfig);

    // Завантаження боєприпасу
    ammo = configLoader->getAmmoParameters(myDroneConfig.ammoName);

    float t_pol;               // час польоту
    float distDuringFall = 0;  // дистанція, яку проходить снаряд за час t_pol (горизонтальна відстань від точки скидання до цілі)

    try {
      distDuringFall = analyticalSolver->getDistDuringFall(t_pol, myDroneConfig, ammo);
    }
    catch (const std::runtime_error& e) {
      throw std::runtime_error("[AnalyticalSolver] КРИТИЧНА ПОМИЛКА: " + std::string(e.what()));
    }

    DEBUG("Горизонтальна дистанція яку проходить дрон за час " << t_pol << " сек. рівна " << distDuringFall << " м.");
    DEBUG("-----------------------------------");

    if (distDuringFall <= 0) {
      throw std::runtime_error("Горизонтальна дистанція повинна бути додатня");
    }

    targetProvider->setArrayTimeStep(myDroneConfig.arrayTimeStep);
    targetProvider->setTargetTimeStep(myDroneConfig.targetTimeStep);
    targetProvider->setTimeScale(myDroneConfig.timeScale);

    curMyDrone.start();
    targetProvider->start();
    processor.start(curMyDrone, distDuringFall, t_pol);

    while (!curMyDrone.isThreadReady() || !targetProvider->isThreadReady() || !processor.isThreadReady()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    processor.missionThread.join();

    curMyDrone.stop();
    targetProvider->stop();
  }
  catch (const std::runtime_error& e) {
    std::cout << e.what() << std::endl;
    return -1;
  }

  return 0;
}