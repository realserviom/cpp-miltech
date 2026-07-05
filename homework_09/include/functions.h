#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "Types.h"
#include <cstdio>
#include <vector>
#include <chrono>



int getIndexByMinValue(const std::vector<float>& targetTimes);

void saveOutputFileByStep(int length, const std::vector<SimStep>& steps);

Coord normalize(const Coord& c);

float calculateLength(const Coord& c);

std::chrono::duration<float> getDurationTime(std::chrono::high_resolution_clock::time_point startTime, double dt);

std::chrono::high_resolution_clock::time_point getNextTimePoint(std::chrono::high_resolution_clock::time_point startTime,
                                                                double dt,
                                                                int counter);

template <typename T>
int countFilledElements(const T arr[], int size, T marker);

#include "functions.tpp" // Підключаємо файл з реалізацією шаблонів

#endif