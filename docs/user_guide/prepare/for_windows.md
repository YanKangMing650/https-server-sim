# Windows 环境准备指南

**版本**: v1.0
**创建日期**: 2026-02-15

---

## 目录

1. [前置依赖安装](#1-前置依赖安装)
2. [三方库下载](#2-三方库下载)
3. [三方库编译 - MSVC 版本](#3-三方库编译---msvc-版本)
4. [三方库编译 - MinGW 版本](#4-三方库编译---mingw-版本)

---

## 1. 前置依赖安装

### 1.1 使用 MSVC 编译（推荐）

| 工具 | 最低版本 | 下载地址 |
|-----|---------|---------|
| Visual Studio | 2019 (MSVC 19.20) | https://visualstudio.microsoft.com/ |
| CMake | 3.15+ | https://cmake.org/download/ |
| Perl (for Tongsuo) | 5.30+ | https://strawberryperl.com/ |

**安装步骤**:

1. 安装 Visual Studio 2019 或更高版本，选择 "使用 C++ 的桌面开发" 工作负载
2. 安装 CMake，安装时选择 "Add CMake to the system PATH"
3. 安装 Strawberry Perl（用于编译 Tongsuo）

---

### 1.2 使用 MinGW 编译

推荐使用 **MSYS2** 环境：

1. 下载并安装 MSYS2: https://www.msys2.org/

2. 打开 **MSYS2 MinGW 64-bit** 终端，执行：

```bash
# 更新包管理器
pacman -Syu

# 安装工具链
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-make
pacman -S perl
pacman -S git
```

---

## 2. 三方库下载

在项目根目录下执行：

```cmd
# 创建 third_party 目录
mkdir third_party
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

## 3. 三方库编译 - MSVC 版本

使用 **x64 Native Tools Command Prompt for VS 2019**（或2022）终端执行。

### 3.1 Tongsuo (铜锁)

```cmd
cd third_party\tongsuo
perl Configure VC-WIN64A no-shared --prefix=%cd%\install enable-ssl-trace enable-sm2 enable-sm3 enable-sm4
nmake
nmake install_sw
```

编译输出：`third_party/tongsuo/install/`

---

### 3.2 nghttp2

```cmd
cd third_party\nghttp2
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=%cd%\..\install ^
  -DENABLE_LIB_ONLY=ON ^
  -DBUILD_SHARED_LIBS=OFF ^
  -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
cmake --install . --config Release
```

> **注意**: Visual Studio 版本号对应关系：
> - VS 2019: `"Visual Studio 16 2019"`
> - VS 2022: `"Visual Studio 17 2022"`

编译输出：`third_party/nghttp2/install/`

---

### 3.3 spdlog

```cmd
cd third_party\spdlog
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=%cd%\..\install ^
  -DSPDLOG_BUILD_SHARED=OFF ^
  -DSPDLOG_BUILD_EXAMPLE=OFF ^
  -DSPDLOG_BUILD_TESTS=OFF ^
  -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
cmake --install . --config Release
```

编译输出：`third_party/spdlog/install/`

---

### 3.4 GoogleTest

```cmd
cd third_party\googletest
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=%cd%\..\install ^
  -DBUILD_SHARED_LIBS=OFF ^
  -Dgtest_build_samples=OFF ^
  -Dgtest_build_tests=OFF ^
  -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
cmake --install . --config Release
```

编译输出：`third_party/googletest/install/`

---

### 3.5 无需编译的库

以下库是纯头文件库，无需编译，直接 include 使用：

- **nlohmann/json**: `third_party/json/include/nlohmann/`
- **concurrentqueue**: `third_party/concurrentqueue/`

---

## 4. 三方库编译 - MinGW 版本

使用 **MSYS2 MinGW 64-bit** 终端执行。

### 4.1 Tongsuo (铜锁)

```bash
cd third_party/tongsuo
./config mingw64 --prefix=$(pwd)/install no-shared enable-ssl-trace enable-sm2 enable-sm3 enable-sm4
make -j$(nproc)
make install_sw
```

编译输出：`third_party/tongsuo/install/`

---

### 4.2 nghttp2

```bash
cd third_party/nghttp2
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/../install \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_LIB_ONLY=ON \
  -DBUILD_SHARED_LIBS=OFF \
  -G "MinGW Makefiles"
mingw32-make -j$(nproc)
mingw32-make install
```

编译输出：`third_party/nghttp2/install/`

---

### 4.3 spdlog

```bash
cd third_party/spdlog
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/../install \
  -DCMAKE_BUILD_TYPE=Release \
  -DSPDLOG_BUILD_SHARED=OFF \
  -DSPDLOG_BUILD_EXAMPLE=OFF \
  -DSPDLOG_BUILD_TESTS=OFF \
  -G "MinGW Makefiles"
mingw32-make -j$(nproc)
mingw32-make install
```

编译输出：`third_party/spdlog/install/`

---

### 4.4 GoogleTest

```bash
cd third_party/googletest
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/../install \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -Dgtest_build_samples=OFF \
  -Dgtest_build_tests=OFF \
  -G "MinGW Makefiles"
mingw32-make -j$(nproc)
mingw32-make install
```

编译输出：`third_party/googletest/install/`

---

### 4.5 无需编译的库

同 MSVC 版本，以下库无需编译：

- **nlohmann/json**: `third_party/json/include/nlohmann/`
- **concurrentqueue**: `third_party/concurrentqueue/`

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

---

## 常见问题

### Q: Tongsuo 编译失败，提示找不到 Perl
A: 确保安装了 Strawberry Perl，并且已添加到系统 PATH。

### Q: CMake 报错 "could not find generator"
A: 确认 Visual Studio 版本与生成器名称匹配，或使用 `cmake -G` 查看可用生成器列表。

### Q: MinGW 编译时路径有空格
A: 建议将项目放在无空格的路径下，如 `C:\projects\https-server-sim\`

---

**文档结束**
