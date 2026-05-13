#ifndef HID_INTERFACE_H
#define HID_INTERFACE_H

#include "core/UsbDevice.h"
#include <vector>
#include <cstdint>

// HID接口通信类
// 负责HID接口的初始化、端点查找和数据收发
class HidInterface {
public:
    // 构造函数
    // 参数: device - USB设备引用, interface_num - 接口编号
    HidInterface(UsbDevice& device, int interface_num);
    
    // 析构函数：确保资源释放
    ~HidInterface();

    // 初始化HID接口（声明接口并查找端点）
    // 返回: true成功, false失败
    bool init();
    
    // 清理HID接口（释放接口）
    void cleanup();

    // 发送HID报告（中断传输）
    // 参数: data - 数据向量, timeout - 超时时间(ms)
    // 返回: 实际发送字节数或错误码
    int sendReport(const std::vector<uint8_t>& data, unsigned int timeout = 1000);
    
    // 接收HID报告（中断传输）
    // 参数: data - 输出数据向量, timeout - 超时时间(ms)
    // 返回: 实际接收字节数或错误码
    int receiveReport(std::vector<uint8_t>& data, unsigned int timeout = 1000);

    // 发送控制报告（控制传输）
    // 参数: request_type - 请求类型, request - 请求码
    //      value - 值, index - 索引
    //      data - 数据向量, timeout - 超时时间(ms)
    // 返回: 实际传输字节数或错误码
    int sendControlReport(uint8_t request_type, uint8_t request,
                          uint16_t value, uint16_t index,
                          const std::vector<uint8_t>& data,
                          unsigned int timeout = 1000);

    // 查找HID端点（遍历配置描述符）
    // 返回: 0成功, -1失败
    int findEndpoints();
    
    // 获取输入端点地址
    // 返回: 输入端点地址
    uint8_t getInEndpoint() const;
    
    // 获取输出端点地址
    // 返回: 输出端点地址
    uint8_t getOutEndpoint() const;

private:
    UsbDevice& device_;         // USB设备引用
    int interface_num_;         // 接口编号
    uint8_t in_endpoint_;       // 输入端点地址
    uint8_t out_endpoint_;      // 输出端点地址
    uint16_t in_max_packet_;    // 输入端点最大包大小
    uint16_t out_max_packet_;   // 输出端点最大包大小
    bool initialized_;          // 初始化状态标志
};

#endif
