# Utils模块CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(https_server_sim_utils)

# 源文件列表
set(UTILS_SOURCES
    source/error.cpp
    source/time.cpp
    source/buffer.cpp
    source/logger.cpp
    source/statistics.cpp
)

# 头文件列表（用于IDE展示）
set(UTILS_HEADERS
    include/utils/error.hpp
    include/utils/time.hpp
    include/utils/buffer.hpp
    include/utils/logger.hpp
    include/utils/lockfree_queue.hpp
    include/utils/statistics.hpp
)

# 创建静态库
add_library(utils STATIC ${UTILS_SOURCES} ${UTILS_HEADERS})

# 设置C++标准
target_compile_features(utils PUBLIC cxx_std_17)

# 包含目录
target_include_directories(utils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 平台特定设置
if(WIN32)
    target_compile_definitions(utils PRIVATE _WIN32_WINNT=0x0601)
endif()

# 测试子目录
# add_subdirectory(test)
