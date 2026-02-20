# Core层测试 CMakeLists.txt
# 包含单元测试和集成测试
# 使用统一的 BUILD_TESTS 宏控制

cmake_minimum_required(VERSION 3.15)

# =============================================================================
# gtest 库
# =============================================================================

if(NOT TARGET gtest)
    add_library(gtest STATIC
        ${THIRD_PARTY_DIR}/googletest/googletest/src/gtest-all.cc
    )
    target_include_directories(gtest
        PUBLIC
            ${THIRD_PARTY_DIR}/googletest/googletest
            ${THIRD_PARTY_DIR}/googletest/googletest/include
    )
endif()

# =============================================================================
# 单元测试 - core_unit_test
# =============================================================================

if(CORE_TEST_SOURCES)
    # 编译core_unit_test可执行文件
    add_executable(core_unit_test
        ${CORE_TEST_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/core_unit_test_main.cpp
    )
    target_link_libraries(core_unit_test
        PRIVATE
            server
            connection
            protocol
            msg_center
            callback
            debug_chain
            config
            utils
            gtest
            pthread
    )

    # 注册到CTest
    add_test(NAME CoreUnitTests COMMAND core_unit_test)
endif()

# =============================================================================
# 集成测试 - core_integration_test
# =============================================================================

add_subdirectory(integration_tests)

# 文件结束
