#pragma once
#include <stdexcept> // ОБОВ'ЯЗКОВО додаємо для std::runtime_error
#include <iostream>
#include <fstream>
#include <string>
#include "types.h"
#include "debug.h"
#include "json.hpp"

using json = nlohmann::json;

enum class LoaderType { FILE };

// інтерфейс
class IConfigLoader {
public:
    virtual void debug(DroneConfig &myDrone) = 0;
    virtual void init(DroneConfig &myDrone) = 0;
    virtual const AmmoParams* getAmmoParameters(const char* name_to_find) = 0;
    virtual ~IConfigLoader() {}
};

// абстрактний клас
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



//  Реалізація (FileConfigLoader)
class FileConfigLoader : public AbstractConfigProvider {
public:

    static constexpr int MAX_AMMO_TYPES = 5;
    std::string m_filePath_droneConfig;
    std::string m_filePath_listAmmo;

    FileConfigLoader(const std::string& filePathDroneConfig, const std::string& filePathListAmmo) 
        : m_filePath_droneConfig(filePathDroneConfig), m_filePath_listAmmo(filePathListAmmo)  {}

protected:

    void loadAmmo() override {
        std::ifstream fin(m_filePath_listAmmo);

        if (!fin.is_open()) {
            throw std::runtime_error("Не вдалося відкрити файл конфігурації: " + m_filePath_listAmmo);
        }
        
        json j;
        try {
            fin >> j;
        } catch (const json::parse_error& e) {
            throw std::runtime_error("Помилка парсингу файлу " + m_filePath_listAmmo + ": " + std::string(e.what()));
            fin.close();
        }

        ammoList.clear();

        if (j.is_array()) {
            for (const auto& ammoJson : j) {
                AmmoParams singleAmmo;
                std::string nameStr = ammoJson.value("name", "Unknown");
                // std::snprintf(singleAmmo.name, sizeof(singleAmmo.name), "%s", nameStr.c_str());

                strncpy(singleAmmo.name, nameStr.c_str(), sizeof(singleAmmo.name) - 1);
                singleAmmo.name[sizeof(singleAmmo.name) - 1] = '\0';
          
                singleAmmo.mass = ammoJson.value("mass", 0.0f);
                singleAmmo.drag = ammoJson.value("drag", 0.0f);
                singleAmmo.lift = ammoJson.value("lift", 0.0f);

                ammoList.push_back(singleAmmo);
            }
        }
        
        fin.close();
    }

    void tunningDrone(DroneConfig &myDrone) override {

        std::ifstream fin(m_filePath_droneConfig);

        if (!fin.is_open()) {
            throw std::runtime_error("Не вдалося відкрити файл конфігурації: " + m_filePath_droneConfig);
        }
        
        json j;
        try {
            fin >> j;
        } catch (const json::parse_error& e) {
            throw std::runtime_error("Помилка парсингу файлу " + m_filePath_droneConfig + ": " + std::string(e.what()));
        }

        myDrone.startPos.x    = j["drone"]["position"]["x"];
        myDrone.startPos.y    = j["drone"]["position"]["y"];
        myDrone.altitude      = j["drone"]["altitude"];
        myDrone.initialDir    = j["drone"]["initialDirection"];
        myDrone.attackSpeed   = j["drone"]["attackSpeed"];
        myDrone.accelPath     = j["drone"]["accelerationPath"];
        myDrone.angularSpeed  = j["drone"]["angularSpeed"];
        myDrone.turnThreshold = j["drone"]["turnThreshold"];
        myDrone.arrayTimeStep = j["targetArrayTimeStep"];
        myDrone.simTimeStep   = j["simulation"]["timeStep"];
        myDrone.hitRadius     = j["simulation"]["hitRadius"];

        strncpy(myDrone.ammoName, j["ammo"].get<std::string>().c_str(), sizeof(myDrone.ammoName) - 1);      

        fin.close();
    }

    virtual ~FileConfigLoader() override = default;


};

// Фабрика
inline IConfigLoader* createLoader(LoaderType type, const char* file_drone_config_name, const char* file_list_ammo_name) {
    switch (type) {
        case LoaderType::FILE:
            return new FileConfigLoader(file_drone_config_name, file_list_ammo_name);
        default:
            return nullptr;
    }
}