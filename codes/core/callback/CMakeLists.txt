# Callback模块CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(https_server_sim_callback)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CALLBACK_SOURCES
    source/callback.cpp
)

set(CALLBACK_HEADERS
    include/callback/client_context.h
    include/callback/callback.h
    include/callback/callback.hpp
    # 兼容性头文件
    include/callback/client_context.hpp
    include/callback/callback_strategy.hpp
)

add_library(callback STATIC ${CALLBACK_SOURCES} ${CALLBACK_HEADERS})
target_compile_features(callback PUBLIC cxx_std_17)
target_include_directories(callback PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 测试（默认不构建，需要时设置 BUILD_CALLBACK_TESTS=ON）
option(BUILD_CALLBACK_TESTS "Build Callback module unit tests" OFF)
if(BUILD_CALLBACK_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
