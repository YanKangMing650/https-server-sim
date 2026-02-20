"""
HTTPS Server Simulator - Sim Client Config
配置加载模块
"""

import json
from dataclasses import dataclass
from typing import List, Dict, Optional


@dataclass
class TlsConfig:
    """TLS配置"""
    enabled: bool = True
    version: str = "1.2"
    cert_path: str = ""
    key_path: str = ""
    ca_path: str = ""
    cipher_suite: str = "TLS_AES_256_GCM_SHA384"


@dataclass
class SelfConfig:
    """自身配置"""
    ip: str = "127.0.0.1"
    port: int = 20001
    tls: TlsConfig = None
    auto_offline_seconds: int = 300

    def __post_init__(self):
        if self.tls is None:
            self.tls = TlsConfig()


@dataclass
class TargetServerConfig:
    """目标Server配置"""
    ip: str = "127.0.0.1"
    port: int = 19443


@dataclass
class RequestConfig:
    """请求配置"""
    method: str = "POST"
    path: str = "/api/test"
    headers: Dict[str, str] = None
    body: str = ""

    def __post_init__(self):
        if self.headers is None:
            self.headers = {}


@dataclass
class DebugConfig:
    """调试配置"""
    force_error_code: int = 0
    response_timeout_ms: int = 5000


@dataclass
class MessageConfig:
    """报文配置"""
    request: RequestConfig = None
    debug: DebugConfig = None

    def __post_init__(self):
        if self.request is None:
            self.request = RequestConfig()
        if self.debug is None:
            self.debug = DebugConfig()


@dataclass
class SimClientConfig:
    """模拟客户端完整配置"""
    self: SelfConfig = None
    target_server: TargetServerConfig = None
    messages: List[MessageConfig] = None

    def __post_init__(self):
        if self.self is None:
            self.self = SelfConfig()
        if self.target_server is None:
            self.target_server = TargetServerConfig()
        if self.messages is None:
            self.messages = []


def load_config(config_path: str) -> SimClientConfig:
    """
    从JSON文件加载配置

    Args:
        config_path: 配置文件路径

    Returns:
        SimClientConfig: 加载后的配置对象
    """
    with open(config_path, 'r', encoding='utf-8') as f:
        data = json.load(f)

    # 解析self配置
    self_data = data.get('self', {})
    tls_data = self_data.get('tls', {})
    tls_config = TlsConfig(
        enabled=tls_data.get('enabled', True),
        version=tls_data.get('version', '1.2'),
        cert_path=tls_data.get('cert_path', ''),
        key_path=tls_data.get('key_path', ''),
        ca_path=tls_data.get('ca_path', ''),
        cipher_suite=tls_data.get('cipher_suite', 'TLS_AES_256_GCM_SHA384')
    )
    self_config = SelfConfig(
        ip=self_data.get('ip', '127.0.0.1'),
        port=self_data.get('port', 20001),
        tls=tls_config,
        auto_offline_seconds=self_data.get('auto_offline_seconds', 300)
    )

    # 解析target_server配置
    target_data = data.get('target_server', {})
    target_config = TargetServerConfig(
        ip=target_data.get('ip', '127.0.0.1'),
        port=target_data.get('port', 19443)
    )

    # 解析messages配置
    messages = []
    for msg_data in data.get('messages', []):
        req_data = msg_data.get('request', {})
        debug_data = msg_data.get('debug', {})

        request = RequestConfig(
            method=req_data.get('method', 'POST'),
            path=req_data.get('path', '/api/test'),
            headers=req_data.get('headers', {}),
            body=req_data.get('body', '')
        )
        debug = DebugConfig(
            force_error_code=debug_data.get('force_error_code', 0),
            response_timeout_ms=debug_data.get('response_timeout_ms', 5000)
        )
        messages.append(MessageConfig(request=request, debug=debug))

    return SimClientConfig(
        self=self_config,
        target_server=target_config,
        messages=messages
    )
