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
#include <span>

// Визначення константи Пі, якщо її немає в cmath
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

auto main(int argc, char** argv) -> int
{
  auto args = std::span(argv, static_cast<size_t>(argc));

  if (argc < 3) {
    std::cerr << "Usage: " << args[0] << " <input_path> <output_path>" << "\n";
    return 1;
  }

  std::string outputPath = args[2];

  setlocale(LC_ALL, "");

  DroneInput input{};

  if (!readInputData(args[1], input)) {
    return 1;
  }

  const Ammunition* selectedAmmo = findAmmunition(input.name_ammo.data());

  if (selectedAmmo) {
    std::cout << "Знайдено боєприпас: " << selectedAmmo->name.data() << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Параметри: m=" << selectedAmmo->m << ", d=" << selectedAmmo->d << ", l=" << selectedAmmo->l << "\n";
  }
  else {
    std::cout << "Помилка: боєприпас " << input.name_ammo.data() << " не знайдено!" << "\n";
    return 1;
  }

  double t_pol = calculateFlightTime(selectedAmmo, input, GRAVIT);

  double dist = t_pol > 0 ? calculateDist(t_pol, input.V0, selectedAmmo->m, selectedAmmo->d, selectedAmmo->l, GRAVIT) : 0.00;  // h в умові

  if (dist < 0)
    dist = 0.0;

  std::cout << "Горизонтальна дистанція яку проходить дрон за час " << t_pol << " сек. рівна " << dist << " м." << "\n";

  double length = calculateLength(input.targetX, input.targetY, input.xd, input.yd);  // D в умові

  std::cout << "Відстань до цілі pівна " << length << " м." << "\n";

  double ratio = 0.0;
  double xd_i = 0.0;
  double yd_i = 0.0;
  double fireX = 0.0;
  double fireY = 0.0;

  // у випадку якщо в нас дрон над цілю
  if (std::abs(length) < EPSILON) {
    // координати проміжної точки
    // якщо висота 0 ми не повинні враховувати input.accelerationPath
    xd_i = input.targetX - dist - ((dist > 0) ? input.accelerationPath : 0);
    yd_i = input.targetY;

    // координати скиду
    fireX = input.targetX - dist;
    fireY = input.targetY;

    std::cout << "Проміжна точка xd_i, yd_i " << xd_i << ", " << yd_i << "\n";
    std::cout << "Точка скиду fireX, fireY " << fireX << ", " << fireY << "\n";

    saveFireCoordinates(outputPath, fireX, fireY, xd_i, yd_i);
  }
  else {
    if (dist + input.accelerationPath > length) {
      ratio = (dist + input.accelerationPath) / length;
      xd_i = input.targetX - (input.targetX - input.xd) * ratio;
      yd_i = input.targetY - (input.targetY - input.yd) * ratio;

      std::cout << "Проміжна точка xd_i, yd_i " << xd_i << ", " << yd_i << "\n";
    }

    ratio = (length - dist) / length;                       // ~ (140 - 60) / 140
    fireX = input.xd + (input.targetX - input.xd) * ratio;  // 100 + (200 - 100) * ratio = 157
    fireY = input.yd + (input.targetY - input.yd) * ratio;  // 100 + (200 - 100) * ratio = 157

    std::cout << "Точка скиду fireX, fireY " << fireX << ", " << fireY << "\n";

    saveFireCoordinates(outputPath, fireX, fireY, xd_i, yd_i);
  }

  return 0;
}