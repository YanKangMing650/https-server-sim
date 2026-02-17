# Utils测试CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(utils_test)

# 查找GoogleTest
find_package(GTest QUIET)

if(GTest_FOUND)
    enable_testing()

    # 测试源文件
    set(TEST_SOURCES
        test_utils.cpp
    )

    add_executable(test_utils ${TEST_SOURCES})
    target_link_libraries(test_utils
        PRIVATE
            utils
            GTest::gtest
            GTest::gtest_main
    )

    target_compile_features(test_utils PRIVATE cxx_std_17)

    include(GoogleTest)
    gtest_discover_tests(test_utils)
else()
    message(STATUS "GoogleTest not found, utils tests will not be built")
endif()
