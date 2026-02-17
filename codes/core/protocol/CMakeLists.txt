# Protocol模块CMakeLists.txt

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

# Protocol模块测试用例
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/test_protocol.cpp)
    set(PROTOCOL_TEST_SOURCES
        test/test_protocol.cpp
    )
    list(APPEND CORE_TEST_SOURCES ${PROTOCOL_TEST_SOURCES})
    set(CORE_TEST_SOURCES ${CORE_TEST_SOURCES} PARENT_SCOPE)
endif()
