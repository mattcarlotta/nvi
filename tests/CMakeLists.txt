add_subdirectory(googletest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_library(${PROJECT_NAME}_test ${SRC_FILES})

file(GLOB TEST_SRC_FILES ${CMAKE_SOURCE_DIR}/tests/*.cpp)
add_executable(tests ${TEST_SRC_FILES})
target_link_libraries(tests ${PROJECT_NAME}_test gtest gtest_main)
target_include_directories(tests PUBLIC ${SRC_FOLDER} ${gtest_SOURCE_DIR}/include)

add_test(NAME tests COMMAND tests)
