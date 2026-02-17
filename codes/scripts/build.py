#!/usr/bin/env python3
"""
HTTPS Server Simulator - 跨平台编译脚本

支持的平台:
  - linux: Linux (GCC/Clang)
  - windows: Windows (MSVC/MinGW)
  - mac: macOS (Clang)

使用方式:
  python build.py [options]

示例:
  python build.py                          # 本地默认编译
  python build.py --platform windows      # 交叉编译到Windows
  python build.py --toolchain clang       # 使用Clang编译
  python build.py --clean                  # 清理构建目录
  python build.py --release                # Release模式
  python build.py --tests                  # 构建测试
"""

import argparse
import os
import sys
import subprocess
import platform
from pathlib import Path


def get_project_root() -> Path:
    """获取项目根目录"""
    script_dir = Path(__file__).resolve().parent
    return script_dir.parent.parent


def get_build_dir() -> Path:
    """获取构建目录"""
    return get_project_root() / "build"


def get_codes_dir() -> Path:
    """获取codes目录（CMakeLists.txt所在）"""
    return get_project_root() / "codes"


def get_third_party_dir() -> Path:
    """获取第三方库目录"""
    return get_project_root() / "third_party"


def detect_host_platform() -> str:
    """检测主机平台"""
    system = platform.system().lower()
    if system == "linux":
        return "linux"
    elif system == "darwin":
        return "mac"
    elif system == "windows":
        return "windows"
    else:
        return system


def get_cmake_generator(platform_name: str, toolchain: str = None) -> str:
    """获取CMake生成器"""
    if platform_name == "windows":
        if toolchain == "mingw":
            return "MinGW Makefiles"
        else:
            return "Visual Studio 17 2022"
    elif platform_name in ["linux", "mac"]:
        return "Unix Makefiles"
    return "Unix Makefiles"


def get_toolchain_file(platform_name: str, host_platform: str) -> str:
    """获取交叉编译工具链文件（如果需要）"""
    if platform_name == host_platform:
        return None

    # 交叉编译场景
    toolchain_dir = get_project_root() / "codes" / "scripts" / "toolchains"
    if platform_name == "windows" and host_platform in ["linux", "mac"]:
        return str(toolchain_dir / "mingw-w64.cmake")
    elif platform_name == "linux" and host_platform in ["windows", "mac"]:
        return str(toolchain_dir / "linux-gcc.cmake")
    elif platform_name == "mac" and host_platform in ["linux", "windows"]:
        return str(toolchain_dir / "macos-clang.cmake")
    return None


def run_command(cmd: list, cwd: Path = None, check: bool = True) -> int:
    """运行命令"""
    print(f"\n>>> 执行命令: {' '.join(cmd)}")
    print(f">>> 工作目录: {cwd or os.getcwd()}")
    try:
        result = subprocess.run(cmd, cwd=cwd, check=check)
        return result.returncode
    except subprocess.CalledProcessError as e:
        print(f"错误: 命令执行失败，返回码: {e.returncode}")
        return e.returncode


def clean_build(build_dir: Path) -> int:
    """清理构建目录"""
    if build_dir.exists():
        print(f"清理构建目录: {build_dir}")
        import shutil
        shutil.rmtree(build_dir)
    return 0


def configure_cmake(
    build_dir: Path,
    source_dir: Path,
    platform_name: str,
    toolchain: str = None,
    build_type: str = "Debug",
    build_tests: bool = True,
    use_tongsuo: bool = True,
    host_platform: str = None,
) -> int:
    """配置CMake"""
    if not build_dir.exists():
        build_dir.mkdir(parents=True, exist_ok=True)

    cmake_args = ["cmake", str(source_dir)]

    # 生成器
    generator = get_cmake_generator(platform_name, toolchain)
    cmake_args.extend(["-G", generator])

    # 平台特定选项
    if platform_name == "windows" and toolchain != "mingw":
        cmake_args.extend(["-A", "x64"])

    # 交叉编译工具链
    if host_platform and platform_name != host_platform:
        toolchain_file = get_toolchain_file(platform_name, host_platform)
        if toolchain_file and Path(toolchain_file).exists():
            cmake_args.extend([f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}"])

    # 构建类型
    cmake_args.append(f"-DCMAKE_BUILD_TYPE={build_type}")

    # 编译器选择
    if toolchain:
        if toolchain == "gcc":
            cmake_args.extend([
                "-DCMAKE_C_COMPILER=gcc",
                "-DCMAKE_CXX_COMPILER=g++"
            ])
        elif toolchain == "clang":
            cmake_args.extend([
                "-DCMAKE_C_COMPILER=clang",
                "-DCMAKE_CXX_COMPILER=clang++"
            ])

    # 测试选项
    cmake_args.append(f"-DBUILD_TESTS={'ON' if build_tests else 'OFF'}")

    # TLS库选择
    cmake_args.append(f"-DUSE_TONGSUO={'ON' if use_tongsuo else 'OFF'}")

    return run_command(cmake_args, cwd=build_dir)


def build_project(
    build_dir: Path,
    platform_name: str,
    build_type: str = "Debug",
    parallel: int = None,
) -> int:
    """构建项目"""
    if not parallel:
        import multiprocessing
        parallel = multiprocessing.cpu_count()

    build_args = ["cmake", "--build", ".", "--config", build_type]

    if platform_name in ["linux", "mac"]:
        build_args.extend(["--", f"-j{parallel}"])
    else:
        build_args.extend(["--", f"/m:{parallel}"])

    return run_command(build_args, cwd=build_dir)


def run_tests(build_dir: Path, build_type: str = "Debug") -> int:
    """运行测试"""
    test_args = ["ctest", "--output-on-failure", "--build-config", build_type]
    return run_command(test_args, cwd=build_dir)


def main():
    parser = argparse.ArgumentParser(
        description="HTTPS Server Simulator - 跨平台编译脚本",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s                          # 本地默认编译
  %(prog)s --platform windows      # 交叉编译到Windows
  %(prog)s --toolchain clang       # 使用Clang编译
  %(prog)s --clean                  # 清理构建目录
  %(prog)s --release                # Release模式
  %(prog)s --tests                  # 构建并运行测试
        """
    )

    # 平台选择
    parser.add_argument(
        "--platform",
        choices=["linux", "windows", "mac"],
        help="目标平台 (默认: 自动检测)",
    )

    # 工具链选择
    parser.add_argument(
        "--toolchain",
        choices=["gcc", "clang", "msvc", "mingw"],
        help="编译工具链 (默认: 平台默认)",
    )

    # 构建类型
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Debug模式 (默认)",
    )
    parser.add_argument(
        "--release",
        action="store_true",
        help="Release模式",
    )

    # 清理选项
    parser.add_argument(
        "--clean",
        action="store_true",
        help="清理构建目录后重新构建",
    )

    # 测试选项
    parser.add_argument(
        "--tests",
        action="store_true",
        help="构建并运行测试",
    )

    # 并行编译
    parser.add_argument(
        "-j", "--jobs",
        type=int,
        help="并行编译任务数",
    )

    # TLS库选择
    parser.add_argument(
        "--use-openssl",
        action="store_true",
        help="使用OpenSSL代替Tongsuo（铜锁）",
    )

    # 仅配置
    parser.add_argument(
        "--configure-only",
        action="store_true",
        help="仅配置CMake，不构建",
    )

    # 仅构建
    parser.add_argument(
        "--build-only",
        action="store_true",
        help="仅构建，不重新配置",
    )

    args = parser.parse_args()

    # 确定目标平台
    host_platform = detect_host_platform()
    target_platform = args.platform or host_platform

    # 确定构建类型
    build_type = "Release" if args.release else "Debug"

    # 确定工具链
    toolchain = args.toolchain
    if not toolchain:
        if target_platform == "windows":
            toolchain = "msvc"
        elif target_platform == "mac":
            toolchain = "clang"
        else:
            toolchain = "gcc"

    # 验证平台和工具链组合
    if target_platform == "windows" and toolchain not in ["msvc", "mingw"]:
        print(f"警告: Windows平台推荐使用msvc或mingw，当前: {toolchain}")
    if target_platform == "mac" and toolchain != "clang":
        print(f"警告: macOS平台推荐使用clang，当前: {toolchain}")

    # 获取目录
    project_root = get_project_root()
    build_dir = get_build_dir()
    codes_dir = get_codes_dir()

    # 打印配置信息
    print("=" * 60)
    print("HTTPS Server Simulator - 编译配置")
    print("=" * 60)
    print(f"项目根目录: {project_root}")
    print(f"构建目录: {build_dir}")
    print(f"主机平台: {host_platform}")
    print(f"目标平台: {target_platform}")
    print(f"工具链: {toolchain}")
    print(f"构建类型: {build_type}")
    print(f"构建测试: {'是' if args.tests else '否'}")
    print(f"TLS库: {'OpenSSL' if args.use_openssl else 'Tongsuo (铜锁)'}")
    print("=" * 60)

    # 清理
    if args.clean:
        clean_build(build_dir)

    # 配置CMake
    if not args.build_only:
        ret = configure_cmake(
            build_dir=build_dir,
            source_dir=codes_dir,
            platform_name=target_platform,
            toolchain=toolchain,
            build_type=build_type,
            build_tests=args.tests,
            use_tongsuo=not args.use_openssl,
            host_platform=host_platform,
        )
        if ret != 0:
            print(f"\n错误: CMake配置失败 (返回码: {ret})")
            return ret

    # 仅配置模式
    if args.configure_only:
        print("\nCMake配置完成")
        return 0

    # 构建
    ret = build_project(
        build_dir=build_dir,
        platform_name=target_platform,
        build_type=build_type,
        parallel=args.jobs,
    )
    if ret != 0:
        print(f"\n错误: 构建失败 (返回码: {ret})")
        return ret

    # 运行测试
    if args.tests:
        print("\n" + "=" * 60)
        print("运行测试")
        print("=" * 60)
        ret = run_tests(build_dir, build_type)
        if ret != 0:
            print(f"\n警告: 部分测试失败 (返回码: {ret})")

    print("\n" + "=" * 60)
    print("构建完成!")
    print("=" * 60)
    print(f"构建产物目录: {build_dir}")
    print(f"库文件: {build_dir / 'lib'}")
    print(f"可执行文件: {build_dir / 'bin'}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
