# Protocol测试CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(protocol_test)

find_package(GTest QUIET)

if(GTest_FOUND)
    enable_testing()

    set(TEST_SOURCES test_protocol.cpp)
    add_executable(test_protocol ${TEST_SOURCES})
    target_link_libraries(test_protocol
        PRIVATE protocol utils GTest::gtest GTest::gtest_main)
    target_compile_features(test_protocol PRIVATE cxx_std_17)

    include(GoogleTest)
    gtest_discover_tests(test_protocol)
endif()
