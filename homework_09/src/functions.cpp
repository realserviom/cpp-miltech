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
#include "Debug.h"

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

void saveFireCoordinates(double fireX, double fireY, double xd_i = 0, double yd_i = 0)
{
  std::ofstream outFile("../data/output.txt");

  if (outFile.is_open()) {
    if (xd_i) {
      outFile << xd_i << " " << yd_i << " ";
    }

    outFile << fireX << " " << fireY << std::endl;

    outFile.close();
    std::cout << "Дані успішно збережено у файл output.txt" << std::endl;
  }
  else {
    std::cerr << "Помилка: не вдалося відкрити файл для запису!" << std::endl;
  }
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

    // stepEntry["dropPoint"] = {{"x", it->dropPoint.x}, {"y", it->dropPoint.y}};
    // stepEntry["aimPoint"] = {{"x", it->aimPoint.x}, {"y", it->aimPoint.y}};
    // stepEntry["predictedTarget"] = {{"x", it->predictedTarget.x}, {"y", it->predictedTarget.y}};

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

double normalizeAngle(double angle)
{
  while (angle > M_PI)
    angle -= 2.0 * M_PI;
  while (angle < -M_PI)
    angle += 2.0 * M_PI;
  return angle;
}

// Coord predictTargetPosition2(std::stack<Target>& targetStack, float t_pol, float stepTime) const
// {
//   DEBUG("розраховуємо рухом по колу");
//   // Тимчасово витягуємо точки
//   Target t3 = targetStack.top();
//   targetStack.pop();
//   Target t2 = targetStack.top();
//   targetStack.pop();
//   Target t1 = targetStack.top();
//   targetStack.pop();

//   // повертаємо їх назад у стек (у зворотному порядку! пізніше переробимо через масив або щось інше придумаємо)
//   // не знаю як правильно чи можете підказати?
//   targetStack.push(t1);
//   targetStack.push(t2);
//   targetStack.push(t3);

//   double x1 = t1.pos.x, y1 = t1.pos.y;
//   double x2 = t2.pos.x, y2 = t2.pos.y;
//   double x3 = t3.pos.x, y3 = t3.pos.y;

//   // 1. Обчислюємо швидкості на двох відрізках
//   double v1x = (x2 - x1) / stepTime;
//   double v1y = (y2 - y1) / stepTime;

//   double v2x = (x3 - x2) / stepTime;
//   double v2y = (y3 - y2) / stepTime;

//   // 2. Обчислюємо прискорення
//   double ax = (v2x - v1x) / stepTime;
//   double ay = (v2y - v1y) / stepTime;

//   // 3. Прогнозуємо позицію за формулою кінематики
//   // S = V * t + (a * t^2) / 2

//   Coord predictedPos;
//   predictedPos.x = x3 + (v2x * t_pol) + (0.5 * ax * t_pol * t_pol);
//   predictedPos.y = y3 + (v2y * t_pol) + (0.5 * ay * t_pol * t_pol);

//   return predictedPos;
// }

// Функція для передбачення позиції
Coord predictTargetPosition(std::stack<Target>& targetStack, float t_pol, float stepTime)
{
  DEBUG("розраховуємо рухом по колу");
  // Тимчасово витягуємо точки
  Target t3 = targetStack.top();
  targetStack.pop();
  Target t2 = targetStack.top();
  targetStack.pop();
  Target t1 = targetStack.top();
  targetStack.pop();

  // повертаємо їх назад у стек (у зворотному порядку! пізніше переробимо через масив або щось інше придумаємо)
  // не знаю як правильно чи можете підказати?
  targetStack.push(t1);
  targetStack.push(t2);
  targetStack.push(t3);

  // Координати
  double x1 = t1.pos.x, y1 = t1.pos.y;
  double x2 = t2.pos.x, y2 = t2.pos.y;
  double x3 = t3.pos.x, y3 = t3.pos.y;

  // Знаходимо центр кола (xc, yc)
  double D = 2 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));

  // Якщо D == 0, точки лежать на одній прямій.
  if (std::abs(D) < 1e-6) {
    // Точки лежать на прямій, коло неможливо побудувати коло! Розраховуємо лінійно
    return t3.pos + t3.velocity * t_pol;
  }

  double sq1 = x1 * x1 + y1 * y1;
  double sq2 = x2 * x2 + y2 * y2;
  double sq3 = x3 * x3 + y3 * y3;

  double xc = (sq1 * (y2 - y3) + sq2 * (y3 - y1) + sq3 * (y1 - y2)) / D;
  double yc = (sq1 * (x3 - x2) + sq2 * (x1 - x3) + sq3 * (x2 - x1)) / D;

  // Радіус кола
  double R = std::hypot(x3 - xc, y3 - yc);

  // Знаходимо поточні кути для t2 і t3 відносно центру
  double angle2 = std::atan2(y2 - yc, x2 - xc);
  double angle3 = std::atan2(y3 - yc, x3 - xc);

  // Знаходимо зміну кута (кутову швидкість)
  double deltaAngle = normalizeAngle(angle3 - angle2);
  double angularVelocity = deltaAngle / stepTime;

  // Прогнозуємо новий кут через t_pol секунд
  double predictedAngle = angle3 + (angularVelocity * t_pol);

  // Розраховуємо координати прогнозованої точки
  Coord predictedPos;
  predictedPos.x = xc + R * std::cos(predictedAngle);
  predictedPos.y = yc + R * std::sin(predictedAngle);

  return predictedPos;
}