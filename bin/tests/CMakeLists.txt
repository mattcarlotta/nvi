add_library(
    ${PROJECT_NAME}_test 
    ${CMAKE_SOURCE_DIR}/src/arg.cpp 
    ${CMAKE_SOURCE_DIR}/src/env.cpp
)

add_executable(test_arg test-arg.cpp)
add_executable(test_env test-env.cpp)
target_link_libraries(test_arg ${PROJECT_NAME}_test)
target_link_libraries(test_env ${PROJECT_NAME}_test)
target_include_directories(test_arg PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(test_env PUBLIC ${CMAKE_SOURCE_DIR}/src)

add_test(NAME test_arg COMMAND test_arg -f .env)
add_test(NAME test_env COMMAND test_env)
