#ifndef CONSOLE_SINK_H
#define CONSOLE_SINK_H

#include <string>

/**
 * @brief 控制台输出 Sink 抽象接口
 * 
 * 所有具体的输出目标（终端、Qt窗口、文件、网络等）都需要实现此接口。
 * Console 类通过此接口将输出分发到各个注册的目标，实现平台解耦。
 * 
 * 设计原则：
 * - 接口最小化，只定义必须的输出方法
 * - 不依赖任何平台特定代码（无 Qt、无 Windows API 等）
 * - 具体平台实现继承此类并提供自己的 write 实现
 * 
 * 使用示例（以 Qt 窗口为例）：
 *   class QtConsoleSink : public ConsoleSink {
 *   public:
 *       void write(const std::string& text) override {
 *           // 将 text 发送到 Qt 文本控件
 *       }
 *   };
 */
class ConsoleSink {
public:
    virtual ~ConsoleSink() = default;

    /**
     * @brief 写入文本到输出目标
     * @param text 要输出的文本内容（UTF-8 编码）
     * 
     * 此方法会被 Console 类调用，传入完整的输出文本。
     * 实现者需要在此方法中将文本转换为目标平台格式并输出。
     * 
     * 注意：
     * - 此方法可能被多线程调用，实现需要保证线程安全
     * - 文本可能包含换行符，实现需要正确处理
     */
    virtual void write(const std::string& text) = 0;
};

#endif // CONSOLE_SINK_H
