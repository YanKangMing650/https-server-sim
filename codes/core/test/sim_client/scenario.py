"""
HTTPS Server Simulator - Sim Client Scenario
场景执行模块
"""

import time
from typing import List, Tuple
from .config import SimClientConfig, MessageConfig
from .client import SimClient


class ScenarioExecutor:
    """场景执行器"""

    def __init__(self, config: SimClientConfig):
        self.config = config
        self.client = SimClient(config)
        self.results: List[Tuple[MessageConfig, bool, str]] = []

    def run(self) -> bool:
        """
        执行场景

        Returns:
            bool: 是否全部成功
        """
        print("=" * 60)
        print("Sim Client Scenario Execution")
        print("=" * 60)
        print(f"Client: {self.config.self.ip}:{self.config.self.port}")
        print(f"Target: {self.config.target_server.ip}:{self.config.target_server.port}")
        print(f"Messages: {len(self.config.messages)}")
        print("=" * 60)

        # 连接
        if not self.client.connect():
            print("[Scenario] Connection failed, aborting")
            return False

        # 按顺序发送报文
        all_success = True
        for idx, message in enumerate(self.config.messages):
            print(f"\n[Scenario] Sending message {idx + 1}/{len(self.config.messages)}...")
            success, response = self.client.send_message(message)
            self.results.append((message, success, response or ""))

            if success:
                print(f"[Scenario] Message {idx + 1}: SUCCESS")
            else:
                print(f"[Scenario] Message {idx + 1}: FAILED")
                all_success = False

        # 断开连接
        self.client.disconnect()

        # 打印总结
        self._print_summary()

        return all_success

    def _print_summary(self):
        """打印执行总结"""
        print("\n" + "=" * 60)
        print("Execution Summary")
        print("=" * 60)
        total = len(self.results)
        successful = sum(1 for _, success, _ in self.results if success)
        print(f"Total: {total}")
        print(f"Successful: {successful}")
        print(f"Failed: {total - successful}")
        print("=" * 60)


def run_scenario(config_path: str) -> bool:
    """
    运行场景

    Args:
        config_path: 配置文件路径

    Returns:
        bool: 是否全部成功
    """
    from .config import load_config
    config = load_config(config_path)
    executor = ScenarioExecutor(config)
    return executor.run()
