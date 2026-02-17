# Server测试CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(server_test)

find_package(GTest QUIET)

if(GTest_FOUND)
    enable_testing()

    set(TEST_SOURCES test_server.cpp)
    add_executable(test_server ${TEST_SOURCES})
    target_link_libraries(test_server
        PRIVATE server config connection callback utils GTest::gtest GTest::gtest_main)
    target_compile_features(test_server PRIVATE cxx_std_17)

    include(GoogleTest)
    gtest_discover_tests(test_server)
endif()
