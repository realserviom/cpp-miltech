#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include "types.h"
#include "json.hpp"

using json = nlohmann::json;

enum class ProviderType { JSON };

class ITargetProvider {
public:
    virtual int getTargetCount() = 0;
    virtual Coord getTargetPositionInCounter(int index, int counter) = 0;
    virtual void loadTargets() = 0;
    virtual Coord** getTargets() = 0;
    virtual ~ITargetProvider() {}
};

class AbstractTargetProvider : public ITargetProvider {
protected:
    int m_targetCount = 0;
    int m_timeSteps = 0;
    const int m_numberCounterInTimeSpot;
    Coord** m_targets = nullptr; 

public:
    AbstractTargetProvider(int numberCounterInTimeSpot) 
        : m_numberCounterInTimeSpot(numberCounterInTimeSpot) {}

    virtual ~AbstractTargetProvider() override {}

    int getTargetCount() override {
        return m_targetCount;
    }

    Coord** getTargets() override {
        return this->m_targets;
    }

    int getNumberCounterInTimeSpot() const {
        return m_numberCounterInTimeSpot;
    }
    
    int getTimeIteration(int &counter) {
        
        const int wholeRangeCounters = this->m_numberCounterInTimeSpot * this->m_timeSteps; 
        
        // якщо в нас поточна ітерація більша чи рівна wholeRangeCounters тоді берем остачу від цілочисленого
        // ділення на wholeRangeCounters
        const int new_counter = counter >= wholeRangeCounters ?
            static_cast<int>(counter % wholeRangeCounters) : counter;

        return static_cast<int>(std::floor(new_counter / this->m_numberCounterInTimeSpot));
    }

    int getNextIteration(int &iteration) {
    
        if (iteration == (this->m_timeSteps - 1)) {
            return 0;
        }
        
        return iteration + 1;
    }

    Coord getTargetPositionInCounter(int index, int counter) override {
        int timeIteration = this->getTimeIteration(counter);
        
        if (this->m_targets && index >= 0 && index < m_targetCount) {
            return this->m_targets[index][timeIteration];
        }
        return Coord{0.0, 0.0};
    }

};

// 3. Конкретна реалізація (JsonTargetProvider)
class JsonTargetProvider : public AbstractTargetProvider {
public:

    std::string m_filePath;
    JsonTargetProvider(const std::string& jsonFilePath, int& numberCounterInTimeSpot) 
        : AbstractTargetProvider(numberCounterInTimeSpot), m_filePath(jsonFilePath)
    {
        loadTargets();
    }

    void loadTargets() override {
          std::ifstream fin(m_filePath);

        if (!fin.is_open()) {
            std::cout << "[JsonTargetProvider] Помилка файлу " << m_filePath << std::endl;
            return;
        }
        
        json j;
        try {
            fin >> j;
        } catch (const json::parse_error& e) {
            std::cout << "[JsonTargetProvider] Помилка парсингу: " << e.what() << std::endl;
            fin.close();
            return;
        }

        m_targetCount = j["targetCount"]; 
        m_timeSteps = j["timeSteps"];

        m_targets = new Coord*[m_targetCount];

        for (int i = 0; i < m_targetCount; i++) {
            m_targets[i] = new Coord[m_timeSteps];
            auto& positionsJson = j["targets"][i]["positions"];
            
            for (int t = 0; t < m_timeSteps; t++) {
                m_targets[i][t].x = positionsJson[t]["x"];
                m_targets[i][t].y = positionsJson[t]["y"];
            }
        }
        
        fin.close();
    }

    virtual ~JsonTargetProvider() override {
        if (m_targets != nullptr) {
            for (int i = 0; i < m_targetCount; i++) {
                delete[] m_targets[i];
            }
            delete[] m_targets;
        }
    }

};

// Додаємо необов'язковий третій параметр для передачі лічильника ініціалізації
inline ITargetProvider* createProvider(ProviderType type, const char* file_name, int& numberCounterInTimeSpot) {
    switch (type) {
        case ProviderType::JSON:
            return new JsonTargetProvider(file_name, numberCounterInTimeSpot);
        default:
            return nullptr;
    }
}