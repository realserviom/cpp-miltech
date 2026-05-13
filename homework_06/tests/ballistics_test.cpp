#include <gtest/gtest.h>
#include "ballistics.hpp"
#include "types.hpp"

#ifndef DATA_PATH
#error "DATA_PATH is not defined! Check your CMakeLists.txt"
#endif

#ifndef FIXTURES_PATH
#error "FIXTURES_PATH is not defined! Check your CMakeLists.txt"
#endif

TEST(BallisticsMath, TestIsWorking)
{
  EXPECT_TRUE(true) << "тест TEST is working!";
}

// Тест на випадок, якщо файл не існує
TEST(BallisticsInputTest, FileNotFound)
{
  DroneInput input;

  // Передаємо неіснуючий шлях
  bool result = readInputData("non_existent.txt", input);

  EXPECT_FALSE(result);
}

// Тест на некоректний формат даних у файлі
TEST(BallisticsInputTest, ReadCorruptedFile)
{
  DroneInput input;

  // FIXTURES_PATH прилетить із CMake
  std::string fullPath = std::string(FIXTURES_PATH) + "test_corrupted.txt";

  printf("Помилка: fullPath файл %s\n", fullPath.c_str());

  // Очікуємо, що метод поверне помилку або кине виключення
  EXPECT_FALSE(readInputData(fullPath.c_str(), input));
}

// Тест на випадок, якщо боєприпас не існує
TEST(BallisticsInputTest, UnknownAmmunitionType)
{
  // Шукаємо те, чого точно немає (поки що)
  auto result = findAmmunition("Nuclear_Bomb");

  // Якщо повертає nullptr тест виконується
  EXPECT_EQ(result, nullptr);
}

// Тест провірка на додатній час падіння
TEST(BallisticsMath, calculateFlightTime)
{
  // Шукаємо тестовий боєприпас
  auto ammo = findAmmunition("TEST-AMMO");

  DroneInput input;

  // Передаємо тествову конфігурацію
  std::string fullPath = std::string(FIXTURES_PATH) + "test_ammo.txt";
  bool result = readInputData(fullPath.c_str(), input);

  double flightTime = calculateFlightTime(ammo, input, GRAVIT);

  EXPECT_GT(flightTime, 0);
}

// Тест провірка на обробку даних при висоті дрона = 0
TEST(BallisticsMath, ZD_EQ_ZERO)
{
  // Шукаємо тестовий боєприпас
  auto ammo = findAmmunition("TEST-AMMO");

  DroneInput input;

  // Передаємо тествову конфігурацію для висоти 0
  std::string fullPath = std::string(FIXTURES_PATH) + "test_ammo_zd0.txt";
  bool result = readInputData(fullPath.c_str(), input);

  double flightTime = calculateFlightTime(ammo, input, GRAVIT);

  EXPECT_EQ(flightTime, 0);
}

// Тест провірка на обробку даних при висоті дрона < 0
TEST(BallisticsMath, ZD_LESS_ZERO)
{
  // Шукаємо тестовий боєприпас
  auto ammo = findAmmunition("TEST-AMMO");

  DroneInput input;

  // Передаємо тествову конфігурацію для висоти 0
  std::string fullPath = std::string(FIXTURES_PATH) + "test_ammo_zd_less0.txt";
  bool result = readInputData(fullPath.c_str(), input);

  double flightTime = calculateFlightTime(ammo, input, GRAVIT);

  EXPECT_EQ(flightTime, 0);
}

TEST(BallisticsMath, ZeroVelocity3)
{
  // EXPECT_EQ(calculateDistance(0, 45), 0);
}