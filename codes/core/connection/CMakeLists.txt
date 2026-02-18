# Connection模块CMakeLists.txt

set(CONNECTION_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/connection_manager.cpp
)

set(CONNECTION_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/connection/connection.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/connection/connection_manager.hpp
)

add_library(connection STATIC ${CONNECTION_SOURCES} ${CONNECTION_HEADERS})
target_compile_features(connection PUBLIC cxx_std_17)
target_include_directories(connection PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(connection PUBLIC utils protocol)

# Connection模块测试用例
set(CONNECTION_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test/test_connection.cpp
)

# 添加测试到全局测试列表
list(APPEND CORE_TEST_SOURCES ${CONNECTION_TEST_SOURCES})
set(CORE_TEST_SOURCES ${CORE_TEST_SOURCES} PARENT_SCOPE)
