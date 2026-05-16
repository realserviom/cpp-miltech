/******************************************************************************

Welcome to GDB Online.
GDB online is an online compiler and debugger tool for C, C++, Python, Java, PHP, Ruby, Perl,
C#, OCaml, VB, Swift, Pascal, Fortran, Haskell, Objective-C, Assembly, HTML, CSS, JS, SQLite, Prolog.
Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "types.hpp"
#include "ballistics.hpp"
#include <iomanip>
#include <vector>

// Визначення константи Пі, якщо її немає в cmath
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

auto main(int argc, char** argv) -> int
{
  std::vector<std::string> args(argv, argv + argc);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

  if (argc < 3) {
    std::cerr << "Usage: " << args[0] << " <input_path> <output_path>" << "\n";
    return 1;
  }

  std::string output_path = args[2];

  setlocale(LC_ALL, "");

  DroneInput input{};

  if (!read_input_data(args[1], input)) {
    return 1;
  }

  const Ammunition* selected_ammo = find_ammunition(input.name_ammo_.data());

  if (selected_ammo != nullptr) {
    std::cout << "Знайдено боєприпас: " << selected_ammo->name_.data() << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Параметри: m=" << selected_ammo->m_ << ", d=" << selected_ammo->d_ << ", l=" << selected_ammo->l_ << "\n";
  }
  else {
    std::cout << "Помилка: боєприпас " << input.name_ammo_.data() << " не знайдено!" << "\n";
    return 1;
  }

  double t_pol = calculate_flight_time(selected_ammo, input, kGravit);

  double dist =
    t_pol > 0 ? calculate_dist(t_pol, input.v0_, selected_ammo->m_, selected_ammo->d_, selected_ammo->l_, kGravit) : 0.00;  // h в умові

  if (dist < 0) {
    dist = 0.0;
  }

  std::cout << "Горизонтальна дистанція яку проходить дрон за час " << t_pol << " сек. рівна " << dist << " м." << "\n";

  double length = calculate_length(input);  // D в умові

  std::cout << "Відстань до цілі pівна " << length << " м." << "\n";

  DropPoints points = calculate_drop_points(input, dist, length, kEpsilon);

  if (points.is_middle_point_) {
    std::cout << "Проміжна точка xd_i, yd_i " << points.xd_i_ << ", " << points.yd_i_ << "\n";
  }
  std::cout << "Точка скиду fireX, fireY " << points.fire_x_ << ", " << points.fire_y_ << "\n";

  save_fire_coordinates(output_path, points);

  return 0;
}