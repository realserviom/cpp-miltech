#include "../../include/providers/JsonTargetProvider.h"
#include "../../include/json.hpp"
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

JsonTargetProvider::JsonTargetProvider(const std::string& jsonFilePath)
  : m_filePath(jsonFilePath)
{
}

void JsonTargetProvider::loadTargets()
{
  std::ifstream fin(m_filePath);

  if (!fin.is_open()) {
    throw std::runtime_error("Не вдалося відкрити файл конфігурації: " + m_filePath);
  }

  json j;
  try {
    fin >> j;
  }
  catch (const json::parse_error& e) {
    throw std::runtime_error("Помилка парсингу файлу " + m_filePath + ": " + std::string(e.what()));
    fin.close();
  }

  m_targetCount = j["targetCount"];
  m_timeSteps = j["timeSteps"];

  m_targets = new Coord*[m_targetCount];

  for (int i = 0; i < m_targetCount; i++) {
    m_targets[i] = new Coord[m_timeSteps];
    auto& positionsJson = j["targets"][i]["positions"];

    for (int t = 0; t < m_timeSteps; t++) {
      m_targets[i][t].x = positionsJson[t]["x"];
      m_targets[i][t].y = positionsJson[t]["y"];
    }
  }

  fin.close();
}

JsonTargetProvider::~JsonTargetProvider()
{
  if (m_targets != nullptr) {
    for (int i = 0; i < m_targetCount; i++) {
      delete[] m_targets[i];
    }
    delete[] m_targets;
  }
}