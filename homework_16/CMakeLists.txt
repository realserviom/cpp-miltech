cmake_minimum_required(VERSION 3.20)
project(i2c CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(i2c src/main.cpp)
target_compile_options(i2c PRIVATE -Wall -Wextra -pedantic)
#target_include_directories(ugv_odometry PRIVATE src/include)
