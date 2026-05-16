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
  bool result = read_input_data("non_existent.txt", input);

  EXPECT_FALSE(result);
}

// Тест на некоректний формат даних у файлі
TEST(BallisticsInputTest, ReadCorruptedFile)
{
  DroneInput input;

  // FIXTURES_PATH прилетить із CMake
  std::string full_path = std::string(FIXTURES_PATH) + "test_corrupted.txt";

  printf("Помилка: fullPath файл %s\n", full_path.c_str());

  // Очікуємо, що метод поверне помилку або кине виключення
  EXPECT_FALSE(read_input_data(full_path.c_str(), input));
}

// Тест на випадок, якщо боєприпас не існує
TEST(BallisticsInputTest, UnknownAmmunitionType)
{
  // Шукаємо те, чого точно немає (поки що)
  auto result = find_ammunition("Nuclear_Bomb");

  // Якщо повертає nullptr тест виконується
  EXPECT_EQ(result, nullptr);
}

// Тест провірка на додатній час падіння
TEST(BallisticsMath, calculateFlightTime)
{
  // Шукаємо тестовий боєприпас
  auto ammo = find_ammunition("TEST-AMMO");

  DroneInput input;

  // Передаємо тествову конфігурацію
  std::string fullPath = std::string(FIXTURES_PATH) + "test_ammo.txt";
  bool result = read_input_data(fullPath.c_str(), input);

  double flight_time = calculate_flight_time(ammo, input, kGravit);

  EXPECT_GT(flight_time, 0);
}

// Тест провірка на обробку даних при висоті дрона = 0
TEST(BallisticsMath, ZD_EQ_ZERO)
{
  // Шукаємо тестовий боєприпас
  auto ammo = find_ammunition("TEST-AMMO");

  DroneInput input;

  // Передаємо тествову конфігурацію для висоти 0
  std::string fullPath = std::string(FIXTURES_PATH) + "test_ammo_zd0.txt";
  bool result = read_input_data(fullPath.c_str(), input);

  double flightTime = calculate_flight_time(ammo, input, kGravit);

  EXPECT_EQ(flightTime, 0);
}

// Тест провірка на обробку даних при висоті дрона < 0
TEST(BallisticsMath, ZD_LESS_ZERO)
{
  // Шукаємо тестовий боєприпас
  auto ammo = find_ammunition("TEST-AMMO");

  DroneInput input;

  // Передаємо тествову конфігурацію для висоти 0
  std::string fullPath = std::string(FIXTURES_PATH) + "test_ammo_zd_less0.txt";
  bool result = read_input_data(fullPath.c_str(), input);

  double flightTime = calculate_flight_time(ammo, input, kGravit);

  EXPECT_EQ(flightTime, 0);
}

// тест для порівняння результатів через EXPECT_FLOAT_EQ
TEST(BallisticsMath, CALCULATE_POINTS)
{
  auto ammo = find_ammunition("M67");

  DroneInput input;

  std::string fullPath = std::string(FIXTURES_PATH) + "test_ammo.txt";
  bool result = read_input_data(fullPath.c_str(), input);

  if (!result) {
    FAIL() << "Не вдалося зчитати вхідні дані з файлу: " << fullPath;
  }

  double t_pol = calculate_flight_time(ammo, input, kGravit);

  double dist = calculate_dist(t_pol, input.v0_, ammo->m_, ammo->d_, ammo->l_, kGravit);

  double length = calculate_length(input);

  DropPoints points = calculate_drop_points(input, dist, length, kEpsilon);

  EXPECT_FLOAT_EQ(points.fire_x_, 173.859);
  EXPECT_FLOAT_EQ(points.fire_y_, 173.859);
  EXPECT_FALSE(points.is_middle_point_);
}

// Один негативний тест
TEST(BallisticsMath, ZD_LESS_ZERO_NEGATIVE)
{
  // Шукаємо тестовий боєприпас
  auto ammo = find_ammunition("TEST-AMMO");

  DroneInput input;

  // Передаємо тествову конфігурацію для висоти 0
  std::string fullPath = std::string(FIXTURES_PATH) + "test_ammo_zd_less0.txt";
  bool result = read_input_data(fullPath.c_str(), input);

  float flight_time = static_cast<float>(calculate_flight_time(ammo, input, kGravit));

  EXPECT_FLOAT_EQ(flight_time, 1);
}