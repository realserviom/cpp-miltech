#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <string.h>
#include <fstream>
#include "functions.h"
#include "Types.h"
#include <stdbool.h>
#include "json.hpp"

using json = nlohmann::ordered_json;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// в нас 60 точок часу і 5 цілей це 300 комбінацій
#define CACHE_SIZE 1024

Coord normalize(const Coord& c)
{
  float L = std::hypot(c.x, c.y);
  if (L < 1e-6f)
    return {0, 0};
  return c / L;
}

float calculateLength(const Coord& c)
{
  return std::hypot(c.x, c.y);
}

int getIndexByMinValue(const std::vector<float>& targetTimes)
{
  if (targetTimes.empty())
    return -1;

  auto minIt = targetTimes.begin();

  for (auto it = targetTimes.begin() + 1; it != targetTimes.end(); ++it) {
    if (*it < *minIt) {
      minIt = it;
    }
  }

  return std::distance(targetTimes.begin(), minIt);
}

void saveOutputFileByStep(int length, const std::vector<SimStep>& steps)
{
  json out;
  out["totalSteps"] = length;

  printf("============== length = %d ===========\n", length);

  out["steps"] = json::array();

  auto endIt = (static_cast<size_t>(length) <= steps.size()) ? steps.begin() + length : steps.end();

  for (auto it = steps.begin(); it != endIt; ++it) {
    json stepEntry;

    stepEntry["position"] = {{"x", it->pos.x}, {"y", it->pos.y}};

    stepEntry["direction"] = it->direction;
    stepEntry["state"] = it->state;
    stepEntry["targetIndex"] = it->targetIdx;
    stepEntry["timeSecSinceStart"] = it->timeSecSinceStart;

    stepEntry["dropPoint"] = {{"x", it->dropPoint.x}, {"y", it->dropPoint.y}};
    stepEntry["aimPoint"] = {{"x", it->aimPoint.x}, {"y", it->aimPoint.y}};
    stepEntry["predictedTarget"] = {{"x", it->predictedTarget.x}, {"y", it->predictedTarget.y}};
    stepEntry["counter"] = it->counter;

    out["steps"].push_back(stepEntry);
  }

  std::ofstream fout("../data/output.json");
  fout << out.dump(2);
  fout.close();
}

std::chrono::duration<float> getDurationTime(std::chrono::high_resolution_clock::time_point startTime, double dt)
{
  auto endTime = std::chrono::high_resolution_clock::now();
  double delta = std::chrono::duration<double>(endTime - startTime).count();

  if (delta > dt) {
    throw std::runtime_error("[function.cpp] КРИТИЧНА ПОМИЛКА: час виконання більший за крок " + std::to_string(dt));
  }

  // Повертаємо саме ДУРЕЙШН (тривалість)
  return std::chrono::duration<float>(static_cast<float>(dt - delta));
}

std::chrono::high_resolution_clock::time_point getNextTimePoint(std::chrono::high_resolution_clock::time_point startTime,
                                                                double dt,
                                                                int counter)
{
  // Рахуємо, в який момент часу цей крок закінчитися з врахуванням timeScale
  double totalTargetTime = static_cast<double>(counter) * dt;

  // Перетворюємо totalTargetTime у duration на базі double
  auto durationOffset = std::chrono::duration<double>(totalTargetTime);

  // Додаємо до startTime. Тепер C++ збереже ідеальну точність у наносекундах
  return startTime + std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(durationOffset);
}