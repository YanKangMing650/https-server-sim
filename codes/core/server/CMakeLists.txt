# Server模块CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(https_server_sim_server)

set(SERVER_SOURCES
    source/server.cpp
)

set(SERVER_HEADERS
    include/server/server.hpp
)

add_library(server STATIC ${SERVER_SOURCES} ${SERVER_HEADERS})
target_compile_features(server PUBLIC cxx_std_17)
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(server PUBLIC config connection callback utils)

# add_subdirectory(test)
