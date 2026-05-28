<!-- 本文件用于说明 src/utils 模块的错误码工具职责和在协议层中的使用方式。 -->

# utils 模块逻辑说明

## 模块职责

`src/utils` 当前主要提供基础错误码定义，供协议层等模块返回统一错误状态。

核心文件：

- `src/utils/ec.h`

## 依赖关系

```mermaid
flowchart LR
    protocol["protocol"] --> utils["utils / ec.h"]
```

## 错误码使用流程

```mermaid
flowchart TB
    A["业务模块检测错误"] --> B["使用 EC_SET(module, code) 组合错误码"]
    B --> C["返回 ECType_t"]
    C --> D["调用方判断是否等于 EC_NONE"]
    D --> E{"是否失败"}
    E -->|是| F["记录日志或中断流程"]
    E -->|否| G["继续执行"]
```

## 在 HPHPT 中的使用

```mermaid
flowchart TB
    A["hpt_decode"] --> B{"长度是否合法"}
    B -->|否| C["EC_MODULE_HPHPT + EC_PACK_LEN"]
    B -->|是| D{"SOF 是否正确"}
    D -->|否| E["EC_MODULE_HPHPT + EC_HEAD_SOF"]
    D -->|是| F{"CRC 是否正确"}
    F -->|否| G["EC_MODULE_HPHPT + CRC 错误码"]
    F -->|是| H["EC_NONE"]
```

## 当前状态

- 工具模块很轻量，主要支撑错误码。
- 协议层已经通过 `EC_SET(EC_MODULE_HPHPT, code)` 组合模块错误码。
- 错误码到可读字符串的转换能力暂未看到完整封装。

## 改进建议

1. 增加错误码到字符串的转换函数，便于日志和 UI 展示。
2. 为每个模块分配稳定错误码范围，避免后续冲突。
3. 在协议、USB 和 UI 层之间统一错误处理策略。
4. 如果错误码会跨设备或协议传输，应明确字节序和字段宽度。
