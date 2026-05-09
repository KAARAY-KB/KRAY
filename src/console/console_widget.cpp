#include "console_widget.h"
#include <QScrollBar>
#ifdef __WIN32__
#include <windows.h>
#endif

// 构造函数：初始化控件属性和信号槽连接
ConsoleWidget::ConsoleWidget(QWidget* parent)
    : QTextEdit(parent)
    , m_sink(this)
    , m_maxLines(10000)
{
    // 设置为只读模式，防止用户编辑控制台输出
    setReadOnly(true);
    resize(600, 460);
    // 设置等宽字体，便于查看日志和对齐
    setFont(QFont("Maple Mono NF CN", 12));
    setWindowTitle("Console");

    // 设置控制台外观
    setStyleSheet(
        // 设置文本编辑器的外观
        "QTextEdit { "
            "color:rgb(230, 230, 230); "
            "background-color:rgb(50, 50, 50); "
            "selection-color:rgb(50, 50, 50); "
            "selection-background-color:rgb(230, 230, 230); "
        "}"
        //设置垂直滑块整体-背景颜色为透明、距离上边距57
        "QScrollBar:vertical { "
            "background: transparent; "
            "background-color:rgb(50, 50, 50);"
            "width:6px;"
        "}"
        //设置垂直滑块内部滚动条的样式-颜色为白色、圆角、宽度
        "QScrollBar::handle:vertical { "
            "background-color:rgb(100, 100, 100);"
            "border-radius:3px;"
            "width:6px;"
            "min-height:2px; "
        "}"
        //隐藏上下的箭头按钮
        "QScrollBar::add-line:vertical, "
        "QScrollBar::sub-line:vertical { "
            "height:0px; "
            "border: none;"
            "background: none;"
        "}"
    );
    
#ifdef __WIN32__
    // 设置窗口背景色为暗黑色，避免边框显示白色
    HWND hwnd = HWND(this->winId());
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)CreateSolidBrush(RGB(50, 50, 50)));
#endif

    // 连接信号槽：确保跨线程安全更新 UI
    // Qt::QueuedConnection 保证槽函数在接收者所在线程执行
    connect(this, &ConsoleWidget::textReady,
            this, &ConsoleWidget::appendText,
            Qt::QueuedConnection);
}
void ConsoleWidget::show_top(void)
{
    if (this->isMinimized())
    {
        this->showNormal();
    }
#ifdef __WIN32__
    SetWindowPos(HWND(this->winId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetWindowPos(HWND(this->winId()), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif
    this->show();
    this->activateWindow();
}


// 获取内部 Sink 指针，用于注册到 Console
ConsoleSink* ConsoleWidget::sink() const {
    return const_cast<WidgetSink*>(&m_sink);
}

// 设置最大行数限制
void ConsoleWidget::setMaxLines(int maxLines) {
    QMutexLocker locker(&m_mutex);
    m_maxLines = maxLines;
}

// 在 UI 线程追加文本
void ConsoleWidget::appendText(const QString& text) {
    QMutexLocker locker(&m_mutex);

    // 追加文本到末尾
    moveCursor(QTextCursor::End);
    insertPlainText(text);

    // 检查是否超过最大行数，超过则删除旧行
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::Start);
    int lineCount = document()->lineCount();
    while (lineCount > m_maxLines) {
        // 删除第一行
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.removeSelectedText();
        cursor.deleteChar(); // 删除换行符
        lineCount = document()->lineCount();
    }

    // 自动滚动到最新内容
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}
