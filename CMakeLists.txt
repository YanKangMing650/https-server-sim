# HTTPS Server Simulator CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(https_server_sim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置第三方库目录
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

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

# 添加 nlohmann/json 头文件目录
if(EXISTS ${THIRD_PARTY_DIR}/nlohmann)
    include_directories(${THIRD_PARTY_DIR}/nlohmann)
endif()

# 添加 googletest 头文件目录（如果存在）
if(EXISTS ${THIRD_PARTY_DIR}/googletest)
    include_directories(
        ${THIRD_PARTY_DIR}/googletest/googletest
        ${THIRD_PARTY_DIR}/googletest/googletest/include
    )
endif()

# 添加 core 层
add_subdirectory(codes/core)
