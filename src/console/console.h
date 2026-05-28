// 本文件定义全局日志门面 Console：提供 Console::out() 兼容接口，
// 以及带级别、模块标签、频率限流的 LogStream，并集中管理 sink 注册/注销/分发。
#ifndef CONSOLE_H
#define CONSOLE_H

#include <iostream>
#include <sstream>
#include <mutex>
#include <vector>
#include <string>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include "console_sink.h"

/**
 * @brief 全局控制台输出管理器
 *
 * 提供两层接口：
 *   1) 兼容旧调用：Console::out() << "..." << std::endl;
 *      —— 不做级别/标签过滤，原样分发到所有 sink。
 *   2) 推荐使用：Console::info("USB") << "device opened";
 *      —— 自动附加时间戳/级别/模块标签，支持过滤与限流。
 *
 * 线程安全：sink 列表与过滤状态均由互斥锁保护。
 */
class Console {
public:
    /**
     * @brief 日志级别，按严重程度从低到高排列
     */
    enum class Level : int {
        Debug = 0, // 详细调试信息
        Info  = 1, // 常规运行信息
        Warn  = 2, // 警告，不影响功能
        Error = 3, // 错误，需要关注
    };

    /**
     * @brief 临时日志代理对象
     *
     * 在 LogStream 析构（一行表达式结束）时，将累积内容附加前缀分发到 sink。
     * 若级别低于全局阈值或标签被禁用，则不分发，<< 调用基本无开销。
     */
    class LogStream {
    public:
        // 由 Console::log 工厂构造；active=false 时所有 << 调用都被丢弃
        LogStream(Level lvl, const char* tag, bool active);
        ~LogStream();

        LogStream(const LogStream&) = delete;
        LogStream& operator=(const LogStream&) = delete;
        LogStream(LogStream&& other) noexcept;

        // 通用 << 重载：仅在 active 时把值写入内部缓冲
        template <typename T>
        LogStream& operator<<(const T& v) {
            if (m_active) m_buffer << v;
            return *this;
        }

        // 支持 std::endl / std::flush 等流操控符
        LogStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
            if (m_active) m_buffer << manip;
            return *this;
        }

    private:
        bool m_active;                  // 此条日志是否需要实际分发
        Level m_level;                  // 日志级别
        std::string m_tag;              // 模块标签，可为空
        std::ostringstream m_buffer;    // 行内容缓冲
    };

    /**
     * @brief 旧接口：返回原始 ostream，等价于无级别/标签的裸输出
     */
    static std::ostream& out();

    /**
     * @brief 新接口：构造带级别和标签的临时日志流
     * @param lvl 日志级别
     * @param tag 模块标签，nullptr 表示无标签
     * @return LogStream 临时对象，离开作用域时自动 flush
     */
    static LogStream log(Level lvl, const char* tag = nullptr);

    // 各级别便捷工厂
    static LogStream debug(const char* tag = nullptr);
    static LogStream info (const char* tag = nullptr);
    static LogStream warn (const char* tag = nullptr);
    static LogStream error(const char* tag = nullptr);

    /**
     * @brief 设置全局最低日志级别，低于此级别的 LogStream 会被静默丢弃
     */
    static void  setMinLevel(Level lvl);
    static Level getMinLevel();

    /**
     * @brief 模块标签过滤：默认全部启用，可显式禁用某些标签
     */
    static void setTagEnabled(const std::string& tag, bool enabled);
    static bool getTagEnabled(const std::string& tag);

    /**
     * @brief 高频日志限流工具
     * @param key 限流键，通常用 "模块-动作"，例如 "usb-read"
     * @param intervalMs 同一 key 两次返回 true 的最小时间间隔（毫秒）
     * @return true 表示当前调用允许输出日志；false 表示应跳过
     *
     * 使用示例：
     *   if (Console::shouldLog("usb-read", 500))
     *       Console::debug("USB") << "bulk read " << n << " bytes";
     */
    static bool shouldLog(const std::string& key, int intervalMs);

    /**
     * @brief 注册一个输出 Sink；调用者负责保证 Sink 生命周期
     * @param sink 指向 ConsoleSink 实现的原始指针
     * 
     * 注册的 Sink 会接收所有通过 Console::out() 输出的内容。
     * 可注册多个 Sink，实现同时输出到终端、窗口、文件等。
     * 
     * 注意：Sink 的生命周期由调用者管理，Console 不持有所有权。
     */
    static void registerSink(ConsoleSink* sink);

    /**
     * @brief 注销指定 Sink；返回后保证不会再有调度发往该 sink
     * @param sink 要注销的 Sink 指针
     */
    static void unregisterSink(ConsoleSink* sink);

    /**
     * @brief 清空所有 Sink（一般仅在退出时使用）
     */
    static void clearSinks();

    /**
     * @brief 输出启动 banner，标志日志通道已就绪
     */
    static void abort_msg();

    /**
     * @brief 把一行已格式化文本分发到所有 sink（供 LogStream 析构调用）
     */
    static void dispatch(const std::string& line);

private:
    // 兼容旧接口的流缓冲：按行聚合并分发到 sink
    class ConsoleStreamBuf : public std::streambuf {
    protected:
        // 重写 overflow，处理单个字符输出
        int_type overflow(int_type ch) override;
        // 重写 sync，处理缓冲区同步（如遇到 endl 时）
        int sync() override;
    private:
        std::string m_buffer; // 行缓冲，遇到 \n 或 flush 时分发
    };

    // 实际向所有 sink 写文本的内部函数（持锁，不再加锁）
    static void writeToSinks(const std::string& text);

    static ConsoleStreamBuf s_streamBuf;     // 全局流缓冲区
    static std::ostream     s_ostream;       // 全局输出流 ostream
    static std::vector<ConsoleSink*> s_sinks;// 已注册 sink 列表（非拥有）
    static std::mutex       s_mutex;         // sink 列表与分发互斥锁

    // 过滤与限流状态使用独立锁，避免与 s_mutex 嵌套
    static Level            s_minLevel;
    static std::unordered_set<std::string> s_disabledTags;
    static std::unordered_map<std::string, std::chrono::steady_clock::time_point> s_rateLimitMap;
    static std::mutex       s_filterMutex;
};

#endif // CONSOLE_H
