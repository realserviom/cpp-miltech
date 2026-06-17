#include "config/FileConfigLoader.h"
#include "json.hpp"
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

FileConfigLoader::FileConfigLoader(const std::string& filePathDroneConfig, const std::string& filePathListAmmo)
  : m_filePath_droneConfig(filePathDroneConfig)
  , m_filePath_listAmmo(filePathListAmmo)
{
}

void FileConfigLoader::loadAmmo()
{
  std::ifstream fin(m_filePath_listAmmo);

  if (!fin.is_open()) {
    throw std::runtime_error("Не вдалося відкрити файл конфігурації: " + m_filePath_listAmmo);
  }

  json j;

  try {
    fin >> j;
  }
  catch (const json::parse_error& e) {
    throw std::runtime_error("Помилка парсингу файлу " + m_filePath_listAmmo + ": " + std::string(e.what()));
    fin.close();
  }

  ammoList.clear();

  if (j.is_array()) {
    for (const auto& ammoJson : j) {
      AmmoParams singleAmmo;

      std::string nameStr = ammoJson.value("name", "Unknown");
      singleAmmo.mass = ammoJson.value("mass", 0.0f);
      singleAmmo.drag = ammoJson.value("drag", 0.0f);
      singleAmmo.lift = ammoJson.value("lift", 0.0f);
      ammoList[nameStr] = singleAmmo;
    }
  }

  fin.close();
}

void FileConfigLoader::tunningDrone(DroneConfig& myDrone)
{
  std::ifstream fin(m_filePath_droneConfig);

  if (!fin.is_open()) {
    throw std::runtime_error("Не вдалося відкрити файл конфігурації: " + m_filePath_droneConfig);
  }

  json j;
  try {
    fin >> j;
  }
  catch (const json::parse_error& e) {
    throw std::runtime_error("Помилка парсингу файлу " + m_filePath_droneConfig + ": " + std::string(e.what()));
  }

  myDrone.startPos.x = j["drone"]["position"]["x"];
  myDrone.startPos.y = j["drone"]["position"]["y"];
  myDrone.altitude = j["drone"]["altitude"];
  myDrone.initialDir = j["drone"]["initialDirection"];
  myDrone.attackSpeed = j["drone"]["attackSpeed"];
  myDrone.accelPath = j["drone"]["accelerationPath"];
  myDrone.angularSpeed = j["drone"]["angularSpeed"];
  myDrone.turnThreshold = j["drone"]["turnThreshold"];
  myDrone.arrayTimeStep = j["targetArrayTimeStep"];
  myDrone.timeStep = j["simulation"]["timeStep"];
  myDrone.timeScale = j["simulation"]["timeScale"];
  myDrone.hitRadius = j["simulation"]["hitRadius"];
  myDrone.ammoName = j["ammo"].get<std::string>();

  fin.close();
}
