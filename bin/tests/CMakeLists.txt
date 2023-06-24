add_library(
    ${PROJECT_NAME}_test 
    ${CMAKE_SOURCE_DIR}/src/arg.cpp 
    ${CMAKE_SOURCE_DIR}/src/env.cpp
    ${CMAKE_SOURCE_DIR}/src/json.cpp
    ${CMAKE_SOURCE_DIR}/src/load_config.cpp
)

add_executable(test_arg test-arg.cpp)
add_executable(test_env test-env.cpp)
add_executable(test_load_config test-load-config.cpp)
target_link_libraries(test_arg ${PROJECT_NAME}_test)
target_link_libraries(test_env ${PROJECT_NAME}_test)
target_link_libraries(test_load_config ${PROJECT_NAME}_test)
target_include_directories(test_arg PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(test_env PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(test_load_config PUBLIC ${CMAKE_SOURCE_DIR}/src)

add_test(NAME test_arg COMMAND test_arg --file test.env)
add_test(NAME test_env COMMAND test_env)
add_test(NAME test_load_config COMMAND test_load_config)
