# Callback测试CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(callback_test)

find_package(GTest QUIET)

if(GTest_FOUND)
    enable_testing()

    set(TEST_SOURCES test_callback.cpp)
    add_executable(test_callback ${TEST_SOURCES})
    target_link_libraries(test_callback
        PRIVATE callback utils config GTest::gtest GTest::gtest_main)
    target_compile_features(test_callback PRIVATE cxx_std_17)

    include(GoogleTest)
    gtest_discover_tests(test_callback)
endif()
