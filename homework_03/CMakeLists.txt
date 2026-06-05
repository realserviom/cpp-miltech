project(targets LANGUAGES CXX)

# Створюємо бібліотеку що буде "вшита" у виконуваний файл
add_library(targets_lib STATIC 
    src/functions.cpp 
    include/functions.h
    include/functions.tpp
)

# Вказуємо, де шукати заголовочні файли (.hpp)
target_include_directories(targets_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Створюємо виконуваний файл
add_executable(targets src/main.cpp)

# Додаємо суворі перевірки
target_compile_options(targets PRIVATE 
    -Wall 
    -Wextra 
    -pedantic
)

# Поєднуємо їх (лінкуємо)
# Тепер targets знає про функції з targets_lib
target_link_libraries(targets PRIVATE targets_lib)

# Встановлюємо назву бінарника
set_target_properties(targets PROPERTIES OUTPUT_NAME "targets")