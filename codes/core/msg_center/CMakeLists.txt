# MsgCenter模块CMakeLists.txt

set(MSGCENTER_SOURCES
    source/event.cpp
)

set(MSGCENTER_HEADERS
    include/msg_center/event.hpp
)

add_library(msg_center STATIC ${MSGCENTER_SOURCES} ${MSGCENTER_HEADERS})
target_compile_features(msg_center PUBLIC cxx_std_17)
target_include_directories(msg_center PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(msg_center PUBLIC utils)

# MsgCenter模块测试用例
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/test_msg_center.cpp)
    set(MSGCENTER_TEST_SOURCES
        test/test_msg_center.cpp
    )
    list(APPEND CORE_TEST_SOURCES ${MSGCENTER_TEST_SOURCES})
    set(CORE_TEST_SOURCES ${CORE_TEST_SOURCES} PARENT_SCOPE)
endif()
