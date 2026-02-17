# Config模块CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(https_server_sim_config)

set(CONFIG_SOURCES
    source/config.cpp
)

set(CONFIG_HEADERS
    include/config/config.hpp
)

add_library(config STATIC ${CONFIG_SOURCES} ${CONFIG_HEADERS})
target_compile_features(config PUBLIC cxx_std_17)
target_include_directories(config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(config PUBLIC utils)

# add_subdirectory(test)
