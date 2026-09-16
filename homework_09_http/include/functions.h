#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "Types.h"
#include <cstdio>
#include <vector>
#include <unistd.h>

void saveFireCoordinates(double fireX, double fireY, double xd_i, double yd_i);
#include <chrono>
#include "RollingTargetStack.h"
#include <json.hpp>
#include <chrono>

using json_noordered = nlohmann::json;

int getIndexByMinValue(const std::vector<float>& targetTimes);

Coord normalize(const Coord& c);

float calculateLength(const Coord& c);

double normalizeAngle(float angle);

std::chrono::duration<float> getDurationTime(std::chrono::high_resolution_clock::time_point startTime, double dt);

std::chrono::high_resolution_clock::time_point getNextTimePoint(const std::chrono::high_resolution_clock::time_point startTime,
                                                                double dt,
                                                                int counter);

double normalizeAngle(double angle);

Coord predictTargetPosition(const RollingTargetStack& targetStack, float t_pol, float stepTime, int target);

bool sendSimulationResults(const std::string& testId);
bool checkSimulationResults(const std::string& testId);
int openUart(const char* dev);

template <typename T>
int countFilledElements(const T arr[], int size, T marker);

#include "functions.tpp" // Підключаємо файл з реалізацією шаблонів

#endif