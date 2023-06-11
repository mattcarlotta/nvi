cmake_minimum_required(VERSION 3.26.3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(EXECUTABLE_OUTPUT_PATH  "../")
project(nvi)

project(${PROJECT_NAME} VERSION 1.0)
add_executable(${PROJECT_NAME} main.cpp arg.cpp env.cpp)
