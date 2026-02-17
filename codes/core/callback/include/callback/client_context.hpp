#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace https_server_sim {
namespace callback {

// HTTP方法枚举
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    PATCH,
    UNKNOWN
};

// HTTP请求信息
struct HttpRequest {
    HttpMethod method;
    std::string path;
    std::string version;
    std::vector<std::pair<std::string, std::string>> headers;
    std::string body;

    HttpRequest();
};

// HTTP响应信息
struct HttpResponse {
    int status_code;
    std::string status_text;
    std::vector<std::pair<std::string, std::string>> headers;
    std::string body;

    HttpResponse();
};

// Client上下文
class ClientContext {
public:
    ClientContext();
    ~ClientContext();

    uint64_t get_connection_id() const;
    void set_connection_id(uint64_t id);

    uint16_t get_server_port() const;
    void set_server_port(uint16_t port);

    const std::string& get_client_ip() const;
    void set_client_ip(const std::string& ip);

    uint16_t get_client_port() const;
    void set_client_port(uint16_t port);

    const HttpRequest& get_request() const;
    HttpRequest& get_request();
    void set_request(const HttpRequest& req);

    const HttpResponse& get_response() const;
    HttpResponse& get_response();
    void set_response(const HttpResponse& resp);

    void reset();

private:
    uint64_t connection_id_;
    uint16_t server_port_;
    std::string client_ip_;
    uint16_t client_port_;
    HttpRequest request_;
    HttpResponse response_;
};

} // namespace callback
} // namespace https_server_sim
