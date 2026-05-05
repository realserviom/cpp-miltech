project(telemetry_check LANGUAGES CXX)

# Створюємо бібліотеку що буде "вшита" у виконуваний файл
add_library(telemetry STATIC 
    src/telemetry.cpp 
    include/telemetry.hpp
)

# Вказуємо, де шукати заголовочні файли (.hpp)
target_include_directories(telemetry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Додаємо суворі перевірки
target_compile_options(telemetry PRIVATE 
    -Wall 
    -Wextra 
    -pedantic
)

# Створюємо виконуваний файл
add_executable(telemetry_check src/main.cpp)

# Поєднуємо їх (лінкуємо)
# Тепер telemetry_app знає про функції з telemetry_lib
target_link_libraries(telemetry_check PRIVATE telemetry)

# Встановлюємо назву бінарника
set_target_properties(telemetry_check PROPERTIES OUTPUT_NAME "telemetry")