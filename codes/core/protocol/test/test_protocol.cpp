// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: test_protocol.cpp
//  描述: Protocol模块单元测试
//  版权: Copyright (c) 2026
// =============================================================================
#include "protocol/protocol.hpp"
#include "protocol/config_converter.hpp"
#include "protocol/protocol_handler_factory.hpp"
#include "utils/buffer.hpp"
#include <gtest/gtest.h>
#include <cstring>
#include <vector>

namespace https_server_sim {
namespace protocol {
namespace test {

// ==================== ProtocolTypes测试 ====================

class ProtocolTypesTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ProtocolTypesTest, CertConfigDefaultValues) {
    CertConfig config;
    EXPECT_TRUE(config.cert_path.empty());
    EXPECT_TRUE(config.key_path.empty());
    EXPECT_TRUE(config.ca_path.empty());
    EXPECT_FALSE(config.use_gmssl);
}

TEST_F(ProtocolTypesTest, TlsConfigDefaultValues) {
    TlsConfig config;
    EXPECT_TRUE(config.cipher_suites.empty());
    EXPECT_EQ(config.alpn_protocols, ALPN_HTTP11);  // 默认值已设置为http/1.1
    EXPECT_TRUE(config.enable_tls_1_3);
    EXPECT_TRUE(config.enable_tls_1_2);
}

TEST_F(ProtocolTypesTest, Http2SettingsDefaultValues) {
    Http2Settings settings;
    EXPECT_EQ(settings.header_table_size, 4096u);
    EXPECT_EQ(settings.enable_push, 0u);
    EXPECT_EQ(settings.max_concurrent_streams, 100u);
    EXPECT_EQ(settings.initial_window_size, 65535u);
    EXPECT_EQ(settings.max_frame_size, 16384u);
    EXPECT_EQ(settings.max_header_list_size, 0xFFFFFFFFu);
}

TEST_F(ProtocolTypesTest, Http2SettingsReset) {
    Http2Settings settings;
    settings.header_table_size = 8192;
    settings.reset();
    EXPECT_EQ(settings.header_table_size, 4096u);
}

// ==================== HttpRequest测试 ====================

class HttpRequestTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HttpRequestTest, DefaultConstructor) {
    HttpRequest req;
    EXPECT_TRUE(req.method.empty());
    EXPECT_TRUE(req.path.empty());
    EXPECT_EQ(req.version, "HTTP/1.1");
    EXPECT_TRUE(req.headers.empty());
    EXPECT_TRUE(req.body.empty());
    EXPECT_EQ(req.content_length, 0u);
    EXPECT_TRUE(req.debug_token.empty());
}

TEST_F(HttpRequestTest, Reset) {
    HttpRequest req;
    req.method = "GET";
    req.path = "/test";
    req.headers["Host"] = "example.com";
    req.content_length = 100;
    req.debug_token = "test-token";

    req.reset();

    EXPECT_TRUE(req.method.empty());
    EXPECT_TRUE(req.path.empty());
    EXPECT_EQ(req.version, "HTTP/1.1");
    EXPECT_TRUE(req.headers.empty());
    EXPECT_EQ(req.content_length, 0u);
    EXPECT_TRUE(req.debug_token.empty());
}

TEST_F(HttpRequestTest, Clear) {
    HttpRequest req;
    req.method = "POST";
    req.clear();
    EXPECT_TRUE(req.method.empty());
}

// ==================== HttpResponse测试 ====================

class HttpResponseTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HttpResponseTest, DefaultConstructor) {
    HttpResponse resp;
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_EQ(resp.status_text, "OK");
    EXPECT_TRUE(resp.headers.empty());
    EXPECT_TRUE(resp.body.empty());
}

TEST_F(HttpResponseTest, SetStatus) {
    HttpResponse resp;
    resp.set_status(404, "Not Found");
    EXPECT_EQ(resp.status_code, 404);
    EXPECT_EQ(resp.status_text, "Not Found");
}

TEST_F(HttpResponseTest, AddHeader) {
    HttpResponse resp;
    resp.add_header("Content-Type", "text/plain");
    EXPECT_EQ(resp.headers["Content-Type"], "text/plain");
}

TEST_F(HttpResponseTest, SetBody) {
    HttpResponse resp;
    const uint8_t data[] = { 'H', 'e', 'l', 'l', 'o' };
    resp.set_body(data, 5);
    EXPECT_EQ(resp.body.size(), 5u);
    EXPECT_EQ(resp.body[0], 'H');
    EXPECT_EQ(resp.body[4], 'o');
}

TEST_F(HttpResponseTest, Reset) {
    HttpResponse resp;
    resp.status_code = 500;
    resp.add_header("X-Test", "value");
    resp.reset();
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_TRUE(resp.headers.empty());
}

// ==================== Http2Stream测试 ====================

class Http2StreamTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(Http2StreamTest, DefaultConstructor) {
    Http2Stream stream;
    EXPECT_EQ(stream.stream_id, 0u);
    EXPECT_EQ(stream.state, Http2StreamState::IDLE);
    EXPECT_EQ(stream.send_window, 65535);
    EXPECT_EQ(stream.recv_window, 65535);
}

TEST_F(Http2StreamTest, Init) {
    Http2Stream stream;
    stream.init(1);
    EXPECT_EQ(stream.stream_id, 1u);
    EXPECT_EQ(stream.state, Http2StreamState::IDLE);
}

TEST_F(Http2StreamTest, Reset) {
    Http2Stream stream;
    stream.init(3);
    stream.state = Http2StreamState::OPEN;
    stream.reset();
    EXPECT_EQ(stream.stream_id, 0u);
    EXPECT_EQ(stream.state, Http2StreamState::IDLE);
}

// ==================== HttpParser测试 ====================

class HttpParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer_ = std::make_unique<utils::Buffer>();
        parser_.init(buffer_.get());
    }
    void TearDown() override {}

    std::unique_ptr<utils::Buffer> buffer_;
    HttpParser parser_;
};

TEST_F(HttpParserTest, ReadLineComplete) {
    const char* test_data = "GET / HTTP/1.1\r\n";
    buffer_->write(reinterpret_cast<const uint8_t*>(test_data), strlen(test_data));

    char line[256];
    size_t line_len = 0;
    int ret = parser_.read_line(line, sizeof(line), &line_len);

    EXPECT_EQ(ret, PROTOCOL_OK);
    EXPECT_EQ(line_len, 14u);
    EXPECT_STREQ(line, "GET / HTTP/1.1");
}

TEST_F(HttpParserTest, ReadLineIncomplete) {
    const char* test_data = "GET / HTTP/1.1";
    buffer_->write(reinterpret_cast<const uint8_t*>(test_data), strlen(test_data));

    char line[256];
    size_t line_len = 0;
    int ret = parser_.read_line(line, sizeof(line), &line_len);

    EXPECT_EQ(ret, PROTOCOL_ERROR_EAGAIN);  // EAGAIN
}

TEST_F(HttpParserTest, ReadLineEmpty) {
    const char* test_data = "\r\n";
    buffer_->write(reinterpret_cast<const uint8_t*>(test_data), strlen(test_data));

    char line[256];
    size_t line_len = 0;
    int ret = parser_.read_line(line, sizeof(line), &line_len);

    EXPECT_EQ(ret, PROTOCOL_OK);
    EXPECT_EQ(line_len, 0u);
}

TEST_F(HttpParserTest, ParseHeaderValid) {
    const char* line = "Host: example.com";
    std::string key, value;
    int ret = parser_.parse_header(line, strlen(line), &key, &value);

    EXPECT_EQ(ret, PROTOCOL_OK);
    EXPECT_EQ(key, "Host");
    EXPECT_EQ(value, "example.com");
}

TEST_F(HttpParserTest, ParseHeaderWithWhitespace) {
    const char* line = "  Host  :   example.com   ";
    std::string key, value;
    int ret = parser_.parse_header(line, strlen(line), &key, &value);

    EXPECT_EQ(ret, PROTOCOL_OK);
    EXPECT_EQ(key, "Host");
    EXPECT_EQ(value, "example.com");
}

TEST_F(HttpParserTest, ParseHeaderNoColon) {
    const char* line = "InvalidHeader";
    std::string key, value;
    int ret = parser_.parse_header(line, strlen(line), &key, &value);

    EXPECT_EQ(ret, PROTOCOL_ERROR_INVALID);
}

TEST_F(HttpParserTest, FindHeaderExists) {
    std::map<std::string, std::string> headers;
    headers["Host"] = "example.com";

    std::string value;
    bool found = parser_.find_header(headers, "Host", &value);

    EXPECT_TRUE(found);
    EXPECT_EQ(value, "example.com");
}

TEST_F(HttpParserTest, FindHeaderCaseInsensitive) {
    std::map<std::string, std::string> headers;
    headers["HOST"] = "example.com";

    std::string value;
    bool found = parser_.find_header(headers, "host", &value);

    EXPECT_TRUE(found);
}

TEST_F(HttpParserTest, FindHeaderNotExists) {
    std::map<std::string, std::string> headers;
    std::string value;
    bool found = parser_.find_header(headers, "X-Not-Exists", &value);

    EXPECT_FALSE(found);
}

TEST_F(HttpParserTest, ParseRequestLineValid) {
    const char* line = "GET / HTTP/1.1";
    std::string method, path, version;
    int ret = parser_.parse_request_line(line, strlen(line), &method, &path, &version);

    EXPECT_EQ(ret, PROTOCOL_OK);
    EXPECT_EQ(method, "GET");
    EXPECT_EQ(path, "/");
    EXPECT_EQ(version, "HTTP/1.1");
}

TEST_F(HttpParserTest, ParseRequestLineInvalidVersion) {
    const char* line = "GET / HTTP/1.0";
    std::string method, path, version;
    int ret = parser_.parse_request_line(line, strlen(line), &method, &path, &version);

    EXPECT_EQ(ret, PROTOCOL_ERROR_VERSION);
}

TEST_F(HttpParserTest, ParseRequestLineInvalidFormat) {
    const char* line = "GET /";
    std::string method, path, version;
    int ret = parser_.parse_request_line(line, strlen(line), &method, &path, &version);

    EXPECT_EQ(ret, PROTOCOL_ERROR_INVALID);
}

TEST_F(HttpParserTest, BuildResponse) {
    HttpResponse resp;
    resp.set_status(200, "OK");
    resp.add_header("Content-Type", "text/plain");
    const char* body = "Hello";
    resp.set_body(reinterpret_cast<const uint8_t*>(body), 5);

    uint8_t buf[1024];
    size_t out_len = 0;
    int ret = parser_.build_response(resp, buf, sizeof(buf), &out_len);

    EXPECT_EQ(ret, PROTOCOL_OK);
    EXPECT_GT(out_len, 0u);

    std::string result(reinterpret_cast<char*>(buf), out_len);
    EXPECT_NE(result.find("HTTP/1.1 200 OK"), std::string::npos);
    EXPECT_NE(result.find("Content-Type: text/plain"), std::string::npos);
    EXPECT_NE(result.find("Content-Length: 5"), std::string::npos);
    EXPECT_NE(result.find("Hello"), std::string::npos);
}

// ==================== Hpack测试 ====================

class HpackTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HpackTest, EncodeDecode) {
    std::map<std::string, std::string> headers;
    headers[":method"] = "GET";
    headers[":path"] = "/";
    headers[":scheme"] = "https";

    std::vector<uint8_t> encoded;
    int ret = hpack_encode(headers, &encoded);
    EXPECT_EQ(ret, PROTOCOL_OK);
    EXPECT_GT(encoded.size(), 0u);

    std::map<std::string, std::string> decoded;
    ret = hpack_decode(encoded.data(), encoded.size(), &decoded);
    EXPECT_EQ(ret, PROTOCOL_OK);

    // 验证解码结果与原始输入一致
    EXPECT_EQ(decoded.size(), headers.size());
    for (const auto& pair : headers) {
        auto it = decoded.find(pair.first);
        EXPECT_NE(it, decoded.end());
        if (it != decoded.end()) {
            EXPECT_EQ(it->second, pair.second);
        }
    }
}

TEST_F(HpackTest, EncodeDecodeWithStaticTable) {
    // 测试使用静态表索引的头部编码解码
    std::map<std::string, std::string> headers;
    headers[":status"] = "200";
    headers["content-type"] = "text/plain";

    std::vector<uint8_t> encoded;
    int ret = hpack_encode(headers, &encoded);
    EXPECT_EQ(ret, PROTOCOL_OK);
    EXPECT_GT(encoded.size(), 0u);

    std::map<std::string, std::string> decoded;
    ret = hpack_decode(encoded.data(), encoded.size(), &decoded);
    EXPECT_EQ(ret, PROTOCOL_OK);
}

TEST_F(HpackTest, EncodeDecodeEmptyHeaders) {
    std::map<std::string, std::string> headers;

    std::vector<uint8_t> encoded;
    int ret = hpack_encode(headers, &encoded);
    EXPECT_EQ(ret, PROTOCOL_OK);

    std::map<std::string, std::string> decoded;
    ret = hpack_decode(encoded.data(), encoded.size(), &decoded);
    EXPECT_EQ(ret, PROTOCOL_OK);
    EXPECT_TRUE(decoded.empty());
}

TEST_F(HpackTest, EncoderSetMaxSize) {
    HpackEncoder encoder;
    encoder.set_max_dynamic_table_size(8192);
    EXPECT_EQ(encoder.get_max_dynamic_table_size(), 8192u);
}

TEST_F(HpackTest, DecoderSetMaxSize) {
    HpackDecoder decoder;
    decoder.set_max_dynamic_table_size(8192);
    EXPECT_EQ(decoder.get_max_dynamic_table_size(), 8192u);
}

// ==================== ProtocolFactory测试 ====================

class ProtocolFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ProtocolFactoryTest, SelectProtocolByAlpnH2) {
    ProtocolFactory& factory = ProtocolFactory::instance();
    ProtocolType type = factory.select_protocol_by_alpn("h2");
    EXPECT_EQ(type, ProtocolType::HTTP_2);
}

TEST_F(ProtocolFactoryTest, SelectProtocolByAlpnHttp11) {
    ProtocolFactory& factory = ProtocolFactory::instance();
    ProtocolType type = factory.select_protocol_by_alpn("http/1.1");
    EXPECT_EQ(type, ProtocolType::HTTP_1_1);
}

TEST_F(ProtocolFactoryTest, SelectProtocolByAlpnUnknown) {
    ProtocolFactory& factory = ProtocolFactory::instance();
    ProtocolType type = factory.select_protocol_by_alpn("unknown");
    EXPECT_EQ(type, ProtocolType::HTTP_1_1);
}

TEST_F(ProtocolFactoryTest, CreateHttp1Handler) {
    ProtocolFactory& factory = ProtocolFactory::instance();
    ProtocolHandler* handler = factory.create_handler(ProtocolType::HTTP_1_1);
    ASSERT_NE(handler, nullptr);
    EXPECT_EQ(handler->get_protocol_type(), ProtocolType::HTTP_1_1);
    factory.destroy_handler(handler);
}

TEST_F(ProtocolFactoryTest, CreateHttp2Handler) {
    ProtocolFactory& factory = ProtocolFactory::instance();
    ProtocolHandler* handler = factory.create_handler(ProtocolType::HTTP_2);
    ASSERT_NE(handler, nullptr);
    EXPECT_EQ(handler->get_protocol_type(), ProtocolType::HTTP_2);
    factory.destroy_handler(handler);
}

// ==================== Http1Handler测试 ====================

class Http1HandlerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(Http1HandlerTest, GetProtocolType) {
    Http1Handler handler;
    EXPECT_EQ(handler.get_protocol_type(), ProtocolType::HTTP_1_1);
}

TEST_F(Http1HandlerTest, GetTlsHandler) {
    Http1Handler handler;
    EXPECT_NE(handler.get_tls_handler(), nullptr);
}

TEST_F(Http1HandlerTest, Reset) {
    Http1Handler handler;
    handler.reset();
    SUCCEED();
}

// ==================== Http2Handler测试 ====================

class Http2HandlerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(Http2HandlerTest, GetProtocolType) {
    Http2Handler handler;
    EXPECT_EQ(handler.get_protocol_type(), ProtocolType::HTTP_2);
}

TEST_F(Http2HandlerTest, GetTlsHandler) {
    Http2Handler handler;
    EXPECT_NE(handler.get_tls_handler(), nullptr);
}

TEST_F(Http2HandlerTest, Reset) {
    Http2Handler handler;
    handler.reset();
    SUCCEED();
}

// ==================== TlsHandler测试 ====================

class TlsHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TlsHandlerTest, DefaultState) {
    TlsHandler handler;
    EXPECT_FALSE(handler.is_handshake_done());
    EXPECT_EQ(handler.get_error_code(), 0);
}

TEST_F(TlsHandlerTest, SetGetError) {
    TlsHandler handler;
    handler.set_error(-10, "Test error");
    EXPECT_EQ(handler.get_error_code(), -10);
    EXPECT_EQ(handler.get_error_msg(), "Test error");
}

TEST_F(TlsHandlerTest, Reset) {
    TlsHandler handler;
    handler.set_error(-10, "Test error");
    handler.reset();
    EXPECT_EQ(handler.get_error_code(), 0);
}

// ==================== 国密证书测试 ====================

class GmsslCertTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(GmsslCertTest, CertConfigGmsslFields) {
    CertConfig config;
    EXPECT_TRUE(config.sm2_cert_path.empty());
    EXPECT_TRUE(config.sm2_key_path.empty());
    EXPECT_TRUE(config.sm2_sign_cert_path.empty());
    EXPECT_TRUE(config.sm2_sign_key_path.empty());
    EXPECT_FALSE(config.use_gmssl);
}

TEST_F(GmsslCertTest, CertConfigWithSm2CertOnly) {
    CertConfig config;
    config.sm2_cert_path = "/path/to/sm2_cert.pem";
    EXPECT_FALSE(config.sm2_cert_path.empty());
    EXPECT_TRUE(config.sm2_key_path.empty());
}

TEST_F(GmsslCertTest, CertConfigWithSm2SignCertOnly) {
    CertConfig config;
    config.sm2_sign_cert_path = "/path/to/sm2_sign_cert.pem";
    config.sm2_sign_key_path = "/path/to/sm2_sign_key.pem";
    EXPECT_FALSE(config.sm2_sign_cert_path.empty());
    EXPECT_FALSE(config.sm2_sign_key_path.empty());
}

TEST_F(GmsslCertTest, CertConfigWithDualCerts) {
    CertConfig config;
    config.sm2_cert_path = "/path/to/sm2_enc_cert.pem";
    config.sm2_key_path = "/path/to/sm2_enc_key.pem";
    config.sm2_sign_cert_path = "/path/to/sm2_sign_cert.pem";
    config.sm2_sign_key_path = "/path/to/sm2_sign_key.pem";
    config.use_gmssl = true;

    EXPECT_FALSE(config.sm2_cert_path.empty());
    EXPECT_FALSE(config.sm2_key_path.empty());
    EXPECT_FALSE(config.sm2_sign_cert_path.empty());
    EXPECT_FALSE(config.sm2_sign_key_path.empty());
    EXPECT_TRUE(config.use_gmssl);
}

// ==================== ConfigConverter测试 ====================

class ConfigConverterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ConfigConverterTest, ConvertCertConfigBasic) {
    config::CertificatesConfig src;
    src.cert_path = "/path/to/cert.pem";
    src.key_path = "/path/to/key.pem";
    src.ca_path = "/path/to/ca.pem";

    CertConfig dst;
    ConfigConverter::convert_cert_config(src, dst);

    EXPECT_EQ(dst.cert_path, "/path/to/cert.pem");
    EXPECT_EQ(dst.key_path, "/path/to/key.pem");
    EXPECT_EQ(dst.ca_path, "/path/to/ca.pem");
    EXPECT_FALSE(dst.use_gmssl);
}

TEST_F(ConfigConverterTest, ConvertCertConfigWithGmssl) {
    config::CertificatesConfig src;
    src.sm2_cert_path = "/path/to/sm2_cert.pem";
    src.sm2_key_path = "/path/to/sm2_key.pem";

    CertConfig dst;
    ConfigConverter::convert_cert_config(src, dst);

    EXPECT_TRUE(dst.use_gmssl);
    EXPECT_EQ(dst.sm2_cert_path, "/path/to/sm2_cert.pem");
    EXPECT_EQ(dst.sm2_key_path, "/path/to/sm2_key.pem");
}

TEST_F(ConfigConverterTest, ConvertCertConfigGmsslOnlyCert) {
    config::CertificatesConfig src;
    src.sm2_cert_path = "/path/to/sm2_cert.pem";

    CertConfig dst;
    ConfigConverter::convert_cert_config(src, dst);

    EXPECT_TRUE(dst.use_gmssl);
    EXPECT_EQ(dst.sm2_cert_path, "/path/to/sm2_cert.pem");
}

TEST_F(ConfigConverterTest, ConvertCertConfigGmsslOnlyKey) {
    config::CertificatesConfig src;
    src.sm2_key_path = "/path/to/sm2_key.pem";

    CertConfig dst;
    ConfigConverter::convert_cert_config(src, dst);

    EXPECT_TRUE(dst.use_gmssl);
    EXPECT_EQ(dst.sm2_key_path, "/path/to/sm2_key.pem");
}

TEST_F(ConfigConverterTest, ConvertCertConfigWithSm2SignCert) {
    config::CertificatesConfig src;
    src.sm2_sign_cert_path = "/path/to/sm2_sign_cert.pem";
    src.sm2_sign_key_path = "/path/to/sm2_sign_key.pem";

    CertConfig dst;
    ConfigConverter::convert_cert_config(src, dst);

    EXPECT_TRUE(dst.use_gmssl);
    EXPECT_EQ(dst.sm2_sign_cert_path, "/path/to/sm2_sign_cert.pem");
    EXPECT_EQ(dst.sm2_sign_key_path, "/path/to/sm2_sign_key.pem");
}

TEST_F(ConfigConverterTest, ConvertCertConfigWithDualSm2Certs) {
    config::CertificatesConfig src;
    src.sm2_cert_path = "/path/to/sm2_enc_cert.pem";
    src.sm2_key_path = "/path/to/sm2_enc_key.pem";
    src.sm2_sign_cert_path = "/path/to/sm2_sign_cert.pem";
    src.sm2_sign_key_path = "/path/to/sm2_sign_key.pem";

    CertConfig dst;
    ConfigConverter::convert_cert_config(src, dst);

    EXPECT_TRUE(dst.use_gmssl);
    EXPECT_EQ(dst.sm2_cert_path, "/path/to/sm2_enc_cert.pem");
    EXPECT_EQ(dst.sm2_key_path, "/path/to/sm2_enc_key.pem");
    EXPECT_EQ(dst.sm2_sign_cert_path, "/path/to/sm2_sign_cert.pem");
    EXPECT_EQ(dst.sm2_sign_key_path, "/path/to/sm2_sign_key.pem");
}

TEST_F(ConfigConverterTest, ConvertCertConfigAllFields) {
    config::CertificatesConfig src;
    src.cert_path = "/path/to/rsa_cert.pem";
    src.key_path = "/path/to/rsa_key.pem";
    src.ca_path = "/path/to/ca.pem";
    src.sm2_cert_path = "/path/to/sm2_enc_cert.pem";
    src.sm2_key_path = "/path/to/sm2_enc_key.pem";
    src.sm2_sign_cert_path = "/path/to/sm2_sign_cert.pem";
    src.sm2_sign_key_path = "/path/to/sm2_sign_key.pem";

    CertConfig dst;
    ConfigConverter::convert_cert_config(src, dst);

    EXPECT_TRUE(dst.use_gmssl);
    EXPECT_EQ(dst.cert_path, "/path/to/rsa_cert.pem");
    EXPECT_EQ(dst.key_path, "/path/to/rsa_key.pem");
    EXPECT_EQ(dst.ca_path, "/path/to/ca.pem");
    EXPECT_EQ(dst.sm2_cert_path, "/path/to/sm2_enc_cert.pem");
    EXPECT_EQ(dst.sm2_key_path, "/path/to/sm2_enc_key.pem");
    EXPECT_EQ(dst.sm2_sign_cert_path, "/path/to/sm2_sign_cert.pem");
    EXPECT_EQ(dst.sm2_sign_key_path, "/path/to/sm2_sign_key.pem");
}

TEST_F(ConfigConverterTest, ConvertTlsConfigHttp1Only) {
    config::Config config;
    config::Http2Config http2_config;
    http2_config.enabled = false;
    config.set_http2(http2_config);

    TlsConfig dst;
    ConfigConverter::convert_tls_config(config, dst);

    EXPECT_EQ(dst.alpn_protocols, ALPN_HTTP11);
    EXPECT_EQ(dst.enable_tls_1_3, DEFAULT_ENABLE_TLS_1_3);
    EXPECT_EQ(dst.enable_tls_1_2, DEFAULT_ENABLE_TLS_1_2);
}

TEST_F(ConfigConverterTest, ConvertTlsConfigHttp2Enabled) {
    config::Config config;
    config::Http2Config http2_config;
    http2_config.enabled = true;
    config.set_http2(http2_config);

    TlsConfig dst;
    ConfigConverter::convert_tls_config(config, dst);

    EXPECT_EQ(dst.alpn_protocols, ALPN_HTTP2_HTTP11);
}

TEST_F(ConfigConverterTest, ConvertTlsConfigCipherSuites) {
    config::Config config;
    config::CertificatesConfig cert_config;
    cert_config.cipher_suite = "ECDHE-ECDSA-AES256-GCM-SHA384";
    config.set_certificates(cert_config);

    TlsConfig dst;
    ConfigConverter::convert_tls_config(config, dst);

    EXPECT_EQ(dst.cipher_suites, "ECDHE-ECDSA-AES256-GCM-SHA384");
}

// ==================== ProtocolHandlerFactory测试 ====================

class ProtocolHandlerFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ProtocolHandlerFactoryTest, CreateWithHttp2Disabled) {
    config::Config config;
    config::Http2Config http2_config;
    http2_config.enabled = false;
    config.set_http2(http2_config);

    auto handler = ProtocolHandlerFactory::create(config);
    ASSERT_NE(handler, nullptr);
    EXPECT_EQ(handler->get_protocol_type(), ProtocolType::HTTP_1_1);
}

// TODO: 当HTTP/2完整实现后，此测试用例需要更新为期望返回HTTP_2类型
TEST_F(ProtocolHandlerFactoryTest, CreateWithHttp2Enabled_ReturnsHttp1HandlerForNow) {
    config::Config config;
    config::Http2Config http2_config;
    http2_config.enabled = true;
    config.set_http2(http2_config);

    auto handler = ProtocolHandlerFactory::create(config);
    ASSERT_NE(handler, nullptr);
    // 当前版本简化实现：即使HTTP/2启用也返回Http1Handler
    EXPECT_EQ(handler->get_protocol_type(), ProtocolType::HTTP_1_1);
}

TEST_F(ProtocolHandlerFactoryTest, CreateWithDefaultConfig) {
    config::Config config;

    auto handler = ProtocolHandlerFactory::create(config);
    ASSERT_NE(handler, nullptr);
    EXPECT_EQ(handler->get_protocol_type(), ProtocolType::HTTP_1_1);
}

} // namespace test
} // namespace protocol
} // namespace https_server_sim

// 文件结束
