# Connection测试CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(connection_test)

find_package(GTest QUIET)

if(GTest_FOUND)
    enable_testing()

    set(TEST_SOURCES test_connection.cpp)
    add_executable(test_connection ${TEST_SOURCES})
    target_link_libraries(test_connection
        PRIVATE connection utils protocol GTest::gtest GTest::gtest_main)
    target_compile_features(test_connection PRIVATE cxx_std_17)

    include(GoogleTest)
    gtest_discover_tests(test_connection)
endif()
