#include <string_view>
#ifndef CONSTANTS_H
#define CONSTANTS_H

// кількість цілей   
inline constexpr int NUMBER_OF_TARGETS = 5;

// максимальна кількість ітерацій
inline constexpr int MAX_STEPS = 10000;
inline constexpr double GRAVIT = 9.81;
inline constexpr std::string_view FILE_OUTPUT = "./data/output.json";
inline constexpr std::string_view HOST = "http://cppmiltech.com.ua";
inline constexpr std::string_view API_KEY = "dz12-vX7mK4qT9r2w";
inline constexpr std::string_view STUDENT_ID = "1015";
inline constexpr float MAX_ACCEL = 0.1f;      // Газуємо на повну
inline constexpr float MAX_TURN_RATE = 0.1f;  // Обороти на повну

#endif