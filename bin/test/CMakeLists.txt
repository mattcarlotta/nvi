include_directories(${TEST_SOURCE_DIR}/src)
add_executable(test_args args.cpp)
add_executable(test_env env.cpp)
target_link_libraries(test_args lib)
target_link_libraries(test_env lib)

add_test(NAME test_args COMMAND test_args)
add_test(NAME test_env COMMAND test_env)
