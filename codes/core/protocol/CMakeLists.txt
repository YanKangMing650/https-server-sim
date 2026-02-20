# Protocol模块CMakeLists.txt

set(PROTOCOL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/protocol_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/http_message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/http_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/http2_stream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/hpack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/tls_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/config_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/protocol_handler_factory.cpp
)

set(PROTOCOL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/protocol/protocol.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/protocol/protocol_types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/protocol/protocol_utils.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/protocol/protocol_handler.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/protocol/protocol_factory.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/protocol/config_converter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/protocol/protocol_handler_factory.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/protocol/http_message.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/protocol/http_parser.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/protocol/http2_stream.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/protocol/hpack.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/protocol/tls_handler.hpp
)

add_library(protocol STATIC ${PROTOCOL_SOURCES} ${PROTOCOL_HEADERS})
target_compile_features(protocol PUBLIC cxx_std_17)
target_include_directories(protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../connection/include)
target_include_directories(protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../callback/include)
target_include_directories(protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../config/include)

# 尝试查找OpenSSL（可选）
find_package(OpenSSL QUIET)

# 添加tongsuo include目录（如果存在）
if(DEFINED THIRD_PARTY_DIR AND EXISTS ${THIRD_PARTY_DIR}/tongsuo)
    target_include_directories(protocol PUBLIC ${THIRD_PARTY_DIR}/tongsuo/include)
endif()

if(OPENSSL_FOUND)
    target_include_directories(protocol PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_compile_definitions(protocol PUBLIC HAVE_OPENSSL=1)
    target_link_libraries(protocol PUBLIC OpenSSL::SSL OpenSSL::Crypto)
else()
    # 没有OpenSSL时，定义宏禁用完整功能
    target_compile_definitions(protocol PUBLIC HAVE_OPENSSL=0)
endif()

target_link_libraries(protocol PUBLIC utils callback)

# Protocol模块测试用例
set(PROTOCOL_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test/test_protocol.cpp
)

# 添加测试到全局测试列表
list(APPEND CORE_TEST_SOURCES ${PROTOCOL_TEST_SOURCES})
set(CORE_TEST_SOURCES ${CORE_TEST_SOURCES} PARENT_SCOPE)
