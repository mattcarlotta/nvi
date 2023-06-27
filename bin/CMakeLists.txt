cmake_minimum_required(VERSION 3.26.3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(nvi)

set(SRC_FOLDER ${CMAKE_SOURCE_DIR}/src)
file(GLOB SRC_FILES ${SRC_FOLDER}/*.cpp)
list(REMOVE_ITEM SRC_FILES ${SRC_FOLDER}/main.cpp)

project(${PROJECT_NAME} VERSION 1.0)
add_subdirectory(${SRC_FOLDER})

enable_testing()
add_subdirectory(tests)
