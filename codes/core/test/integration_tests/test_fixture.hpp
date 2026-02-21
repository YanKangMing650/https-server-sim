// =============================================================================
//  HTTPS Server Simulator - Integration Test Fixture
//  文件: test_fixture.hpp
//  描述: 集成测试通用夹具
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#include <gtest/gtest.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <map>
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include "server/server.hpp"
#include "config/config.hpp"
#include "utils/statistics.hpp"

namespace https_server_sim {
namespace integration_test {

// 原子计数器，用于生成唯一临时文件名和端口
static std::atomic<uint64_t> temp_file_counter{0};
static std::atomic<uint16_t> test_port_counter{19000};
static std::atomic<uint16_t> client_port_counter{30000};

// RAII临时文件包装器
class TempFile {
public:
    explicit TempFile(const std::string& content) {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
        uint64_t counter = temp_file_counter.fetch_add(1);
        path_ = "/tmp/test_integration_config_" +
               std::to_string(timestamp) + "_" +
               std::to_string(counter) + ".json";
        std::ofstream file(path_);
        file << content;
        file.close();
    }

    ~TempFile() {
        if (!path_.empty()) {
            unlink(path_.c_str());
        }
    }

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;

    TempFile(TempFile&& other) noexcept : path_(std::move(other.path_)) {
        other.path_.clear();
    }

    TempFile& operator=(TempFile&& other) noexcept {
        if (this != &other) {
            if (!path_.empty()) {
                unlink(path_.c_str());
            }
            path_ = std::move(other.path_);
            other.path_.clear();
        }
        return *this;
    }

    const std::string& path() const { return path_; }

private:
    std::string path_;
};

// HTTP 响应结构
struct HttpResponse {
    int status_code;
    std::string status_text;
    std::map<std::string, std::string> headers;
    std::string body;

    HttpResponse() : status_code(0) {}
};

// 简单的测试客户端（纯 C++ 实现）
class TestClient {
public:
    TestClient(const std::string& server_ip, uint16_t server_port)
        : server_ip_(server_ip), server_port_(server_port),
          client_port_(client_port_counter.fetch_add(1)),
          sock_fd_(-1) {}

    ~TestClient() {
        disconnect();
    }

    // 连接到服务器
    bool connect() {
        sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd_ < 0) {
            return false;
        }

        // 设置端口复用
        int opt = 1;
        setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // 绑定客户端端口
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        client_addr.sin_family = AF_INET;
        client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        client_addr.sin_port = htons(client_port_);

        if (bind(sock_fd_, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
            close(sock_fd_);
            sock_fd_ = -1;
            return false;
        }

        // 连接到服务器
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(server_ip_.c_str());
        server_addr.sin_port = htons(server_port_);

        // 设置超时
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        setsockopt(sock_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock_fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        if (::connect(sock_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sock_fd_);
            sock_fd_ = -1;
            return false;
        }

        return true;
    }

    // 发送简单的 HTTP 请求（非 TLS，用于基础测试）
    // 注意：真实项目需要 TLS 支持，这里用简化版本验证 Server 启动和监听
    bool send_simple_request(const std::string& method,
                             const std::string& path,
                             const std::map<std::string, std::string>& headers = {},
                             const std::string& body = "",
                             HttpResponse* response = nullptr) {
        if (sock_fd_ < 0) {
            return false;
        }

        // 构建请求
        std::ostringstream request;
        request << method << " " << path << " HTTP/1.1\r\n";
        request << "Host: " << server_ip_ << ":" << server_port_ << "\r\n";
        request << "Connection: close\r\n";

        for (const auto& header : headers) {
            request << header.first << ": " << header.second << "\r\n";
        }

        if (!body.empty()) {
            request << "Content-Length: " << body.size() << "\r\n";
        }

        request << "\r\n";

        if (!body.empty()) {
            request << body;
        }

        std::string req_str = request.str();

        // 发送
        ssize_t sent = send(sock_fd_, req_str.c_str(), req_str.size(), 0);
        if (sent != (ssize_t)req_str.size()) {
            return false;
        }

        // 接收响应（如果需要）
        if (response) {
            return receive_response(response);
        }

        return true;
    }

    void disconnect() {
        if (sock_fd_ >= 0) {
            close(sock_fd_);
            sock_fd_ = -1;
        }
    }

private:
    bool receive_response(HttpResponse* response) {
        char buffer[8192];
        std::string response_str;

        ssize_t n;
        while ((n = recv(sock_fd_, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[n] = '\0';
            response_str += buffer;
        }

        if (response_str.empty()) {
            return false;
        }

        // 简单解析响应
        parse_response(response_str, response);
        return true;
    }

    void parse_response(const std::string& str, HttpResponse* response) {
        std::istringstream iss(str);
        std::string line;

        // 解析状态行
        if (std::getline(iss, line)) {
            std::istringstream line_ss(line);
            std::string http_version;
            line_ss >> http_version;
            line_ss >> response->status_code;
            std::getline(line_ss, response->status_text);
            // 去除开头的空格
            if (!response->status_text.empty() && response->status_text[0] == ' ') {
                response->status_text = response->status_text.substr(1);
            }
            // 去除末尾的 \r
            if (!response->status_text.empty() && response->status_text.back() == '\r') {
                response->status_text.pop_back();
            }
        }

        // 解析 Headers
        while (std::getline(iss, line)) {
            if (line == "\r" || line.empty()) {
                break;
            }
            // 去除末尾的 \r
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                // 去除 value 开头的空格
                if (!value.empty() && value[0] == ' ') {
                    value = value.substr(1);
                }
                response->headers[key] = value;
            }
        }

        // 剩余的是 Body
        std::ostringstream body_ss;
        while (std::getline(iss, line)) {
            body_ss << line << "\n";
        }
        response->body = body_ss.str();
    }

    std::string server_ip_;
    uint16_t server_port_;
    uint16_t client_port_;
    int sock_fd_;
};

// 获取唯一测试端口
inline uint16_t get_test_port() {
    return test_port_counter.fetch_add(1);
}

// 获取有效的配置JSON
inline std::string get_valid_config_json(uint16_t port = 19443, bool debug_enabled = false) {
    return R"({
        "listens": [
            {"ip": "127.0.0.1", "port": )" + std::to_string(port) + R"(, "enabled": true}
        ],
        "certificates": {
            "cert_path": "",
            "key_path": "",
            "ca_path": "",
            "cipher_suite": "TLS_AES_256_GCM_SHA384"
        },
        "debug": {
            "enabled": )" + std::string(debug_enabled ? "true" : "false") + R"(,
            "debug_points": []
        },
        "callbacks": {
            "callbacks_dir": "",
            "callbacks": []
        },
        "logging": {
            "level": "INFO",
            "file": "",
            "console_output": true
        },
        "http2": {
            "enabled": true,
            "allow_h2c": false
        }
    })";
}

// SimClient 运行结果
struct SimClientResult {
    bool success;
    int exit_code;
    std::string output;

    SimClientResult() : success(false), exit_code(-1) {}
};

// Python SimClient 调用器
class SimClientRunner {
public:
    SimClientRunner(const std::string& test_dir)
        : test_dir_(test_dir) {}

    // 运行 SimClient 场景
    SimClientResult run_scenario(const std::string& config_json) {
        SimClientResult result;

        // 创建临时配置文件
        TempFile config_file(config_json);

        // 构建命令
        std::string cmd = "python3 " + test_dir_ + "/start_test_client.py " + config_file.path();

        // 执行命令
        result = execute_command(cmd);
        return result;
    }

    // 构建简单的单请求配置
    static std::string build_single_request_config(
            uint16_t client_port,
            uint16_t server_port,
            const std::string& method = "POST",
            const std::string& path = "/api/test",
            const std::map<std::string, std::string>& headers = {},
            const std::string& body = "",
            int force_error_code = 0,
            int response_timeout_ms = 5000) {
        std::ostringstream ss;
        ss << "{\n"
           << "    \"self\": {\n"
           << "        \"ip\": \"127.0.0.1\",\n"
           << "        \"port\": " << client_port << ",\n"
           << "        \"tls\": {\n"
           << "            \"enabled\": true,\n"
           << "            \"version\": \"1.2\",\n"
           << "            \"cert_path\": \"\",\n"
           << "            \"key_path\": \"\",\n"
           << "            \"ca_path\": \"\",\n"
           << "            \"cipher_suite\": \"TLS_AES_256_GCM_SHA384\"\n"
           << "        },\n"
           << "        \"auto_offline_seconds\": 300\n"
           << "    },\n"
           << "    \"target_server\": {\n"
           << "        \"ip\": \"127.0.0.1\",\n"
           << "        \"port\": " << server_port << "\n"
           << "    },\n"
           << "    \"messages\": [\n"
           << "        {\n"
           << "            \"request\": {\n"
           << "                \"method\": \"" << method << "\",\n"
           << "                \"path\": \"" << path << "\",\n"
           << "                \"headers\": {\n";

        // 添加 headers
        bool first = true;
        for (const auto& kv : headers) {
            if (!first) ss << ",\n";
            first = false;
            ss << "                    \"" << kv.first << "\": \"" << kv.second << "\"";
        }

        ss << "\n"
           << "                },\n"
           << "                \"body\": \"" << escape_json(body) << "\"\n"
           << "            },\n"
           << "            \"debug\": {\n"
           << "                \"force_error_code\": " << force_error_code << ",\n"
           << "                \"response_timeout_ms\": " << response_timeout_ms << "\n"
           << "            }\n"
           << "        }\n"
           << "    ]\n"
           << "}\n";
        return ss.str();
    }

private:
    static std::string escape_json(const std::string& s) {
        std::string result;
        for (char c : s) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }

    SimClientResult execute_command(const std::string& cmd) {
        SimClientResult result;

        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            result.success = false;
            return result;
        }

        char buffer[4096];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result.output += buffer;
        }

        int exit_code = pclose(pipe);
        result.exit_code = WEXITSTATUS(exit_code);
        result.success = (result.exit_code == 0);

        return result;
    }

    std::string test_dir_;
};

// 集成测试夹具基类
class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个测试开始前的设置
        test_dir_ = get_test_directory();
    }

    void TearDown() override {
        // 每个测试结束后的清理
    }

    // 等待指定毫秒
    void sleep_ms(int milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    // 启动 Server 并等待其就绪
    bool start_server(server::Server& server, const std::string& config_path) {
        if (server.init(config_path) != 0) {
            return false;
        }
        if (server.start() != 0) {
            return false;
        }
        // 等待 Server 启动完成
        sleep_ms(100);
        return true;
    }

    // 获取测试目录路径
    std::string get_test_directory() {
        // 获取当前可执行文件路径，然后推导测试目录
        char exe_path[1024];
        ssize_t count = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
        if (count > 0) {
            std::string path(exe_path, count);
            // 从 bin/core_integration_test 推导到 codes/core/test
            size_t bin_pos = path.find("/bin/");
            if (bin_pos != std::string::npos) {
                return path.substr(0, bin_pos) + "/../codes/core/test";
            }
        }
        // 回退到默认路径
        return "/Users/kangbenben/Documents/codes/ai-code/YanKangMing650/https-server-sim/codes/core/test";
    }

    std::string test_dir_;
};

} // namespace integration_test
} // namespace https_server_sim

// 文件结束
