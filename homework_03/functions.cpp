#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <iomanip>
#include <string.h>
#include <fstream>
#include "constants.h"
#include "functions.h"
#include "types.h"
#include <stdbool.h>
#include "json.hpp"
#include "debug.h"

using json = nlohmann::ordered_json;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// в нас 60 точок часу і 5 цілей це 300 комбінацій
#define CACHE_SIZE 1024

    // static const AmmoParams ammoTable[] = {
    //     {"VOG-17",      0.35f, 0.07f, 0.0f},
    //     {"M67",         0.60f, 0.10f, 0.0f},
    //     {"RKG-3",       1.20f, 0.10f, 0.0f},
    //     {"GLIDING-VOG", 0.45f, 0.10f, 1.0f},
    //     {"GLIDING-RKG", 1.40f, 0.10f, 1.0f}
    // };
    
void fillAmmoData(AmmoParams* ammo) {
    // Індекс 0
    strcpy(ammo[0].name, "VOG-17");
    ammo[0].mass = 0.35f; ammo[0].drag = 0.07f; ammo[0].lift = 0.0f;

    // Індекс 1
    strcpy(ammo[1].name, "M67");
    ammo[1].mass = 0.60f; ammo[1].drag = 0.10f; ammo[1].lift = 0.0f;

    // Індекс 2
    strcpy(ammo[2].name, "RKG-3");
    ammo[2].mass = 1.20f; ammo[2].drag = 0.10f; ammo[2].lift = 0.0f;

    // Індекс 3
    strcpy(ammo[3].name, "GLIDING-VOG");
    ammo[3].mass = 0.45f; ammo[3].drag = 0.10f; ammo[3].lift = 1.0f;

    // Індекс 4
    strcpy(ammo[4].name, "GLIDING-RKG");
    ammo[4].mass = 1.40f; ammo[4].drag = 0.10f; ammo[4].lift = 1.0f;
}

const AmmoParams* getAmmoParameters(AmmoParams* ammoTable, int tableSize, const char* name_to_find) {

    for (int i = 0; i < tableSize; i++) {
        if (strcmp(ammoTable[i].name, name_to_find) == 0) {
            return &ammoTable[i];
        }
    }

    return nullptr;
}

bool getDataFromInputTxtFile(DroneConfig &config) {

    std::ifstream fin("config.json");

    if (!fin.is_open()) {
        std::cout << "Помилка: Не вдалося відкрити файл config.json! Перевірте, чи він існує." << std::endl;
        return false;
    }
    
    json j;
    try {
        fin >> j;
    } catch (const json::parse_error& e) {
        std::cout << "Помилка парсингу JSON: " << e.what() << std::endl;
        return false;
    }

    config.startPos.x    = j["drone"]["position"]["x"];
    config.startPos.y    = j["drone"]["position"]["y"];
    config.altitude      = j["drone"]["altitude"];
    config.initialDir    = j["drone"]["initialDirection"];
    config.attackSpeed   = j["drone"]["attackSpeed"];
    config.accelPath     = j["drone"]["accelerationPath"];
    config.angularSpeed  = j["drone"]["angularSpeed"];
    config.turnThreshold = j["drone"]["turnThreshold"];
    config.arrayTimeStep = j["targetArrayTimeStep"];
    config.simTimeStep   = j["simulation"]["timeStep"];
    config.hitRadius     = j["simulation"]["hitRadius"];

    strncpy(config.ammoName, j["ammo"].get<std::string>().c_str(), sizeof(config.ammoName) - 1);
    
    DEBUG("=== Вхідні дані з файлу (структура) ===");
    DEBUG("Координати (x, y):    " << std::fixed << std::setprecision(2) << config.startPos.x << " м, " << config.startPos.y << " м");
    DEBUG("Висота (altitude):    " << config.altitude << " м");
    DEBUG("Напрямок (dir):       " << config.initialDir << " рад.");
    DEBUG("Швидкість атаки:      " << config.attackSpeed << " м/c");
    DEBUG("Шлях розгону:         " << config.accelPath << " м");
    DEBUG("Боєприпас:            " << config.ammoName);
    DEBUG("Крок часу масиву:     " << config.arrayTimeStep << " с");
    DEBUG("Крок симуляції:       " << config.simTimeStep << " с");
    DEBUG("Радіус ураження:      " << config.hitRadius << " м");
    DEBUG("Кутова швидкість:     " << config.angularSpeed << " рад/c");
    DEBUG("Поріг повороту:       " << config.turnThreshold << " рад");
    DEBUG("---------------------------------------");

    fin.close();
    return true;
}


Coord** loadTargetCoordinates(int &targetCount) {

    std::ifstream fin("targets.json");

    if (!fin.is_open()) {
        std::cout << "Помилка: Не вдалося відкрити файл targets.json! Перевірте, чи він існує." << std::endl;
        return nullptr;
    }
    
    json j;
    try {
        fin >> j;
    } catch (const json::parse_error& e) {
        std::cout << "Помилка парсингу JSON: " << e.what() << std::endl;
        return nullptr;
    }

    targetCount = j["targetCount"];
    int timeSteps = j["timeSteps"];

    Coord** targets = new Coord*[targetCount];

    for (int i = 0; i < targetCount; i++) {
        targets[i] = new Coord[timeSteps];
        auto& positionsJson = j["targets"][i]["positions"];
        
        for (int t = 0; t < timeSteps; t++) {
            targets[i][t].x = positionsJson[t]["x"];
            targets[i][t].y = positionsJson[t]["y"];
        }
    }
    
    fin.close();
    return targets;
}


bool loadTargetCoordinates_old(double targetX[NUMBER_OF_TARGETS][NUMBER_OF_TIMES], 
    double targetY[NUMBER_OF_TARGETS][NUMBER_OF_TIMES]) {
    FILE *file = fopen("targets.txt", "r");

    if (file == NULL) {
        printf("Помилка: Не вдалося відкрити файл targets.txt\n");
        return false;
    }

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 60; j++) {
            fscanf(file, "%lf", &targetX[i][j]);
        }
    }
    
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 60; j++) {
            fscanf(file, "%lf", &targetY[i][j]);
        }
    }
    
    fclose(file);
    return true;
}

bool getTimePol(float &t_pol, const float &d, const float &l, const float &m, const float &zd, const float &attackSpeed) {

    float a, b, c;
    
    // Обчислення параметрів a,b,c
    a = (d * physics::GRAVIT * m) - (2 * std::pow(d, 2) * l * attackSpeed);
    b = -3.0 * physics::GRAVIT * std::pow(m, 2) + 3.0 * d * l * m * attackSpeed;
    c = 6.0 * std::pow(m, 2) * zd;
    
    //std::cout << "\nПараметри a,b,c:\na = " << a << "\nb = " << b << "\nc = " << c << std::endl;

    if (a == 0) {
        std::cout << "Це не кубічне рівняння (a = 0). Original a =" << a << std::endl;
        return false;
    }

    // Обчислення допоміжних параметрів p та q
    float p = -std::pow(b, 2) / (3.0 * std::pow(a, 2));
    float q = (2.0 * std::pow(b, 3)) / (27.0 * std::pow(a, 3)) + (c / a);

    std::cout << std::fixed << std::setprecision(4);
    //std::cout << "\nПроміжні параметри:\np = " << p << "\nq = " << q << std::endl;

    // Перевірка умови для трьох дійсних коренів (p < 0)
    if (p >= -1e-7f) {
        std::cout << "Модель спрощена: p має бути < 0 для даного методу." << std::endl;
        return false;
    }

    float arg = (3.0 * q / (2.0 * p)) * std::sqrt(-3.0 / p);

    // Перевірка діапазону для acos [-1; 1]
    if (arg < -1.0 || arg > 1.0) {
        std::cout << "Помилка: Аргумент acos (" << arg << ") поза діапазоном [-1, 1]." << std::endl;
        return false;
    }

    t_pol = 2.0 * std::sqrt(-p / 3.0) * std::cos((std::acos(arg) + 4.0 * M_PI) / 3.0) - (b / (3.0 * a));
    
    // std::cout << "-----------------------------------" << std::endl;
    // std::cout << "Результат (час польоту t_pol): " << t_pol << " сек." << std::endl;
    // std::cout << "-----------------------------------" << std::endl;
    
    if (t_pol <= 0) {
        std::cout << "Час повинен бути додатній." << std::endl;
        return false;
    }
    
    return true;
}

float calculateLength(double targetX, double targetY, double xd, double yd) {
    return std::sqrt(std::pow((targetX - xd), 2) + std::pow((targetY - yd), 2));
}

Coord normalize(const Coord& c) {
    float L = std::hypot(c.x, c.y);
    if (L < 1e-6f) return {0, 0};
    return c / L;
}

float length(const Coord& c) {
    return std::hypot(c.x, c.y);
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
double calculateDistDuringFall(double t, double V0, double m, double d, double l, double g = 9.81) {
    
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

int getTimeIteration(int &counter, const int &numberCounterInTimeSpot, const int &numberOfTimes) {
    
    // wholeRangeCounters число всіх ітерацій за весь період руху дрона протягом numberOfTimes міток часу
    const int wholeRangeCounters = numberCounterInTimeSpot * numberOfTimes; 
    
    // якщо в нас поточна ітерація більша чи рівна wholeRangeCounters тоді берем остачу від цілочисленого
    // ділення на wholeRangeCounters
    const int new_counter = counter >= wholeRangeCounters ?
        static_cast<int>(counter % wholeRangeCounters) : counter;

    return static_cast<int>(std::floor(new_counter / numberCounterInTimeSpot));
}

int getNextIteration(int iteration) {
    
    if (iteration == (NUMBER_OF_TIMES - 1)) {
        return 0;
    }
    
    return iteration + 1;
}


// Класична та дуже швидка функція хешування для рядків (алгоритм djb2)
// використаємо для повернення відстанні між точками
unsigned long hash_string(const char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}


// Структура для комірки кешу
typedef struct {
    bool is_active;      // Чи є тут запис
    char key[64];        // Оригінальний рядок (щоб перевіряти колізії)
    float result;        // Сам закешований результат
} CacheEntry;


float calculateLengthWithCache(double &targetX, double &targetY, double &xd, double &yd) {
    
    static CacheEntry cache[CACHE_SIZE];
    char current_key[64];

    // Створюємо ключ із вхідних даних (4 параметри через підкреслення)
    // Використовуємо %.8f для дробних чисел, щоб ключ був стабільним
    snprintf(current_key, sizeof(current_key), "%.8f_%.8f_%.8f_%.8f", targetX, targetY, xd, yd);
    
    long cache_current_key = hash_string(current_key);
    
    printf("--- cache_current_key = %lu  ---\n", cache_current_key);

    unsigned long index = cache_current_key % CACHE_SIZE;
    
    printf("--- long index = %lu \n", index);

    // Перевіряємо кеш
    if (cache[index].is_active && strcmp(cache[index].key, current_key) == 0) {
        printf("--- cache is ---\n");
        return cache[index].result; // Повертаємо метри з кешу
    }

    // Якщо в кеші немає — рахуємо
    float length = std::sqrt(std::pow((targetX - xd), 2) + std::pow((targetY - yd), 2));
    
    // Зберігаємо в кеш
    cache[index].is_active = true;
    // копіювання строки current_key бо байтам в структуру
    strcpy(cache[index].key, current_key);
    cache[index].result = length;

    return length;
}

int getIndexByMinValue(float targetTimes[5]) {
    int minIndex = 0;
    
    int size = 5;

    // 2. Починаємо перевірку з другого елемента (індекс 1)
    for (int i = 1; i < size; i++) {
        // Якщо знаходимо елемент, який менший за наш поточний мінімум
        if (targetTimes[i] < targetTimes[minIndex]) {
            minIndex = i; // Запам'ятовуємо новий індекс
        }
    }
    
    return minIndex;
}

void saveOutputFileByStep(int length, SimStep* steps)
{
    
    json out;
    out["totalSteps"] = length + 1;

    printf("============== length = %d ===========\n", length + 1);
    out["steps"] = json::array();

    for (int i = 0; i <= length; i++) {
        json stepEntry;
        
        stepEntry["position"] = {
            {"x", steps[i].pos.x}, 
            {"y", steps[i].pos.y}
        };
        
        stepEntry["direction"] = steps[i].direction;
        stepEntry["state"] = steps[i].state;
        stepEntry["targetIndex"] = steps[i].targetIdx;

        
        stepEntry["dropPoint"]       = {{"x", steps[i].dropPoint.x}, {"y", steps[i].dropPoint.y}};
        stepEntry["aimPoint"]        = {{"x", steps[i].aimPoint.x}, {"y", steps[i].aimPoint.y}};
        stepEntry["predictedTarget"] = {{"x", steps[i].predictedTarget.x}, {"y", steps[i].predictedTarget.y}};
       
        out["steps"].push_back(stepEntry);
    }

    std::ofstream fout("output.json");
    fout << out.dump(2);
    fout.close();
}

