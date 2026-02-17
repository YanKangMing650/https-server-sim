# Config测试CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(config_test)

find_package(GTest QUIET)

if(GTest_FOUND)
    enable_testing()

    set(TEST_SOURCES test_config.cpp)
    add_executable(test_config ${TEST_SOURCES})
    target_link_libraries(test_config
        PRIVATE config utils GTest::gtest GTest::gtest_main)
    target_compile_features(test_config PRIVATE cxx_std_17)

    include(GoogleTest)
    gtest_discover_tests(test_config)
endif()
