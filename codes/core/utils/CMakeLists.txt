# Utils模块CMakeLists.txt

set(UTILS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/time.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/statistics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/config.cpp
)

set(UTILS_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utils/error.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utils/time.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utils/buffer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utils/logger.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utils/lockfree_queue.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utils/statistics.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utils/config.hpp
)

add_library(utils STATIC ${UTILS_SOURCES} ${UTILS_HEADERS})
target_compile_features(utils PUBLIC cxx_std_17)
target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(utils PRIVATE ${THIRD_PARTY_DIR})

# Utils模块测试用例
set(UTILS_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test/test_utils.cpp
)

# 添加测试到全局测试列表
list(APPEND CORE_TEST_SOURCES ${UTILS_TEST_SOURCES})
set(CORE_TEST_SOURCES ${CORE_TEST_SOURCES} PARENT_SCOPE)
