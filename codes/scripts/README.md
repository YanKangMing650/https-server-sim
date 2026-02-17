# 编译脚本使用说明

本目录包含 HTTPS Server Simulator 的跨平台编译脚本。

## 目录结构

```
scripts/
├── build.py          # Python版本编译脚本（推荐，跨平台）
├── build.sh          # Bash版本编译脚本（Linux/macOS）
├── README.md         # 本文档
└── toolchains/       # 交叉编译工具链配置
    ├── mingw-w64.cmake     # Windows (MinGW-w64)
    ├── linux-gcc.cmake      # Linux (GCC)
    └── macos-clang.cmake    # macOS (Clang)
```

## 使用方式

### 方式一：Python 脚本（推荐）

Python 脚本支持所有平台（Windows/Linux/macOS）。

**前置要求**: Python 3.6+

```bash
# 进入 scripts 目录
cd codes/scripts

# 本地默认编译（Debug模式）
python build.py

# Release模式编译
python build.py --release

# 清理后重新编译
python build.py --clean

# 编译并运行测试
python build.py --tests

# 使用 Clang 编译
python build.py --toolchain clang

# 交叉编译到 Windows
python build.py --platform windows

# 完整示例：清理 + Release模式 + 测试 + 使用OpenSSL
python build.py --clean --release --tests --use-openssl
```

### 方式二：Bash 脚本（Linux/macOS）

```bash
# 进入 scripts 目录
cd codes/scripts

# 添加执行权限（如果需要）
chmod +x build.sh

# 本地默认编译
./build.sh

# Release模式 + 测试
./build.sh --release --tests

# 使用 Clang 编译
./build.sh --toolchain clang

# 交叉编译到 Windows
./build.sh --platform windows
```

## 命令行参数

### build.py 参数

| 参数 | 说明 |
|-----|------|
| `--platform <linux/windows/mac>` | 目标平台 |
| `--toolchain <gcc/clang/msvc/mingw>` | 编译工具链 |
| `--debug` | Debug 模式（默认） |
| `--release` | Release 模式 |
| `--clean` | 清理构建目录 |
| `--tests` | 构建并运行测试 |
| `--use-openssl` | 使用 OpenSSL（默认使用 Tongsuo） |
| `-j, --jobs <n>` | 并行编译任务数 |
| `--configure-only` | 仅配置 CMake |
| `--build-only` | 仅构建，不重新配置 |
| `--help` | 显示帮助 |

### build.sh 参数

| 参数 | 说明 |
|-----|------|
| `--platform <linux/windows/mac>` | 目标平台 |
| `--toolchain <gcc/clang>` | 编译工具链 |
| `--debug` | Debug 模式（默认） |
| `--release` | Release 模式 |
| `--clean` | 清理构建目录 |
| `--tests` | 构建并运行测试 |
| `--use-openssl` | 使用 OpenSSL |
| `-j, --jobs <n>` | 并行编译任务数 |
| `--configure-only` | 仅配置 CMake |
| `--build-only` | 仅构建 |
| `--help` | 显示帮助 |

## 使用示例

### 基本使用

```bash
# 默认编译（Debug）
python build.py

# Release 编译
python build.py --release

# 清理后重新编译
python build.py --clean
```

### 工具链选择

```bash
# 使用 GCC
python build.py --toolchain gcc

# 使用 Clang
python build.py --toolchain clang

# Windows 下使用 MinGW
python build.py --platform windows --toolchain mingw
```

### 测试相关

```bash
# 构建并运行测试
python build.py --tests

# 只运行测试（不重新编译）
python build.py --build-only --tests
```

### TLS 库选择

```bash
# 使用 Tongsuo（铜锁，默认）
python build.py

# 使用 OpenSSL
python build.py --use-openssl
```

### 交叉编译

```bash
# 从 Linux/macOS 编译到 Windows
python build.py --platform windows

# 从 Windows/macOS 编译到 Linux
python build.py --platform linux
```

## 构建产物

编译完成后，文件结构如下：

```
build/
├── lib/           # 库文件（静态库 + 动态库）
│   ├── libhs_utils_static.a
│   ├── libhs_config_static.a
│   ├── ...
│   └── libhttps_server_core.so  # 核心动态库
├── bin/           # 可执行文件
│   ├── test_utils
│   ├── test_server
│   └── ...
└── ...
```

## 交叉编译注意事项

### 编译到 Windows

在 Linux/macOS 上交叉编译到 Windows 需要安装 MinGW-w64：

**Ubuntu/Debian**:
```bash
sudo apt install mingw-w64
```

**macOS (Homebrew)**:
```bash
brew install mingw-w64
```

### 编译到 macOS

从其他平台交叉编译到 macOS 比较复杂，建议直接在 macOS 上编译。

如果必须交叉编译，可以使用 [osxcross](https://github.com/tpoechtrager/osxcross) 项目。

## 故障排查

### CMake 配置失败

确保已安装 CMake 3.15+：
```bash
cmake --version
```

### 编译器找不到

确保工具链已安装并在 PATH 中：
```bash
# 检查 GCC
gcc --version

# 检查 Clang
clang --version
```

### 三方库缺失

确保 `third_party/` 目录下有以下库：
- tongsuo 或 openssl
- json (nlohmann/json)
- googletest

## 更多帮助

运行 `python build.py --help` 或 `./build.sh --help` 查看完整参数列表。
