project(targets3 LANGUAGES CXX)

#  Автоматично збираємо всі .cpp файли з папки src та її підпапок
file(GLOB_RECURSE SRC_FILES "src/*.cpp")

# ВИЛУЧАЄМО main.cpp зі списку бібліотеки
list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

# Збираємо всі заголовні файли, щоб вони гарно відображались у дереві VS Code
file(GLOB_RECURSE HEADERS "include/*.h" "include/*.tpp")

# Створюємо бібліотеку з усіх знайдених файлів (окрім main.cpp)
add_library(targets_lib3 STATIC ${SRC_FILES} ${HEADERS})

# Вказуємо, де шукати заголовочні файли (.hpp)
target_include_directories(targets_lib3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Створюємо виконуваний файл
add_executable(targets3 src/main.cpp)

# Додаємо суворі перевірки
target_compile_options(targets3 PRIVATE 
    -Wall 
    -Wextra 
    -pedantic
    -fsanitize=address -fno-omit-frame-pointer -g
)

# Поєднуємо їх (лінкуємо)
# Тепер targets знає про функції з targets_lib
target_link_libraries(targets3 PRIVATE targets_lib3 PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
target_link_options(targets3 PRIVATE -fsanitize=address)

# Встановлюємо назву бінарника
set_target_properties(targets3 PROPERTIES OUTPUT_NAME "targets3")