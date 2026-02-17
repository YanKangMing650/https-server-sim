# Connection模块CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(https_server_sim_connection)

set(CONNECTION_SOURCES
    source/connection.cpp
)

set(CONNECTION_HEADERS
    include/connection/connection.hpp
)

add_library(connection STATIC ${CONNECTION_SOURCES} ${CONNECTION_HEADERS})
target_compile_features(connection PUBLIC cxx_std_17)
target_include_directories(connection PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(connection PUBLIC utils protocol)

# add_subdirectory(test)
