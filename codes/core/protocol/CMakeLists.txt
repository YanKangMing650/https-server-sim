# Protocol模块CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(https_server_sim_protocol)

set(PROTOCOL_SOURCES
    source/protocol_handler.cpp
)

set(PROTOCOL_HEADERS
    include/protocol/protocol_handler.hpp
)

add_library(protocol STATIC ${PROTOCOL_SOURCES} ${PROTOCOL_HEADERS})
target_compile_features(protocol PUBLIC cxx_std_17)
target_include_directories(protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(protocol PUBLIC utils)

# add_subdirectory(test)
