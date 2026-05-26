#pragma once
#include "Drone.h"
#include "./interfaces/IBallisticSolver.h"
#include "./interfaces/IConfigLoader.h"
#include "./interfaces/ITargetProvider.h"
#include <vector>


class MissionProcessor {
private:
    // Вказівники на наші стратегії
    ITargetProvider* m_targetProvider = nullptr;
    IBallisticSolver* m_solver = nullptr;
    IConfigLoader* m_configLoader = nullptr;

    // масив який містить час підльоту до кожної цілі
    std::vector<float> targetTimes;
        
    // масив який містить відстань під поточного положення дрону до кожної цілі
    std::vector<float> targetDistances;
        
    // масив який містить кут напрямку для кожної цілі відносто осі X в радіанах 
    std::vector<float> targetAngles;   


    Drone init(DroneConfig& myDrone, const AmmoParams*& ammo, int& numberCounterInTimeSpot, int&numberOfTargets);

public:
    MissionProcessor(ITargetProvider* targetProvider = nullptr, 
                     IBallisticSolver* solver = nullptr, 
                     IConfigLoader* configLoader = nullptr)
        : m_targetProvider(targetProvider), m_solver(solver), m_configLoader(configLoader) {}

    void setTargetProvider(ITargetProvider* targetProvider);
    void setBallisticSolver(IBallisticSolver* solver);
    void setConfigLoader(IConfigLoader* configLoader);
    void executeMission();
};