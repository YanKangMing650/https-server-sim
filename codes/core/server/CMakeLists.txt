# Server模块CMakeLists.txt

set(SERVER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/server.cpp
)

set(SERVER_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/server/server.hpp
)

add_library(server STATIC ${SERVER_SOURCES} ${SERVER_HEADERS})
target_compile_features(server PUBLIC cxx_std_17)
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(server PUBLIC config connection msg_center callback utils)

# Server模块测试用例
set(SERVER_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test/test_server.cpp
)

# 添加测试到全局测试列表
list(APPEND CORE_TEST_SOURCES ${SERVER_TEST_SOURCES})
set(CORE_TEST_SOURCES ${CORE_TEST_SOURCES} PARENT_SCOPE)
