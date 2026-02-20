// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: tls_handler.cpp
//  描述: TlsHandler类实现 - TLS防腐层完整实现
//  版权: Copyright (c) 2026
// =============================================================================
#include "protocol/tls_handler.hpp"
#include <cstring>
#include <cstdio>
#include <memory>

#if HAVE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#endif

namespace https_server_sim {
namespace protocol {

// ==================== RAII资源包装器 ====================
namespace details {

// FILE* 包装器
struct FileCloser {
    void operator()(FILE* fp) const {
        if (fp) {
            fclose(fp);
        }
    }
};

using UniqueFilePtr = std::unique_ptr<FILE, FileCloser>;

#if HAVE_OPENSSL

// X509* 包装器
struct X509Deleter {
    void operator()(X509* x509) const {
        if (x509) {
            X509_free(x509);
        }
    }
};

using UniqueX509Ptr = std::unique_ptr<X509, X509Deleter>;

// EVP_PKEY* 包装器
struct EvpPkeyDeleter {
    void operator()(EVP_PKEY* pkey) const {
        if (pkey) {
            EVP_PKEY_free(pkey);
        }
    }
};

using UniqueEvpPkeyPtr = std::unique_ptr<EVP_PKEY, EvpPkeyDeleter>;

#endif // HAVE_OPENSSL

} // namespace details

#if HAVE_OPENSSL

// ==================== ALPN选择回调 ====================
static int AlpnSelectCallback(SSL* ssl, const unsigned char** out,
                               unsigned char* outlen, const unsigned char* in,
                               unsigned int inlen, void* arg) {
    (void)ssl;

    // 获取TlsHandler实例
    TlsHandler* handler = static_cast<TlsHandler*>(arg);
    if (!handler) {
        return SSL_TLSEXT_ERR_NOACK;
    }

    // 解析服务器配置的ALPN协议列表（格式: "h2,http/1.1"）
    const std::string& server_alpn = handler->get_alpn_protocols();

    // 我们优先按服务器配置的顺序选择
    // 首先解析服务器支持的协议列表
    std::vector<std::string> server_protocols;
    size_t pos = 0;
    while (pos < server_alpn.size()) {
        size_t comma = server_alpn.find(',', pos);
        if (comma == std::string::npos) {
            comma = server_alpn.size();
        }
        std::string proto = server_alpn.substr(pos, comma - pos);
        // 去除首尾空格
        size_t start = proto.find_first_not_of(" \t");
        size_t end = proto.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos && start <= end) {
            std::string trimmed = proto.substr(start, end - start + 1);
            // 跳过空字符串
            if (!trimmed.empty()) {
                server_protocols.push_back(std::move(trimmed));
            }
        }
        pos = comma + 1;
    }

    // 如果没有有效的协议，直接返回
    if (server_protocols.empty()) {
        return SSL_TLSEXT_ERR_NOACK;
    }

    // 遍历客户端支持的协议，按服务器配置的优先级选择
    const unsigned char* client_end = in + inlen;

    // 按服务器配置的优先级查找
    for (const auto& server_proto : server_protocols) {
        const unsigned char* client_ptr = in;
        while (client_ptr < client_end) {
            unsigned int len = *client_ptr++;
            if (client_ptr + len > client_end) {
                break;
            }
            if (len == server_proto.size() &&
                memcmp(client_ptr, server_proto.data(), len) == 0) {
                *out = client_ptr;
                *outlen = len;
                return SSL_TLSEXT_ERR_OK;
            }
            client_ptr += len;
        }
    }

    // 没有匹配的协议
    return SSL_TLSEXT_ERR_NOACK;
}

#endif // HAVE_OPENSSL

// ==================== TlsHandler实现 ====================

TlsHandler::TlsHandler()
    : conn_(nullptr)
    , ssl_ctx_(nullptr)
    , ssl_(nullptr)
    , ssl_read_bio_(nullptr)
    , ssl_write_bio_(nullptr)
    , initialized_(false)
    , handshake_done_(false)
    , read_buffer_(nullptr)
    , write_buffer_(nullptr)
    , cert_path_()
    , key_path_()
    , ca_path_()
    , cert_type_(CertType::RSA)
    , use_gmssl_(false)
    , error_code_(0)
    , error_msg_()
    , alpn_protocols_("h2,http/1.1")
{
    // 标记条件编译下可能未使用的字段
    (void)ssl_ctx_;
    (void)ssl_;
    (void)ssl_read_bio_;
    (void)ssl_write_bio_;
}

TlsHandler::~TlsHandler() {
    close();
}

int TlsHandler::init(Connection* conn,
                     const CertConfig& cert_config,
                     const TlsConfig& tls_config) {
    conn_ = conn;

    // 保存国密配置标志
    use_gmssl_ = cert_config.use_gmssl;

    if (!tls_config.alpn_protocols.empty()) {
        alpn_protocols_ = tls_config.alpn_protocols;
    }

    cert_path_ = cert_config.cert_path;
    key_path_ = cert_config.key_path;
    ca_path_ = cert_config.ca_path;

#if HAVE_OPENSSL
    // 1. 初始化SSL上下文
    int ret = init_ssl_context(cert_config);
    if (ret != PROTOCOL_OK) {
        return ret;
    }

    // 2. 加载证书
    ret = load_certificates(cert_config);
    if (ret != PROTOCOL_OK) {
        return ret;
    }

    // 3. 配置证书
    ret = configure_certificates(cert_config);
    if (ret != PROTOCOL_OK) {
        return ret;
    }

    // 4. 配置加密套件
    ret = configure_cipher_suites(tls_config);
    if (ret != PROTOCOL_OK) {
        return ret;
    }

    // 5. 配置ALPN
    ret = configure_alpn();
    if (ret != PROTOCOL_OK) {
        return ret;
    }

    // 6. 创建SSL对象
    ssl_ = SSL_new(static_cast<SSL_CTX*>(ssl_ctx_));
    if (!ssl_) {
        set_error(PROTOCOL_ERROR_TLS, "SSL_new failed");
        return PROTOCOL_ERROR_TLS;
    }

    // 7. 设置内存BIO
    ret = setup_bio();
    if (ret != PROTOCOL_OK) {
        return ret;
    }

    // 设置SSL为服务器模式
    SSL_set_accept_state(static_cast<SSL*>(ssl_));
#endif // HAVE_OPENSSL

    initialized_ = true;
    return PROTOCOL_OK;
}

int TlsHandler::read(uint8_t* data, size_t len, size_t* out_len) {
    // 检查初始化状态
    if (!initialized_) {
        set_error(PROTOCOL_ERROR_INVALID, "TlsHandler not initialized");
        return PROTOCOL_ERROR_INVALID;
    }

    if (out_len == nullptr) {
        set_error(PROTOCOL_ERROR_INVALID, "out_len is null");
        return PROTOCOL_ERROR_INVALID;
    }

    *out_len = 0;

#if HAVE_OPENSSL
    if (!ssl_ || !handshake_done_) {
        set_error(PROTOCOL_ERROR_INVALID, "SSL not initialized or handshake not done");
        return PROTOCOL_ERROR_INVALID;
    }

    // 步骤1: 从Connection read_buffer_读取密文 -> SSL read BIO
    int ret = pump_read_bio();
    if (ret < 0) {
        return ret;
    }

    // 步骤2: 从SSL读取明文
    ret = SSL_read(static_cast<SSL*>(ssl_), data, static_cast<int>(len));
    if (ret <= 0) {
        int err = SSL_get_error(static_cast<SSL*>(ssl_), ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            // 需要更多数据
            // 步骤3: 将SSL write BIO中的数据 -> Connection write_buffer_
            pump_write_bio();
            return PROTOCOL_ERROR_EAGAIN;
        } else {
            // 真实错误
            set_error(PROTOCOL_ERROR_TLS, "SSL_read failed");
            return PROTOCOL_ERROR_TLS;
        }
    }

    *out_len = static_cast<size_t>(ret);

    // 步骤3: 将SSL write BIO中的数据 -> Connection write_buffer_
    pump_write_bio();

    return PROTOCOL_OK;
#else
    // 没有OpenSSL，返回EAGAIN（桩代码行为）
    (void)data;
    (void)len;
    return PROTOCOL_ERROR_EAGAIN;
#endif
}

int TlsHandler::write(const uint8_t* data, size_t len, size_t* out_len) {
    // 检查初始化状态
    if (!initialized_) {
        set_error(PROTOCOL_ERROR_INVALID, "TlsHandler not initialized");
        return PROTOCOL_ERROR_INVALID;
    }

    if (out_len == nullptr) {
        set_error(PROTOCOL_ERROR_INVALID, "out_len is null");
        return PROTOCOL_ERROR_INVALID;
    }

    *out_len = 0;

#if HAVE_OPENSSL
    if (!ssl_ || !handshake_done_) {
        set_error(PROTOCOL_ERROR_INVALID, "SSL not initialized or handshake not done");
        return PROTOCOL_ERROR_INVALID;
    }

    // 步骤1: 从Connection read_buffer_读取密文 -> SSL read BIO
    // (SSL_write可能需要读取数据)
    int ret = pump_read_bio();
    if (ret < 0) {
        return ret;
    }

    // 步骤2: 写入明文到SSL
    ret = SSL_write(static_cast<SSL*>(ssl_), data, static_cast<int>(len));
    if (ret <= 0) {
        int err = SSL_get_error(static_cast<SSL*>(ssl_), ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            // 需要更多数据或等待写入
            // 步骤3: 将SSL write BIO中的数据 -> Connection write_buffer_
            pump_write_bio();
            return PROTOCOL_ERROR_EAGAIN;
        } else {
            // 真实错误
            set_error(PROTOCOL_ERROR_TLS, "SSL_write failed");
            return PROTOCOL_ERROR_TLS;
        }
    }

    *out_len = static_cast<size_t>(ret);

    // 步骤3: 将SSL write BIO中的数据 -> Connection write_buffer_
    pump_write_bio();

    return PROTOCOL_OK;
#else
    // 没有OpenSSL，直接返回成功（桩代码行为）
    (void)data;
    *out_len = len;
    return PROTOCOL_OK;
#endif
}

int TlsHandler::continue_handshake() {
    // 检查初始化状态
    if (!initialized_) {
        set_error(PROTOCOL_ERROR_INVALID, "TlsHandler not initialized");
        return PROTOCOL_ERROR_INVALID;
    }

#if HAVE_OPENSSL
    if (!ssl_) {
        set_error(PROTOCOL_ERROR_INVALID, "SSL not initialized");
        return PROTOCOL_ERROR_INVALID;
    }

    // 步骤1: 从Connection read_buffer_读取密文 -> SSL read BIO
    int ret = pump_read_bio();
    if (ret < 0) {
        return ret;
    }

    // 步骤2: 执行握手
    ret = SSL_do_handshake(static_cast<SSL*>(ssl_));
    if (ret != 1) {
        int err = SSL_get_error(static_cast<SSL*>(ssl_), ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            // 握手进行中，需要更多数据
            // 步骤3: 将SSL write BIO中的数据 -> Connection write_buffer_
            pump_write_bio();
            return PROTOCOL_ERROR_EAGAIN;
        } else {
            // 真实错误
            set_error(PROTOCOL_ERROR_TLS, "SSL_do_handshake failed");
            return PROTOCOL_ERROR_TLS;
        }
    }

    // 握手成功
    handshake_done_ = true;

    // 步骤3: 将SSL write BIO中的数据 -> Connection write_buffer_
    pump_write_bio();
#else
    // 没有OpenSSL，直接设置握手完成（桩代码行为）
    handshake_done_ = true;
#endif

    return PROTOCOL_OK;
}

bool TlsHandler::is_handshake_done() const {
    return handshake_done_;
}

std::string TlsHandler::get_selected_alpn() const {
#if HAVE_OPENSSL
    if (!ssl_) {
        return "";
    }

    const unsigned char* alpn = nullptr;
    unsigned int alpn_len = 0;
    SSL_get0_alpn_selected(static_cast<SSL*>(ssl_), &alpn, &alpn_len);

    if (alpn && alpn_len > 0) {
        return std::string(reinterpret_cast<const char*>(alpn), alpn_len);
    }

    return "";
#else
    // 没有OpenSSL，返回默认http/1.1
    return "http/1.1";
#endif
}

int TlsHandler::shutdown() {
#if HAVE_OPENSSL
    if (!ssl_) {
        return PROTOCOL_OK;
    }

    // 尝试优雅关闭 - 使用循环处理双向关闭
    int retry_count = 0;
    const int MAX_RETRIES = 2;
    int ret = 0;

    // SSL_shutdown可能需要调用多次来完成双向关闭
    while (retry_count < MAX_RETRIES) {
        ret = SSL_shutdown(static_cast<SSL*>(ssl_));
        if (ret == 1) {
            // 关闭成功完成
            break;
        }
        if (ret == 0) {
            // 需要继续处理：发送数据并再次尝试
            pump_write_bio();
            retry_count++;
            continue;
        }
        // ret < 0: 检查错误类型
        int err = SSL_get_error(static_cast<SSL*>(ssl_), ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            pump_write_bio();
            retry_count++;
            continue;
        }
        // 其他错误：记录但继续，因为我们正在关闭
        break;
    }

    // 最后确保发送所有待处理数据
    pump_write_bio();
#endif

    return PROTOCOL_OK;
}

int TlsHandler::close() {
#if HAVE_OPENSSL
    if (ssl_) {
        SSL_free(static_cast<SSL*>(ssl_));
        ssl_ = nullptr;
    }
    if (ssl_ctx_) {
        SSL_CTX_free(static_cast<SSL_CTX*>(ssl_ctx_));
        ssl_ctx_ = nullptr;
    }
    ssl_read_bio_ = nullptr;  // BIO由SSL_free释放
    ssl_write_bio_ = nullptr;
#endif

    initialized_ = false;
    handshake_done_ = false;
    error_code_ = 0;
    error_msg_.clear();
    return PROTOCOL_OK;
}

void TlsHandler::set_error(int code, const std::string& msg) {
    error_code_ = code;
    error_msg_ = msg;
}

int TlsHandler::get_error_code() const {
    return error_code_;
}

const std::string& TlsHandler::get_error_msg() const {
    return error_msg_;
}

void TlsHandler::reset() {
    handshake_done_ = false;
    error_code_ = 0;
    error_msg_.clear();
}

void TlsHandler::set_read_buffer(utils::Buffer* buffer) {
    read_buffer_ = buffer;
}

void TlsHandler::set_write_buffer(utils::Buffer* buffer) {
    write_buffer_ = buffer;
}

// ==================== 私有方法实现 ====================

#if HAVE_OPENSSL

int TlsHandler::init_ssl_context(const CertConfig& config) {
    (void)config;

    // 使用TLS_server_method()创建SSL上下文
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    ssl_ctx_ = SSL_CTX_new(TLS_server_method());
#else
    ssl_ctx_ = SSL_CTX_new(SSLv23_server_method());
    SSL_CTX_set_options(static_cast<SSL_CTX*>(ssl_ctx_), SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
#endif

    if (!ssl_ctx_) {
        set_error(PROTOCOL_ERROR_TLS, "SSL_CTX_new failed");
        return PROTOCOL_ERROR_TLS;
    }

    // 设置推荐的SSL选项
    SSL_CTX_set_options(static_cast<SSL_CTX*>(ssl_ctx_), SSL_OP_ALL | SSL_OP_NO_COMPRESSION);

    return PROTOCOL_OK;
}

int TlsHandler::load_certificates(const CertConfig& config) {
    // 注意：cert_path_、key_path_、ca_path_已在init()中设置，避免重复赋值

    // 如果证书路径为空，我们不尝试加载（测试场景）
    if (cert_path_.empty() || key_path_.empty()) {
        cert_type_ = CertType::RSA;
        return PROTOCOL_OK;
    }

    // 尝试读取证书以识别类型（使用RAII管理所有资源）
    details::UniqueFilePtr fp(fopen(cert_path_.c_str(), "r"));
    if (fp) {
        details::UniqueX509Ptr x509(PEM_read_X509(fp.get(), nullptr, nullptr, nullptr));
        if (x509) {
            details::UniqueEvpPkeyPtr pkey(X509_get_pubkey(x509.get()));
            if (pkey) {
                int key_type = EVP_PKEY_id(pkey.get());
#ifdef EVP_PKEY_SM2
                if (key_type == EVP_PKEY_SM2) {
                    cert_type_ = CertType::SM2;
                } else
#endif
                if (key_type == EVP_PKEY_EC) {
                    cert_type_ = CertType::ECDSA;
                } else {
                    cert_type_ = CertType::RSA;
                }
            }
        }
    }

    return PROTOCOL_OK;
}

int TlsHandler::load_ca_certificate(const std::string& ca_path) {
    ca_path_ = ca_path;

    if (ca_path_.empty()) {
        return PROTOCOL_OK;
    }

    if (!ssl_ctx_) {
        set_error(PROTOCOL_ERROR_INVALID, "SSL_CTX not initialized");
        return PROTOCOL_ERROR_INVALID;
    }

    if (SSL_CTX_load_verify_locations(static_cast<SSL_CTX*>(ssl_ctx_), ca_path_.c_str(), nullptr) != 1) {
        set_error(PROTOCOL_ERROR_TLS, "Failed to load CA certificate");
        return PROTOCOL_ERROR_TLS;
    }

    return PROTOCOL_OK;
}

int TlsHandler::configure_certificates(const CertConfig& config) {
    (void)config;

    if (!ssl_ctx_) {
        set_error(PROTOCOL_ERROR_INVALID, "SSL_CTX not initialized");
        return PROTOCOL_ERROR_INVALID;
    }

    // 如果证书路径为空，我们不尝试加载（测试场景）
    if (cert_path_.empty() || key_path_.empty()) {
        return PROTOCOL_OK;
    }

    // 加载证书链
    if (SSL_CTX_use_certificate_chain_file(static_cast<SSL_CTX*>(ssl_ctx_), cert_path_.c_str()) != 1) {
        set_error(PROTOCOL_ERROR_TLS, "Failed to load certificate chain");
        return PROTOCOL_ERROR_TLS;
    }

    // 加载私钥
    if (SSL_CTX_use_PrivateKey_file(static_cast<SSL_CTX*>(ssl_ctx_), key_path_.c_str(), SSL_FILETYPE_PEM) != 1) {
        set_error(PROTOCOL_ERROR_TLS, "Failed to load private key");
        return PROTOCOL_ERROR_TLS;
    }

    // 验证私钥
    if (SSL_CTX_check_private_key(static_cast<SSL_CTX*>(ssl_ctx_)) != 1) {
        set_error(PROTOCOL_ERROR_TLS, "Private key does not match certificate");
        return PROTOCOL_ERROR_TLS;
    }

    return PROTOCOL_OK;
}

int TlsHandler::configure_cipher_suites(const TlsConfig& config) {
    if (!ssl_ctx_) {
        set_error(PROTOCOL_ERROR_INVALID, "SSL_CTX not initialized");
        return PROTOCOL_ERROR_INVALID;
    }

    // 检查是否使用国密（优先使用配置标志，其次根据证书类型判断）
    if (use_gmssl_ || cert_type_ == CertType::SM2) {
        // 配置国密SM2/SM3/SM4套件
        const char* gm_ciphers = "SM2-WITH-SMS4-SM3";
        if (SSL_CTX_set_cipher_list(static_cast<SSL_CTX*>(ssl_ctx_), gm_ciphers) != 1) {
            set_error(PROTOCOL_ERROR_TLS, "Failed to set GMSSL cipher list");
            return PROTOCOL_ERROR_TLS;
        }
        // 国密主要使用TLS 1.2
#ifdef SSL_OP_NO_TLSv1_3
        SSL_CTX_set_options(static_cast<SSL_CTX*>(ssl_ctx_), SSL_OP_NO_TLSv1_3);
#endif
    } else if (!config.cipher_suites.empty()) {
        // 使用用户指定的加密套件
        if (SSL_CTX_set_cipher_list(static_cast<SSL_CTX*>(ssl_ctx_), config.cipher_suites.c_str()) != 1) {
            set_error(PROTOCOL_ERROR_TLS, "Failed to set cipher list");
            return PROTOCOL_ERROR_TLS;
        }
    } else {
        // 使用默认加密套件
        const char* default_ciphers =
            "ECDHE-ECDSA-AES128-GCM-SHA256:"
            "ECDHE-RSA-AES128-GCM-SHA256:"
            "ECDHE-ECDSA-AES256-GCM-SHA384:"
            "ECDHE-RSA-AES256-GCM-SHA384";
        if (SSL_CTX_set_cipher_list(static_cast<SSL_CTX*>(ssl_ctx_), default_ciphers) != 1) {
            set_error(PROTOCOL_ERROR_TLS, "Failed to set default cipher list");
            return PROTOCOL_ERROR_TLS;
        }
    }

    // 配置TLS版本
#ifdef SSL_OP_NO_TLSv1_2
    if (!config.enable_tls_1_2) {
        SSL_CTX_set_options(static_cast<SSL_CTX*>(ssl_ctx_), SSL_OP_NO_TLSv1_2);
    }
#endif
#ifdef SSL_OP_NO_TLSv1_3
    if (!config.enable_tls_1_3) {
        SSL_CTX_set_options(static_cast<SSL_CTX*>(ssl_ctx_), SSL_OP_NO_TLSv1_3);
    }
#endif

    return PROTOCOL_OK;
}

int TlsHandler::configure_alpn() {
    if (!ssl_ctx_) {
        set_error(PROTOCOL_ERROR_INVALID, "SSL_CTX not initialized");
        return PROTOCOL_ERROR_INVALID;
    }

    // 设置ALPN选择回调
    SSL_CTX_set_alpn_select_cb(static_cast<SSL_CTX*>(ssl_ctx_), AlpnSelectCallback, this);

    // 注意：在服务器端，我们不需要调用 SSL_CTX_set_alpn_protos
    // 服务器端是通过回调函数 AlpnSelectCallback 来选择协议的
    // 我们的回调函数已经支持从 alpn_protocols_ 中选择合适的协议

    return PROTOCOL_OK;
}

int TlsHandler::setup_bio() {
    if (!ssl_) {
        set_error(PROTOCOL_ERROR_INVALID, "SSL not initialized");
        return PROTOCOL_ERROR_INVALID;
    }

    // 创建内存BIO对
    BIO* bio1 = nullptr;
    BIO* bio2 = nullptr;

    if (BIO_new_bio_pair(&bio1, &bio2, 0) != 1) {
        set_error(PROTOCOL_ERROR_TLS, "BIO_new_bio_pair failed");
        return PROTOCOL_ERROR_TLS;
    }

    ssl_read_bio_ = bio1;
    ssl_write_bio_ = bio2;

    // 设置BIO到SSL
    SSL_set_bio(static_cast<SSL*>(ssl_), static_cast<BIO*>(ssl_read_bio_), static_cast<BIO*>(ssl_write_bio_));

    return PROTOCOL_OK;
}

int TlsHandler::pump_read_bio() {
    // 从Connection read_buffer_读取数据，写入SSL read BIO
    // 处理部分写入的情况
    if (!read_buffer_ || !ssl_read_bio_) {
        return PROTOCOL_ERROR_INVALID;
    }

    size_t readable = read_buffer_->readable_bytes();
    if (readable == 0) {
        return 0;  // 没有数据需要读取
    }

    const uint8_t* data = read_buffer_->read_ptr();
    size_t total_written = 0;

    while (total_written < readable) {
        // 尝试写入剩余数据
        int written = BIO_write(static_cast<BIO*>(ssl_read_bio_),
                               data + total_written,
                               static_cast<int>(readable - total_written));
        if (written <= 0) {
            // BIO_WRITE返回0或负数表示错误或重试
            if (!BIO_should_retry(static_cast<BIO*>(ssl_read_bio_))) {
                // 真实错误
                set_error(PROTOCOL_ERROR_TLS, "BIO_write failed");
                return PROTOCOL_ERROR_TLS;
            }
            // 应该重试，退出循环，等待下次调用
            break;
        }
        total_written += static_cast<size_t>(written);
    }

    if (total_written > 0) {
        // 消耗已写入BIO的数据
        read_buffer_->skip(total_written);
    }

    return static_cast<int>(total_written);
}

int TlsHandler::pump_write_bio() {
    // 从SSL write BIO读取数据，写入Connection write_buffer_
    // 使用BIO_ctrl_pending检查待读取数据量
    if (!write_buffer_ || !ssl_write_bio_) {
        return PROTOCOL_ERROR_INVALID;
    }

    size_t total_read = 0;
    uint8_t buf[TEMP_BUFFER_SIZE];  // 临时缓冲区

    while (true) {
        // 检查SSL write BIO中有多少待读取的数据
        size_t pending = BIO_ctrl_pending(static_cast<BIO*>(ssl_write_bio_));
        if (pending == 0) {
            break;  // 没有更多数据
        }

        // 读取数据（每次最多读取sizeof(buf)）
        int to_read = (sizeof(buf) < pending) ? static_cast<int>(sizeof(buf)) : static_cast<int>(pending);
        int read = BIO_read(static_cast<BIO*>(ssl_write_bio_), buf, to_read);
        if (read <= 0) {
            // BIO_read返回0或负数表示错误或重试
            if (!BIO_should_retry(static_cast<BIO*>(ssl_write_bio_))) {
                // 真实错误
                set_error(PROTOCOL_ERROR_TLS, "BIO_read failed");
                return PROTOCOL_ERROR_TLS;
            }
            // 需要重试，退出循环
            break;
        }

        // 将读取的数据写入Connection的write_buffer_
        if (write_buffer_->write(buf, static_cast<size_t>(read)) != static_cast<size_t>(read)) {
            set_error(PROTOCOL_ERROR_BUFFER, "Write buffer full");
            return PROTOCOL_ERROR_BUFFER;
        }

        total_read += static_cast<size_t>(read);
    }

    return static_cast<int>(total_read);
}

#else // !HAVE_OPENSSL

// 没有OpenSSL时的空实现
int TlsHandler::init_ssl_context(const CertConfig& config) {
    (void)config;
    return PROTOCOL_OK;
}

int TlsHandler::load_certificates(const CertConfig& config) {
    (void)config;
    cert_type_ = CertType::RSA;
    return PROTOCOL_OK;
}

int TlsHandler::load_ca_certificate(const std::string& ca_path) {
    (void)ca_path;
    return PROTOCOL_OK;
}

int TlsHandler::configure_certificates(const CertConfig& config) {
    (void)config;
    return PROTOCOL_OK;
}

int TlsHandler::configure_cipher_suites(const TlsConfig& config) {
    (void)config;
    return PROTOCOL_OK;
}

int TlsHandler::configure_alpn() {
    return PROTOCOL_OK;
}

int TlsHandler::setup_bio() {
    return PROTOCOL_OK;
}

int TlsHandler::pump_read_bio() {
    return 0;
}

int TlsHandler::pump_write_bio() {
    return 0;
}

#endif // HAVE_OPENSSL

bool TlsHandler::is_sm2_cert() const {
    return cert_type_ == CertType::SM2;
}

const std::string& TlsHandler::get_alpn_protocols() const {
    return alpn_protocols_;
}

} // namespace protocol
} // namespace https_server_sim

// 文件结束
