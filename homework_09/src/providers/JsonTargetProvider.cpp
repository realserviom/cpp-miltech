#include "providers/JsonTargetProvider.h"
#include "json.hpp"
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

JsonTargetProvider::JsonTargetProvider(const std::string& jsonFilePath)
  : AbstractTargetProvider(jsonFilePath)
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

  // можна було використати std::vector<std::vector<Coord>>
  // але мені щось не дуже подобається
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

Coord JsonTargetProvider::getTargetPosition(const int target, const float time)
{
  // ітерація координати (кожних myDrone.arrayTimeStep секунд нова координата)
  int futureIterationForTarget = getIterationByTime(time, getArrayTimeStep());

  int nextFutureIterationForTarget = getNextIteration(futureIterationForTarget);

  // час що залишився
  float remainderTimeInSpot = std::fmod(time, arrayTimeStep);

  Coord targetPosIteration = getTargetPositionInIteration(target, futureIterationForTarget);
  Coord targetPosNextIteration = getTargetPositionInIteration(target, nextFutureIterationForTarget);

  Coord deltaTargetPos = targetPosNextIteration - targetPosIteration;

  // швидкість Vtarget це швидкість зміни координатів
  Coord Vtarget = deltaTargetPos / getArrayTimeStep();

  // прогнозована позиція цілі
  return targetPosIteration + Vtarget * remainderTimeInSpot;
}

int JsonTargetProvider::getNextIteration(int& iteration)
{
  if (iteration == (this->m_timeSteps - 1)) {
    return 0;
  }

  return iteration + 1;
}

Coord JsonTargetProvider::getTargetPositionInCounter(int& targetId, const int& counter)
{
  int timeIteration = this->getIterationByCounter(counter);

  if (this->m_targets && targetId >= 0 && targetId < m_targetCount) {
    return this->m_targets[targetId][timeIteration];
  }
  return Coord{0.0, 0.0};
}

int JsonTargetProvider::getIterationByTime(float time, const float& arrayTimeStep)
{
  const int wholeRangeTime = arrayTimeStep * this->m_timeSteps;

  if (time > wholeRangeTime) {
    time = std::fmod(time, wholeRangeTime);
  }
  return std::floor(time / arrayTimeStep);
}

Coord JsonTargetProvider::getTargetPositionInIteration(const int& index, int& timeIteration)
{
  if (this->m_targets && index >= 0 && index < m_targetCount) {
    return this->m_targets[index][timeIteration];
  }
  return Coord{0.0, 0.0};
}

// Coord JsonTargetProvider::getTargetNextPos(int& targetId, const int& counter)
// {
//   int timeIteration = getIterationByCounter(counter);
//   int nextIteration = getNextIteration(timeIteration);

//   return getTargetPositionInIteration(targetId, nextIteration);
// }

int JsonTargetProvider::getIterationByCounter(const int& counter)
{
  const int wholeRangeCounters = this->m_numberCounterInTimeSpot * this->m_timeSteps;

  // якщо в нас поточна ітерація більша чи рівна wholeRangeCounters тоді берем остачу від цілочисленого
  // ділення на wholeRangeCounters
  const int new_counter = counter >= wholeRangeCounters ? static_cast<int>(counter % wholeRangeCounters) : counter;

  return static_cast<int>(std::floor(new_counter / this->m_numberCounterInTimeSpot));
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
