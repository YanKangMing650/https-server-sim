# DebugChain模块CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(https_server_sim_debug_chain)

set(DEBUGCHAIN_SOURCES
    source/debug_chain.cpp
)

set(DEBUGCHAIN_HEADERS
    include/debug_chain/debug_config.hpp
    include/debug_chain/debug_context.hpp
    include/debug_chain/debug_handler.hpp
    include/debug_chain/debug_chain.hpp
)

add_library(debug_chain STATIC ${DEBUGCHAIN_SOURCES} ${DEBUGCHAIN_HEADERS})
target_compile_features(debug_chain PUBLIC cxx_std_17)
target_include_directories(debug_chain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(debug_chain PUBLIC utils)

# add_subdirectory(test)
