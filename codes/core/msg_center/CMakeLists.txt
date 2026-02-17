# MsgCenter模块CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(https_server_sim_msg_center)

set(MSGCENTER_SOURCES
    source/event.cpp
)

set(MSGCENTER_HEADERS
    include/msg_center/event.hpp
)

add_library(msg_center STATIC ${MSGCENTER_SOURCES} ${MSGCENTER_HEADERS})
target_compile_features(msg_center PUBLIC cxx_std_17)
target_include_directories(msg_center PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(msg_center PUBLIC utils)

# add_subdirectory(test)
