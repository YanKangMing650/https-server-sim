# =============================================================================
#  HTTPS Server Simulator - Integration Tests CMakeLists.txt
#  描述: 集成测试构建配置
#  版权: Copyright (c) 2026
# =============================================================================

cmake_minimum_required(VERSION 3.15)

# 集成测试源文件列表
set(INTEGRATION_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/integration_test_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_server_startup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_basic_requests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_debug_chain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_callback_and_more.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_client_disconnect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_http.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_tls.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_performance.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_stress.cpp
)

# 创建集成测试可执行文件
add_executable(core_integration_test ${INTEGRATION_TEST_SOURCES})

# 依赖的头文件路径
target_include_directories(core_integration_test
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
target_link_libraries(core_integration_test
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

# C++标准
target_compile_features(core_integration_test PRIVATE cxx_std_17)

# 注册到CTest
add_test(NAME CoreIntegrationTests COMMAND core_integration_test)

# 文件结束
