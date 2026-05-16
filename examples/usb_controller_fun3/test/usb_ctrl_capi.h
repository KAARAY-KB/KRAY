// ============================================================================
// usb_ctrl_capi.h - USB 控制器 C 语言接口
//
// 功能说明：
//   为 UsbController C++ 类提供 C 语言接口，供 Python ctypes 调用。
//   使用不透明指针（opaque pointer）模式封装 C++ 对象。
//
// 内存管理规则：
//   - 所有返回 char* 的函数，调用者需调用 usb_ctrl_free_str() 释放
//   - 句柄通过 usb_ctrl_create() 创建，usb_ctrl_destroy() 销毁
// ============================================================================

#ifndef USB_CTRL_CAPI_H
#define USB_CTRL_CAPI_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 不透明句柄类型
typedef void* UsbCtrlHandle;

// ============================================================================
// 创建/销毁
// ============================================================================

// 创建 USB 控制器实例
// @return 控制器句柄
UsbCtrlHandle usb_ctrl_create(void);

// 销毁 USB 控制器实例
// @param h 控制器句柄
void usb_ctrl_destroy(UsbCtrlHandle h);

// ============================================================================
// 基础设置
// ============================================================================

// 设置调试级别
// @param h     控制器句柄
// @param level 调试级别 (0-4)
void usb_ctrl_set_debug(UsbCtrlHandle h, int level);

// 刷新设备列表
// @param h 控制器句柄
void usb_ctrl_refresh(UsbCtrlHandle h);

// 获取设备数量
// @param h 控制器句柄
// @return 设备数量
int usb_ctrl_device_count(UsbCtrlHandle h);

// ============================================================================
// 设备查询（返回 malloc 分配的字符串，需调用 usb_ctrl_free_str 释放）
// ============================================================================

// 列出所有 USB 设备
// @param h 控制器句柄
// @return 设备列表字符串
char* usb_ctrl_list_devices(UsbCtrlHandle h);

// 列出设备详细信息
// @param h 控制器句柄
// @return 设备详情字符串
char* usb_ctrl_list_devices_detail(UsbCtrlHandle h);

// 列出设备树形结构
// @param h 控制器句柄
// @return 设备树字符串
char* usb_ctrl_list_devices_tree(UsbCtrlHandle h);

// 列出 HID 设备
// @param h 控制器句柄
// @return HID 设备列表字符串
char* usb_ctrl_list_hid_devices(UsbCtrlHandle h);

// 获取指定设备详情
// @param h     控制器句柄
// @param index 设备索引
// @return 设备详情字符串
char* usb_ctrl_device_detail(UsbCtrlHandle h, int index);

// ============================================================================
// HID 设备操作
// ============================================================================

// 按索引打开 HID 设备
// @param h     控制器句柄
// @param index 设备索引
// @return 1=成功, 0=失败
int usb_ctrl_open_hid(UsbCtrlHandle h, int index);

// 按 VID:PID 打开 HID 设备
// @param h   控制器句柄
// @param vid 厂商 ID
// @param pid 产品 ID
// @return 1=成功, 0=失败
int usb_ctrl_open_hid_vid_pid(UsbCtrlHandle h, uint16_t vid, uint16_t pid);

// 关闭 HID 设备
// @param h 控制器句柄
void usb_ctrl_close_hid(UsbCtrlHandle h);

// 检查 HID 设备是否已打开
// @param h 控制器句柄
// @return 1=已打开, 0=未打开
int usb_ctrl_is_hid_open(UsbCtrlHandle h);

// 获取 HID 设备信息
// @param h 控制器句柄
// @return 设备信息字符串
char* usb_ctrl_hid_info(UsbCtrlHandle h);

// ============================================================================
// HID 数据传输（返回 JSON 格式字符串，需调用 usb_ctrl_free_str 释放）
// ============================================================================

// 同步 HID 读取
// @param h         控制器句柄
// @param len       读取长度
// @param timeout_ms 超时时间
// @return JSON 结果字符串
char* usb_ctrl_hid_read(UsbCtrlHandle h, int len, unsigned int timeout_ms);

// 读取最新 HID 报告
// @param h              控制器句柄
// @param len            读取长度
// @param drain_timeout_ms 排空超时
// @return JSON 结果字符串
char* usb_ctrl_hid_read_latest(UsbCtrlHandle h, int len, unsigned int drain_timeout_ms);

// 同步 HID 写入
// @param h         控制器句柄
// @param data      写入数据
// @param data_len  数据长度
// @param timeout_ms 超时时间
// @return JSON 结果字符串
char* usb_ctrl_hid_write(UsbCtrlHandle h, const uint8_t* data, int data_len, unsigned int timeout_ms);

// 获取 HID 特性报告
// @param h          控制器句柄
// @param report_id  报告 ID
// @param len        读取长度
// @param timeout_ms 超时时间
// @return JSON 结果字符串
char* usb_ctrl_hid_get_feature(UsbCtrlHandle h, uint8_t report_id, int len, unsigned int timeout_ms);

// 发送 HID 特性报告
// @param h          控制器句柄
// @param data       报告数据
// @param data_len   数据长度
// @param timeout_ms 超时时间
// @return JSON 结果字符串
char* usb_ctrl_hid_send_feature(UsbCtrlHandle h, const uint8_t* data, int data_len, unsigned int timeout_ms);

// ============================================================================
// 批量传输
// ============================================================================

// 批量读取
// @param h         控制器句柄
// @param endpoint  端点地址
// @param len       读取长度
// @param timeout_ms 超时时间
// @return JSON 结果字符串
char* usb_ctrl_bulk_read(UsbCtrlHandle h, uint8_t endpoint, int len, unsigned int timeout_ms);

// 批量写入
// @param h         控制器句柄
// @param endpoint  端点地址
// @param data      写入数据
// @param data_len  数据长度
// @param timeout_ms 超时时间
// @return JSON 结果字符串
char* usb_ctrl_bulk_write(UsbCtrlHandle h, uint8_t endpoint, const uint8_t* data, int data_len, unsigned int timeout_ms);

// ============================================================================
// 异步传输
// ============================================================================

// 启动异步引擎
// @param h 控制器句柄
void usb_ctrl_async_start(UsbCtrlHandle h);

// 停止异步引擎
// @param h 控制器句柄
void usb_ctrl_async_stop(UsbCtrlHandle h);

// 获取异步待处理数量
// @param h 控制器句柄
// @return 待处理数量
int usb_ctrl_async_pending(UsbCtrlHandle h);

// 启动连续异步读取（结果通过 usb_ctrl_async_poll 获取）
// @param h          控制器句柄
// @param len        读取长度
// @param timeout_ms 超时时间
void usb_ctrl_async_read_continuous(UsbCtrlHandle h, int len, unsigned int timeout_ms);

// 停止连续异步读取
// @param h 控制器句柄
void usb_ctrl_async_stop_continuous(UsbCtrlHandle h);

// 轮询异步读取结果（非阻塞）
// 返回一条结果字符串，无结果时返回 NULL
// @param h 控制器句柄
// @return JSON 结果字符串或 NULL
char* usb_ctrl_async_poll(UsbCtrlHandle h);

// ============================================================================
// 热插拔
// ============================================================================

// 检查平台是否支持热插拔
// @return 1=支持, 0=不支持
int usb_ctrl_is_hotplug_supported(void);

// 启动热插拔监听
// @param h 控制器句柄
// @return 1=成功, 0=失败
int usb_ctrl_hotplug_start(UsbCtrlHandle h);

// 停止热插拔监听
// @param h 控制器句柄
void usb_ctrl_hotplug_stop(UsbCtrlHandle h);

// 检查是否正在监听
// @param h 控制器句柄
// @return 1=监听中, 0=未监听
int usb_ctrl_is_hotplug_listening(UsbCtrlHandle h);

// 轮询热插拔事件（非阻塞）
// 返回一条事件字符串，无事件时返回 NULL
// @param h 控制器句柄
// @return 事件字符串或 NULL
char* usb_ctrl_hotplug_poll(UsbCtrlHandle h);

// ============================================================================
// 内存管理
// ============================================================================

// 释放 C API 返回的字符串
// @param s 字符串指针
void usb_ctrl_free_str(char* s);

#ifdef __cplusplus
}
#endif

#endif // USB_CTRL_CAPI_H
