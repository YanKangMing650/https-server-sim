# Server模块CMakeLists.txt

set(SERVER_SOURCES
    source/server.cpp
)

set(SERVER_HEADERS
    include/server/server.hpp
)

add_library(server STATIC ${SERVER_SOURCES} ${SERVER_HEADERS})
target_compile_features(server PUBLIC cxx_std_17)
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(server PUBLIC config connection callback utils)

# Server模块测试用例
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/test_server.cpp)
    set(SERVER_TEST_SOURCES
        test/test_server.cpp
    )
    list(APPEND CORE_TEST_SOURCES ${SERVER_TEST_SOURCES})
    set(CORE_TEST_SOURCES ${CORE_TEST_SOURCES} PARENT_SCOPE)
endif()
