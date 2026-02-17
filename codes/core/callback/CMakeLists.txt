# Callback模块CMakeLists.txt

set(CALLBACK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/callback.cpp
)

set(CALLBACK_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/callback/client_context.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/callback/callback.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/callback/callback.hpp
)

add_library(callback STATIC ${CALLBACK_SOURCES} ${CALLBACK_HEADERS})
target_compile_features(callback PUBLIC cxx_std_17)
target_include_directories(callback PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Callback模块测试用例
set(CALLBACK_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test/test_callback.cpp
)

# 添加测试到全局测试列表
list(APPEND CORE_TEST_SOURCES ${CALLBACK_TEST_SOURCES})
set(CORE_TEST_SOURCES ${CORE_TEST_SOURCES} PARENT_SCOPE)
