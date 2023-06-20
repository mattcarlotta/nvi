cmake_minimum_required(VERSION 3.26.3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(nvi)

include(CTest)
project(${PROJECT_NAME} VERSION 1.0)
add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
