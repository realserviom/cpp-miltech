#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "types.h"
#include <cstdio>
#include <vector>


float calculateLength(double targetX, double targetY, double xd, double yd);

void saveFireCoordinates(double fireX, double fireY, double xd_i, double yd_i);

int getIndexByMinValue(std::vector<float>& targetTimes);

inline const char* getDroneStateName(DroneState state) {
    switch (state) {
        case STOPPED:      return "STOPPED";
        case ACCELERATING: return "ACCELERATING";
        case DECELERATING: return "DECELERATING";
        case TURNING:      return "TURNING";
        case MOVING:       return "MOVING";
        default:           return "UNKNOWN";
    }
}

void saveOutputFileByStep(int length, SimStep* steps);

Coord normalize(const Coord& c);

float length(const Coord& c);

template <typename T>
int countFilledElements(const T arr[], int size, T marker);

#include "functions.tpp" // Підключаємо файл з реалізацією шаблонів

#endif