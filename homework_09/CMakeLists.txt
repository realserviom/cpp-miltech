cmake_minimum_required(VERSION 3.20)
project(targets3 LANGUAGES CXX)

# C++20 стандарт
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------------------------------------
# Залежності (cpp-httplib)
# ------------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.18.0
)
FetchContent_MakeAvailable(httplib)

# ------------------------------------------------------------------------------
# Збір джерел та оголошення бібліотеки
# ------------------------------------------------------------------------------
# Автоматично шукаємо всі .cpp файли у папці src
file(GLOB_RECURSE SRC_FILES "src/*.cpp")

# Вилучаємо main.cpp зі списку джерел для бібліотеки
list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

# Збираємо всі заголовні файли, щоб вони гарно відображались у дереві VS Code
file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp" "include/*.tpp")

# Створюємо статичну бібліотеку проєкту
add_library(targets_lib3 STATIC ${SRC_FILES} ${HEADERS})

# Додаємо шляхи до include-директорій
target_include_directories(targets_lib3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Лінкуємо httplib до нашої бібліотеки
target_link_libraries(targets_lib3 PRIVATE httplib::httplib)

# ------------------------------------------------------------------------------
# Виконуваний файл (Executable)
# ------------------------------------------------------------------------------
add_executable(targets3 src/main.cpp)

# Прапорці компілятора та санітайзери
target_compile_options(targets3 PRIVATE 
    -Wall 
    -Wextra 
    -pedantic
    -fsanitize=address -fno-omit-frame-pointer -g
)

# Лінкування бібліотек та санітайзера до бінарника
target_link_libraries(targets3 PRIVATE 
    targets_lib3 
    -fsanitize=address -fno-omit-frame-pointer -g
)
target_link_options(targets3 PRIVATE -fsanitize=address)

<<<<<<< HEAD
# Встановлюємо назву бінарника
set_target_properties(targets3 PROPERTIES OUTPUT_NAME "targets3")

# Ігноруємо специфічні для MAVLink зауваження
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -Wno-address-of-packed-member
        -Wno-pedantic
    )
endif()
=======
set_target_properties(targets3 PROPERTIES OUTPUT_NAME "targets3")
>>>>>>> feature/homework-011
