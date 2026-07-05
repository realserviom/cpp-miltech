#include <iostream>
#include <memory>
#include <stdlib.h>
#include <cmath>
#include "Types.h"
#include "config/ComponentFactory.h"
#include "MissionProcessor.h"
#include <iomanip>

// Визначення константи Пі, якщо її немає в cmath
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main()
{
  try {
    std::shared_ptr<IConfigLoader> configLoader = createLoader(LoaderType::FILE, "../data/config.json", "../data/ammo.json");

    std::shared_ptr<ITargetProvider> targetProvider = createProvider(ProviderType::TIME, "../data/targets.json");

    std::shared_ptr<IBallisticSolver> analyticalSolver = createSolver(SolverType::TABLE);

    MissionProcessor processor(targetProvider, analyticalSolver, configLoader);

    DroneConfig myDroneConfig;

    const AmmoParams* ammo = nullptr;

    // ініціалізація параметрів дрона і початкових параметрів руху
    processor.init(myDroneConfig, ammo);

    // Поточний стан дрона
    Drone curMyDrone(myDroneConfig);

    // Завантаження боєприпасу
    ammo = configLoader->getAmmoParameters(myDroneConfig.ammoName);

    if (ammo == nullptr) {
      throw std::runtime_error("Помилка: боєприпас " + std::string(myDroneConfig.ammoName) + " не знайдено в базі!");
    }

    DEBUG("Знайдено боєприпас: " << myDroneConfig.ammoName);
    DEBUG("Параметри: mass: " << std::fixed << std::setprecision(3) << ammo->mass << ", drag: " << ammo->drag << ", lift: " << ammo->lift);

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