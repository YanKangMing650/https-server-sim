# macOS 环境准备指南

**版本**: v1.0
**创建日期**: 2026-02-15

---

## 目录

1. [前置依赖安装](#1-前置依赖安装)
2. [三方库下载](#2-三方库下载)
3. [三方库编译](#3-三方库编译)

---

## 1. 前置依赖安装

### 1.1 安装 Xcode Command Line Tools

打开终端，执行：

```bash
xcode-select --install
```

如果已安装，会提示 "command line tools are already installed"。

---

### 1.2 安装 Homebrew（推荐）

Homebrew 是 macOS 的包管理器，用于安装 CMake 等工具：

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

安装完成后，执行：

```bash
brew install cmake
```

验证安装：

```bash
cmake --version  # 需要 >= 3.15
clang --version  # 需要 >= Xcode 10.0 (Clang 10.0)
make --version
```

---

### 1.3 依赖清单

| 工具 | 最低版本 | 安装方式 |
|-----|---------|---------|
| Xcode Command Line Tools | Xcode 10.0+ | `xcode-select --install` |
| CMake | 3.15+ | `brew install cmake` |
| Git | 2.0+ | Xcode 自带或 `brew install git` |

---

## 2. 三方库下载

在项目根目录下执行：

```bash
# 创建 third_party 目录
mkdir -p third_party
cd third_party

# 1. Tongsuo (铜锁) - TLS/国密库
git clone --depth 1 --branch 8.4.0 https://github.com/Tongsuo-Project/Tongsuo.git tongsuo

# 2. nghttp2 - HTTP/2 库
git clone --depth 1 --branch v1.58.0 https://github.com/nghttp2/nghttp2.git nghttp2

# 3. nlohmann/json - JSON 解析库（头文件-only）
git clone --depth 1 --branch v3.11.3 https://github.com/nlohmann/json.git json

# 4. spdlog - 日志库
git clone --depth 1 --branch v1.12.0 https://github.com/gabime/spdlog.git spdlog

# 5. concurrentqueue - 无锁队列（头文件-only）
git clone --depth 1 --branch v1.0.4 https://github.com/cameron314/concurrentqueue.git concurrentqueue

# 6. GoogleTest - 单元测试框架
git clone --depth 1 --branch v1.14.0 https://github.com/google/googletest.git googletest
```

下载完成后目录结构：

```
third_party/
├── tongsuo/
├── nghttp2/
├── json/
├── spdlog/
├── concurrentqueue/
└── googletest/
```

---

## 3. 三方库编译

### 3.1 Tongsuo (铜锁)

```bash
cd third_party/tongsuo
./config --prefix=$(pwd)/install no-shared enable-ssl-trace enable-sm2 enable-sm3 enable-sm4
make -j$(sysctl -n hw.ncpu)
make install_sw
```

> **说明**:
> - `no-shared`: 只编译静态库
> - `enable-sm2/enable-sm3/enable-sm4`: 启用国密算法
> - `-j$(sysctl -n hw.ncpu)`: 使用所有 CPU 核心并行编译

编译输出：`third_party/tongsuo/install/`

---

### 3.2 nghttp2

```bash
cd third_party/nghttp2
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/../install \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_LIB_ONLY=ON \
  -DBUILD_SHARED_LIBS=OFF
make -j$(sysctl -n hw.ncpu)
make install
```

> **说明**:
> - `ENABLE_LIB_ONLY=ON`: 只编译库，不编译工具和示例
> - `BUILD_SHARED_LIBS=OFF`: 编译静态库

编译输出：`third_party/nghttp2/install/`

---

### 3.3 spdlog

```bash
cd third_party/spdlog
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/../install \
  -DCMAKE_BUILD_TYPE=Release \
  -DSPDLOG_BUILD_SHARED=OFF \
  -DSPDLOG_BUILD_EXAMPLE=OFF \
  -DSPDLOG_BUILD_TESTS=OFF
make -j$(sysctl -n hw.ncpu)
make install
```

编译输出：`third_party/spdlog/install/`

---

### 3.4 GoogleTest

```bash
cd third_party/googletest
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/../install \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -Dgtest_build_samples=OFF \
  -Dgtest_build_tests=OFF
make -j$(sysctl -n hw.ncpu)
make install
```

编译输出：`third_party/googletest/install/`

---

### 3.5 无需编译的库

以下库是纯头文件库，无需编译，直接 include 使用：

| 库 | 头文件路径 |
|----|-----------|
| nlohmann/json | `third_party/json/include/nlohmann/` |
| concurrentqueue | `third_party/concurrentqueue/` |

---

## 编译完成验证

编译完成后，检查以下目录是否存在：

```
third_party/
├── tongsuo/install/
│   ├── include/
│   └── lib/
├── nghttp2/install/
│   ├── include/
│   └── lib/
├── json/include/nlohmann/
├── spdlog/install/
│   ├── include/
│   └── lib/
├── concurrentqueue/
└── googletest/install/
    ├── include/
    └── lib/
```

验证库文件：

```bash
# 检查 Tongsuo 库
ls -lh third_party/tongsuo/install/lib/libssl.a
ls -lh third_party/tongsuo/install/lib/libcrypto.a

# 检查 nghttp2 库
ls -lh third_party/nghttp2/install/lib/libnghttp2.a

# 检查 spdlog 库
ls -lh third_party/spdlog/install/lib/libspdlog.a

# 检查 gtest 库
ls -lh third_party/googletest/install/lib/libgtest.a
ls -lh third_party/googletest/install/lib/libgtest_main.a
```

---

## Apple Silicon (M1/M2/M3) 说明

上述编译命令在 Apple Silicon 芯片上同样适用。Tongsuo 和其他库都原生支持 arm64 架构。

验证架构：

```bash
lipo -info third_party/tongsuo/install/lib/libssl.a
# 输出应为: Non-fat file: ... is architecture: arm64
```

---

## 常见问题

### Q: `xcode-select --install` 报错
A: 如果命令行工具安装失败，可以直接从 Apple Developer 网站下载 Command Line Tools for Xcode。

### Q: Tongsuo 编译时提示找不到头文件
A: 确保 Xcode Command Line Tools 已正确安装，执行 `xcode-select -p` 检查路径。

### Q: 编译很慢
A: 确保使用了 `-j$(sysctl -n hw.ncpu)` 参数启用并行编译。

### Q: 如何清理编译产物重新编译？
A: 删除对应库的 `build/` 目录或执行 `make clean`。

---

**文档结束**
