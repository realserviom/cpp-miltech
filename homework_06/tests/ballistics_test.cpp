#include <gtest/gtest.h>
#include "ballistics.hpp"
#include "types.hpp"

#ifndef DATA_PATH
#error "DATA_PATH is not defined! Check your CMakeLists.txt"
#endif

TEST(BallisticsMath, TestIsWorking)
{
  EXPECT_TRUE(true) << "тест TEST is working!";
}

TEST(BallisticsMath, ZeroVelocity)
{
  // EXPECT_EQ(calculateDistance(0, 45), 0);
}

TEST(BallisticsMath, ZeroVelocity2)
{
  // EXPECT_EQ(calculateDistance(0, 45), 0);
}

TEST(BallisticsMath, ZeroVelocity3)
{
  // EXPECT_EQ(calculateDistance(0, 45), 0);
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

  // DATA_PATH прилетить із CMake
  std::string fullPath = std::string(DATA_PATH) + "/test_corrupted.txt";

  printf("Помилка: fullPath файл %s\n", fullPath.c_str());

  // Очікуємо, що метод поверне помилку або кине виключення
  EXPECT_FALSE(readInputData(fullPath.c_str(), input));
}

// Тест на випадок, якщо боєприпас не існує
TEST(BallisticsInputTest, UnknownAmmunitionType)
{
  // Шукаємо те, чого точно немає
  auto result = findAmmunition("Nuclear_Bomb");

  // Якщо повертає nullptr тест виконується
  EXPECT_EQ(result, nullptr);
}