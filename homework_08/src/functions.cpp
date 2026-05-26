#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <string.h>
#include <fstream>
#include "functions.h"
#include "Types.h"
#include <stdbool.h>
#include "../include/json.hpp"

using json = nlohmann::ordered_json;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// в нас 60 точок часу і 5 цілей це 300 комбінацій
#define CACHE_SIZE 1024
   

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

void saveFireCoordinates(double fireX, double fireY, double xd_i = 0, double yd_i = 0) {
    
    std::ofstream outFile("../data/output.txt");

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

// Структура для комірки кешу
typedef struct {
    bool is_active;      // Чи є тут запис
    char key[64];        // Оригінальний рядок (щоб перевіряти колізії)
    float result;        // Сам закешований результат
} CacheEntry;


int getIndexByMinValue(std::vector<float>& targetTimes) {
    int minIndex = 0;

    // 2. Починаємо перевірку з другого елемента (індекс 1)
    for (size_t i = 1; i < targetTimes.size(); i++) {
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

    std::ofstream fout("../data/output.json");
    fout << out.dump(2);
    fout.close();
}

