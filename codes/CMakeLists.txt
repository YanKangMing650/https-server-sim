# 顶层CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(https_server_sim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 第三方库路径
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)

# 添加第三方库头文件路径
include_directories(
    ${THIRD_PARTY_DIR}/json/include
    ${THIRD_PARTY_DIR}/googletest/googletest/include
    ${THIRD_PARTY_DIR}/spdlog/include
)

# Core层
add_subdirectory(core)

# API层（暂不实现）
# add_subdirectory(api)
