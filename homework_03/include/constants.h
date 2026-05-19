#ifndef CONSTANTS_H
#define CONSTANTS_H

// кількість цілей   
inline constexpr int NUMBER_OF_TARGETS = 5;

// кількість часових проміжків
inline constexpr int NUMBER_OF_TIMES = 60;

// максимальна кількість ітерацій
inline constexpr int MAX_STEPS = 10000;

namespace physics {
    inline constexpr double GRAVIT = 9.81;
}

#endif