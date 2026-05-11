project(drone_attack CXX)

add_library(ballistics STATIC 
    src/ballistics.cpp
    include/ballistics.hpp
)

target_include_directories(ballistics PUBLIC include)

target_compile_options(ballistics PUBLIC -Wall -Wextra -pedantic)

add_executable(drone_attack src/main.cpp)

target_link_libraries(drone_attack PRIVATE ballistics)

