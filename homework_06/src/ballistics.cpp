#include <iostream>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include "types.hpp"
#include <unistd.h>  // Для getcwd
#include <climits>   // Для PATH_MAX
#include <filesystem>
#include <cstring>
#include <string_view>
#include <algorithm>
#include <iomanip>

// NOLINTBEGIN(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)
auto calculate_length(DroneInput& input) -> double
{
  return std::sqrt(std::pow((input.target_x_ - input.xd_), 2) + std::pow((input.target_y_ - input.yd_), 2));
}
// NOLINTEND(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)

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
// NOLINTBEGIN(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers, readability-identifier-length)
auto calculate_dist(double t, double v0, double m, double d, double l, double g = kGravit) -> double
{
  if (t <= 0.00) {
    return 0.00;
  }

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
  double term_t1 = t * v0;
  double term_t2 = -(d * std::pow(t, 2) * v0) / (2.0 * m);
  double term_t3 = (std::pow(t, 3) * (6.0 * d * g * l * m - 6.0 * d2 * (l2 - 1.0) * v0)) / (36.0 * m2);

  double num_t4 =
    3.0 * d3 * l2_plus_1 * l2 * v0 + 6.0 * d3 * l2_plus_1 * std::pow(l, 4) * v0 - 6.0 * d2 * g * (std::pow(l, 4) + l2 + 1.0) * l * m;

  double term_t4 = (std::pow(t, 4) * num_t4) / (36.0 * std::pow(l2_plus_1, 2) * m3);

  double term_t5 = (std::pow(t, 5) * (3.0 * d3 * g * std::pow(l, 3) * m - 3.0 * d4 * l2 * l2_plus_1 * v0)) / (36.0 * l2_plus_1 * m4);

  // Повертаємо суму всіх частин
  double dist = term_t3 + term_t5 + term_t4 + term_t2 + term_t1;

  return (dist <= 0.00) ? 0.00 : dist;
}
// NOLINTEND(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers, readability-identifier-length)

void save_fire_coordinates(std::string& output_path, DropPoints& points)
{
  std::ofstream out_file(output_path);

  if (out_file.is_open()) {
    if (points.is_middle_point_) {
      out_file << points.xd_i_ << " " << points.yd_i_ << " ";
    }

    out_file << points.fire_x_ << " " << points.fire_y_ << '\n';

    out_file.close();
    std::cout << "Дані успішно збережено у файл output.txt" << '\n';
  }
  else {
    std::cerr << "Помилка: не вдалося відкрити файл для запису!" << '\n';
  }
}

auto find_ammunition(std::string_view name) -> const Ammunition*
{
  for (const auto& ammo : kAmmoTable) {
    if (ammo.name_.data() == name) {
      return &ammo;
    }
  }
  return nullptr;
}

auto read_input_data(std::string_view filename, DroneInput& data) -> bool
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

  if (!(file >> data.xd_ >> data.yd_ >> data.zd_ >> data.target_x_ >> data.target_y_ >> data.v0_ >> data.acceleration_path_ >>
        temp_ammo_name)) {
    std::cerr << "Помилка: Файл має неправильний формат або неповні дані.\n";
    return false;
  }

  // 3. Безпечно копіюємо назву в std::array (name_ammo)
  data.name_ammo_.fill('\0');  // Очищуємо масив
  std::copy_n(temp_ammo_name.begin(), std::min(temp_ammo_name.size(), data.name_ammo_.size() - 1), data.name_ammo_.begin());

  // 4. Вивід даних (використовуємо iomanip для точності)
  std::cout << "=== Вхідні дані з файлу ===\n" << std::fixed << std::setprecision(2);
  std::cout << "Дрон (xd, yd, zd): " << data.xd_ << ", " << data.yd_ << ", " << data.zd_ << " м\n";
  std::cout << "Ціль (targetX, Y): " << data.target_x_ << ", " << data.target_y_ << "\n";
  std::cout << "Швидкість V0: " << data.v0_ << " м/с, Розгін: " << data.acceleration_path_ << " м\n";
  std::cout << "Боєприпас: " << data.name_ammo_.data() << "\n";
  std::cout << "===========================\n";

  return true;
}

// NOLINTBEGIN(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers, readability-identifier-length)
auto calculate_flight_time(const Ammunition* selected_ammo, const DroneInput& input, double gravit) -> double
{
  if (selected_ammo == nullptr) {
    return -1.0;
  }

  if (input.zd_ < 1e-3) {
    return 0.0;
  }

  double a = 0.00;
  double b = 0.00;
  double c = 0.00;

  // Обчислення параметрів a, b, c
  a = (selected_ammo->d_ * gravit * selected_ammo->m_) - (2 * std::pow(selected_ammo->d_, 2) * selected_ammo->l_ * input.v0_);
  b = -3.0 * gravit * std::pow(selected_ammo->m_, 2) + 3.0 * selected_ammo->d_ * selected_ammo->l_ * selected_ammo->m_ * input.v0_;
  c = 6.0 * std::pow(selected_ammo->m_, 2) * input.zd_;

  if (std::abs(a) < 1e-12) {
    std::cerr << "Помилка: Це не кубічне рівняння (a = 0)." << '\n';
    return -1.0;
  }

  // Обчислення допоміжних параметрів p та q
  double p = -std::pow(b, 2) / (3.0 * std::pow(a, 2));
  double q = (2.0 * std::pow(b, 3)) / (27.0 * std::pow(a, 3)) + (c / a);

  // Перевірка умови для трьох дійсних коренів (p < 0)
  if (p >= -1e-12) {
    std::cerr << "Помилка: p має бути < 0 для даного методу." << '\n';
    return -1.0;
  }

  double arg = (3.0 * q / (2.0 * p)) * std::sqrt(-3.0 / p);

  // Перевірка діапазону для acos [-1; 1]
  if (arg < -1.0 || arg > 1.0) {
    std::cerr << "Помилка: Аргумент acos (" << arg << ") поза діапазоном [-1, 1]." << '\n';
    return -1.0;
  }

  // Використовуємо M_PI з cmath (або 3.141592653589793)
  double t_pol = 2.0 * std::sqrt(-p / 3.0) * std::cos((std::acos(arg) + 4.0 * M_PI) / 3.0) - (b / (3.0 * a));

  if (t_pol <= 0) {
    std::cerr << "Помилка: Розрахований час польоту від'ємний або нуль." << '\n';
    return -1.0;
  }

  return t_pol;
}
// NOLINTEND(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers, readability-identifier-length)

// Нова оптимізована функція розрахунку координатів скиду
auto calculate_drop_points(const DroneInput& input, double dist, double length, double epsilon) -> DropPoints
{
  DropPoints points;

  // Перевіряємо, чи дрон безпосередньо над ціллю
  if (std::abs(length) < epsilon) {
    // Якщо висота 0 (або дрон на місці), acceleration_path_ не враховуємо
    double accel = (dist > 0) ? input.acceleration_path_ : 0.0;

    points.xd_i_ = input.target_x_ - dist - accel;
    points.yd_i_ = input.target_y_;
    points.fire_x_ = input.target_x_ - dist;
    points.fire_y_ = input.target_y_;
    points.is_middle_point_ = true;
  }
  else {
    // Ініціалізуємо дефолтні значення для xd_i, yd_i (на випадок, якщо умова if нижче не виконається)
    points.xd_i_ = input.xd_;
    points.yd_i_ = input.yd_;

    if (dist + input.acceleration_path_ > length) {
      double ratio_i = (dist + input.acceleration_path_) / length;
      points.xd_i_ = input.target_x_ - (input.target_x_ - input.xd_) * ratio_i;
      points.yd_i_ = input.target_y_ - (input.target_y_ - input.yd_) * ratio_i;
      points.is_middle_point_ = true;
    }

    double ratio_fire = (length - dist) / length;
    points.fire_x_ = input.xd_ + (input.target_x_ - input.xd_) * ratio_fire;
    points.fire_y_ = input.yd_ + (input.target_y_ - input.yd_) * ratio_fire;
  }

  return points;
}