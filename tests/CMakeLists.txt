add_subdirectory(googletest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_library(${PROJECT_NAME}_test ${SRC_FILES})

add_executable(tests test-main.cpp test-arg.cpp test-config.cpp test-parser.cpp)
target_link_libraries(tests ${PROJECT_NAME}_test gtest gtest_main)
target_include_directories(tests PUBLIC ${SRC_FOLDER} ${gtest_SOURCE_DIR}/include)

add_test(NAME tests COMMAND tests)
