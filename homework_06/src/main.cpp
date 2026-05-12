/******************************************************************************

Welcome to GDB Online.
GDB online is an online compiler and debugger tool for C, C++, Python, Java, PHP, Ruby, Perl,
C#, OCaml, VB, Swift, Pascal, Fortran, Haskell, Objective-C, Assembly, HTML, CSS, JS, SQLite, Prolog.
Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include "types.hpp"
#include "ballistics.hpp"

// Визначення константи Пі, якщо її немає в cmath
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(int argc, char** argv)
{
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <input_path> <output_path>" << std::endl;
    return 1;
  }

  std::string outputPath = argv[2];

  const double GRAVIT = 9.81;  // Стандартне значення в м/с²

  setlocale(LC_ALL, "");

  DroneInput input;

  if (!readInputData(argv[1], input)) {
    return 1;
  }

  const Ammunition* selectedAmmo = findAmmunition(input.name_ammo);

  if (selectedAmmo) {
    printf("Знайдено боєприпас: %s\n", selectedAmmo->name);
    printf("Параметри: m=%.2f, d=%.2f, l=%.2f\n", selectedAmmo->m, selectedAmmo->d, selectedAmmo->l);
  }
  else {
    printf("Помилка: боєприпас %s не знайдено в базі!\n", input.name_ammo);
    return 1;
  }

  double t_pol = calculateFlightTime(selectedAmmo, input, GRAVIT);

  std::cout << "\n-----------------------------------" << std::endl;
  std::cout << "Результат (час польоту t_pol): " << t_pol << " сек." << std::endl;
  std::cout << "-----------------------------------" << std::endl;

  if (t_pol <= 0) {
    std::cout << "Час повинен бути додатній." << std::endl;
    return 1;
  }

  double dist = calculateDist(t_pol, input.V0, selectedAmmo->m, selectedAmmo->d, selectedAmmo->l, GRAVIT);  // h в умові

  std::cout << "Горизонтальна дистанція яку проходить дрон за час " << t_pol << " сек. рівна " << dist << " м." << std::endl;

  if (dist <= 0) {
    std::cout << "Горизонтальна дистанція повинна бути додатня" << std::endl;
    return 1;
  }

  double length = calculateLength(input.targetX, input.targetY, input.xd, input.yd);  // D в умові

  std::cout << "Відстань до цілі pівна " << length << " м." << std::endl;

  double ratio;
  double xd_i = 0;
  double yd_i = 0;
  double fireX;
  double fireY;

  if (length == 0.0) {
    xd_i = input.targetX - dist - input.accelerationPath;
    yd_i = 0;
    fireX = input.targetX - dist;
    fireY = 0;

    std::cout << "Проміжна точка xd_i, yd_i " << xd_i << ", " << yd_i << std::endl;
    std::cout << "Точка скиду fireX, fireY " << fireX << ", " << fireY << std::endl;

    saveFireCoordinates(outputPath, fireX, fireY, xd_i, yd_i);
  }
  else {
    if (dist + input.accelerationPath > length) {
      ratio = (dist + input.accelerationPath) / length;
      xd_i = input.targetX - (input.targetX - input.xd) * ratio;
      yd_i = input.targetY - (input.targetY - input.yd) * ratio;

      std::cout << "Проміжна точка xd_i, yd_i " << xd_i << ", " << yd_i << std::endl;
    }

    ratio = (length - dist) / length;                       // ~ (140 - 60) / 140
    fireX = input.xd + (input.targetX - input.xd) * ratio;  // 100 + (200 - 100) * ratio = 157
    fireY = input.yd + (input.targetY - input.yd) * ratio;  // 100 + (200 - 100) * ratio = 157

    std::cout << "Точка скиду fireX, fireY " << fireX << ", " << fireY << std::endl;

    saveFireCoordinates(outputPath, fireX, fireY, xd_i, yd_i);
  }

  return 0;
}