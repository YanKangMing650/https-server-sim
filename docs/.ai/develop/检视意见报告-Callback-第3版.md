# 编码检视意见报告 - Callback模块

## 评分
- **总分**: 100分
- **扣分**: 1.7分
- **最终得分**: 98.3分

---

## 问题列表

| 级别 | 位置 | 问题描述 | 扣分 |
|-----|------|---------|------|
| 一般 | 详细设计文档:35 | 模块路径与实际不符。文档写"codes/core/source/callback/"，实际在"codes/core/callback/source/" | 1分 |
| 一般 | 详细设计文档:36 | 头文件路径与实际不符。文档写"codes/core/include/callback/"，实际在"codes/core/callback/include/callback/" | 0分（已合并到前一个问题） |
| 建议 | client_context.hpp | 文件冗余，仅包含client_context.h，无实际内容 | 0.1分 |
| 建议 | 详细设计文档:362-383 | 文档中register_callback示例代码使用callback_map_[port]赋值，实际代码使用emplace | 0.1分 |
| 建议 | 详细设计文档:382 | 文档示例返回0，实际代码返回CALLBACK_SUCCESS | 0.1分 |
| 建议 | 详细设计文档:491 | deregister_callback示例返回0，实际代码返回CALLBACK_SUCCESS | 0.1分 |
| 建议 | 详细设计文档:573 | invoke_parse_callback示例返回0，实际代码返回CALLBACK_SUCCESS | 0.1分 |
| 建议 | 详细设计文档:659 | invoke_reply_callback示例返回0，实际代码返回CALLBACK_SUCCESS | 0.1分 |
| 建议 | callback.cpp:1304 | C接口实现中全局使用using namespace https_server_sim，可能导致命名冲突 | 0.1分 |
| 建议 | callback.hpp:9 | 头文件包含方式不一致，使用"callback.h"应该为"callback/callback.h" | 0.1分 |
| 建议 | callback.cpp:29 | register_callback使用emplace，但文档示例用[]赋值，保持一致性更好 | 0.1分 |

---

**详细设计文档评分**: 98.3分
