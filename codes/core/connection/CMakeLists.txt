# Connection模块CMakeLists.txt

set(CONNECTION_SOURCES
    source/connection.cpp
)

set(CONNECTION_HEADERS
    include/connection/connection.hpp
)

add_library(connection STATIC ${CONNECTION_SOURCES} ${CONNECTION_HEADERS})
target_compile_features(connection PUBLIC cxx_std_17)
target_include_directories(connection PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(connection PUBLIC utils protocol)

# Connection模块测试用例
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/test_connection.cpp)
    set(CONNECTION_TEST_SOURCES
        test/test_connection.cpp
    )
    list(APPEND CORE_TEST_SOURCES ${CONNECTION_TEST_SOURCES})
    set(CORE_TEST_SOURCES ${CORE_TEST_SOURCES} PARENT_SCOPE)
endif()
