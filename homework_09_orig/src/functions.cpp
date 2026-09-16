#include <stdlib.h>
#include <cmath>
#include "functions.h"
#include "RollingTargetStack.h"
#include "Types.h"
#include <stdbool.h>
#include <fcntl.h>
#include <termios.h>
#include <chrono>
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

// Функція налаштування UART
int openUart(const char* dev)
{
  int fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd < 0) {
    std::perror("Помилка відкриття UART");
    return -1;
  }

  termios tio{};
  tcgetattr(fd, &tio);
  cfmakeraw(&tio);  // 8N1, сирий бінарний режим
  cfsetispeed(&tio, B115200);
  cfsetospeed(&tio, B115200);  // швидкість 115200
  tio.c_cflag |= (CLOCAL | CREAD);
  tcsetattr(fd, TCSANOW, &tio);

  return fd;
}

Coord predictTargetPosition(const RollingTargetStack& targetStack, float t_pol, float stepTime, int target)
{
  std::size_t stackSize = targetStack.size(target);

  if (stackSize < targetStack.m_maxSize) {
    const auto& lastPoint = targetStack.top(target);
    return lastPoint.pos + lastPoint.velocity * t_pol;
  }

  const auto& t1 = targetStack.at(target, 0);
  const auto& t2 = targetStack.at(target, targetStack.m_maxSize / 2);
  const auto& t3 = targetStack.at(target, targetStack.m_maxSize - 1);

  double x1 = t1.pos.x, y1 = t1.pos.y;
  double x2 = t2.pos.x, y2 = t2.pos.y;
  double x3 = t3.pos.x, y3 = t3.pos.y;

  // Обчислюємо швидкості на двох відрізках
  double v1x = (x2 - x1);
  double v1y = (y2 - y1);

  // DEBUG("v1x: " << std::fixed << std::setprecision(15) << v1x << ", v1y: " << std::fixed << std::setprecision(15) << v1y);

  double v2x = (x3 - x2);
  double v2y = (y3 - y2);

  // Обчислюємо прискорення
  double ax = (v2x - v1x);
  double ay = (v2y - v1y);

  double v_mod = std::sqrt(v2x * v2x + v2y * v2y);
  double scaleX = 1.0;
  double scaleY = 1.0;

  if (v_mod > 0.0001) {
    scaleX = 0.5 / (1.0 + std::abs(v2x) / v_mod);
    scaleY = 0.5 / (1.0 + std::abs(v2y) / v_mod);
  }

  // Прогнозуємо позицію за формулою кінематики
  Coord predictedPos;
  predictedPos.x = x3 + (v2x * t_pol) + (0.5 * ax * t_pol * t_pol) * scaleX;
  predictedPos.y = y3 + (v2y * t_pol) + (0.5 * ay * t_pol * t_pol) * scaleY;

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