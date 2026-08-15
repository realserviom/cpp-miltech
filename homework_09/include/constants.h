#ifndef CONSTANTS_H
#define CONSTANTS_H

// кількість цілей   
inline constexpr int NUMBER_OF_TARGETS = 5;

// максимальна кількість ітерацій
inline constexpr int MAX_STEPS = 10000;

inline constexpr double GRAVIT = 9.81;

inline constexpr float MAX_ACCEL = 0.1f;      // Газуємо на повну
inline constexpr float MAX_TURN_RATE = 0.1f;  // Обороти на повну

#endif