# MsgCenter模块CMakeLists.txt

set(MSGCENTER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/event_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/event_loop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/worker_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/io_thread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/msg_center.cpp
)

set(MSGCENTER_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/msg_center/event.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/msg_center/event_queue.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/msg_center/event_loop.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/msg_center/worker_pool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/msg_center/io_thread.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/msg_center/msg_center.hpp
)

add_library(msg_center STATIC ${MSGCENTER_SOURCES} ${MSGCENTER_HEADERS})
target_compile_features(msg_center PUBLIC cxx_std_17)
target_include_directories(msg_center PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(msg_center PUBLIC utils)

# MsgCenter模块测试用例
set(MSGCENTER_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test/test_msg_center.cpp
)

# 添加测试到全局测试列表
list(APPEND CORE_TEST_SOURCES ${MSGCENTER_TEST_SOURCES})
set(CORE_TEST_SOURCES ${CORE_TEST_SOURCES} PARENT_SCOPE)
