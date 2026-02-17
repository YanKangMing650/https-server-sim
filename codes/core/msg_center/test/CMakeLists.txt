# MsgCenter测试CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(msgcenter_test)

find_package(GTest QUIET)

if(GTest_FOUND)
    enable_testing()

    set(TEST_SOURCES test_msgcenter.cpp)
    add_executable(test_msgcenter ${TEST_SOURCES})
    target_link_libraries(test_msgcenter
        PRIVATE msg_center utils GTest::gtest GTest::gtest_main)
    target_compile_features(test_msgcenter PRIVATE cxx_std_17)

    include(GoogleTest)
    gtest_discover_tests(test_msgcenter)
endif()
