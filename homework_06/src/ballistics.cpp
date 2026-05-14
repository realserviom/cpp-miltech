#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <string.h>
#include <fstream>
#include "types.hpp"
#include <unistd.h>  // Для getcwd
#include <limits.h>  // Для PATH_MAX
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <algorithm>
#include <iomanip>

auto calculateLength(float targetX, float targetY, float xd, float yd) -> double
{
  return std::sqrt(std::pow((targetX - xd), 2) + std::pow((targetY - yd), 2));
}

/**
 * Функція для відстані що проходить боєприпас після скиду за розширеною моделлю 5-го степеня
 * @param t  - поточний час польоту
 * @param V0 - початкова швидкість (attackSpeed)
 * @param m  - маса боєприпасу
 * @param d  - коефіцієнт опору (drag)
 * @param l  - коефіцієнт планування (lift)
 * @param g  - гравітація (за замовчуванням 9.81)
 * @return   - відстань
 */
auto calculateDist(double t, double V0, double m, double d, double l, double g = 9.81) -> double
{
  // Допоміжні змінні для швидкості та чистоти коду
  double l2 = l * l;
  double l2_plus_1 = l2 + 1.0;
  double m2 = m * m;
  double m3 = m2 * m;
  double m4 = m3 * m;

  double d2 = d * d;
  double d3 = d2 * d;
  double d4 = d3 * d;

  // Розрахунок окремих доданків (термів)
  double term_t1 = t * V0;
  double term_t2 = -(d * std::pow(t, 2) * V0) / (2.0 * m);
  double term_t3 = (std::pow(t, 3) * (6.0 * d * g * l * m - 6.0 * d2 * (l2 - 1.0) * V0)) / (36.0 * m2);

  double num_t4 =
    3.0 * d3 * l2_plus_1 * l2 * V0 + 6.0 * d3 * l2_plus_1 * std::pow(l, 4) * V0 - 6.0 * d2 * g * (std::pow(l, 4) + l2 + 1.0) * l * m;

  double term_t4 = (std::pow(t, 4) * num_t4) / (36.0 * std::pow(l2_plus_1, 2) * m3);

  double term_t5 = (std::pow(t, 5) * (3.0 * d3 * g * std::pow(l, 3) * m - 3.0 * d4 * l2 * l2_plus_1 * V0)) / (36.0 * l2_plus_1 * m4);

  // Повертаємо суму всіх частин
  return term_t3 + term_t5 + term_t4 + term_t2 + term_t1;
}

void saveFireCoordinates(std::string outputPath, double fireX, double fireY, double xd_i = 0, double yd_i = 0)
{
  std::ofstream outFile(outputPath);

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

auto findAmmunition(std::string_view name) -> const Ammunition*
{
  for (const auto& ammo : ammoTable) {
    if (ammo.name.data() == name) {
      return &ammo;
    }
  }
  return nullptr;
}

auto readInputData(std::string_view filename, DroneInput& data) -> bool
{
  // 1. Відкриваємо файл
  std::ifstream file(filename.data());

  if (!file.is_open()) {
    std::cerr << "Помилка: Не вдалося відкрити файл " << filename << "\n";
    std::cerr << "Поточна робоча директорія: " << std::filesystem::current_path() << "\n";
    return false;
  }

  // 2. Зчитуємо дані
  // Тимчасовий рядок для назви, щоб безпечно перенести її в std::array
  std::string temp_ammo_name;

  if (!(file >> data.xd >> data.yd >> data.zd >> data.targetX >> data.targetY >> data.V0 >> data.accelerationPath >> temp_ammo_name)) {
    std::cerr << "Помилка: Файл має неправильний формат або неповні дані.\n";
    return false;
  }

  // 3. Безпечно копіюємо назву в std::array (name_ammo)
  data.name_ammo.fill('\0');  // Очищуємо масив
  std::copy_n(temp_ammo_name.begin(), std::min(temp_ammo_name.size(), data.name_ammo.size() - 1), data.name_ammo.begin());

  // 4. Вивід даних (використовуємо iomanip для точності)
  std::cout << "=== Вхідні дані з файлу ===\n" << std::fixed << std::setprecision(2);
  std::cout << "Дрон (xd, yd, zd): " << data.xd << ", " << data.yd << ", " << data.zd << " м\n";
  std::cout << "Ціль (targetX, Y): " << data.targetX << ", " << data.targetY << "\n";
  std::cout << "Швидкість V0: " << data.V0 << " м/с, Розгін: " << data.accelerationPath << " м\n";
  std::cout << "Боєприпас: " << data.name_ammo.data() << "\n";
  std::cout << "===========================\n";

  return true;
}

double calculateFlightTime(const Ammunition* selectedAmmo, const DroneInput& input, double gravit)
{
  if (!selectedAmmo)
    return -1.0;

  if (input.zd < 1e-3) {
    return 0.0;
  }

  double a, b, c;
  // Обчислення параметрів a, b, c
  a = (selectedAmmo->d * gravit * selectedAmmo->m) - (2 * std::pow(selectedAmmo->d, 2) * selectedAmmo->l * input.V0);
  b = -3.0 * gravit * std::pow(selectedAmmo->m, 2) + 3.0 * selectedAmmo->d * selectedAmmo->l * selectedAmmo->m * input.V0;
  c = 6.0 * std::pow(selectedAmmo->m, 2) * input.zd;

  if (std::abs(a) < 1e-12) {
    std::cerr << "Помилка: Це не кубічне рівняння (a = 0)." << std::endl;
    return -1.0;
  }

  // Обчислення допоміжних параметрів p та q
  double p = -std::pow(b, 2) / (3.0 * std::pow(a, 2));
  double q = (2.0 * std::pow(b, 3)) / (27.0 * std::pow(a, 3)) + (c / a);

  // Перевірка умови для трьох дійсних коренів (p < 0)
  if (p >= -1e-12) {
    std::cerr << "Помилка: p має бути < 0 для даного методу." << std::endl;
    return -1.0;
  }

  double arg = (3.0 * q / (2.0 * p)) * std::sqrt(-3.0 / p);

  // Перевірка діапазону для acos [-1; 1]
  if (arg < -1.0 || arg > 1.0) {
    std::cerr << "Помилка: Аргумент acos (" << arg << ") поза діапазоном [-1, 1]." << std::endl;
    return -1.0;
  }

  // Використовуємо M_PI з cmath (або 3.141592653589793)
  double t_pol = 2.0 * std::sqrt(-p / 3.0) * std::cos((std::acos(arg) + 4.0 * M_PI) / 3.0) - (b / (3.0 * a));

  if (t_pol <= 0) {
    std::cerr << "Помилка: Розрахований час польоту від'ємний або нуль." << std::endl;
    return -1.0;
  }

  return t_pol;
}

// void test()
// {
//   int* p = NULL;                     // Старий стиль, modernize-use-nullptr має спрацювати
//   char* buffer = (char*)malloc(10);  // C-style cast та malloc
// }