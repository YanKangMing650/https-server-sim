# =============================================================================
#  HTTPS Server Simulator - Integration Tests CMakeLists.txt
#  描述: 集成测试构建配置
#  版权: Copyright (c) 2026
# =============================================================================

cmake_minimum_required(VERSION 3.15)

# 集成测试源文件列表（待填充）
set(INTEGRATION_TEST_SOURCES
    # 待添加集成测试用例
)

# 创建集成测试可执行文件（仅当有源文件时）
if(INTEGRATION_TEST_SOURCES)
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
            gtest_main
            pthread
    )

    # C++标准
    target_compile_features(core_integration_test PRIVATE cxx_std_17)

    # 注册到CTest
    add_test(NAME CoreIntegrationTests COMMAND core_integration_test)
endif()

# 文件结束
