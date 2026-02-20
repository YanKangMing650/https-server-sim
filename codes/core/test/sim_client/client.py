"""
HTTPS Server Simulator - Sim Client
客户端核心模块
"""

import socket
import ssl
import threading
import time
from typing import Optional, Tuple
from .config import SimClientConfig, MessageConfig


class SimClient:
    """模拟HTTPS客户端"""

    DEBUG_HEADER_ERROR_CODE = "X-Debug-Force-Error-Code"
    DEBUG_HEADER_TIMEOUT = "X-Debug-Response-Timeout-Ms"

    def __init__(self, config: SimClientConfig):
        self.config = config
        self._socket: Optional[socket.socket] = None
        self._ssl_socket: Optional[ssl.SSLSocket] = None
        self._connected = False
        self._auto_offline_timer: Optional[threading.Timer] = None
        self._running = False

    def connect(self) -> bool:
        """
        连接到目标Server

        Returns:
            bool: 连接是否成功
        """
        try:
            # 创建socket并绑定到指定IP和端口
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self._socket.bind((self.config.self.ip, self.config.self.port))
            self._socket.settimeout(10.0)

            # 连接到目标Server
            target_addr = (self.config.target_server.ip, self.config.target_server.port)
            print(f"[Client] Connecting to {target_addr[0]}:{target_addr[1]}...")
            self._socket.connect(target_addr)

            # 包装TLS
            if self.config.self.tls.enabled:
                self._ssl_socket = self._wrap_tls(self._socket)
            else:
                self._ssl_socket = self._socket

            self._connected = True
            print(f"[Client] Connected from {self.config.self.ip}:{self.config.self.port}")

            # 启动自动下线计时器
            self._start_auto_offline_timer()

            return True
        except Exception as e:
            print(f"[Client] Connection failed: {e}")
            self._cleanup()
            return False

    def _wrap_tls(self, sock: socket.socket) -> ssl.SSLSocket:
        """
        包装TLS

        Args:
            sock: 原始socket

        Returns:
            ssl.SSLSocket: TLS包装后的socket
        """
        tls_config = self.config.self.tls

        # 创建SSL上下文
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        context.check_hostname = False

        # 加载证书
        if tls_config.cert_path and tls_config.key_path:
            context.load_cert_chain(
                certfile=tls_config.cert_path,
                keyfile=tls_config.key_path
            )

        # 加载CA证书
        if tls_config.ca_path:
            context.load_verify_locations(cafile=tls_config.ca_path)
            context.verify_mode = ssl.CERT_REQUIRED
        else:
            context.verify_mode = ssl.CERT_NONE

        # 设置密码套件
        if tls_config.cipher_suite:
            context.set_ciphers(tls_config.cipher_suite)

        # 包装socket
        ssl_sock = context.wrap_socket(
            sock,
            server_hostname=self.config.target_server.ip
        )
        print(f"[Client] TLS handshake completed")
        return ssl_sock

    def send_message(self, message: MessageConfig) -> Tuple[bool, Optional[str]]:
        """
        发送单个报文并接收响应

        Args:
            message: 报文配置

        Returns:
            Tuple[bool, Optional[str]]: (是否成功, 响应内容)
        """
        if not self._connected or not self._ssl_socket:
            print(f"[Client] Not connected")
            return False, None

        try:
            # 构建请求
            request = self._build_request(message)
            print(f"[Client] Sending request:\n{request[:200]}...")

            # 发送请求
            self._ssl_socket.sendall(request.encode('utf-8'))

            # 接收响应
            timeout_seconds = message.debug.response_timeout_ms / 1000.0
            response = self._receive_response(timeout_seconds)

            if response:
                print(f"[Client] Received response:\n{response[:200]}...")
            else:
                print(f"[Client] No response received")

            return True, response
        except Exception as e:
            print(f"[Client] Send message failed: {e}")
            return False, None

    def _build_request(self, message: MessageConfig) -> str:
        """
        构建HTTP请求

        Args:
            message: 报文配置

        Returns:
            str: HTTP请求字符串
        """
        req = message.request

        # 复制headers并添加调试headers
        headers = dict(req.headers)

        # 添加调试headers
        if message.debug.force_error_code != 0:
            headers[self.DEBUG_HEADER_ERROR_CODE] = str(message.debug.force_error_code)
        if message.debug.response_timeout_ms != 5000:
            headers[self.DEBUG_HEADER_TIMEOUT] = str(message.debug.response_timeout_ms)

        # 添加Content-Length
        if req.body and 'Content-Length' not in headers:
            headers['Content-Length'] = str(len(req.body))

        # 构建请求行
        request_line = f"{req.method} {req.path} HTTP/1.1\r\n"

        # 构建headers
        headers_str = ""
        for key, value in headers.items():
            headers_str += f"{key}: {value}\r\n"

        # 添加Host头
        if 'Host' not in headers:
            headers_str += f"Host: {self.config.target_server.ip}:{self.config.target_server.port}\r\n"

        # 构建完整请求
        request = request_line + headers_str + "\r\n"
        if req.body:
            request += req.body

        return request

    def _receive_response(self, timeout_seconds: float) -> Optional[str]:
        """
        接收响应

        Args:
            timeout_seconds: 超时时间（秒）

        Returns:
            Optional[str]: 响应内容，失败返回None
        """
        if not self._ssl_socket:
            return None

        self._ssl_socket.settimeout(timeout_seconds)

        try:
            response_parts = []
            while True:
                chunk = self._ssl_socket.recv(4096)
                if not chunk:
                    break
                response_parts.append(chunk.decode('utf-8', errors='ignore'))

                # 简单判断是否接收完整（检查Content-Length或双重\r\n）
                full_response = ''.join(response_parts)
                if '\r\n\r\n' in full_response:
                    # 检查是否有Content-Length
                    if 'Content-Length:' in full_response:
                        import re
                        match = re.search(r'Content-Length:\s*(\d+)', full_response, re.IGNORECASE)
                        if match:
                            content_length = int(match.group(1))
                            headers_end = full_response.find('\r\n\r\n') + 4
                            body_received = len(full_response) - headers_end
                            if body_received >= content_length:
                                break
                    else:
                        break

            return ''.join(response_parts) if response_parts else None
        except socket.timeout:
            print(f"[Client] Receive timeout after {timeout_seconds}s")
            return None
        except Exception as e:
            print(f"[Client] Receive failed: {e}")
            return None

    def _start_auto_offline_timer(self):
        """启动自动下线计时器"""
        if self.config.self.auto_offline_seconds > 0:
            self._auto_offline_timer = threading.Timer(
                self.config.self.auto_offline_seconds,
                self._auto_offline
            )
            self._auto_offline_timer.daemon = True
            self._auto_offline_timer.start()
            print(f"[Client] Auto-offline timer started: {self.config.self.auto_offline_seconds}s")

    def _auto_offline(self):
        """自动下线"""
        print(f"[Client] Auto-offline triggered")
        self.disconnect()

    def disconnect(self):
        """断开连接"""
        print(f"[Client] Disconnecting...")
        self._cleanup()

    def _cleanup(self):
        """清理资源"""
        if self._auto_offline_timer:
            self._auto_offline_timer.cancel()
            self._auto_offline_timer = None

        if self._ssl_socket:
            try:
                self._ssl_socket.close()
            except:
                pass
            self._ssl_socket = None

        if self._socket:
            try:
                self._socket.close()
            except:
                pass
            self._socket = None

        self._connected = False

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.disconnect()
