# DebugChain测试CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(debugchain_test)

find_package(GTest QUIET)

if(GTest_FOUND)
    enable_testing()

    set(TEST_SOURCES test_debugchain.cpp)
    add_executable(test_debugchain ${TEST_SOURCES})
    target_link_libraries(test_debugchain
        PRIVATE debug_chain utils GTest::gtest GTest::gtest_main)
    target_compile_features(test_debugchain PRIVATE cxx_std_17)

    include(GoogleTest)
    gtest_discover_tests(test_debugchain)
endif()
