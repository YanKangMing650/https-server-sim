#include "callback/callback_strategy.hpp"

namespace https_server_sim {
namespace callback {

// HttpRequest
HttpRequest::HttpRequest()
    : method(HttpMethod::GET)
    , version("HTTP/1.1")
{
}

// HttpResponse
HttpResponse::HttpResponse()
    : status_code(200)
    , status_text("OK")
{
}

// ClientContext
ClientContext::ClientContext()
    : connection_id_(0)
    , server_port_(0)
    , client_port_(0)
{
}

ClientContext::~ClientContext() {
}

uint64_t ClientContext::get_connection_id() const { return connection_id_; }
void ClientContext::set_connection_id(uint64_t id) { connection_id_ = id; }

uint16_t ClientContext::get_server_port() const { return server_port_; }
void ClientContext::set_server_port(uint16_t port) { server_port_ = port; }

const std::string& ClientContext::get_client_ip() const { return client_ip_; }
void ClientContext::set_client_ip(const std::string& ip) { client_ip_ = ip; }

uint16_t ClientContext::get_client_port() const { return client_port_; }
void ClientContext::set_client_port(uint16_t port) { client_port_ = port; }

const HttpRequest& ClientContext::get_request() const { return request_; }
HttpRequest& ClientContext::get_request() { return request_; }
void ClientContext::set_request(const HttpRequest& req) { request_ = req; }

const HttpResponse& ClientContext::get_response() const { return response_; }
HttpResponse& ClientContext::get_response() { return response_; }
void ClientContext::set_response(const HttpResponse& resp) { response_ = resp; }

void ClientContext::reset() {
    connection_id_ = 0;
    server_port_ = 0;
    client_ip_.clear();
    client_port_ = 0;
    request_ = HttpRequest();
    response_ = HttpResponse();
}

// DefaultCallbackStrategy
DefaultCallbackStrategy::DefaultCallbackStrategy() {
}

DefaultCallbackStrategy::~DefaultCallbackStrategy() {
}

const std::string& DefaultCallbackStrategy::get_name() const {
    static const std::string name = "default";
    return name;
}

bool DefaultCallbackStrategy::execute(ClientContext& context) {
    auto& resp = context.get_response();
    resp.status_code = 200;
    resp.status_text = "OK";
    resp.body = "{\"status\":\"ok\"}";
    resp.headers.clear();
    resp.headers.emplace_back("Content-Type", "application/json");
    resp.headers.emplace_back("Content-Length", std::to_string(resp.body.size()));
    return true;
}

// CallbackManager
CallbackManager::CallbackManager() {
}

CallbackManager::~CallbackManager() {
}

void CallbackManager::set_callbacks_dir(const std::string& dir) {
    callbacks_dir_ = dir;
}

const std::string& CallbackManager::get_callbacks_dir() const {
    return callbacks_dir_;
}

bool CallbackManager::load_script(const std::string& script_path, uint16_t server_port) {
    (void)script_path;
    strategies_[server_port] = std::make_unique<DefaultCallbackStrategy>();
    return true;
}

void CallbackManager::unload_script(uint16_t server_port) {
    strategies_.erase(server_port);
}

CallbackStrategy* CallbackManager::get_strategy(uint16_t server_port) {
    auto it = strategies_.find(server_port);
    if (it != strategies_.end()) {
        return it->second.get();
    }
    return &default_strategy_;
}

bool CallbackManager::execute_callback(ClientContext& context) {
    CallbackStrategy* strategy = get_strategy(context.get_server_port());
    return strategy->execute(context);
}

void CallbackManager::register_strategy(uint16_t server_port, std::unique_ptr<CallbackStrategy> strategy) {
    strategies_[server_port] = std::move(strategy);
}

} // namespace callback
} // namespace https_server_sim
