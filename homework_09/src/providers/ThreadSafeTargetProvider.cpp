#include "providers/ThreadSafeTargetProvider.h"
#include "json.hpp"
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include "Debug.h"
#include <cmath>
#include "functions.h"
#include <algorithm>

using json = nlohmann::json;

ThreadSafeTargetProvider::ThreadSafeTargetProvider(const std::string& jsonFilePath)
  : AbstractTargetProvider(jsonFilePath)
{
}

void ThreadSafeTargetProvider::init(int& numberCounterInTimeSpot)
{
  // 1. Спочатку викликаємо оригінальний метод з базового класу!
  // Він завантажить JSON і правильно встановить m_targetCount.
  AbstractTargetProvider::init(numberCounterInTimeSpot);

  // 2. Тепер m_targetCount має реальне значення (наприклад, 5).
  // Можемо безпечно виділяти пам'ять для векторів:
  currentPositions.resize(m_targetCount, Coord{0.0, 0.0});
  prevPositions.resize(m_targetCount, Coord{0.0, 0.0});

  DEBUG("Вектори цілей успішно ініціалізовано. Кількість: " << m_targetCount);
}

void ThreadSafeTargetProvider::loadTargets()
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

Coord ThreadSafeTargetProvider::getTargetPosition(const int target, const float time)
{
  throw std::runtime_error(
    "Не можна тут використовувати getTargetPosition з часом, бо це не потокобезпечний метод. Використовуйте getTargetPosition без часу.");
}

Target ThreadSafeTargetProvider::getTargetPosition(const int targetId)
{
  std::lock_guard<std::mutex> lock(targetMutex);
  Target targetPos;
  targetPos.pos = currentPositions[targetId];
  targetPos.velocity = (prevPositions[targetId].x != 0.00 && prevPositions[targetId].y != 0.00)
                         ? (currentPositions[targetId] - prevPositions[targetId]) / getTargetTimeStep() / getTimeScale()
                         : Coord{0.0, 0.0};

  // DEBUG("Target " << targetId << " pos: (" << targetPos.pos.x << ", " << targetPos.pos.y << "), velocity: (" << targetPos.velocity.x <<
  // ", "
  //                 << targetPos.velocity.y << ")");
  return targetPos;
}

void ThreadSafeTargetProvider::setTargetPosition()
{
  prevPositions = currentPositions;

  for (int i = 0; i < m_targetCount; i++) {
    const float time = stepCount * getTargetTimeStep();

    // ітерація координати (кожних myDrone.arrayTimeStep секунд нова координата)
    const float arrayTimeStep = getArrayTimeStep();
    int iterationForTarget = getIterationByTime(time, arrayTimeStep);

    int nextIterationForTarget = getNextIteration(iterationForTarget);

    // час що залишився
    float remainderTimeInSpot = std::fmod(time, arrayTimeStep);

    Coord targetPosIteration = getTargetPositionInIteration(i, iterationForTarget);
    Coord targetPosNextIteration = getTargetPositionInIteration(i, nextIterationForTarget);

    Coord deltaTargetPos = targetPosNextIteration - targetPosIteration;

    // швидкість Vtarget це швидкість зміни координатів
    Coord Vtarget = deltaTargetPos / arrayTimeStep;

    // прогнозована позиція цілі
    currentPositions[i] = targetPosIteration + Vtarget * remainderTimeInSpot;
  }
}

int ThreadSafeTargetProvider::getNextIteration(int& iteration)
{
  if (iteration == (this->m_timeSteps - 1)) {
    return 0;
  }

  return iteration + 1;
}

Coord ThreadSafeTargetProvider::getTargetPositionInCounter(int& targetId, const int& counter)
{
  int timeIteration = this->getIterationByCounter(counter);

  if (this->m_targets && targetId >= 0 && targetId < m_targetCount) {
    return this->m_targets[targetId][timeIteration];
  }
  return Coord{0.0, 0.0};
}

int ThreadSafeTargetProvider::getIterationByTime(float time, const float& arrayTimeStep)
{
  const int wholeRangeTime = arrayTimeStep * this->m_timeSteps;

  if (time > wholeRangeTime) {
    time = std::fmod(time, wholeRangeTime);
  }
  return std::floor(time / arrayTimeStep);
}

Coord ThreadSafeTargetProvider::getTargetPositionInIteration(const int& index, int& timeIteration)
{
  if (this->m_targets && index >= 0 && index < m_targetCount) {
    return this->m_targets[index][timeIteration];
  }
  return Coord{0.0, 0.0};
}

// Coord ThreadSafeTargetProvider::getTargetNextPos(int& targetId, const int& counter)
// {
//   int timeIteration = getIterationByCounter(counter);
//   int nextIteration = getNextIteration(timeIteration);

//   return getTargetPositionInIteration(targetId, nextIteration);
// }

int ThreadSafeTargetProvider::getIterationByCounter(const int& counter)
{
  const int wholeRangeCounters = this->m_numberCounterInTimeSpot * this->m_timeSteps;

  // якщо в нас поточна ітерація більша чи рівна wholeRangeCounters тоді берем остачу від цілочисленого
  // ділення на wholeRangeCounters
  const int new_counter = counter >= wholeRangeCounters ? static_cast<int>(counter % wholeRangeCounters) : counter;

  return static_cast<int>(std::floor(new_counter / this->m_numberCounterInTimeSpot));
}

ThreadSafeTargetProvider::~ThreadSafeTargetProvider()
{
  if (m_targets != nullptr) {
    for (int i = 0; i < m_targetCount; i++) {
      delete[] m_targets[i];
    }
    delete[] m_targets;
  }
}

// === БАГАТОПОТОЧНИЙ ІНТЕРФЕЙС ===
void ThreadSafeTargetProvider::start()
{
  DEBUG("--- start target thread! ---");
  running = true;
  targetsThread = std::thread(&ThreadSafeTargetProvider::physicsLoop, this);
}

void ThreadSafeTargetProvider::stop()
{
  running = false;
  if (targetsThread.joinable()) {
    targetsThread.join();
  }
}

bool ThreadSafeTargetProvider::isThreadReady() const
{
  return isReady;
}

void ThreadSafeTargetProvider::physicsLoop()
{
  isReady = true;
  stepCount = 0;

  // Беремо абсолютний час старту симуляції
  auto startTime = std::chrono::high_resolution_clock::now();

  try {
    while (running) {
      {
        std::lock_guard<std::mutex> lock(targetMutex);
        setTargetPosition();
      }

      auto nextTimePoint = getNextTimePoint(startTime, getTargetTimeStep() / getTimeScale(), stepCount);
      // Кажемо операційній системі прокинутися в певній точці"
      std::this_thread::sleep_until(nextTimePoint);

      stepCount++;
    }
  }
  // Перехоплюємо стандартні помилки C++ (наприклад, std::out_of_range, std::runtime_error)
  catch (const std::exception& e) {
    std::cerr << "[КРИТИЧНА ПОМИЛКА ПОТОКУ ЦІЛЕЙ]: " << e.what() << '\n';
    running = false;  // Зупиняємо цикл
  }
  // Перехоплюємо будь-які інші невідомі помилки
  catch (...) {
    std::cerr << "[КРИТИЧНА ПОМИЛКА ПОТОКУ ЦІЛЕЙ]: Невідомий виняток!\n";
    running = false;
  }

  // Гарантуємо, що прапорець скинеться навіть у випадку аварійної зупинки циклу
  isReady = false;
}