project(telemetry_app LANGUAGES CXX)

# Створюємо бібліотеку що буде "вшита" у виконуваний файл
add_library(telemetry_lib STATIC 
    src/telemetry.cpp 
    include/telemetry.hpp
)

# Вказуємо, де шукати заголовочні файли (.hpp)
target_include_directories(telemetry_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Додаємо суворі перевірки
target_compile_options(telemetry_lib PRIVATE 
    -Wall 
    -Wextra 
    -pedantic
)

# Створюємо виконуваний файл
add_executable(telemetry_app src/main.cpp)

# Поєднуємо їх (лінкуємо)
# Тепер telemetry_app знає про функції з telemetry_lib
target_link_libraries(telemetry_app PRIVATE telemetry_lib)

# Встановлюємо назву бінарника
set_target_properties(telemetry_app PROPERTIES OUTPUT_NAME "telemetry")