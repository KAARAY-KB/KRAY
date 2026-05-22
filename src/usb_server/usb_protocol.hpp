// ============================================================================
// usb_protocol.hpp - USB Server 二进制通信协议（头文件）
//
// 协议格式：
//   [2B msg_type][2B payload_len][NB payload]
//
// 字节序：小端（Little-Endian）
//
// 消息类型：
//   0x0001 ~ 0x00FF 请求（客户端 → 服务端）
//   0x0100 ~ 0x01FF 响应（服务端 → 客户端）
//   0x0200 ~ 0x02FF 通知（服务端 → 客户端，主动推送）
// ============================================================================

#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace usb_srv {

// ============================================================================
// 消息类型常量
// ============================================================================

// --- 请求类型（客户端 → 服务端）---
constexpr uint16_t REQ_REFRESH_DEVICES    = 0x0001;  // 刷新设备列表
constexpr uint16_t REQ_LIST_DEVICES       = 0x0002;  // 获取设备简要列表
constexpr uint16_t REQ_LIST_DEVICES_DETAIL= 0x0003;  // 获取设备详细列表
constexpr uint16_t REQ_DEVICE_COUNT       = 0x0004;  // 获取设备数量
constexpr uint16_t REQ_DEVICE_DETAIL      = 0x0005;  // 获取指定设备详情
constexpr uint16_t REQ_FIND_BY_VID_PID    = 0x0006;  // 按 VID/PID 查找设备

constexpr uint16_t REQ_OPEN_HID           = 0x0010;  // 打开 HID 设备（按索引）
constexpr uint16_t REQ_OPEN_HID_VID_PID   = 0x0011;  // 按 VID/PID 打开 HID
constexpr uint16_t REQ_CLOSE_HID          = 0x0012;  // 关闭 HID 设备
constexpr uint16_t REQ_IS_HID_OPEN        = 0x0013;  // 查询 HID 是否打开
constexpr uint16_t REQ_HID_READ           = 0x0014;  // HID 读取
constexpr uint16_t REQ_HID_WRITE          = 0x0015;  // HID 写入
constexpr uint16_t REQ_HID_GET_FEATURE    = 0x0016;  // 获取 HID 特性报告
constexpr uint16_t REQ_HID_SEND_FEATURE   = 0x0017;  // 发送 HID 特性报告

constexpr uint16_t REQ_HOTPLUG_START      = 0x0020;  // 启动热插拔监听
constexpr uint16_t REQ_HOTPLUG_STOP       = 0x0021;  // 停止热插拔监听

constexpr uint16_t REQ_PING               = 0x00FF;  // 心跳检测

// --- 响应类型（服务端 → 客户端）---
constexpr uint16_t RSP_OK                 = 0x0100;  // 成功响应
constexpr uint16_t RSP_ERROR              = 0x0101;  // 错误响应
constexpr uint16_t RSP_PONG               = 0x01FF;  // 心跳回复

// --- 通知类型（服务端 → 客户端，主动推送）---
constexpr uint16_t NTF_HOTPLUG_EVENT      = 0x0200;  // 热插拔事件通知

// ============================================================================
// 协议头大小
// ============================================================================
constexpr size_t HEADER_SIZE = 4;  // 2B type + 2B length

// ============================================================================
// Message - 协议消息结构体
// ============================================================================
struct Message {
    uint16_t type = 0;              // 消息类型
    std::vector<uint8_t> payload;   // 消息载荷

    // 判断消息是否为空
    bool empty() const { return type == 0 && payload.empty(); }
};

// ============================================================================
// 协议编解码函数
// ============================================================================

// 将消息编码为字节流
// @param msg 消息结构体
// @return 编码后的字节流
std::vector<uint8_t> encode(const Message& msg);

// 从字节流解码消息（流式解析，支持粘包/拆包）
// @param in_buf   输入缓冲区（追加新数据后调用）
// @param out_msgs 解码成功的消息列表（输出）
void decode(std::vector<uint8_t>& in_buf, std::vector<Message>& out_msgs);

// ============================================================================
// 载荷辅助：序列化/反序列化基本类型（小端序）
// ============================================================================

// 向载荷追加 uint16_t（小端序）
void put_u16(std::vector<uint8_t>& buf, uint16_t val);

// 向载荷追加 uint32_t（小端序）
void put_u32(std::vector<uint8_t>& buf, uint32_t val);

// 向载荷追加字节序列
void put_bytes(std::vector<uint8_t>& buf, const uint8_t* data, size_t len);

// 向载荷追加字符串（4B 长度 + 内容）
void put_str(std::vector<uint8_t>& buf, const std::string& s);

// 从载荷偏移处读取 uint16_t
uint16_t get_u16(const std::vector<uint8_t>& buf, size_t offset);

// 从载荷偏移处读取 uint32_t
uint32_t get_u32(const std::vector<uint8_t>& buf, size_t offset);

// 从载荷偏移处读取字符串（4B 长度 + 内容）
std::string get_str(const std::vector<uint8_t>& buf, size_t offset);

} // namespace usb_srv
