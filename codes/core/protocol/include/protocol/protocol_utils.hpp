// =============================================================================
//  HTTPS Server Simulator - Protocol Module
//  文件: protocol_utils.hpp
//  描述: Protocol模块公共工具函数
//  版权: Copyright (c) 2026
// =============================================================================
#pragma once

#ifdef _WIN32
#include <string.h>
#else
#include <cstring>
#endif

namespace https_server_sim {
namespace protocol {

// 跨平台大小写不敏感字符串比较
inline int StrCaseCmp(const char* a, const char* b) {
#ifdef _WIN32
    return _stricmp(a, b);
#else
    return strcasecmp(a, b);
#endif
}

} // namespace protocol
} // namespace https_server_sim

// 文件结束
