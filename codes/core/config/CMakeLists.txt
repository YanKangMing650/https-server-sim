# Config模块CMakeLists.txt

set(CONFIG_SOURCES
    source/config.cpp
)

set(CONFIG_HEADERS
    include/config/config.hpp
)

add_library(config STATIC ${CONFIG_SOURCES} ${CONFIG_HEADERS})
target_compile_features(config PUBLIC cxx_std_17)
target_include_directories(config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(config PUBLIC utils)

# Config模块测试用例
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/test_config.cpp)
    set(CONFIG_TEST_SOURCES
        test/test_config.cpp
    )
    list(APPEND CORE_TEST_SOURCES ${CONFIG_TEST_SOURCES})
    set(CORE_TEST_SOURCES ${CORE_TEST_SOURCES} PARENT_SCOPE)
endif()
