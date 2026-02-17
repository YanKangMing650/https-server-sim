# DebugChain模块CMakeLists.txt

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

# DebugChain模块测试用例
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/test_debug_chain.cpp)
    set(DEBUGCHAIN_TEST_SOURCES
        test/test_debug_chain.cpp
    )
    list(APPEND CORE_TEST_SOURCES ${DEBUGCHAIN_TEST_SOURCES})
    set(CORE_TEST_SOURCES ${CORE_TEST_SOURCES} PARENT_SCOPE)
endif()
