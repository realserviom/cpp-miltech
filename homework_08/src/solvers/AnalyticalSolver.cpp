#include <iostream>
#include "Types.h"
#include "constants.h"
#include "solvers/AnalyticalSolver.h"
#include <cmath>
#include <iomanip>


double AnalyticalSolver::getDistDuringFall(float &t_pol, DroneConfig &myDrone, const AmmoParams *ammo) {
    const double g = GRAVIT;
    const double d = ammo->drag;
    const double l = ammo->lift;
    const double m = ammo->mass;
    const double V0 = myDrone.attackSpeed;

    t_pol = this->getTimePol(d, l, m, myDrone.altitude, V0);

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
    double term_t1 = t_pol * V0;
    double term_t2 = -(d * std::pow(t_pol, 2) * V0) / (2.0 * m);
    double term_t3 = (std::pow(t_pol, 3) * (6.0 * d * g * l * m - 6.0 * d2 * (l2 - 1.0) * V0)) / (36.0 * m2);

    double num_t4 = 3.0 * d3 * l2_plus_1 * l2 * V0 
                + 6.0 * d3 * l2_plus_1 * std::pow(l, 4) * V0 
                - 6.0 * d2 * g * (std::pow(l, 4) + l2 + 1.0) * l * m;
                
    double term_t4 = (std::pow(t_pol, 4) * num_t4) / (36.0 * std::pow(l2_plus_1, 2) * m3);

    double term_t5 = (std::pow(t_pol, 5) * (3.0 * d3 * g * std::pow(l, 3) * m - 3.0 * d4 * l2 * l2_plus_1 * V0)) / (36.0 * l2_plus_1 * m4);

    // Повертаємо суму всіх частин
    return term_t3 + term_t5 + term_t4 + term_t2 + term_t1;
}

float AnalyticalSolver::getTimePol(const float &d, const float &l, const float &m, const float &zd, const float &attackSpeed) {

    float a, b, c;
    
    // Обчислення параметрів a,b,c
    a = (d * GRAVIT * m) - (2 * std::pow(d, 2) * l * attackSpeed);
    b = -3.0 * GRAVIT * std::pow(m, 2) + 3.0 * d * l * m * attackSpeed;
    c = 6.0 * std::pow(m, 2) * zd;
    
    //std::cout << "\nПараметри a,b,c:\na = " << a << "\nb = " << b << "\nc = " << c << std::endl;

    if (a == 0) {
        throw std::runtime_error("Це не кубічне рівняння (a = 0). Original a =" + std::to_string(a));
    }

    // Обчислення допоміжних параметрів p та q
    float p = -std::pow(b, 2) / (3.0 * std::pow(a, 2));
    float q = (2.0 * std::pow(b, 3)) / (27.0 * std::pow(a, 3)) + (c / a);

    std::cout << std::fixed << std::setprecision(4);
    //std::cout << "\nПроміжні параметри:\np = " << p << "\nq = " << q << std::endl;

    // Перевірка умови для трьох дійсних коренів (p < 0)
    if (p >= -1e-7f) {
        throw std::runtime_error("Модель спрощена: p має бути < 0 для даного методу.");
    }

    float arg = (3.0 * q / (2.0 * p)) * std::sqrt(-3.0 / p);

    // Перевірка діапазону для acos [-1; 1]
    if (arg < -1.0 || arg > 1.0) {
        throw std::runtime_error("Помилка: Аргумент acos (" + std::to_string(arg) + ") поза діапазоном [-1, 1].");
    }

    float t_pol = 2.0 * std::sqrt(-p / 3.0) * std::cos((std::acos(arg) + 4.0 * M_PI) / 3.0) - (b / (3.0 * a));
    
    // std::cout << "-----------------------------------" << std::endl;
    // std::cout << "Результат (час польоту t_pol): " << t_pol << " сек." << std::endl;
    // std::cout << "-----------------------------------" << std::endl;
    
    if (t_pol <= 0) {
        throw std::runtime_error("Час повинен бути додатній.");
    }
    
    return t_pol;
}

