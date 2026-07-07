#include <cstdint>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <string.h>
#include <fstream>
#include "functions.h"
#include "RollingTargetStack.h"
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

    // Округлення до 2 знаків після коми

    float angle = std::round(it->direction * 100.0) / 100.0;

    if (angle < 0) {
      angle += 2.0 * M_PI;
    }

    stepEntry["direction"] = angle;

    stepEntry["state"] = it->state;
    stepEntry["targetIndex"] = it->targetIdx;

    // Округлення до 2 знаків після коми
    stepEntry["timeSecSinceStart"] = std::round(it->timeSecSinceStart * 100.0) / 100.0;

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

Coord predictTargetPosition(const RollingTargetStack& targetStack, float t_pol, float stepTime, int target)
{
  std::size_t stackSize = targetStack.size(target);

  std::cout << "\n================ [DEBUG TARGETSTACK: " << target << "] ================\n";
  std::cout << "Кількість точок у стеку (stackSize): " << stackSize << "\n\n";

  std::cout << "Кількість m_maxSize у (stackSize): " << targetStack.m_maxSize << "\n\n";

  if (stackSize < targetStack.m_maxSize) {
    const auto& lastPoint = targetStack.top(target);
    return lastPoint.pos + lastPoint.velocity * t_pol;
  }

  const auto& t1 = targetStack.at(target, 0);
  const auto& t2 = targetStack.at(target, targetStack.m_maxSize / 2);
  const auto& t3 = targetStack.at(target, targetStack.m_maxSize - 1);

  const auto& t11 = targetStack.at(target, 1);
  const auto& t12 = targetStack.at(target, 2);
  const auto& t13 = targetStack.at(target, 3);
  const auto& t14 = targetStack.at(target, 4);
  const auto& t15 = targetStack.at(target, 5);
  const auto& t16 = targetStack.at(target, 6);
  const auto& t17 = targetStack.at(target, 7);
  const auto& t18 = targetStack.at(target, 8);
  const auto& t19 = targetStack.at(target, 9);
  const auto& t110 = targetStack.at(target, 10);
  const auto& t111 = targetStack.at(target, 11);
  const auto& t112 = targetStack.at(target, 12);
  const auto& t113 = targetStack.at(target, 13);
  const auto& t114 = targetStack.at(target, 14);
  const auto& t115 = targetStack.at(target, 15);
  const auto& t116 = targetStack.at(target, 16);
  const auto& t117 = targetStack.at(target, 17);
  const auto& t118 = targetStack.at(target, 18);
  const auto& t119 = targetStack.at(target, 19);
  const auto& t120 = targetStack.at(target, 20);

  double x1 = t1.pos.x, y1 = t1.pos.y;
  double x2 = t2.pos.x, y2 = t2.pos.y;
  double x3 = t3.pos.x, y3 = t3.pos.y;

  // Обчислюємо швидкості на двох відрізках
  double v1x = (x2 - x1);
  double v1y = (y2 - y1);

  double v2x = (x3 - x2);
  double v2y = (y3 - y2);

  // Обчислюємо прискорення
  double ax = (v2x - v1x);
  double ay = (v2y - v1y);

  std::cout << "=== TARGET LOGS ===" << std::endl;
  std::cout << "Positions:\n"
            << "  t1 (Oldest):  x = " << x1 << ",\ty = " << y1 << "\n"
            << "  t2 (Mid):     x = " << x2 << ",\ty = " << y2 << "\n"
            << "  t3 (Newest):  x = " << x3 << ",\ty = " << y3 << "\n"
            << "  t1 (Oldest+1):  x = " << t11.pos.x << ",\ty = " << t11.pos.y << "\n"
            << "  t1 (Oldest+2):  x = " << t12.pos.x << ",\ty = " << t12.pos.y << "\n"
            << "  t1 (Oldest+3):  x = " << t13.pos.x << ",\ty = " << t13.pos.y << "\n"
            << "  t1 (Oldest+4):  x = " << t14.pos.x << ",\ty = " << t14.pos.y << "\n"
            << "  t1 (Oldest+5):  x = " << t15.pos.x << ",\ty = " << t15.pos.y << "\n"
            << "  t1 (Oldest+6):  x = " << t16.pos.x << ",\ty = " << t16.pos.y << "\n"
            << "  t1 (Oldest+7):  x = " << t17.pos.x << ",\ty = " << t17.pos.y << "\n"
            << "  t1 (Oldest+8):  x = " << t18.pos.x << ",\ty = " << t18.pos.y << "\n"
            << "  t1 (Oldest+9):  x = " << t19.pos.x << ",\ty = " << t19.pos.y << "\n"
            << "  t1 (Oldest+10): x = " << t110.pos.x << ",\ty = " << t110.pos.y << "\n"
            << "  t1 (Oldest+11): x = " << t111.pos.x << ",\ty = " << t111.pos.y << "\n"
            << "  t1 (Oldest+12): x = " << t112.pos.x << ",\ty = " << t112.pos.y << "\n"
            << "  t1 (Oldest+13): x = " << t113.pos.x << ",\ty = " << t113.pos.y << "\n"
            << "  t1 (Oldest+14): x = " << t114.pos.x << ",\ty = " << t114.pos.y << "\n"
            << "  t1 (Oldest+15): x = " << t115.pos.x << ",\ty = " << t115.pos.y << "\n"
            << "  t1 (Oldest+16): x = " << t116.pos.x << ",\ty = " << t116.pos.y << "\n"
            << "  t1 (Oldest+17): x = " << t117.pos.x << ",\ty = " << t117.pos.y << "\n"
            << "  t1 (Oldest+18): x = " << t118.pos.x << ",\ty = " << t118.pos.y << "\n"
            << "  t1 (Oldest+19): x = " << t119.pos.x << ",\ty = " << t119.pos.y << "\n"
            << "  t1 (Oldest+20): x = " << t120.pos.x << ",\ty = " << t120.pos.y << "\n\n";

  std::cout << "Velocities:\n"
            << "  v1 (t1->t2):  x = " << v1x << ",\ty = " << v1y << "\n"
            << "  v2 (t2->t3):  x = " << v2x << ",\ty = " << v2y << "\n\n";

  std::cout << "Acceleration:\n"
            << "  ax = " << ax << ",\tay = " << ay << "\n";
  std::cout << "===================" << std::endl;

  // Прогнозуємо позицію за формулою кінематики
  Coord predictedPos;
  predictedPos.x = x3 + (v2x * t_pol) + (0.5 * ax * t_pol * t_pol);
  predictedPos.y = y3 + (v2y * t_pol) + (0.5 * ay * t_pol * t_pol);

  return predictedPos;
}

// Функція приведення кута до діапазону [-PI; PI]
double normalizeAngle(float angle)
{
  while (angle > M_PI)
    angle -= 2.0 * M_PI;
  while (angle < -M_PI)
    angle += 2.0 * M_PI;
  return angle;
}