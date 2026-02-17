# macOS Clang 交叉编译工具链配置
# 用于从 Linux/Windows 交叉编译到 macOS
# 注意: macOS交叉编译比较复杂，通常建议在macOS本机编译

set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# macOS SDK 路径（需要根据实际情况调整）
set(OSXCROSS_SDK "/opt/osxcross/SDK/MacOSX12.0.sdk")
set(OSXCROSS_TARGET_DIR "/opt/osxcross")

# 编译器
set(CMAKE_C_COMPILER ${OSXCROSS_TARGET_DIR}/bin/o64-clang)
set(CMAKE_CXX_COMPILER ${OSXCROSS_TARGET_DIR}/bin/o64-clang++)

# sysroot
set(CMAKE_SYSROOT ${OSXCROSS_SDK})

# macOS目标版本
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment version")

# 搜索行为
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# macOS特定标志
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
