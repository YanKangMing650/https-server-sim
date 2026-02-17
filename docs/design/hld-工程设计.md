# HTTPS Server 模拟器 - 工程设计文档

**版本**: v4
**创建日期**: 2026-02-17
**状态**: 草稿

---

## 版本历史

| 版本 | 日期 | 作者 | 说明 |
|------|------|------|------|
| v1 | 2026-02-16 | Architecture Designer | 初始版本 |
| v2 | 2026-02-16 | Architecture Designer | 补充Tongsuo（铜锁）三方库说明 |
| v3 | 2026-02-16 | Architecture Designer | 更新代码结构规范、CMakeLists规范、测试分类 |
| v4 | 2026-02-17 | Architecture Designer | 重构core层为子模块静态库结构，core整体为动态库 |

---

## 目录

1. [工程概述](#1-工程概述)
2. [目录结构设计](#2-目录结构设计)
3. [头文件组织规范](#3-头文件组织规范)
4. [构建设计](#4-构建设计)
5. [CMakeLists规范](#5-cmakelists规范)
6. [三方库选型与使用](#6-三方库选型与使用)
7. [编译与构建流程](#7-编译与构建流程)
8. [测试工程设计](#8-测试工程设计)
9. [防腐层设计](#9-防腐层设计)

---

## 1. 工程概述

### 1.1 工程目标

本工程设计的目标是：
- 提供清晰的目录结构，便于开发和维护
- 使用CMake构建系统，支持多平台编译
- 合理使用三方库，确保工程可维护性
- 提供完整的测试框架
- 通过防腐层设计隔离外部依赖
- core层采用子模块静态库设计，每个模块独立构建并配有单元测试
- core整体编译为一个动态库供外部使用

### 1.2 技术栈

| 层级 | 技术选型 |
|-----|---------|
| 核心层 | C++17 |
| 适配层 | Python 3.8 |
| 构建系统 | CMake 3.15+ |
| TLS库 | OpenSSL 或 Tongsuo（铜锁，支持国密） |
| JSON解析 | nlohmann/json |
| 单元测试框架 | GoogleTest (GTest) |

---

## 2. 目录结构设计

### 2.1 完整目录结构

```
https-server-sim/
├── docs/
│   ├── design/
│   │   ├── srs-需求分析文档.md
│   │   ├── srs-需求文档.md
│   │   ├── hld-架构设计文档.md
│   │   ├── hld-4+1视图.md
│   │   ├── hld-工程设计.md
│   │   └── modules/              # 子模块详细设计
│   │       ├── module-server.md
│   │       ├── module-connection.md
│   │       ├── module-protocol.md
│   │       ├── module-msgcenter.md
│   │       ├── module-debugchain.md
│   │       ├── module-callback.md
│   │       └── module-utils.md
│   └── .ai/
│       └── hld/                 # 架构设计检视意见
├── codes/
│   ├── CMakeLists.txt           # 顶层CMakeLists.txt（工程描述）
│   ├── api/
│   │   ├── adapt/               # C接口适配层（防腐层）
│   │   │   ├── CMakeLists.txt   # adapt的CMakeLists.txt
│   │   │   ├── include/         # 对外头文件
│   │   │   │   └── server_adapt.h
│   │   │   └── source/          # 源文件 + 非对外头文件
│   │   │       ├── server_adapt.cpp
│   │   │       └── internal.hpp # 非对外头文件放在source内部
│   │   ├── cli/                 # Python CLI适配层
│   │   │   ├── __init__.py
│   │   │   ├── main.py
│   │   │   ├── config.py
│   │   │   └── server.py
│   │   └── app/                 # Python GUI适配层
│   │       ├── __init__.py
│   │       ├── main.py
│   │       ├── main_window.py
│   │       ├── config_panel.py
│   │       └── log_panel.py
│   └── core/
│       ├── CMakeLists.txt       # core的CMakeLists.txt（聚合子模块，输出动态库）
│       ├── server/              # server模块（独立静态库）
│       │   ├── CMakeLists.txt
│       │   ├── include/
│       │   │   └── server/
│       │   │       └── server.hpp
│       │   ├── source/
│       │   │   ├── server.cpp
│       │   │   └── server_internal.hpp
│       │   └── test/            # server模块单元测试
│       │       ├── CMakeLists.txt
│       │       └── test_server.cpp
│       ├── connection/          # connection模块（独立静态库）
│       │   ├── CMakeLists.txt
│       │   ├── include/
│       │   │   └── connection/
│       │   │       ├── connection.hpp
│       │   │       ├── connection_manager.hpp
│       │   │       └── connection_state.hpp
│       │   ├── source/
│       │   │   ├── connection.cpp
│       │   │   ├── connection_manager.cpp
│       │   │   └── connection_internal.hpp
│       │   └── test/            # connection模块单元测试
│       │       ├── CMakeLists.txt
│       │       └── test_connection.cpp
│       ├── protocol/            # protocol模块（独立静态库）
│       │   ├── CMakeLists.txt
│       │   ├── include/
│       │   │   └── protocol/
│       │   │       ├── protocol_handler.hpp
│       │   │       ├── tls_handler.hpp
│       │   │       ├── http1_handler.hpp
│       │   │       └── http2_handler.hpp
│       │   ├── source/
│       │   │   ├── protocol_handler.cpp
│       │   │   ├── tls_handler.cpp
│       │   │   ├── http1_handler.cpp
│       │   │   ├── http2_handler.cpp
│       │   │   └── protocol_internal.hpp
│       │   └── test/            # protocol模块单元测试
│       │       ├── CMakeLists.txt
│       │       └── test_protocol.cpp
│       ├── msg_center/          # msg_center模块（独立静态库）
│       │   ├── CMakeLists.txt
│       │   ├── include/
│       │   │   └── msg_center/
│       │   │       ├── event.hpp
│       │   │       ├── msg_center.hpp
│       │   │       ├── event_queue.hpp
│       │   │       ├── event_loop.hpp
│       │   │       ├── io_thread.hpp
│       │   │       └── worker_pool.hpp
│       │   ├── source/
│       │   │   ├── event.cpp
│       │   │   ├── msg_center.cpp
│       │   │   ├── event_queue.cpp
│       │   │   ├── event_loop.cpp
│       │   │   ├── io_thread.cpp
│       │   │   ├── worker_pool.cpp
│       │   │   └── msg_center_internal.hpp
│       │   └── test/            # msg_center模块单元测试
│       │       ├── CMakeLists.txt
│       │       └── test_msgcenter.cpp
│       ├── debug_chain/         # debug_chain模块（独立静态库）
│       │   ├── CMakeLists.txt
│       │   ├── include/
│       │   │   └── debug_chain/
│       │   │       ├── debug_config.hpp
│       │   │       ├── debug_context.hpp
│       │   │       ├── debug_handler.hpp
│       │   │       ├── debug_chain.hpp
│       │   │       ├── delay_handler.hpp
│       │   │       ├── disconnect_handler.hpp
│       │   │       ├── log_handler.hpp
│       │   │       └── error_code_handler.hpp
│       │   ├── source/
│       │   │   ├── debug_config.cpp
│       │   │   ├── debug_context.cpp
│       │   │   ├── debug_chain.cpp
│       │   │   ├── delay_handler.cpp
│       │   │   ├── disconnect_handler.cpp
│       │   │   ├── log_handler.cpp
│       │   │   ├── error_code_handler.cpp
│       │   │   └── debug_chain_internal.hpp
│       │   └── test/            # debug_chain模块单元测试
│       │       ├── CMakeLists.txt
│       │       └── test_debugchain.cpp
│       ├── callback/            # callback模块（独立静态库）
│       │   ├── CMakeLists.txt
│       │   ├── include/
│       │   │   └── callback/
│       │   │       ├── client_context.hpp
│       │   │       └── callback_strategy.hpp
│       │   ├── source/
│       │   │   ├── callback_strategy.cpp
│       │   │   └── callback_internal.hpp
│       │   └── test/            # callback模块单元测试
│       │       ├── CMakeLists.txt
│       │       └── test_callback.cpp
│       ├── config/              # config模块（独立静态库）
│       │   ├── CMakeLists.txt
│       │   ├── include/
│       │   │   └── config/
│       │   │       └── config.hpp
│       │   ├── source/
│       │   │   ├── config.cpp
│       │   │   └── config_internal.hpp
│       │   └── test/            # config模块单元测试
│       │       ├── CMakeLists.txt
│       │       └── test_config.cpp
│       ├── utils/               # utils模块（独立静态库）
│       │   ├── CMakeLists.txt
│       │   ├── include/
│       │   │   └── utils/
│       │   │       ├── logger.hpp
│       │   │       ├── lockfree_queue.hpp
│       │   │       ├── buffer.hpp
│       │   │       └── statistics.hpp
│       │   ├── source/
│       │   │   ├── logger.cpp
│       │   │   ├── buffer.cpp
│       │   │   ├── statistics.cpp
│       │   │   └── utils_internal.hpp
│       │   └── test/            # utils模块单元测试
│       │       ├── CMakeLists.txt
│       │       └── test_utils.cpp
│       └── test/                # core层集成测试
│           ├── integration_tests/
│           │   ├── CMakeLists.txt
│           │   ├── test_server_startup.cpp
│           │   ├── test_https_request.cpp
│           │   ├── test_concurrent_connections.cpp
│           │   └── test_callback_flow.cpp
│           ├── tools/
│           │   ├── CMakeLists.txt
│           │   ├── cert_generator.cpp
│           │   ├── stress_test.cpp
│           │   └── benchmark.cpp
│           └── sim_client/
│               ├── __init__.py
│               ├── main.py
│               ├── config.py
│               ├── client.py
│               └── scenario.py
├── prompts/
├── scripts/
│   ├── build_project.py
│   ├── server_sim_cli.py
│   └── server_sim_app.py
├── third_party/
│   ├── tongsuo/                         # TLS库（铜锁，支持国密）
│   ├── openssl/                         # TLS库（OpenSSL，备选）
│   ├── json/                            # JSON解析库
│   ├── googletest/                      # 单元测试框架
│   └── ...
├── callbacks/                          # 回调objects
│   ├── CMakeLists.txt
│   ├── callback_1.cpp
│   └── callback_2.cpp
└── README.md
```

### 2.2 Core层模块结构说明

| 模块 | 目录 | 类型 | 说明 |
|-----|------|------|------|
| utils | core/utils/ | 静态库 | 基础工具模块，被所有其他模块依赖 |
| config | core/config/ | 静态库 | 配置模块，依赖utils |
| callback | core/callback/ | 静态库 | 回调策略模块，依赖utils |
| msg_center | core/msg_center/ | 静态库 | 消息中心模块，依赖utils, config |
| debug_chain | core/debug_chain/ | 静态库 | 调试链模块，依赖utils, config |
| protocol | core/protocol/ | 静态库 | 协议处理模块，依赖utils, config, debug_chain |
| connection | core/connection/ | 静态库 | 连接管理模块，依赖utils, config, protocol, msg_center |
| server | core/server/ | 静态库 | 服务主模块，依赖以上所有模块 |
| core (整体) | core/ | 动态库 | 聚合所有子模块，输出最终动态库 |

### 2.3 文件命名规范

| 类型 | 命名规范 | 示例 |
|-----|---------|------|
| 对外头文件 | snake_case，.hpp或.h | server.hpp, connection_manager.h |
| 非对外头文件 | snake_case，.hpp或.h，以_internal后缀 | server_internal.hpp |
| C++源文件 | snake_case，.cpp | server.cpp, connection_manager.cpp |
| Python文件 | snake_case，.py | main.py, config_panel.py |
| CMake文件 | 首字母大写，无后缀 | CMakeLists.txt |
| 配置文件 | snake_case，.json | server_config.json |

---

## 3. 头文件组织规范

### 3.1 头文件分类原则

| 头文件类型 | 存放位置 | 说明 |
|-----------|---------|------|
| 对外头文件 | `<module>/include/` 目录 | 供外部模块调用的接口，需要保持稳定 |
| 非对外头文件 | `<module>/source/` 目录内部 | 模块内部使用，不对外暴露，可随时修改 |

### 3.2 对外头文件规范

**存放位置**: `codes/core/<module>/include/`、`codes/api/adapt/include/`

**要求**:
- 只包含对外暴露的接口
- 保持API稳定，避免频繁修改
- 提供完整的文档注释
- 不包含实现细节
- 尽量减少对其他头文件的依赖

**示例**:
```cpp
// codes/core/server/include/server/server.hpp
#ifndef HTTPS_SERVER_CORE_SERVER_SERVER_HPP
#define HTTPS_SERVER_CORE_SERVER_SERVER_HPP

#include <cstdint>
#include <string>

// 前向声明，不直接包含其他头文件
namespace config { class Config; }
namespace connection { class ConnectionManager; }

namespace server {

/**
 * @brief Server主类
 *
 * 对外接口，负责Server的启动、停止、状态查询
 */
class Server {
public:
    Server();
    ~Server();

    int init(const std::string& config_file);
    int start();
    int stop();
    void cleanup();

private:
    config::Config* config_;
    connection::ConnectionManager* conn_manager_;
};

} // namespace server

#endif // HTTPS_SERVER_CORE_SERVER_SERVER_HPP
```

### 3.3 非对外头文件规范

**存放位置**: `codes/core/<module>/source/` 内部

**要求**:
- 以 `_internal.hpp` 为后缀命名
- 只在模块内部使用
- 可以包含实现细节
- 可以自由修改，不影响外部接口
- 不需要完整的文档注释

**示例**:
```cpp
// codes/core/server/source/server_internal.hpp
#ifndef HTTPS_SERVER_CORE_SERVER_SOURCE_INTERNAL_HPP
#define HTTPS_SERVER_CORE_SERVER_SOURCE_INTERNAL_HPP

#include <server/server.hpp>
#include <connection/connection_manager.hpp>
#include <config/config.hpp>

// 内部辅助函数，不对外暴露
namespace server_internal {

bool validate_config(const config::Config* config);
void setup_signal_handlers();

} // namespace server_internal

#endif // HTTPS_SERVER_CORE_SERVER_SOURCE_INTERNAL_HPP
```

### 3.4 头文件包含规则

| 场景 | 包含方式 |
|------|---------|
| 对外头文件包含其他对外头文件 | 使用 `<>` 包含，相对路径 |
| 源文件包含对外头文件 | 使用 `<>` 包含 |
| 源文件包含非对外头文件 | 使用 `""` 包含 |
| 非对外头文件包含对外头文件 | 使用 `<>` 包含 |
| 非对外头文件包含其他非对外头文件 | 使用 `""` 包含 |

**示例**:
```cpp
// codes/core/server/source/server.cpp
#include <server/server.hpp>           // 对外头文件，<>包含
#include "server_internal.hpp"          // 非对外头文件，""包含
#include <config/config.hpp>            // 对外头文件，<>包含
#include <connection/connection_manager.hpp>

// ... 实现代码 ...
```

---

## 4. 构建设计

### 4.1 编译器要求

| 平台 | 编译器 | 最低版本 |
|-----|-------|---------|
| Windows | MSVC | Visual Studio 2019 (MSVC 19.20) |
| Linux | GCC | GCC 8.0 |
| Linux | Clang | Clang 7.0 |
| Mac | Clang | Xcode 10.0 (Clang 10.0) |

### 4.2 构建工具

- CMake 3.15或更高版本
- Python 3.8或更高版本（适配层）

### 4.3 Core层构建策略

**构建层次**:
1. 每个子模块（server、connection、protocol等）先编译为独立的**静态库**
2. 每个子模块有自己独立的**单元测试**可执行文件
3. 最后将所有子模块静态库聚合，链接成一个**动态库**（libhttps_server_core.so/dll/dylib）
4. api/adapt层封装为**可执行文件**，供Python调用

**模块依赖关系**:
```
utils (基础)
  ├─> config
  ├─> callback
  ├─> msg_center (依赖 config)
  ├─> debug_chain (依赖 config)
  ├─> protocol (依赖 config, debug_chain)
  ├─> connection (依赖 config, protocol, msg_center)
  └─> server (依赖所有模块)
```

---

## 5. CMakeLists规范

### 5.1 CMakeLists组织结构

```
codes/
├── CMakeLists.txt              # 顶层CMakeLists（工程描述）
├── core/
│   ├── CMakeLists.txt          # core聚合CMakeLists（输出动态库）
│   ├── utils/
│   │   └── CMakeLists.txt      # utils模块（静态库+单元测试）
│   ├── config/
│   │   └── CMakeLists.txt      # config模块（静态库+单元测试）
│   ├── callback/
│   │   └── CMakeLists.txt      # callback模块（静态库+单元测试）
│   ├── msg_center/
│   │   └── CMakeLists.txt      # msg_center模块（静态库+单元测试）
│   ├── debug_chain/
│   │   └── CMakeLists.txt      # debug_chain模块（静态库+单元测试）
│   ├── protocol/
│   │   └── CMakeLists.txt      # protocol模块（静态库+单元测试）
│   ├── connection/
│   │   └── CMakeLists.txt      # connection模块（静态库+单元测试）
│   ├── server/
│   │   └── CMakeLists.txt      # server模块（静态库+单元测试）
│   └── test/
│       ├── integration_tests/
│       │   └── CMakeLists.txt  # 集成测试
│       └── tools/
│           └── CMakeLists.txt  # 工具
└── api/
    └── adapt/
        └── CMakeLists.txt      # adapt的CMakeLists（输出可执行文件）
```

### 5.2 CMakeLists基本规则

**每个模块CMakeLists.txt必须满足**:
1. 有一个 `add_library` 用于构建模块静态库
2. 子目录 `test/` 有自己的 CMakeLists.txt，用于构建单元测试
3. 使用明确的源文件列表，不使用 `file(GLOB_RECURSE)`
4. 明确指定依赖关系（target_link_libraries）
5. 清晰的目标名称（hs_<module>_static）

### 5.3 顶层CMakeLists.txt（codes/CMakeLists.txt）

**位置**: `codes/CMakeLists.txt`

**职责**: 描述整个工程，添加子目录，配置三方库

**内容**:
```cmake
cmake_minimum_required(VERSION 3.15)
project(https_server_sim CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# TLS库选择（默认使用Tongsuo铜锁）
option(USE_TONGSUO "Use Tongsuo for TLS (国密支持)" ON)

# 测试选项
option(BUILD_TESTS "Build unit tests and integration tests" ON)

# 三方库路径
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)

# TLS库
if(USE_TONGSUO)
    add_subdirectory(${THIRD_PARTY_DIR}/tongsuo third_party/tongsuo)
    set(TLS_INCLUDE_DIR ${THIRD_PARTY_DIR}/tongsuo/include)
    set(TLS_LIBS ssl crypto)
else()
    add_subdirectory(${THIRD_PARTY_DIR}/openssl third_party/openssl)
    set(TLS_INCLUDE_DIR ${THIRD_PARTY_DIR}/openssl/include)
    set(TLS_LIBS ssl crypto)
endif()

# JSON库
add_subdirectory(${THIRD_PARTY_DIR}/json third_party/json)

# GoogleTest（单元测试框架）
if(BUILD_TESTS)
    add_subdirectory(${THIRD_PARTY_DIR}/googletest third_party/googletest)
    enable_testing()
endif()

# 子目录
add_subdirectory(core)
add_subdirectory(api/adapt)
add_subdirectory(../callbacks callbacks)
```

### 5.4 Core聚合CMakeLists.txt（codes/core/CMakeLists.txt）

**位置**: `codes/core/CMakeLists.txt`

**职责**: 聚合所有子模块，构建最终的core动态库

**内容**:
```cmake
# core的CMakeLists.txt
# 输出：动态库 libhttps_server_core.so
# 策略：先构建各子模块静态库，再聚合成动态库

# 按依赖顺序添加子模块（底层模块先构建）
add_subdirectory(utils)
add_subdirectory(config)
add_subdirectory(callback)
add_subdirectory(msg_center)
add_subdirectory(debug_chain)
add_subdirectory(protocol)
add_subdirectory(connection)
add_subdirectory(server)

# 收集所有子模块静态库
set(CORE_MODULE_LIBS
    hs_server_static
    hs_connection_static
    hs_protocol_static
    hs_msg_center_static
    hs_debug_chain_static
    hs_callback_static
    hs_config_static
    hs_utils_static
)

# 创建core动态库（仅作为聚合，不重新编译源文件）
add_library(https_server_core SHARED
    # 这里可以放置一个空的cpp文件，或者使用下面的技巧
    ${CMAKE_CURRENT_SOURCE_DIR}/core_dummy.cpp
)

# 链接所有子模块静态库
target_link_libraries(https_server_core PUBLIC
    ${CORE_MODULE_LIBS}
    ${TLS_LIBS}
)

# 传递头文件路径（所有子模块的include目录）
target_include_directories(https_server_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/config/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/callback/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/msg_center/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/debug_chain/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/protocol/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/connection/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/server/include>
    $<INSTALL_INTERFACE:include>
)

# 编译定义
if(USE_TONGSUO)
    target_compile_definitions(https_server_core PUBLIC USE_TONGSUO)
endif()

# 集成测试
if(BUILD_TESTS)
    add_subdirectory(test/integration_tests)
    add_subdirectory(test/tools)
endif()
```

**注意**: 需要创建一个空的 `core_dummy.cpp` 文件，仅用于满足CMake对SHARED库至少有一个源文件的要求。

### 5.5 子模块CMakeLists.txt示例（以utils为例）

**位置**: `codes/core/utils/CMakeLists.txt`

**职责**: 构建utils模块静态库 + 单元测试

**内容**:
```cmake
# utils模块 CMakeLists.txt
# 输出：静态库 libhs_utils_static.a

# 源文件列表（明确列出，不使用GLOB）
set(UTILS_SOURCES
    source/logger.cpp
    source/buffer.cpp
    source/statistics.cpp
)

# 创建静态库
add_library(hs_utils_static STATIC ${UTILS_SOURCES})

# 头文件路径
target_include_directories(hs_utils_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 编译定义
if(USE_TONGSUO)
    target_compile_definitions(hs_utils_static PRIVATE USE_TONGSUO)
endif()

# 单元测试
if(BUILD_TESTS)
    add_subdirectory(test)
endif()
```

### 5.6 子模块单元测试CMakeLists.txt示例（utils/test/CMakeLists.txt）

**位置**: `codes/core/utils/test/CMakeLists.txt`

**职责**: 构建utils模块的单元测试可执行文件

**内容**:
```cmake
# utils模块单元测试 CMakeLists.txt
# 输出：可执行文件 test_utils

set(TEST_UTILS_SOURCES
    test_utils.cpp
)

add_executable(test_utils ${TEST_UTILS_SOURCES})

target_include_directories(test_utils PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(test_utils PRIVATE
    hs_utils_static
    gtest
    gtest_main
)

# 注册测试
add_test(NAME UtilsTest COMMAND test_utils)
```

### 5.7 config模块CMakeLists.txt示例（依赖utils）

**位置**: `codes/core/config/CMakeLists.txt`

**内容**:
```cmake
# config模块 CMakeLists.txt
# 输出：静态库 libhs_config_static.a

set(CONFIG_SOURCES
    source/config.cpp
)

add_library(hs_config_static STATIC ${CONFIG_SOURCES})

target_include_directories(hs_config_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 依赖utils模块
target_link_libraries(hs_config_static PUBLIC
    hs_utils_static
    nlohmann_json
)

if(USE_TONGSUO)
    target_compile_definitions(hs_config_static PRIVATE USE_TONGSUO)
endif()

if(BUILD_TESTS)
    add_subdirectory(test)
endif()
```

### 5.8 server模块CMakeLists.txt示例（顶层业务模块）

**位置**: `codes/core/server/CMakeLists.txt`

**内容**:
```cmake
# server模块 CMakeLists.txt
# 输出：静态库 libhs_server_static.a

set(SERVER_SOURCES
    source/server.cpp
)

add_library(hs_server_static STATIC ${SERVER_SOURCES})

target_include_directories(hs_server_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 依赖所有其他模块
target_link_libraries(hs_server_static PUBLIC
    hs_connection_static
    hs_protocol_static
    hs_msg_center_static
    hs_debug_chain_static
    hs_callback_static
    hs_config_static
    hs_utils_static
)

if(USE_TONGSUO)
    target_compile_definitions(hs_server_static PRIVATE USE_TONGSUO)
endif()

if(BUILD_TESTS)
    add_subdirectory(test)
endif()
```

### 5.9 adapt的CMakeLists.txt（codes/api/adapt/CMakeLists.txt）

**位置**: `codes/api/adapt/CMakeLists.txt`

**职责**: 构建adapt可执行文件，供Python调用

**内容**:
```cmake
# adapt的CMakeLists.txt
# 输出：可执行文件 https_server_adapt

set(ADAPT_SOURCES
    source/server_adapt.cpp
)

add_executable(https_server_adapt ${ADAPT_SOURCES})

target_include_directories(https_server_adapt PUBLIC
    include
)

target_link_libraries(https_server_adapt PRIVATE
    https_server_core
)
```

**说明**: api/adapt层若需新增功能，均封装为可执行文件，通过标准输入输出或本地通信与Python交互。

---

## 6. 三方库选型与使用

### 6.1 三方库列表

| 库名称 | 用途 | 版本要求 | 源码路径 |
|-------|------|---------|---------|
| Tongsuo（铜锁） | TLS协议实现（国密首选） | 8.3+ | third_party/tongsuo |
| OpenSSL | TLS协议实现（备选） | 1.1.1+ | third_party/openssl |
| nlohmann/json | JSON解析 | 3.10+ | third_party/json |
| GoogleTest | 单元测试框架 | 1.11+ | third_party/googletest |

### 6.2 Tongsuo（铜锁）使用说明

**用途**:
- TLS握手处理
- 加密解密
- 证书管理
- 国密SM2/SM3/SM4完整支持

**版本要求**:
- Tongsuo 8.3 或更高版本
- 基于OpenSSL 1.1.1分支，完全兼容OpenSSL API

**国密支持**:
- 支持GM/T 0024-2014（TLS 1.2国密）
- 支持GM/T 0126-2022（TLS 1.3国密）
- 支持SM2签名、SM3哈希、SM4加密
- 内置国密Cipher Suites：
  - TLS_ECDHE_SM2_WITH_SM4_SM3（TLS 1.2）
  - TLS_SM2_WITH_SM4_SM3（TLS 1.2）
  - TLS_SM4_GCM_SM3（TLS 1.3）

**使用方式**:
- 通过防腐层（TlsHandler类）封装Tongsuo API
- 核心层不直接调用Tongsuo函数
- 所有Tongsuo调用通过TlsHandler类进行
- 与OpenSSL API兼容，可无缝切换

**CMake切换选项**:
```cmake
option(USE_TONGSUO "Use Tongsuo for TLS (default ON)" ON)

if(USE_TONGSUO)
    add_subdirectory(third_party/tongsuo)
    target_link_libraries(hs_protocol_static PRIVATE ssl crypto)
    target_compile_definitions(hs_protocol_static PRIVATE USE_TONGSUO)
else()
    add_subdirectory(third_party/openssl)
    target_link_libraries(hs_protocol_static PRIVATE ssl crypto)
endif()
```

### 6.3 GoogleTest 使用说明

**用途**:
- 各模块单元测试
- 测试断言
- 测试用例管理

**使用方式**:
- 每个模块的test/目录下有独立的单元测试
- 链接gtest和gtest_main
- 使用add_test注册到CTest

**示例**:
```cpp
#include <gtest/gtest.h>
#include <utils/logger.hpp>

TEST(UtilsTest, LoggerInit) {
    utils::Logger logger;
    EXPECT_TRUE(logger.init("test.log"));
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

---

## 7. 编译与构建流程

### 7.1 依赖安装方式

- **Windows**: 无特殊要求，所有三方库源码编译
- **Linux**: 需要安装基础构建工具（build-essential）
- **Mac**: 需要安装Xcode Command Line Tools

### 7.2 构建步骤

**Linux/Mac**:
```bash
mkdir -p build && cd build
cmake ../codes
make -j$(nproc)
```

**Windows**:
```bash
mkdir build && cd build
cmake ../codes -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

### 7.3 运行测试

```bash
# 运行所有测试（单元测试 + 集成测试）
cd build
ctest --output-on-failure

# 或运行单个模块的单元测试
./core/utils/test/test_utils
./core/server/test/test_server
```

### 7.4 构建产物

| 产物 | 类型 | 位置 | 说明 |
|-----|------|------|------|
| libhs_utils_static.a | 静态库 | build/core/utils/ | utils模块静态库 |
| libhs_config_static.a | 静态库 | build/core/config/ | config模块静态库 |
| libhs_callback_static.a | 静态库 | build/core/callback/ | callback模块静态库 |
| libhs_msg_center_static.a | 静态库 | build/core/msg_center/ | msg_center模块静态库 |
| libhs_debug_chain_static.a | 静态库 | build/core/debug_chain/ | debug_chain模块静态库 |
| libhs_protocol_static.a | 静态库 | build/core/protocol/ | protocol模块静态库 |
| libhs_connection_static.a | 静态库 | build/core/connection/ | connection模块静态库 |
| libhs_server_static.a | 静态库 | build/core/server/ | server模块静态库 |
| libhttps_server_core.so | 动态库 | build/core/ | 核心动态库（聚合所有模块） |
| test_utils | 可执行文件 | build/core/utils/test/ | utils单元测试 |
| test_server | 可执行文件 | build/core/server/test/ | server单元测试 |
| test_* | 可执行文件 | build/core/*/test/ | 各模块单元测试 |
| https_server_adapt | 可执行文件 | build/api/adapt/ | C接口适配层程序 |
| integration_tests | 可执行文件 | build/core/test/integration_tests/ | 集成测试程序 |

---

## 8. 测试工程设计

### 8.1 测试分类

| 测试类型 | 位置 | 说明 |
|---------|------|------|
| 模块单元测试 | codes/core/<module>/test/ | 每个模块独立的单元测试，使用GoogleTest |
| 集成测试 | codes/core/test/integration_tests | 模块间集成测试，测试多个模块协作 |
| 其他工具 | codes/core/test/tools | 辅助工具（证书生成、压力测试、性能基准测试） |
| 测试Client | codes/core/test/sim_client | 测试用Client模拟器（Python） |

### 8.2 单元测试设计

**每个模块必须配有单元测试**:

| 模块 | 测试文件 | 测试范围 |
|-----|---------|---------|
| utils | test_utils.cpp | Logger、Buffer、Statistics等工具类 |
| config | test_config.cpp | 配置加载、验证、访问 |
| callback | test_callback.cpp | 回调策略注册、调用 |
| msg_center | test_msgcenter.cpp | 事件队列、事件循环、消息分发 |
| debug_chain | test_debugchain.cpp | 调试链配置、Handler执行 |
| protocol | test_protocol.cpp | TLS握手、HTTP解析、协议处理 |
| connection | test_connection.cpp | 连接状态机、连接管理 |
| server | test_server.cpp | Server初始化、启动、停止 |

**单元测试示例 - test_utils.cpp**:
```cpp
#include <gtest/gtest.h>
#include <utils/logger.hpp>
#include <utils/buffer.hpp>

namespace utils {
namespace test {

TEST(UtilsTest, LoggerBasic) {
    Logger logger;
    EXPECT_EQ(logger.init("test_utils.log"), 0);
    logger.info("Test message");
    logger.shutdown();
}

TEST(UtilsTest, BufferWriteRead) {
    Buffer buf(1024);
    const uint8_t data[] = "Hello World";
    size_t write_len = 0;

    EXPECT_EQ(buf.write(data, sizeof(data), &write_len), 0);
    EXPECT_EQ(write_len, sizeof(data));

    uint8_t read_buf[128] = {0};
    size_t read_len = 0;
    EXPECT_EQ(buf.read(read_buf, sizeof(read_buf), &read_len), 0);
    EXPECT_EQ(read_len, sizeof(data));
    EXPECT_STREQ((const char*)read_buf, (const char*)data);
}

} // namespace test
} // namespace utils

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

### 8.3 集成测试设计

**测试范围**:
- Server完整启动流程
- HTTPS请求响应完整流程
- 1000并发连接测试
- 回调流程完整测试
- Graceful Shutdown测试

**集成测试CMakeLists.txt示例**:
```cmake
# codes/core/test/integration_tests/CMakeLists.txt

set(INTEGRATION_TEST_SOURCES
    test_server_startup.cpp
    test_https_request.cpp
    test_concurrent_connections.cpp
    test_callback_flow.cpp
)

add_executable(integration_tests ${INTEGRATION_TEST_SOURCES})

target_link_libraries(integration_tests PRIVATE
    https_server_core
    gtest
    gtest_main
)

add_test(NAME IntegrationTest COMMAND integration_tests)
```

### 8.4 其他工具设计

| 工具名称 | 位置 | 说明 |
|---------|------|------|
| cert_generator | codes/core/test/tools/cert_generator.cpp | 证书生成工具 |
| stress_test | codes/core/test/tools/stress_test.cpp | 压力测试工具 |
| benchmark | codes/core/test/tools/benchmark.cpp | 性能基准测试工具 |

### 8.5 测试用Client设计

**功能**:
- 支持HTTPS，可配置证书
- 支持HTTP/1.1和HTTP/2
- 通过JSON Schema配置文件生成报文
- 支持建联、发送报文、接收响应

**JSON Schema配置**:
```json
{
    "$schema": "http://json-schema.org/draft-07/schema#",
    "type": "object",
    "properties": {
        "server": {
            "host": "127.0.0.1",
            "port": 8443,
            "tls_version": "1.2",
            "ca_cert": "/path/to/ca.pem"
        },
        "client": {
            "http_version": "1.1|2",
            "method": "POST",
            "path": "/api/test",
            "headers": {},
            "debug": {}
        },
        "payload": {
            "type": "file|hex|base64|random",
            "value": "/path/to/payload.bin"
        },
        "scenario": {
            "concurrent_clients": 10,
            "requests_per_client": 100,
            "keep_alive": true
        }
    }
}
```

---

## 9. 防腐层设计

### 9.1 防腐层设计目标

防腐层（Anticorruption Layer）的设计目标是：
- 隔离核心层与外部依赖
- 提供统一的接口，隐藏外部库的实现细节
- 便于后续替换外部依赖
- 降低核心层与外部库的耦合度

### 9.2 防腐层结构

```
┌─────────────────────────────────────────────────────────┐
│                    核心层 (Core)                         │
│  Server | Connection | Protocol | MsgCenter | ...      │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│                   防腐层 (Anticorruption Layer)         │
│  ┌──────────────────┐  ┌──────────────────┐          │
│  │  TlsHandler      │  │  Config          │          │
│  │  (Tongsuo/OpenSSL│  │  (JSON封装)      │          │
│  │  封装)           │  │                  │          │
│  └──────────────────┘  └──────────────────┘          │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│                   外部依赖 (External Dependencies)      │
│  ┌──────────────────┐  ┌──────────────────┐          │
│  │ Tongsuo/OpenSSL  │  │  nlohmann/json   │          │
│  └──────────────────┘  └──────────────────┘          │
└─────────────────────────────────────────────────────────┘
```

### 9.3 TlsHandler 防腐层设计

**职责**:
- 封装Tongsuo/OpenSSL的TLS握手接口
- 封装Tongsuo/OpenSSL的加密解密接口
- 封装Tongsuo/OpenSSL的证书管理接口
- 提供统一的TLS处理接口，屏蔽Tongsuo和OpenSSL的差异

**接口定义**:
```cpp
namespace protocol {

class TlsHandler {
public:
    TlsHandler(const config::Config* config);
    ~TlsHandler();

    int init(connection::Connection* conn);
    int read(uint8_t* data, size_t len, size_t* out_len);
    int write(const uint8_t* data, size_t len, size_t* out_len);
    int close();
    bool is_handshake_done() const;
    int continue_handshake();
    SSL* get_ssl();

private:
    int init_ssl_context();
    int configure_certificates();
    int configure_cipher_suites();
    int configure_international_ciphers();
    int configure_sm2_ciphers();
    int configure_custom_ciphers();
};

} // namespace protocol
```

**使用约束**:
- 核心层只能通过TlsHandler类访问TLS功能
- 核心层不能直接包含Tongsuo/OpenSSL头文件
- 所有Tongsuo/OpenSSL调用必须在TlsHandler类内部完成
- 通过USE_TONGSUO宏在编译时切换TLS库实现

### 9.4 Config 防腐层设计

**职责**:
- 封装JSON库的解析接口
- 封装JSON库的访问接口
- 提供统一的配置访问接口

**接口定义**:
```cpp
namespace config {

class Config {
public:
    Config();
    ~Config();

    int load_from_file(const std::string& file_path);
    int validate() const;

    const ServerConfig& get_server() const;
    const CertificatesConfig& get_certificates() const;
    const DebugConfig& get_debug() const;
    const CallbacksConfig& get_callbacks() const;
    const LoggingConfig& get_logging() const;
    const Http2Config& get_http2() const;
};

} // namespace config
```

**使用约束**:
- 核心层只能通过Config类访问配置
- 核心层不能直接包含JSON库头文件
- 所有JSON操作必须在Config类内部完成

### 9.5 防腐层扩展规范

**新增外部依赖时**:
1. 在防腐层创建对应的封装类
2. 定义清晰的接口
3. 核心层通过封装类访问外部依赖
4. 不能在核心层直接引用外部库

**替换外部依赖时**:
1. 保持防腐层接口不变
2. 仅修改防腐层内部实现
3. 核心层代码无需修改

---

**文档结束**
