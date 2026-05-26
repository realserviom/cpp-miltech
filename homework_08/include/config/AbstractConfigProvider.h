#pragma once
#include <iostream>
#include "Types.h"
#include "Debug.h"
#include "../interfaces/IConfigLoader.h"
#include <vector>
#include <cstring>


class AbstractConfigProvider : public IConfigLoader {
protected:
    std::vector<AmmoParams> ammoList;

    virtual void tunningDrone(DroneConfig &myDrone) = 0;
    virtual void loadAmmo() = 0;

public:
    AbstractConfigProvider(){};

    void init(DroneConfig &myDrone) override {
        std::cout << "Preparing drone...\n";
        tunningDrone(myDrone);
        debug(myDrone);
        loadAmmo();
        std::cout << "Drone ready.\n";
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

    const AmmoParams* getAmmoParameters(const char* name_to_find) override {

       for (size_t i = 0; i < ammoList.size(); ++i) {
            if (strcmp(ammoList[i].name, name_to_find) == 0) {
                return &ammoList[i];
            }
        }

        return nullptr;
    }

    virtual ~AbstractConfigProvider() override {}
};