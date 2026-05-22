// ============================================================================
// usb_protocol.cpp - USB Server 二进制通信协议（实现文件）
//
// 实现协议消息的编解码和载荷辅助函数
// ============================================================================

#include "usb_protocol.hpp"
#include <cstring>
#include <algorithm>

namespace usb_srv {

// ============================================================================
// 编码：将 Message 转换为字节流
// 格式：[2B type LE][2B payload_len LE][NB payload]
// ============================================================================
std::vector<uint8_t> encode(const Message& msg) {
    // 预分配空间：头部4字节 + 载荷
    std::vector<uint8_t> buf;
    buf.reserve(HEADER_SIZE + msg.payload.size());

    // 写入消息类型（小端序）
    buf.push_back(static_cast<uint8_t>(msg.type & 0xFF));        // 低字节
    buf.push_back(static_cast<uint8_t>((msg.type >> 8) & 0xFF)); // 高字节

    // 写入载荷长度（小端序）
    uint16_t len = static_cast<uint16_t>(msg.payload.size());
    buf.push_back(static_cast<uint8_t>(len & 0xFF));        // 低字节
    buf.push_back(static_cast<uint8_t>((len >> 8) & 0xFF)); // 高字节

    // 写入载荷数据
    buf.insert(buf.end(), msg.payload.begin(), msg.payload.end());

    return buf;
}

// ============================================================================
// 解码：从字节流中提取完整消息（流式解析）
//
// 处理粘包：一次可能包含多个完整消息
// 处理拆包：一个消息可能分多次到达
// ============================================================================
void decode(std::vector<uint8_t>& in_buf, std::vector<Message>& out_msgs) {
    size_t pos = 0; // 当前解析位置

    while (pos + HEADER_SIZE <= in_buf.size()) {
        // 读取消息类型（小端序）
        uint16_t type = static_cast<uint16_t>(in_buf[pos])
                      | (static_cast<uint16_t>(in_buf[pos + 1]) << 8);

        // 读取载荷长度（小端序）
        uint16_t len = static_cast<uint16_t>(in_buf[pos + 2])
                     | (static_cast<uint16_t>(in_buf[pos + 3]) << 8);

        // 检查数据是否完整（头部 + 载荷）
        if (pos + HEADER_SIZE + len > in_buf.size()) {
            break; // 数据不完整，等待更多数据
        }

        // 构造消息
        Message msg;
        msg.type = type;
        if (len > 0) {
            msg.payload.assign(
                in_buf.begin() + pos + HEADER_SIZE,
                in_buf.begin() + pos + HEADER_SIZE + len
            );
        }
        out_msgs.push_back(std::move(msg));

        // 移动到下一条消息
        pos += HEADER_SIZE + len;
    }

    // 移除已解析的数据，保留未完成的部分
    if (pos > 0) {
        in_buf.erase(in_buf.begin(), in_buf.begin() + pos);
    }
}

// ============================================================================
// 载荷辅助函数
// ============================================================================

// 向载荷追加 uint16_t（小端序）
void put_u16(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
}

// 向载荷追加 uint32_t（小端序）
void put_u32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
}

// 向载荷追加字节序列
void put_bytes(std::vector<uint8_t>& buf, const uint8_t* data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}

// 向载荷追加字符串（4B 长度 + 内容，无\0结尾）
void put_str(std::vector<uint8_t>& buf, const std::string& s) {
    put_u32(buf, static_cast<uint32_t>(s.size()));
    buf.insert(buf.end(), s.begin(), s.end());
}

// 从载荷偏移处读取 uint16_t
uint16_t get_u16(const std::vector<uint8_t>& buf, size_t offset) {
    if (offset + 2 > buf.size()) return 0;
    return static_cast<uint16_t>(buf[offset])
         | (static_cast<uint16_t>(buf[offset + 1]) << 8);
}

// 从载荷偏移处读取 uint32_t
uint32_t get_u32(const std::vector<uint8_t>& buf, size_t offset) {
    if (offset + 4 > buf.size()) return 0;
    return static_cast<uint32_t>(buf[offset])
         | (static_cast<uint32_t>(buf[offset + 1]) << 8)
         | (static_cast<uint32_t>(buf[offset + 2]) << 16)
         | (static_cast<uint32_t>(buf[offset + 3]) << 24);
}

// 从载荷偏移处读取字符串（4B 长度 + 内容）
std::string get_str(const std::vector<uint8_t>& buf, size_t offset) {
    if (offset + 4 > buf.size()) return "";
    uint32_t len = get_u32(buf, offset);
    offset += 4;
    if (offset + len > buf.size()) return "";
    return std::string(buf.begin() + offset, buf.begin() + offset + len);
}

} // namespace usb_srv
