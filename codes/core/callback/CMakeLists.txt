# Callback模块CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(https_server_sim_callback)

set(CALLBACK_SOURCES
    source/callback_strategy.cpp
)

set(CALLBACK_HEADERS
    include/callback/client_context.hpp
    include/callback/callback_strategy.hpp
)

add_library(callback STATIC ${CALLBACK_SOURCES} ${CALLBACK_HEADERS})
target_compile_features(callback PUBLIC cxx_std_17)
target_include_directories(callback PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(callback PUBLIC utils config)

# add_subdirectory(test)
