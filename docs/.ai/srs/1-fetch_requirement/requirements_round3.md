# HTTPS Server 模拟器 - 第三轮需求确认

## 确认日期：2026-02-15

---

## 1. Token记录
- **确认**：暂不做token相关需求

---

## 2. C接口函数签名
- **确认**：按提议的签名实现
```c
// Client信息结构体
typedef struct {
    uint64_t connection_id;    // 连接ID
    const char* client_ip;     // client IP地址
    uint16_t client_port;      // client端口
    uint16_t server_port;      // server监听端口（用于策略选择）
    const char* token;         // token（预留，暂不使用）
    // ... 其他需要的字段
} ClientContext;

// 内容解析函数（异步）
uint32_t AsyncParseContent(const ClientContext* ctx, const uint8_t* in, uint32_t inLen);

// 内容解析函数（异步输出）
uint32_t AsyncReplyContent(const ClientContext* ctx, uint8_t* out, uint32_t* outLen);
```

---

## 3. HTTP header中的debug字段格式
- **确认**：使用XML格式包裹
```html
<debug-server-sim>
...
</debug-server-sim>
```
- 位置在HTTP header末尾

---

## 4. HTTP请求/响应基本格式

### HTTP方法
- **确认**：GET/POST/PUT/DELETE都有可能

### URL路径
- **确认**：不用管，只感知来源IP和端口

### HTTP状态码
- **确认**：按照业内标准返回（200、404等）

---

## 5. 时间转轮
- **确认**：类似libuv的event loop

---

## 6. 测试用的client模拟器

### 开发语言
- **确认**：全Python（不对外发布，独立进程部署）

### 证书支持
- **确认**：需要支持配置证书

### HTTP版本
- **确认**：需要支持HTTP/1.1和HTTP/2

### 报文内容
- **确认**：按照JSON Schema格式配置文件读取

---

## 需求确认完成！
所有关键需求点已确认完毕，可以开始编写完整的SRS文档。
