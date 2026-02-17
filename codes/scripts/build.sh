#!/bin/bash
#
# HTTPS Server Simulator - 编译脚本 (Bash版本)
# 适用于 Linux/macOS
#
# 使用方式:
#   ./build.sh [options]
#
# 选项:
#   --clean           清理构建目录
#   --release         Release模式
#   --debug           Debug模式 (默认)
#   --tests           构建并运行测试
#   --use-openssl     使用OpenSSL代替Tongsuo
#   --toolchain <tc>  工具链: gcc/clang
#   --platform <p>    目标平台: linux/windows/mac
#   -j|--jobs <n>     并行编译任务数
#   --help            显示帮助
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
BUILD_DIR="${PROJECT_ROOT}/build"
CODES_DIR="${PROJECT_ROOT}/codes"

# 默认配置
BUILD_TYPE="Debug"
CLEAN_BUILD=0
BUILD_TESTS=0
USE_TONGSUO=1
TOOLCHAIN=""
TARGET_PLATFORM=""
JOBS=""

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

show_help() {
    cat <<EOF
HTTPS Server Simulator - 编译脚本

使用方式:
  $0 [options]

选项:
  --clean           清理构建目录后重新构建
  --release         Release模式
  --debug           Debug模式 (默认)
  --tests           构建并运行测试
  --use-openssl     使用OpenSSL代替Tongsuo (铜锁)
  --toolchain <tc>  指定工具链: gcc, clang
  --platform <p>    指定目标平台: linux, windows, mac
  -j|--jobs <n>     并行编译任务数
  --configure-only  仅配置CMake
  --build-only      仅构建，不重新配置
  --help            显示此帮助信息

示例:
  $0                          # 本地默认编译 (Debug)
  $0 --release                # Release模式编译
  $0 --clean --tests          # 清理后编译并运行测试
  $0 --toolchain clang        # 使用Clang编译
  $0 --platform windows       # 交叉编译到Windows
EOF
}

# 解析参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN_BUILD=1
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --tests)
            BUILD_TESTS=1
            shift
            ;;
        --use-openssl)
            USE_TONGSUO=0
            shift
            ;;
        --toolchain)
            TOOLCHAIN="$2"
            shift 2
            ;;
        --platform)
            TARGET_PLATFORM="$2"
            shift 2
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        --configure-only)
            CONFIGURE_ONLY=1
            shift
            ;;
        --build-only)
            BUILD_ONLY=1
            shift
            ;;
        --help)
            show_help
            exit 0
            ;;
        *)
            log_error "未知选项: $1"
            show_help
            exit 1
            ;;
    esac
done

# 检测主机平台
detect_host_platform() {
    case "$(uname -s)" in
        Linux*) echo "linux" ;;
        Darwin*) echo "mac" ;;
        MINGW*|MSYS*|CYGWIN*) echo "windows" ;;
        *) echo "$(uname -s)" ;;
    esac
}

HOST_PLATFORM=$(detect_host_platform)
if [ -z "$TARGET_PLATFORM" ]; then
    TARGET_PLATFORM="$HOST_PLATFORM"
fi

# 检测默认工具链
if [ -z "$TOOLCHAIN" ]; then
    if [ "$TARGET_PLATFORM" = "mac" ]; then
        TOOLCHAIN="clang"
    else
        TOOLCHAIN="gcc"
    fi
fi

# 默认并行数
if [ -z "$JOBS" ]; then
    if [ "$(uname -s)" = "Darwin" ]; then
        JOBS=$(sysctl -n hw.ncpu)
    else
        JOBS=$(nproc)
    fi
fi

# 打印配置
echo "=========================================="
echo "HTTPS Server Simulator - 编译配置"
echo "=========================================="
echo "项目根目录: $PROJECT_ROOT"
echo "构建目录:   $BUILD_DIR"
echo "主机平台:   $HOST_PLATFORM"
echo "目标平台:   $TARGET_PLATFORM"
echo "工具链:     $TOOLCHAIN"
echo "构建类型:   $BUILD_TYPE"
echo "并行任务:   $JOBS"
echo "构建测试:   $([ "$BUILD_TESTS" = 1 ] && echo "是" || echo "否")"
echo "TLS库:      $([ "$USE_TONGSUO" = 1 ] && echo "Tongsuo (铜锁)" || echo "OpenSSL")"
echo "=========================================="
echo

# 清理
if [ "$CLEAN_BUILD" = 1 ]; then
    log_info "清理构建目录..."
    rm -rf "$BUILD_DIR"
fi

# 创建构建目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# CMake配置
if [ -z "$BUILD_ONLY" ]; then
    log_info "配置 CMake..."

    CMAKE_ARGS=()

    # 生成器
    if [ "$TARGET_PLATFORM" = "windows" ]; then
        if [ "$TOOLCHAIN" = "mingw" ] || [ "$HOST_PLATFORM" != "windows" ]; then
            CMAKE_ARGS+=("-G" "MinGW Makefiles")
        else
            CMAKE_ARGS+=("-G" "Visual Studio 17 2022" "-A" "x64")
        fi
    else
        CMAKE_ARGS+=("-G" "Unix Makefiles")
    fi

    # 工具链
    if [ "$TOOLCHAIN" = "clang" ]; then
        CMAKE_ARGS+=("-DCMAKE_C_COMPILER=clang" "-DCMAKE_CXX_COMPILER=clang++")
    elif [ "$TOOLCHAIN" = "gcc" ]; then
        CMAKE_ARGS+=("-DCMAKE_C_COMPILER=gcc" "-DCMAKE_CXX_COMPILER=g++")
    fi

    # 交叉编译工具链文件
    if [ "$TARGET_PLATFORM" != "$HOST_PLATFORM" ]; then
        TOOLCHAIN_FILE="${SCRIPT_DIR}/toolchains/"
        if [ "$TARGET_PLATFORM" = "windows" ]; then
            TOOLCHAIN_FILE+="mingw-w64.cmake"
        elif [ "$TARGET_PLATFORM" = "linux" ]; then
            TOOLCHAIN_FILE+="linux-gcc.cmake"
        elif [ "$TARGET_PLATFORM" = "mac" ]; then
            TOOLCHAIN_FILE+="macos-clang.cmake"
        fi
        if [ -f "$TOOLCHAIN_FILE" ]; then
            CMAKE_ARGS+=("-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}")
        else
            log_warn "工具链文件不存在: $TOOLCHAIN_FILE"
        fi
    fi

    # 构建选项
    CMAKE_ARGS+=("-DCMAKE_BUILD_TYPE=${BUILD_TYPE}")
    CMAKE_ARGS+=("-DBUILD_TESTS=$([ "$BUILD_TESTS" = 1 ] && echo "ON" || echo "OFF")")
    CMAKE_ARGS+=("-DUSE_TONGSUO=$([ "$USE_TONGSUO" = 1 ] && echo "ON" || echo "OFF")")

    CMAKE_ARGS+=("$CODES_DIR")

    log_info "CMake参数: ${CMAKE_ARGS[*]}"
    cmake "${CMAKE_ARGS[@]}"
fi

if [ "$CONFIGURE_ONLY" = 1 ]; then
    log_info "CMake配置完成"
    exit 0
fi

# 构建
log_info "构建项目..."
if [ "$TARGET_PLATFORM" = "windows" ] && [ "$HOST_PLATFORM" = "windows" ]; then
    cmake --build . --config "$BUILD_TYPE" -- "/m:$JOBS"
else
    cmake --build . --config "$BUILD_TYPE" -- "-j$JOBS"
fi

# 运行测试
if [ "$BUILD_TESTS" = 1 ]; then
    log_info "运行测试..."
    ctest --output-on-failure --build-config "$BUILD_TYPE"
fi

echo
echo "=========================================="
log_info "构建完成!"
echo "=========================================="
echo "构建产物目录: $BUILD_DIR"
echo "库文件:       $BUILD_DIR/lib"
echo "可执行文件:   $BUILD_DIR/bin"
