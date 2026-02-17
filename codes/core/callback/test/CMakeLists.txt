# Callback模块测试CMakeLists.txt

# 使用third_party中的googletest，不使用系统的
set(GTEST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../third_party/googletest")
if(EXISTS "${GTEST_ROOT}")
    message(STATUS "Building Callback module unit tests with third_party/googletest")

    # 直接编译gtest源文件，避免与系统gtest冲突
    set(GTEST_SOURCES
        "${GTEST_ROOT}/googletest/src/gtest-all.cc"
    )

    add_executable(test_callback
        test_callback.cpp
        ${GTEST_SOURCES}
    )

    target_include_directories(test_callback
        PRIVATE
            "${GTEST_ROOT}/googletest"
            "${GTEST_ROOT}/googletest/include"
    )

    target_link_libraries(test_callback
        PRIVATE
            callback
            pthread
    )

    target_compile_features(test_callback PRIVATE cxx_std_17)

    # 注册测试
    enable_testing()
    add_test(NAME CallbackTest COMMAND test_callback)

else()
    message(WARNING "googletest not found in third_party, skipping Callback module unit tests")
endif()
