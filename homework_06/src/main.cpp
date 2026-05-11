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
#include <iomanip>
#include <string.h>
#include <fstream>


// Визначення константи Пі, якщо її немає в cmath
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


double calculateLength(float targetX, float targetY, float xd, float yd) {
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
double calculateDist(double t, double V0, double m, double d, double l, double g = 9.81) {
    
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

    double num_t4 = 3.0 * d3 * l2_plus_1 * l2 * V0 
                  + 6.0 * d3 * l2_plus_1 * std::pow(l, 4) * V0 
                  - 6.0 * d2 * g * (std::pow(l, 4) + l2 + 1.0) * l * m;
                  
    double term_t4 = (std::pow(t, 4) * num_t4) / (36.0 * std::pow(l2_plus_1, 2) * m3);
   
    double term_t5 = (std::pow(t, 5) * (3.0 * d3 * g * std::pow(l, 3) * m - 3.0 * d4 * l2 * l2_plus_1 * V0)) / (36.0 * l2_plus_1 * m4);

    // Повертаємо суму всіх частин
    return term_t3 + term_t5 + term_t4 + term_t2 + term_t1;
}

void saveFireCoordinates(double fireX, double fireY, double xd_i = 0, double yd_i = 0) {
    
    std::ofstream outFile("output.txt");

    if (outFile.is_open()) {
        
        if (xd_i) {
            outFile << xd_i << " " << yd_i << " ";
        }
        
        outFile << fireX << " " << fireY << std::endl;
        
        outFile.close();
        std::cout << "Дані успішно збережено у файл output.txt" << std::endl;
    } else {
        std::cerr << "Помилка: не вдалося відкрити файл для запису!" << std::endl;
    }
    
    
}


int main()
{
    
    const double GRAVIT = 9.81; // Стандартне значення в м/с²

    setlocale(LC_ALL, ""); 
    
    typedef char def_name_ammo[50];
    typedef char def_type_ammo[30];

    typedef struct {
        def_name_ammo name; // Назва
        float m;            // m (кг)
        float d;            // d (drag)
        float l;            // l (lift)
        def_type_ammo type; // Тип (Вільне падіння / Планеруючий)
    } Ammunition;

    Ammunition ammoTable[] = {
        {"VOG-17",      0.35, 0.07, 0.0, "Вільний"},
        {"M67",         0.6,  0.10, 0.0, "Вільний"},
        {"RKG-3",       1.2,  0.10, 0.0, "Вільний"},
        {"GLIDING-VOG", 0.45, 0.10, 1.0, "Планеруючий"},
        {"GLIDING-RKG", 1.4,  0.10, 1.0, "Планеруючий"}
    };
    
    
    // Змінні для збереження даних
    float xd, yd, zd;
    float targetX, targetY;
    float V0;
    float accelerationPath;
    def_name_ammo name_ammo; // Використовуємо той самий тип тут
    

    // Відкриваємо файл для читання ("r" - read)
    FILE *file = fopen("input.txt", "r");

    // Перевірка, чи файл відкрився
    if (file == NULL) {
        printf("Помилка: Не вдалося відкрити файл input.txt\n");
        return 1;
    }

    // Зчитуємо дані згідно з форматом у прикладі: 
    // 100 100 100 200 200 10 10 VOG-17
    // %f - для float, %s - для рядка (ammo_name)
    if (fscanf(file, "%f %f %f %f %f %f %f %s", 
               &xd, &yd, &zd, 
               &targetX, &targetY, 
               &V0, 
               &accelerationPath, 
               name_ammo) == 8) {
        
        // Вивід розшифрованих параметрів у консоль
        printf("=== Вхідні дані з файлу ===\n");
        printf("Координати дрона (xd, yd, zd): %.2f, %.2f, %.2f м\n", xd, yd, zd);
        printf("Висота скиду (zd):             %.2f м\n", zd);
        printf("Координати цілі (targetX, Y): %.2f, %.2f\n", targetX, targetY);
        printf("Швидкість атаки:              %.2f м/с\n", V0);
        printf("Довжина розгону:              %.2f м\n", accelerationPath);
        printf("Назва боєприпасу:             %s\n", name_ammo);
        printf("===========================\n");
        
    } else {
        printf("Помилка: Файл має неправильний формат або неповні дані.\n");
    }
    
    int tableSize = sizeof(ammoTable) / sizeof(ammoTable[0]);
    float m = 0, d = 0, l = 0;
    bool found = false;
    
    for (int i = 0; i < tableSize; i++) {
        if (strcmp(ammoTable[i].name, name_ammo) == 0) {
            m = ammoTable[i].m;
            d = ammoTable[i].d;
            l = ammoTable[i].l;
            found = true;
            break;
        }
    }
    
    if (found) {
        printf("Знайдено боєприпас: %s\n", name_ammo);
        printf("Параметри: m=%.2f, d=%.2f, l=%.2f\n", m, d, l);
    } else {
        printf("Помилка: боєприпас %s не знайдено в базі!\n", name_ammo);
        return 1;
    }
    
    
    double a, b, c;
    // Обчислення параметрів a,b,c
    a = (d * GRAVIT * m) - (2 * std::pow(d, 2) * l * V0);
    b = -3.0 * GRAVIT * std::pow(m, 2) + 3.0 * d * l * m * V0;
    c = 6.0 * std::pow(m, 2) * zd;
    
    std::cout << "\nПараметри a,b,c:\na = " << a << "\nb = " << b << "\nc = " << c << std::endl;

    if (a == 0) {
        std::cout << "Це не кубічне рівняння (a = 0). Original a =" << a << std::endl;
        return 1;
    }

    // Обчислення допоміжних параметрів p та q
    double p = -std::pow(b, 2) / (3.0 * std::pow(a, 2));
    double q = (2.0 * std::pow(b, 3)) / (27.0 * std::pow(a, 3)) + (c / a);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "\nПроміжні параметри:\np = " << p << "\nq = " << q << std::endl;

    // Перевірка умови для трьох дійсних коренів (p < 0)
    if (p >= -1e-12) {
        std::cout << "Модель спрощена: p має бути < 0 для даного методу." << std::endl;
        return 1;
    }

    double arg = (3.0 * q / (2.0 * p)) * std::sqrt(-3.0 / p);

    // Перевірка діапазону для acos [-1; 1]
    if (arg < -1.0 || arg > 1.0) {
        std::cout << "Помилка: Аргумент acos (" << arg << ") поза діапазоном [-1, 1]." << std::endl;
        return 1;
    }

    double t_pol = 2.0 * std::sqrt(-p / 3.0) * std::cos((std::acos(arg) + 4.0 * M_PI) / 3.0) - (b / (3.0 * a));
    
    std::cout << "\n-----------------------------------" << std::endl;
    std::cout << "Результат (час польоту t_pol): " << t_pol << " сек." << std::endl;
    std::cout << "-----------------------------------" << std::endl;
    
    if (t_pol <= 0) {
        std::cout << "Час повинен бути додатній." << std::endl;
        return 1;
    }
    
    fclose(file);
    
    double dist = calculateDist(t_pol, V0, m, d, l, GRAVIT); // h в умові

    std::cout << "Горизонтальна дистанція яку проходить дрон за час " << t_pol << " сек. рівна " << dist << " м." << std::endl;
    
    if (dist <= 0) {
        std::cout << "Горизонтальна дистанція повинна бути додатня" << std::endl;
        return 1;
    }
    
    double length = calculateLength(targetX, targetY, xd, yd); // D в умові
    
    std::cout << "Відстань до цілі pівна " << length << " м." << std::endl;
    
    double ratio;
    double xd_i = 0;
    double yd_i = 0;
    double fireX;
    double fireY;
    
    if (length == 0.0) {
        xd_i = targetX - dist - accelerationPath;
        yd_i = 0;
        fireX = targetX - dist;
        fireY = 0;
        
        std::cout << "Проміжна точка xd_i, yd_i " << xd_i << ", " << yd_i << std::endl;
        std::cout << "Точка скиду fireX, fireY " << fireX << ", " << fireY << std::endl;
        
        saveFireCoordinates(fireX, fireY, xd_i, yd_i );
    } else {
    
        if (dist + accelerationPath > length) {
            ratio = (dist + accelerationPath) / length;
            xd_i = targetX - (targetX - xd) * ratio; 
            yd_i = targetY - (targetY - yd) * ratio;
            
            std::cout << "Проміжна точка xd_i, yd_i " << xd_i << ", " << yd_i << std::endl;
        }
        
        ratio = (length - dist) / length;    // ~ (140 - 60) / 140
        fireX = xd + (targetX - xd) * ratio; // 100 + (200 - 100) * ratio = 157
        fireY = yd + (targetY - yd) * ratio;  // 100 + (200 - 100) * ratio = 157
        
        std::cout << "Точка скиду fireX, fireY " << fireX << ", " << fireY << std::endl;
        
        saveFireCoordinates(fireX, fireY, xd_i, yd_i );
    }
    
    return 0;
}