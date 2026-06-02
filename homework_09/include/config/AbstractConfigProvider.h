#pragma once
#include <iostream>
#include <iomanip>
#include "Types.h"
#include "Debug.h"
#include "interfaces/IConfigLoader.h"
#include <cstring>
#include <unordered_map>

class AbstractConfigProvider : public IConfigLoader {
protected:
  std::unordered_map<def_ammoName, AmmoParams> ammoList;

  virtual void tunningDrone(DroneConfig &myDrone) = 0;
  virtual void loadAmmo() = 0;

public:
    AbstractConfigProvider(){};

    void init(DroneConfig &myDrone) override {
      LOG("Preparing drone...");
      tunningDrone(myDrone);
      debug(myDrone);
      loadAmmo();
      LOG("Drone ready");
    }

    void debug(DroneConfig &myDrone) override {
        DEBUG("=== Вхідні дані з файлу (структура) ===");
        DEBUG("Координати (x, y):    " << std::fixed << std::setprecision(2) << myDrone.startPos.x << " м, " << myDrone.startPos.y << " м");
        DEBUG("Висота (altitude):    " << myDrone.altitude << " м");
        DEBUG("Напрямок (dir):       " << myDrone.initialDir << " рад.");
        DEBUG("Швидкість атаки:      " << myDrone.attackSpeed << " м/c");
        DEBUG("Шлях розгону:         " << myDrone.accelPath << " м");
        DEBUG("Боєприпас:            " << myDrone.ammoName);
        DEBUG("Крок часу масиву:     " << myDrone.arrayTimeStep << " с");
        DEBUG("Крок симуляції:       " << myDrone.simTimeStep << " с");
        DEBUG("Радіус ураження:      " << myDrone.hitRadius << " м");
        DEBUG("Кутова швидкість:     " << myDrone.angularSpeed << " рад/c");
        DEBUG("Поріг повороту:       " << myDrone.turnThreshold << " рад");
        DEBUG("---------------------------------------");
    }

    const AmmoParams *getAmmoParameters(std::string name_to_find) override
    {
      if (name_to_find.empty()) {
        return nullptr;
      }

      auto it = ammoList.find(name_to_find);
      if (it != ammoList.end()) {
        return &(it->second);
      }

      return nullptr;
    }

    virtual ~AbstractConfigProvider() override {}
};