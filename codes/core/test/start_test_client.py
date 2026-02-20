#!/usr/bin/env python3
"""
HTTPS Server Simulator - Start Test Client
启动测试客户端脚本

用法:
    python start_test_client.py <config_file.json>
"""

import sys
import os

# 添加sim_client到模块路径
script_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, script_dir)

from sim_client.scenario import run_scenario


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <config_file.json>")
        print("Example: python start_test_client.py example_config.json")
        return 1

    config_path = sys.argv[1]

    if not os.path.exists(config_path):
        print(f"Error: Config file not found: {config_path}")
        return 1

    try:
        success = run_scenario(config_path)
        return 0 if success else 1
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
