cmake_minimum_required(VERSION 3.26.3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(
    nvi 
    DESCRIPTION "A stand-alone .env file parser for interpolating and assigning multiple .env files into a process"
)

add_compile_options(-Werror -Wall -Wextra -Wpedantic -std=c++17)
option(COMPILE_SRC "Compile source" ON)
option(COMPILE_TESTS "Compile tests" OFF)
option(INSTALL_BIN_DIR "Binary installation directory" OFF)
option(INSTALL_MAN_DIR "Install man documentation" OFF)
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
