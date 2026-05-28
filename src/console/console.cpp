// 本文件实现 Console 门面：包含旧版 streambuf、新版 LogStream、过滤与限流逻辑。
#include "console.h"
#include <algorithm>
#include <ctime>
#include <iomanip>

// === 静态成员初始化 ===
Console::ConsoleStreamBuf Console::s_streamBuf;
std::ostream Console::s_ostream(&s_streamBuf);
std::vector<ConsoleSink*> Console::s_sinks;
std::mutex Console::s_mutex;

// 默认放行所有级别，调试期间便于查看 Debug 日志
Console::Level Console::s_minLevel = Console::Level::Debug;
std::unordered_set<std::string> Console::s_disabledTags;
std::unordered_map<std::string, std::chrono::steady_clock::time_point> Console::s_rateLimitMap;
std::mutex Console::s_filterMutex;

// === 启动 banner ===
static const char * _console_logo = R"(

    __
    \ \           _____ _____ _____ _____ _____ __    _____
     \ \         |     |     |   | |   __|     |  |  |   __|
      > >        |   --|  |  | | | |__   |  |  |  |__|   __|
     / / _____   |_____|_____|_|___|_____|_____|_____|_____|
    /_/ |_____|

)";

// 把 Level 枚举映射为短文本，用于日志前缀
static const char* level_name(Console::Level lvl) {
    switch (lvl) {
        case Console::Level::Debug: return "DEBUG"; // 调试
        case Console::Level::Info:  return "INFO";  // 信息
        case Console::Level::Warn:  return "WARN";  // 警告
        case Console::Level::Error: return "ERROR"; // 错误
    }
    return "INFO"; // 兜底
}

// 取当前本地时间戳，格式 HH:MM:SS.mmm（避免依赖 Qt，便于纯 C++ 测试）
static std::string now_timestamp() {
    using clock = std::chrono::system_clock;
    auto tp  = clock::now();                                    // 当前时间点
    std::time_t t = clock::to_time_t(tp);                       // 转 time_t
    std::tm tm{};                                               // 拆分到本地时间
#ifdef _WIN32
    localtime_s(&tm, &t);                                       // Windows 安全版本
#else
    localtime_r(&t, &tm);                                       // POSIX 线程安全版本
#endif
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  tp.time_since_epoch()) % 1000;                // 毫秒部分
    std::ostringstream ss;
    ss << std::setfill('0')
       << std::setw(2) << tm.tm_hour << ':'
       << std::setw(2) << tm.tm_min  << ':'
       << std::setw(2) << tm.tm_sec  << '.'
       << std::setw(3) << ms.count();
    return ss.str();
}

// === LogStream 实现 ===
// 构造：仅记录元数据，是否真正输出取决于 active
Console::LogStream::LogStream(Level lvl, const char* tag, bool active)
    : m_active(active)
    , m_level(lvl)
    , m_tag(tag ? tag : "") {}

// 移动：移交缓冲并把源对象标记为非激活，避免重复分发
Console::LogStream::LogStream(LogStream&& other) noexcept
    : m_active(other.m_active)
    , m_level(other.m_level)
    , m_tag(std::move(other.m_tag))
    , m_buffer(std::move(other.m_buffer))
{
    other.m_active = false;
}

// 析构：把缓冲内容打包成一行并分发；不活跃则什么都不做
Console::LogStream::~LogStream() {
    if (!m_active) return;
    std::string body = m_buffer.str();                          // 取出正文
    if (body.empty()) return;                                   // 空日志跳过，减少噪声
    std::ostringstream line;
    line << '[' << now_timestamp() << ']'                       // 时间戳前缀
         << '[' << level_name(m_level) << ']';                  // 级别前缀
    if (!m_tag.empty()) {
        line << '[' << m_tag << ']';                            // 模块标签前缀
    }
    line << ' ' << body;                                        // 正文
    if (line.str().empty() || line.str().back() != '\n') {
        line << '\n';                                           // 保证以换行结尾
    }
    Console::dispatch(line.str());
}

// === LogStream 工厂 ===
// 按级别和标签决定 active 标记
Console::LogStream Console::log(Level lvl, const char* tag) {
    bool active = (static_cast<int>(lvl) >= static_cast<int>(minLevel())); // 级别过滤
    if (active && tag && !isTagEnabled(tag)) {                              // 标签过滤
        active = false;
    }
    return LogStream(lvl, tag, active);
}
Console::LogStream Console::debug(const char* tag) { return log(Level::Debug, tag); }
Console::LogStream Console::info (const char* tag) { return log(Level::Info , tag); }
Console::LogStream Console::warn (const char* tag) { return log(Level::Warn , tag); }
Console::LogStream Console::error(const char* tag) { return log(Level::Error, tag); }

// === 过滤状态 ===
void Console::setMinLevel(Level lvl) {
    std::lock_guard<std::mutex> lock(s_filterMutex);
    s_minLevel = lvl;
}
Console::Level Console::minLevel() {
    std::lock_guard<std::mutex> lock(s_filterMutex);
    return s_minLevel;
}
void Console::setTagEnabled(const std::string& tag, bool enabled) {
    std::lock_guard<std::mutex> lock(s_filterMutex);
    if (enabled) s_disabledTags.erase(tag);                     // 启用：从黑名单移除
    else         s_disabledTags.insert(tag);                    // 禁用：加入黑名单
}
bool Console::isTagEnabled(const std::string& tag) {
    std::lock_guard<std::mutex> lock(s_filterMutex);
    return s_disabledTags.find(tag) == s_disabledTags.end();    // 不在黑名单即启用
}

// === 限流 ===
// 简单的"按 key 周期允许一次"，无需复杂 token bucket
bool Console::shouldLog(const std::string& key, int intervalMs) {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(s_filterMutex);
    auto it = s_rateLimitMap.find(key);
    if (it == s_rateLimitMap.end()) {                           // 首次出现，立即允许
        s_rateLimitMap[key] = now;
        return true;
    }
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - it->second).count();
    if (diff >= intervalMs) {                                   // 已超过限流窗口
        it->second = now;
        return true;
    }
    return false;                                               // 仍在限流窗口内
}

// === 分发 ===
// 给 LogStream 析构与 sink 注销共用的内部函数：持锁遍历 sink
void Console::writeToSinks(const std::string& text) {
    for (const auto& sink : s_sinks) {
        sink->write(text);
    }
}

void Console::dispatch(const std::string& line) {
    std::lock_guard<std::mutex> lock(s_mutex);
    writeToSinks(line);
}

// === sink 管理 ===
std::ostream& Console::out() {
    return s_ostream;
}

void Console::registerSink(ConsoleSink* sink) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_sinks.push_back(sink);
}

void Console::unregisterSink(ConsoleSink* sink) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_sinks.erase(
        std::remove(s_sinks.begin(), s_sinks.end(), sink),
        s_sinks.end()
    );
}

void Console::clearSinks() {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_sinks.clear();
}

void Console::abort_msg() {
    std::lock_guard<std::mutex> lock(s_mutex);
    for (const auto& sink : s_sinks) {
        sink->write(_console_logo);
        sink->write("KRAY Console log output\n");
        sink->write("控制台日志输出\n\n");
    }
}

// === 旧版 streambuf 实现（保持原行为不变） ===
std::streambuf::int_type Console::ConsoleStreamBuf::overflow(int_type ch) {
    if (ch == traits_type::eof()) {
        return ch;
    }
    m_buffer += static_cast<char>(ch);                          // 追加单字符
    if (ch == '\n') {                                           // 行尾：分发整行
        std::lock_guard<std::mutex> lock(Console::s_mutex);
        Console::writeToSinks(m_buffer);
        m_buffer.clear();
    }
    return ch;
}

int Console::ConsoleStreamBuf::sync() {
    if (!m_buffer.empty()) {                                    // flush 时把剩余内容分发出去
        std::lock_guard<std::mutex> lock(Console::s_mutex);
        Console::writeToSinks(m_buffer);
        m_buffer.clear();
    }
    return 0;
}
