#ifndef CONSOLE_WIDGET_H
#define CONSOLE_WIDGET_H

#include <QTextEdit>
#include <QMutex>
#include <QString>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QElapsedTimer>
#include "console_sink.h"

/**
 * @brief Qt 窗口控制台输出适配器
 * 
 * 将 Console 输出重定向到 Qt 文本控件，实现独立的控制台窗口。
 * 继承 ConsoleSink 接口，可注册到 Console 管理器。
 * 
 * 使用方式：
 *   auto widget = new ConsoleWidget();
 *   widget->show();
 *   Console::registerSink(widget->sink());
 * 
 * 设计特点：
 * - 线程安全：使用 QMutex 保护 UI 更新
 * - 跨线程：通过信号槽机制将非 UI 线程的输出转发到 UI 线程
 * - 自动滚动：新输出自动滚动到底部
 * - 颜色支持：可设置不同级别的输出颜色（info/warning/error）
 */
class ConsoleWidget : public QTextEdit {
    Q_OBJECT

public:
    explicit ConsoleWidget(QWidget* parent = nullptr);
    ~ConsoleWidget() = default;

    /**
     * @brief 获取关联的 Sink 指针
     * @return ConsoleSink* 指向内部 Sink 的指针
     * 
     * 使用此方法获取 Sink 并注册到 Console：
     *   Console::registerSink(widget->sink());
     */
    ConsoleSink* sink() const;

    /**
     * @brief 设置最大行数
     * @param maxLines 最大保留行数，超过后自动删除旧行
     * 
     * 用于防止长时间运行时文本控件占用过多内存。
     * 默认值为 10000 行。
     */
    void setMaxLines(int maxLines);

    // 显示窗口并置顶
    void show_top(void);

signals:
    /**
     * @brief 内部信号：用于跨线程文本追加
     * @param text 要追加的文本
     * 
     * 此信号连接到 appendText 槽，确保 UI 更新在正确的线程执行。
     */
    void textReady(const QString& text);

private slots:
    /**
     * @brief 在 UI 线程追加文本
     * @param text 要追加的文本
     */
    void appendText(const QString& text);

private:
    // 判断滚动条是否在底部
    bool isAtBottom() const;

    // 显示/隐藏新数据提示
    void updateNewLogHint();

    // 开始新日志高亮渐隐动画
    void startNewLogHighlight();

    // 更新高亮渐隐
    void updateHighlightFade();

    // 右键菜单事件
    void contextMenuEvent(QContextMenuEvent* event) override;

    // 滚动事件：用户手动滚到底部时隐藏提示
    void scrollContentsBy(int dx, int dy) override;

    // resize 事件：调整提示标签位置
    void resizeEvent(QResizeEvent* event) override;

    // 事件过滤器：处理提示标签点击
    bool eventFilter(QObject* watched, QEvent* event) override;

    // 滚轮事件：Ctrl+滚轮缩放字体
    void wheelEvent(QWheelEvent* event) override;

    /**
     * @brief 内部 Sink 实现类
     * 
     * 将 Console 的 std::string 输出转换为 Qt 信号，
     * 实现跨线程安全的 UI 更新。
     */
    class WidgetSink : public ConsoleSink {
    public:
        explicit WidgetSink(ConsoleWidget* widget) : m_widget(widget) {}

        /**
         * @brief 实现 ConsoleSink 接口
         * @param text 要输出的文本（UTF-8 编码）
         */
        void write(const std::string& text) override {
            // 将 std::string 转换为 QString 并通过信号发送
            // 信号槽机制会自动处理跨线程问题
            emit m_widget->textReady(QString::fromUtf8(text.c_str()));
        }
    private:
        ConsoleWidget* m_widget;  // 关联的窗口实例
    };

    WidgetSink m_sink;              // 内部 Sink 实例
    QMutex m_mutex;                 // 线程安全锁
    int m_maxLines;                 // 最大行数限制
    int m_newLogCount;              // 未读新日志计数
    QLabel* m_newLogHint;           // 新数据提示标签
    int m_highlightStartBlock;      // 新日志起始 block 编号
    QTimer* m_fadeTimer;            // 高亮渐隐定时器
    QElapsedTimer m_fadeElapsed;    // 高亮渐隐计时器
    static const int FADE_DURATION = 2000; // 渐隐持续时间 ms
};

#endif // CONSOLE_WIDGET_H
