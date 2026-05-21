#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "types.h"
#include <cstdio>
#include <vector>


float calculateLength(double targetX, double targetY, double xd, double yd);

void saveFireCoordinates(double fireX, double fireY, double xd_i, double yd_i);

int getIndexByMinValue(std::vector<float>& targetTimes);

inline float calculateSmallArrivalTime(float distance, float attackSpeed, float curDroneSpeed,  float acceleration) {
    
    // рівномірний рух
    if(curDroneSpeed == attackSpeed) {
        return distance / attackSpeed;
    }
    
    // рівноприскорений рух
 
    float v0 = curDroneSpeed;
    float a = acceleration;
    float d = distance;
    
    // (a*t^2)/2 + Vo*t - S = 0;
    // Розвяжемо квадратне рівняння 
    // Дискримінант D = b^2 - 4ac
    float D = v0 * v0 + 2.0f * a * d;

    if (D < 0) return distance / attackSpeed; // На всякий випадок, якщо фізично неможливо

    return (-v0 + std::sqrt(D)) / a;
}

inline float calculateArrivalTime(float distance, float tTurn, float tAccel, float accelPath, float distFall, float speed) {
    return tTurn + tAccel + ((distance - accelPath - distFall) / speed);
}

inline void updateDronePosition(const DroneConfig &myDrone, CurrentDroneParameters &curMyDrone) {
    Coord direction = { (float)cos(curMyDrone.angularState), (float)sin(curMyDrone.angularState) };
    
    Coord velocity = direction * (float)curMyDrone.speed;
    Coord acceleration = direction * (float)myDrone.acceleration;
    
    float dt = (float)myDrone.simTimeStep;
    float stepSq = (dt * dt) / 2.0f;

    if (curMyDrone.state == MOVING) {
        curMyDrone.pos = curMyDrone.pos + (velocity * dt);
    } 
    else if (curMyDrone.state == ACCELERATING) {
        curMyDrone.pos = curMyDrone.pos + (velocity * dt) + (acceleration * stepSq);
    }
    else if (curMyDrone.state == DECELERATING) {
        curMyDrone.pos = curMyDrone.pos + (velocity * dt) - (acceleration * stepSq);
    }
}

// обертаємо дрон згідно нового напрямку
inline bool updateRotation(float targetAngle, float &curAngularState, float radInIteration, float turnThreshold = 0) {
    
    // якщо швидкість повороту в радіанах велика 
    // тобто за одну ітерацію більша ніж різниця положень тоді проставляємо зразу вірний кут
    
    if(std::abs(targetAngle - curAngularState) < radInIteration) {
        curAngularState = targetAngle;
        return false;
    }
    
    if((targetAngle - curAngularState) > turnThreshold) {
        // крутимо проти годинникової стрілки
        curAngularState += radInIteration;
        return true;
        
    } else if((curAngularState - targetAngle) > turnThreshold) {
        // крутимо за годинниковою стрілою
        curAngularState -= radInIteration;
        return true;
    }
    
    return false;
}

// провіряємо чи треба дрон обертати
inline bool needDroneRotation(float targetAngle, float curAngularState, float turnThreshold) {
    return (targetAngle - curAngularState) > turnThreshold || (curAngularState - targetAngle) > turnThreshold;
}

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