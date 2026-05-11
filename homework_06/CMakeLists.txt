project(drone_attack CXX)

add_library(
    ballistics STATIC
    src/ballistics.cpp include/ballistics.hpp # 'include/ballistics.hpp' можна прибрати але з ним
                                              # IDE краще розуміє навігацію по проекту хоча не факт
)

target_include_directories(ballistics PUBLIC include)

target_compile_options(ballistics PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(drone_attack src/main.cpp)

target_compile_options(drone_attack PRIVATE -Wall -Wextra -Wpedantic -Werror)

target_link_libraries(drone_attack PRIVATE ballistics)

add_subdirectory(tests)

find_program(CLANG_TIDY_EXE NAMES "clang-tidy")

if(CLANG_TIDY_EXE)
    message(STATUS "Clang-tidy found: ${CLANG_TIDY_EXE}")
    # Встановлюємо clang-tidy для всіх наступних цілей (targets)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
else()
    message(WARNING "Clang-tidy not found! Static analysis will be skipped.")
endif()
