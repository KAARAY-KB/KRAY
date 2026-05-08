#ifndef CONSOLE_H
#define CONSOLE_H

#include <iostream>
#include <sstream>
#include <mutex>
#include <memory>
#include <vector>
#include "console_sink.h"

/**
 * @brief 全局控制台输出管理器
 * 
 * 提供类似 std::cout 的 C++ 流式输出接口，所有输出通过注册的 Sink 分发到不同平台。
 * 使用方式：Console::out() << "Hello" << " World" << std::endl;
 * 
 * 设计特点：
 * - 用户代码只依赖标准 C++ ostream 接口
 * - 输出通过 Sink 抽象层分发，可动态注册/注销
 * - 线程安全，支持多线程同时输出
 */
class Console {
public:
    /**
     * @brief 获取控制台输出流
     * @return std::ostream& 标准输出流引用
     * 
     * 使用示例：
     *   Console::out() << "Value: " << 42 << std::endl;
     */
    static std::ostream& out();

    /**
     * @brief 注册一个输出 Sink
     * @param sink 指向 ConsoleSink 实现的原始指针
     * 
     * 注册的 Sink 会接收所有通过 Console::out() 输出的内容。
     * 可注册多个 Sink，实现同时输出到终端、窗口、文件等。
     * 
     * 注意：Sink 的生命周期由调用者管理，Console 不持有所有权。
     */
    static void registerSink(ConsoleSink* sink);

    /**
     * @brief 注销指定的 Sink
     * @param sink 要注销的 Sink 指针
     */
    static void unregisterSink(ConsoleSink* sink);

    /**
     * @brief 清空所有已注册的 Sink
     */
    static void clearSinks();

private:
    // 自定义流缓冲区，将输出转发到所有注册的 Sink
    class ConsoleStreamBuf : public std::streambuf {
    protected:
        // 重写 overflow，处理单个字符输出
        int_type overflow(int_type ch) override;
        
        // 重写 sync，处理缓冲区同步（如遇到 endl 时）
        int sync() override;
        
    private:
        std::string m_buffer;  // 行缓冲区
    };

    static ConsoleStreamBuf s_streamBuf;  // 全局流缓冲区
    static std::ostream s_ostream;        // 全局输出流
    static std::vector<ConsoleSink*> s_sinks;  // 已注册的 Sink 列表（非拥有）
    static std::mutex s_mutex;            // 线程安全锁
};

#endif // CONSOLE_H
