#include "utils/error.hpp"

namespace https_server_sim {
namespace utils {

const char* error_code_to_string(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS: return "SUCCESS";
        case ErrorCode::UNKNOWN_ERROR: return "UNKNOWN_ERROR";
        case ErrorCode::INVALID_ARGUMENT: return "INVALID_ARGUMENT";
        case ErrorCode::NULL_POINTER: return "NULL_POINTER";
        case ErrorCode::OUT_OF_MEMORY: return "OUT_OF_MEMORY";
        case ErrorCode::OUT_OF_RANGE: return "OUT_OF_RANGE";
        case ErrorCode::NOT_INITIALIZED: return "NOT_INITIALIZED";
        case ErrorCode::ALREADY_INITIALIZED: return "ALREADY_INITIALIZED";
        case ErrorCode::OPERATION_FAILED: return "OPERATION_FAILED";
        case ErrorCode::TIMEOUT: return "TIMEOUT";
        case ErrorCode::BUFFER_OVERFLOW: return "BUFFER_OVERFLOW";
        case ErrorCode::BUFFER_UNDERFLOW: return "BUFFER_UNDERFLOW";

        case ErrorCode::FILE_NOT_FOUND: return "FILE_NOT_FOUND";
        case ErrorCode::FILE_PERMISSION_DENIED: return "FILE_PERMISSION_DENIED";
        case ErrorCode::FILE_READ_ERROR: return "FILE_READ_ERROR";
        case ErrorCode::FILE_WRITE_ERROR: return "FILE_WRITE_ERROR";
        case ErrorCode::FILE_CLOSE_ERROR: return "FILE_CLOSE_ERROR";

        case ErrorCode::CONFIG_PARSE_ERROR: return "CONFIG_PARSE_ERROR";
        case ErrorCode::CONFIG_MISSING_REQUIRED: return "CONFIG_MISSING_REQUIRED";
        case ErrorCode::CONFIG_INVALID_VALUE: return "CONFIG_INVALID_VALUE";
        case ErrorCode::CONFIG_INVALID_PORT: return "CONFIG_INVALID_PORT";
        case ErrorCode::CONFIG_INVALID_THREAD_COUNT: return "CONFIG_INVALID_THREAD_COUNT";
        case ErrorCode::CONFIG_INVALID_LOG_LEVEL: return "CONFIG_INVALID_LOG_LEVEL";

        case ErrorCode::NETWORK_SOCKET_ERROR: return "NETWORK_SOCKET_ERROR";
        case ErrorCode::NETWORK_BIND_ERROR: return "NETWORK_BIND_ERROR";
        case ErrorCode::NETWORK_LISTEN_ERROR: return "NETWORK_LISTEN_ERROR";
        case ErrorCode::NETWORK_ACCEPT_ERROR: return "NETWORK_ACCEPT_ERROR";
        case ErrorCode::NETWORK_CONNECT_ERROR: return "NETWORK_CONNECT_ERROR";
        case ErrorCode::NETWORK_READ_ERROR: return "NETWORK_READ_ERROR";
        case ErrorCode::NETWORK_WRITE_ERROR: return "NETWORK_WRITE_ERROR";
        case ErrorCode::NETWORK_CLOSED: return "NETWORK_CLOSED";

        case ErrorCode::TLS_INIT_ERROR: return "TLS_INIT_ERROR";
        case ErrorCode::TLS_CERT_ERROR: return "TLS_CERT_ERROR";
        case ErrorCode::TLS_KEY_ERROR: return "TLS_KEY_ERROR";
        case ErrorCode::TLS_HANDSHAKE_ERROR: return "TLS_HANDSHAKE_ERROR";
        case ErrorCode::TLS_ENCRYPT_ERROR: return "TLS_ENCRYPT_ERROR";
        case ErrorCode::TLS_DECRYPT_ERROR: return "TLS_DECRYPT_ERROR";

        case ErrorCode::PROTOCOL_INVALID_FRAME: return "PROTOCOL_INVALID_FRAME";
        case ErrorCode::PROTOCOL_INVALID_HEADER: return "PROTOCOL_INVALID_HEADER";
        case ErrorCode::PROTOCOL_FRAME_TOO_LARGE: return "PROTOCOL_FRAME_TOO_LARGE";
        case ErrorCode::PROTOCOL_STREAM_ERROR: return "PROTOCOL_STREAM_ERROR";
        case ErrorCode::PROTOCOL_GOAWAY: return "PROTOCOL_GOAWAY";

        case ErrorCode::BUFFER_TOO_SMALL: return "BUFFER_TOO_SMALL";
        case ErrorCode::BUFFER_CAPACITY_EXCEEDED: return "BUFFER_CAPACITY_EXCEEDED";
        case ErrorCode::BUFFER_NO_DATA: return "BUFFER_NO_DATA";

        default: return "UNKNOWN_ERROR_CODE";
    }
}

const char* error_code_to_description(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS: return "Operation completed successfully";
        case ErrorCode::UNKNOWN_ERROR: return "An unknown error occurred";
        case ErrorCode::INVALID_ARGUMENT: return "Invalid argument";
        case ErrorCode::NULL_POINTER: return "Null pointer";
        case ErrorCode::OUT_OF_MEMORY: return "Out of memory";
        case ErrorCode::OUT_OF_RANGE: return "Out of range";
        case ErrorCode::NOT_INITIALIZED: return "Not initialized";
        case ErrorCode::ALREADY_INITIALIZED: return "Already initialized";
        case ErrorCode::OPERATION_FAILED: return "Operation failed";
        case ErrorCode::TIMEOUT: return "Operation timed out";
        case ErrorCode::BUFFER_OVERFLOW: return "Buffer overflow";
        case ErrorCode::BUFFER_UNDERFLOW: return "Buffer underflow";

        case ErrorCode::FILE_NOT_FOUND: return "File not found";
        case ErrorCode::FILE_PERMISSION_DENIED: return "Permission denied";
        case ErrorCode::FILE_READ_ERROR: return "File read error";
        case ErrorCode::FILE_WRITE_ERROR: return "File write error";
        case ErrorCode::FILE_CLOSE_ERROR: return "File close error";

        case ErrorCode::CONFIG_PARSE_ERROR: return "Config parse error";
        case ErrorCode::CONFIG_MISSING_REQUIRED: return "Missing required config";
        case ErrorCode::CONFIG_INVALID_VALUE: return "Invalid config value";
        case ErrorCode::CONFIG_INVALID_PORT: return "Invalid port number";
        case ErrorCode::CONFIG_INVALID_THREAD_COUNT: return "Invalid thread count";
        case ErrorCode::CONFIG_INVALID_LOG_LEVEL: return "Invalid log level";

        case ErrorCode::NETWORK_SOCKET_ERROR: return "Socket error";
        case ErrorCode::NETWORK_BIND_ERROR: return "Bind error";
        case ErrorCode::NETWORK_LISTEN_ERROR: return "Listen error";
        case ErrorCode::NETWORK_ACCEPT_ERROR: return "Accept error";
        case ErrorCode::NETWORK_CONNECT_ERROR: return "Connect error";
        case ErrorCode::NETWORK_READ_ERROR: return "Network read error";
        case ErrorCode::NETWORK_WRITE_ERROR: return "Network write error";
        case ErrorCode::NETWORK_CLOSED: return "Connection closed";

        case ErrorCode::TLS_INIT_ERROR: return "TLS init error";
        case ErrorCode::TLS_CERT_ERROR: return "TLS certificate error";
        case ErrorCode::TLS_KEY_ERROR: return "TLS key error";
        case ErrorCode::TLS_HANDSHAKE_ERROR: return "TLS handshake error";
        case ErrorCode::TLS_ENCRYPT_ERROR: return "TLS encrypt error";
        case ErrorCode::TLS_DECRYPT_ERROR: return "TLS decrypt error";

        case ErrorCode::PROTOCOL_INVALID_FRAME: return "Invalid protocol frame";
        case ErrorCode::PROTOCOL_INVALID_HEADER: return "Invalid protocol header";
        case ErrorCode::PROTOCOL_FRAME_TOO_LARGE: return "Frame too large";
        case ErrorCode::PROTOCOL_STREAM_ERROR: return "Stream error";
        case ErrorCode::PROTOCOL_GOAWAY: return "GOAWAY received";

        case ErrorCode::BUFFER_TOO_SMALL: return "Buffer too small";
        case ErrorCode::BUFFER_CAPACITY_EXCEEDED: return "Buffer capacity exceeded";
        case ErrorCode::BUFFER_NO_DATA: return "No data in buffer";

        default: return "Unknown error";
    }
}

} // namespace utils
} // namespace https_server_sim
