# Core层CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(https_server_sim_core)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 编译选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 添加子模块（按依赖顺序）
add_subdirectory(utils)
add_subdirectory(config)
add_subdirectory(callback)
add_subdirectory(debug_chain)
add_subdirectory(protocol)
add_subdirectory(msg_center)
add_subdirectory(connection)
add_subdirectory(server)

# 创建core_dummy.cpp来链接所有静态库为一个动态库
# 或者创建一个聚合库
add_library(core SHARED core_dummy.cpp)
target_link_libraries(core
    PUBLIC
        server
        connection
        msg_center
        protocol
        callback
        debug_chain
        config
        utils
)

target_compile_features(core PUBLIC cxx_std_17)
target_include_directories(core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/server/include
        ${CMAKE_CURRENT_SOURCE_DIR}/connection/include
        ${CMAKE_CURRENT_SOURCE_DIR}/protocol/include
        ${CMAKE_CURRENT_SOURCE_DIR}/msg_center/include
        ${CMAKE_CURRENT_SOURCE_DIR}/callback/include
        ${CMAKE_CURRENT_SOURCE_DIR}/debug_chain/include
        ${CMAKE_CURRENT_SOURCE_DIR}/config/include
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/include
)
