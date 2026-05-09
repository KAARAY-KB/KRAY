#include "console.h"
#include <algorithm>

// 静态成员初始化
Console::ConsoleStreamBuf Console::s_streamBuf;
std::ostream Console::s_ostream(&s_streamBuf);
std::vector<ConsoleSink*> Console::s_sinks;
std::mutex Console::s_mutex;

static const char * _console_logo = R"(
                                               
    _____ _____ _____ _____ _____ __    _____ 
    |     |     |   | |   __|     |  |  |   __|
    |   --|  |  | | | |__   |  |  |  |__|   __|
    |_____|_____|_|___|_____|_____|_____|_____|
                                               
)";

// 获取控制台输出流
std::ostream& Console::out() {
    return s_ostream;
}

// 注册输出 Sink
void Console::registerSink(ConsoleSink* sink) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_sinks.push_back(sink);
}

// 注销指定 Sink
void Console::unregisterSink(ConsoleSink* sink) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_sinks.erase(
        std::remove(s_sinks.begin(), s_sinks.end(), sink),
        s_sinks.end()
    );
}

// 清空所有 Sink
void Console::clearSinks() {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_sinks.clear();
}

void Console::abort_msg() {
    std::lock_guard<std::mutex> lock(s_mutex);
    for (const auto& sink : s_sinks) {
        sink->write(_console_logo);
        sink->write("控制台日志输出\n");
        sink->write("KRAY Console log output\n");
    }
}

// 处理单个字符输出
std::streambuf::int_type Console::ConsoleStreamBuf::overflow(int_type ch) {
    // EOF 直接返回
    if (ch == traits_type::eof()) {
        return ch;
    }

    // 将字符追加到缓冲区
    m_buffer += static_cast<char>(ch);

    // 遇到换行符时，将完整行分发到所有 Sink
    if (ch == '\n') {
        std::lock_guard<std::mutex> lock(Console::s_mutex);
        for (const auto& sink : Console::s_sinks) {
            sink->write(m_buffer);
        }
        m_buffer.clear();
    }

    return ch;
}

// 处理缓冲区同步（如遇到 endl 时）
int Console::ConsoleStreamBuf::sync() {
    // 如果缓冲区有剩余内容，立即分发
    if (!m_buffer.empty()) {
        std::lock_guard<std::mutex> lock(Console::s_mutex);
        for (const auto& sink : Console::s_sinks) {
            sink->write(m_buffer);
        }
        m_buffer.clear();
    }
    return 0;
}
