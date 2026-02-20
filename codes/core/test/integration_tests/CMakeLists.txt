# =============================================================================
#  HTTPS Server Simulator - Integration Tests CMakeLists.txt
#  描述: 集成测试构建配置
#  版权: Copyright (c) 2026
# =============================================================================

cmake_minimum_required(VERSION 3.15)

# 集成测试源文件列表
set(INTEGRATION_TEST_SOURCES
    integration_test_main.cpp
    test_server_startup.cpp
    test_connection_manager.cpp
    test_event_flow.cpp
)

# 创建集成测试可执行文件
add_executable(integration_tests ${INTEGRATION_TEST_SOURCES})

# 依赖的头文件路径
target_include_directories(integration_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../server/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../connection/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../protocol/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../msg_center/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../callback/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../debug_chain/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../config/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../utils/include
)

# 链接依赖库
target_link_libraries(integration_tests
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
        gtest_main
        pthread
)

# C++标准
target_compile_features(integration_tests PRIVATE cxx_std_17)

# 注册到CTest
add_test(NAME IntegrationTests COMMAND integration_tests)

# 文件结束
