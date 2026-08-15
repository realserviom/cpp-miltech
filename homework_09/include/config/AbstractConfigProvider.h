#pragma once
#include <iostream>
#include <iomanip>
#include "Types.h"
#include "Debug.h"
#include "interfaces/IConfigLoader.h"
#include <cstring>

class AbstractConfigProvider : public IConfigLoader {
protected:
  virtual void tunningDrone(DroneConfig &myDrone) = 0;

public:
    AbstractConfigProvider(){};

    void init(DroneConfig &myDrone) override {
      LOG("Preparing drone...");
      tunningDrone(myDrone);
      debug(myDrone);
      LOG("Drone ready");
    }

    void debug(DroneConfig &myDrone) override {
        DEBUG("=== Вхідні дані з файлу (структура) ===");
        DEBUG("Координати (x, y):    " << std::fixed << std::setprecision(2) << myDrone.startPos.x << " м, " << myDrone.startPos.y << " м");
        DEBUG("Висота (altitude):    " << myDrone.altitude << " м");
        DEBUG("Напрямок (dir):       " << myDrone.initialDir << " рад.");
        DEBUG("Швидкість атаки:      " << myDrone.attackSpeed << " м/c");
        DEBUG("Шлях розгону:         " << myDrone.accelPath << " м");
        DEBUG("Крок часу масиву:     " << myDrone.arrayTimeStep << " с");
        DEBUG("Крок симуляції:       " << myDrone.timeStep << " с");
        DEBUG("Радіус ураження:      " << myDrone.hitRadius << " м");
        DEBUG("Кутова швидкість:     " << myDrone.angularSpeed << " рад/c");
        DEBUG("Поріг повороту:       " << myDrone.turnThreshold << " рад");
        DEBUG("---------------------------------------");
    }

    virtual ~AbstractConfigProvider() override {}
};