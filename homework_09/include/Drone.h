#pragma once
#include "Types.h"
#include <memory>
#include <vector>

class IDroneState;

class Drone {
public:
    Coord pos;               // поточна позиція (x, y)
    float speed;             // поточна швидкість
    float angularState;      // поточне положення дрона відносно осі x
    std::unique_ptr<IDroneState> state;  // поточний стан
    int target;              // поточна ціль
    Coord dropPoint;         // точка скиду
    Coord aimPoint;          // куди впаде бомба
    Coord predictedTarget;   // прогнозована позиція цілі

    DroneConfig config;

    explicit Drone(const DroneConfig& config);

    bool updateRotation(float targetAngle, float turnThreshold = 0.0f);

    void updatePosition();

    bool needRotation(float targetAngle, float turnThreshold = 0.0f) const;

    float calculateArrivalTime(float targetAngle, float distance, float distFall) const;

    float calculateSmallArrivalTime(float distance) const;

    void move(const std::vector<float>& targetTimes, const bool& canChangeTarget, const std::vector<float>& targetAngles);

    float changeTarget(const std::vector<float>& targetTimes, const bool& canChangeTarget, const std::vector<float>& targetAngles);
};