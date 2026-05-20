#pragma once
#include "JsonTargetProvider.h"
#include "AnalyticalSolver.h"
#include "FileConfigLoader.h"
#include <iostream>

class MissionProcessor {
private:
    // Вказівники на наші стратегії
    ITargetProvider* m_targetProvider = nullptr;
    IBallisticSolver* m_solver = nullptr;
    IConfigLoader* m_configLoader = nullptr;

public:
    MissionProcessor(ITargetProvider* targetProvider = nullptr, 
                     IBallisticSolver* solver = nullptr, 
                     IConfigLoader* configLoader = nullptr)
        : m_targetProvider(targetProvider), m_solver(solver), m_configLoader(configLoader) {}

    void setTargetProvider(ITargetProvider* targetProvider) {
        m_targetProvider = targetProvider;
    }

    void setBallisticSolver(IBallisticSolver* solver) {
        m_solver = solver;
    }

    void setConfigLoader(IConfigLoader* configLoader) {
        m_configLoader = configLoader;
    }

    void executeMission() {
       
        if (!m_targetProvider || !m_solver || !m_configLoader) {
            std::cout << "[MissionProcessor] Помилка: Не всі компоненти підключені!\n";
            return;
        }

        std::cout << "\n--- ПОЧАТОК МІСІЇ ---\n";

        // лічильник
        int counter = 0;
        
        // масив який містить час підльоту до кожної цілі
        float targetTimes[NUMBER_OF_TARGETS];
        
        // масив який містить відстань під поточного положення дрону до кожної цілі
        float targetDistances[NUMBER_OF_TARGETS];
        
        // масив який містить кут напрямку для кожної цілі відносто осі X в радіанах 
        float targetAngles[NUMBER_OF_TARGETS];   

        DroneConfig myDrone;

        // ініціалізація налаштувань дрона
        try {
            m_configLoader->init(myDrone);

            // запускаємо для розрахунку додаткових параметрів
            myDrone.updateCalculatedParams();

            std::cout << "Дрон успішно налаштований та готовий до польоту!\n";
        }
        catch (const std::runtime_error& e) {
            throw std::runtime_error("[FileConfigLoader] КРИТИЧНА ПОМИЛКА: " + std::string(e.what()));
        }

        float ratio = myDrone.arrayTimeStep / myDrone.simTimeStep;
    
        if (std::abs(ratio - std::floor(ratio)) > 0.0001f) {
            throw std::runtime_error("Помилка: Крок масиву не ділиться націло на крок симуляції!");
        }

        // кожну кількість numberCounterInTimeSpot в нас міняються координати цілей тобто проходить arrayTimeStep секунд
        int numberCounterInTimeSpot =  static_cast<int>(ratio);

        // Отримуємо цілі
        try {
            m_targetProvider->init(numberCounterInTimeSpot);
            std::cout << "Цілі успішно загруженні в систему!\n";
        }
        catch (const std::runtime_error& e) {
            throw std::runtime_error("[JsonTargetProvider] КРИТИЧНА ПОМИЛКА: " + std::string(e.what()));
        }

        CurrentDroneParameters curMyDrone;

        // поточний стан дрона і його кута польоту, координати дрона, ціль
        curMyDrone.pos.x = myDrone.startPos.x;
        curMyDrone.pos.y = myDrone.startPos.y;
        curMyDrone.speed = 0;
        curMyDrone.angularState = myDrone.initialDir;
        curMyDrone.state = STOPPED;
        curMyDrone.target = 0;

        // ключ що відповідає за переключання цілей дрона
        bool keyChangeTarget = true;
    
        DEBUG("Час зупинки або прискорення: " << std::fixed << std::setprecision(2) << myDrone.timeAcceleration << " с");
        DEBUG("Прискорення дрона: " << myDrone.acceleration << " м/с2 ---");
        DEBUG("Величина оберту дрона за ітерацію: " << myDrone.radInIteration << " р/с ---");
        DEBUG("===========================");
    

        SimStep* steps = new SimStep[MAX_STEPS];
        
        const AmmoParams* ammo = m_configLoader->getAmmoParameters(myDrone.ammoName);
        
        if (ammo == nullptr) {
            throw std::runtime_error("Помилка: боєприпас " + std::string(myDrone.ammoName) + " не знайдено в базі!");
        }
        
        DEBUG("Знайдено боєприпас: " << myDrone.ammoName);
        DEBUG("Параметри: mass: " << std::fixed << std::setprecision(2) << ammo->mass 
            << ", drag: " << ammo->drag 
            << ", lift: " << ammo->lift);
            
        float t_pol;
        float distDuringFall = 0;

        try {
            distDuringFall = m_solver->getDistDuringFall(t_pol, myDrone, ammo);
        }
        catch (const std::runtime_error& e) {
           throw std::runtime_error("[AnalyticalSolver] КРИТИЧНА ПОМИЛКА: " + std::string(e.what()));
        }

        DEBUG("Горизонтальна дистанція яку проходить дрон за час " << t_pol << " сек. рівна " << distDuringFall << " м.");
        DEBUG("-----------------------------------");

        if (distDuringFall <= 0) {
            throw std::runtime_error("Горизонтальна дистанція повинна бути додатня");
        }
            
        bool addingStep = false;
        
        // Тут може бути ваша логіка циклу по цілях та виклик m_solver
        // наприклад:
        // auto target = m_targetProvider->getTarget(...);
        // m_solver->calculateTrajectory(...);

        std::cout << "--- МІСІЮ ЗАВЕРШЕНО ---\n";
    }
};