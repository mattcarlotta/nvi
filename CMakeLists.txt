cmake_minimum_required(VERSION 3.26.3)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "release")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-std=c++17 -Werror -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
project(
    nvi 
    DESCRIPTION "A stand-alone .env file parser for interpolating and assigning multiple .env files into a process"
)

option(COMPILE_SRC "Compile source" ON)
option(COMPILE_TESTS "Compile tests" OFF)
option(INSTALL_BIN_DIR "Binary installation directory" OFF)
option(INSTALL_MAN_DIR "Install man documentation" OFF)
option(USE_LOCALHOST_API "Use localhost API" OFF)

set(SRC_FOLDER ${CMAKE_SOURCE_DIR}/src)
file(STRINGS "version" NVI_LIB_VERSION)
configure_file(${SRC_FOLDER}/version.h.in ${SRC_FOLDER}/version.h @ONLY)
file(GLOB SRC_FILES ${SRC_FOLDER}/*.cpp)
list(REMOVE_ITEM SRC_FILES ${SRC_FOLDER}/main.cpp)

if(COMPILE_SRC MATCHES ON)
    add_subdirectory(${SRC_FOLDER})
    if(NOT INSTALL_MAN_DIR MATCHES OFF)
        install(FILES "./docs/nvi.1" DESTINATION ${INSTALL_MAN_DIR})
    endif()
endif()

if(COMPILE_TESTS MATCHES ON)
    enable_testing()
    add_subdirectory(tests)
endif()
