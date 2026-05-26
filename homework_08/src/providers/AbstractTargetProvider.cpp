#include <iostream>
#include "Types.h"
#include "providers/AbstractTargetProvider.h"
#include <cmath>


void AbstractTargetProvider::init(int &numberCounterInTimeSpot) {
    m_numberCounterInTimeSpot = numberCounterInTimeSpot;
    std::cout << "Preparing targets...\n";
    loadTargets(); 
    std::cout << "Targets ready.\n";
}

int AbstractTargetProvider::getTargetCount() {
    return m_targetCount;
}

Coord** AbstractTargetProvider::getTargets() {
    return this->m_targets;
}

int AbstractTargetProvider::getTimeIteration(int &counter) {
    
    const int wholeRangeCounters = this->m_numberCounterInTimeSpot * this->m_timeSteps; 
    
    // якщо в нас поточна ітерація більша чи рівна wholeRangeCounters тоді берем остачу від цілочисленого
    // ділення на wholeRangeCounters
    const int new_counter = counter >= wholeRangeCounters ?
        static_cast<int>(counter % wholeRangeCounters) : counter;

    return static_cast<int>(std::floor(new_counter / this->m_numberCounterInTimeSpot));
}

int AbstractTargetProvider::getNextIteration(int &iteration) {

    if (iteration == (this->m_timeSteps - 1)) {
        return 0;
    }
    
    return iteration + 1;
}

Coord AbstractTargetProvider::getTargetPositionInCounter(int &index, int &counter) {
    int timeIteration = this->getTimeIteration(counter);
    
    if (this->m_targets && index >= 0 && index < m_targetCount) {
        return this->m_targets[index][timeIteration];
    }
    return Coord{0.0, 0.0};
}

Coord AbstractTargetProvider::getTargetPositionInIteration(int &index, int &timeIteration) {
    
    if (this->m_targets && index >= 0 && index < m_targetCount) {
        return this->m_targets[index][timeIteration];
    }
    return Coord{0.0, 0.0};
}
