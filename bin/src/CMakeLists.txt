set(EXECUTABLE_OUTPUT_PATH  "../../")
add_library(lib arg.cpp env.cpp)
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries (${PROJECT_NAME} lib)
