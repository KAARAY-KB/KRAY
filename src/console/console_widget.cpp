#include "console_widget.h"
#include "console.h"
#include <QScrollBar>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QContextMenuEvent>
#include <QTextBlock>
#include <QWheelEvent>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#ifdef _WIN32
#include <windows.h>
#endif

// 创建控制台图标
static QIcon createConsoleIcon()
{
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent); // 填充为透明
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing); // 开启抗锯齿
    // 背景矩形
    painter.setBrush(QColor(50, 50, 50)); // 背景颜色为暗黑色
    painter.setPen(QPen(QColor(220, 220, 220), 2)); // 边框颜色为白色
    painter.drawRoundedRect(2, 2, 28, 28, 5, 5); // 绘制圆角矩形，圆角半径为5
    // 终端提示符 >_
    painter.setPen(QColor(255, 255, 255)); // 文本颜色为白色
    painter.setFont(QFont("Maple Mono NF CN", 10, QFont::Bold)); // 字体为Maple Mono NF CN，大小10，加粗
    painter.drawText(6, 20, ">_"); // 绘制文本">_"

    return QIcon(pixmap);
}

// 构造函数：初始化控件属性和信号槽连接
ConsoleWidget::ConsoleWidget(QWidget* parent)
    : QTextEdit(parent)
    , m_sink(this)
    , m_maxLines(10000)
    , m_newLogCount(0)
    , m_newLogHint(nullptr)
    , m_highlightStartBlock(0)
    , m_fadeTimer(nullptr)
{
    // 设置为只读模式，防止用户编辑控制台输出
    setReadOnly(true);
//    setAttribute(Qt::WA_QuitOnClose, false); // 关闭窗口时不退出应用
    setWindowTitle("Console");
    setWindowIcon(createConsoleIcon());
    setFont(QFont("Maple Mono NF CN", 12));
    resize(660, 460);
    move(0, 0);

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
    
#ifdef _WIN32
    // 设置窗口背景色为暗黑色，避免边框显示白色
    HWND hwnd = HWND(this->winId());
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)CreateSolidBrush(RGB(50, 50, 50)));
#endif

    // 新数据提示标签
    m_newLogHint = new QLabel(this);
    // 设置标签右下角对齐
    m_newLogHint->setAlignment(Qt::AlignBottom | Qt::AlignRight);
    m_newLogHint->setFixedHeight(24);
    m_newLogHint->setCursor(Qt::PointingHandCursor);
    m_newLogHint->setStyleSheet(
        "QLabel { "
            "color: #fff; "
            "background-color: rgba(241, 190, 224, 0.5); "
            "border-radius: 4px; "
            "padding: 2px 10px; "
            "font-size: 12px; "
        "}"
    );
    m_newLogHint->hide();
    m_newLogHint->installEventFilter(this);

    // 高亮渐隐定时器
    m_fadeTimer = new QTimer(this);

    m_fadeTimer->setInterval(66); // ~60fps
    connect(m_fadeTimer, &QTimer::timeout, this, &ConsoleWidget::updateHighlightFade);

    // 连接信号槽：确保跨线程安全更新 UI
    // Qt::QueuedConnection 保证槽函数在接收者所在线程执行
    connect(this, &ConsoleWidget::textReady,
            this, &ConsoleWidget::appendText,
            Qt::QueuedConnection);
}

// 析构：先从 Console 注销 sink，确保后续不再有写操作发到这个 widget
ConsoleWidget::~ConsoleWidget()
{
    Console::unregisterSink(&m_sink);
}

void ConsoleWidget::show_top(void)
{
    if (this->isMinimized())
        this->showNormal();
#ifdef _WIN32
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

// 判断滚动条是否在底部（容差 5 像素）
bool ConsoleWidget::isAtBottom() const {
    QScrollBar* sb = verticalScrollBar();
    return sb->value() >= sb->maximum() - 5;
}

// 更新新数据提示
void ConsoleWidget::updateNewLogHint() {
    if (m_newLogCount > 0 && !isAtBottom()) {
        m_newLogHint->setText(QString("↓ %1 条新日志 ↓").arg(m_newLogCount));
        // 设置为粗体
        QFont font = m_newLogHint->font();
        font.setBold(true);
        m_newLogHint->setFont(font);
        
        m_newLogHint->adjustSize();
        // 放在右下角
        int x = width() - m_newLogHint->width() - verticalScrollBar()->width() - 10;
        int y = height() - m_newLogHint->height() - 10;
        m_newLogHint->move(x, y);
        m_newLogHint->raise();
        m_newLogHint->show();
    } else {
        m_newLogCount = 0;
        m_newLogHint->hide();
    }
}

// 开始新日志高亮渐隐动画
void ConsoleWidget::startNewLogHighlight() {
    m_fadeElapsed.start();
    m_fadeTimer->start();
    updateHighlightFade();
}

// 更新高亮渐隐
void ConsoleWidget::updateHighlightFade() {
    qint64 elapsed = m_fadeElapsed.elapsed();
    if (elapsed >= FADE_DURATION) {
        m_fadeTimer->stop();
        setExtraSelections(QList<QTextEdit::ExtraSelection>());
        return;
    }

    // 线性衰减 alpha：从 80 到 0
    double progress = static_cast<double>(elapsed) / FADE_DURATION;
    int alpha = static_cast<int>(80 * (1.0 - progress));
    QColor hlColor(247, 163, 219, alpha);

    QList<QTextEdit::ExtraSelection> selections;
    QTextDocument* doc = document();
    int totalBlocks = doc->blockCount();

    for (int i = m_highlightStartBlock; i < totalBlocks; ++i) {
        QTextBlock block = doc->findBlockByNumber(i);
        if (!block.isValid()) continue;

        QTextCursor cursor(block);
        cursor.select(QTextCursor::LineUnderCursor);

        QTextEdit::ExtraSelection sel;
        sel.cursor = cursor;
        sel.format.setBackground(hlColor);
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        selections.append(sel);
    }

    setExtraSelections(selections);
}

// 滚动事件：用户滚到底部时隐藏提示
void ConsoleWidget::scrollContentsBy(int dx, int dy) {
    QTextEdit::scrollContentsBy(dx, dy);
    if (isAtBottom()) {
        m_newLogCount = 0;
        m_newLogHint->hide();
    }
}

// resize 事件：调整提示标签位置
void ConsoleWidget::resizeEvent(QResizeEvent* event) {
    QTextEdit::resizeEvent(event);
    updateNewLogHint();
}

// 事件过滤器：点击提示标签跳到底部并高亮新日志
bool ConsoleWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_newLogHint && event->type() == QEvent::MouseButtonPress) {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
        m_newLogCount = 0;
        m_newLogHint->hide();
        startNewLogHighlight();
        return true;
    }
    return QTextEdit::eventFilter(watched, event);
}

// Ctrl+滚轮缩放字体
void ConsoleWidget::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        int delta = event->angleDelta().y();
        QFont f = font();
        int newSize = f.pointSize() + (delta > 0 ? 1 : -1);
        if (newSize >= 5 && newSize <= 50) { // 限制字体大小在 5-50 点之间
            f.setPointSize(newSize);
            setFont(f);
        }
        event->accept();
    } else {
        QTextEdit::wheelEvent(event);
    }
}

// 右键菜单
void ConsoleWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { "
            "color: rgb(230, 230, 230); "
            "background-color: rgb(50, 50, 50); "
            "border: 1px solid rgb(80, 80, 80); "
            "padding: 4px; "
        "}"
        "QMenu::item:selected { "
            "color:rgb(50, 50, 50); "
            "background-color:rgb(230, 230, 230); "
        "}"
    );

    QAction* copyAct = menu.addAction("复制");
    QAction* selAllAct = menu.addAction("全选");
    menu.addSeparator();
    QAction* clearAct = menu.addAction("清空日志");
    QAction* scrollAct = menu.addAction("滚动到底部");
    menu.addSeparator();
    QAction* exportAct = menu.addAction("导出日志…");

    QAction* chosen = menu.exec(event->globalPos());
    if (chosen == copyAct) {
        if (textCursor().hasSelection()) copy();
    } else if (chosen == selAllAct) {
        selectAll();
    } else if (chosen == clearAct) {
        clear();
        m_newLogCount = 0;
        m_newLogHint->hide();
    } else if (chosen == scrollAct) {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    } else if (chosen == exportAct) {
        exportLog();
    }
}

// 把当前文本控件内容以 UTF-8 写入用户选定的文件
void ConsoleWidget::exportLog()
{
    QString defaultName = QString("console_%1.log").arg(
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));   // 以时间戳避免重名
    QString startPath = QDir::current().filePath(defaultName);      // 默认保存到当前目录
    QString path = QFileDialog::getSaveFileName(
        this, "导出日志", startPath,
        "日志文件 (*.log *.txt);;所有文件 (*)");
    if (path.isEmpty()) return;                                     // 用户取消

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {   // 二进制写入避免换行符被改写
        QMessageBox::warning(this, "导出失败",
            QString("无法写入文件:\n%1").arg(path));
        return;
    }
    QByteArray data = toPlainText().toUtf8();                       // 当前可见的日志文本
    qint64 written = file.write(data);
    file.close();
    if (written != data.size()) {                                   // 实际写入字节数不一致视为失败
        QMessageBox::warning(this, "导出失败",
            QString("写入未完成:\n%1").arg(path));
        return;
    }
    Console::info("Console") << "日志已导出到 " << path.toStdString();
}

// 在 UI 线程追加文本
void ConsoleWidget::appendText(const QString& text) {
    QMutexLocker locker(&m_mutex);

    bool atBottom = isAtBottom();
    int savedPos = verticalScrollBar()->value();

    // 通过 QTextDocument 插入文本，不移动 widget 的光标
    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text);

    // 检查是否超过最大行数
    cursor.movePosition(QTextCursor::Start);
    int lineCount = document()->lineCount();
    while (lineCount > m_maxLines) {
        // 删除第一行
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.removeSelectedText();
        cursor.deleteChar(); // 删除换行符
        lineCount = document()->lineCount();
    }

    if (atBottom) {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
        m_newLogCount = 0;
        m_newLogHint->hide();
    } else {
        // 记录新日志起始 block（仅首次）
        if (m_newLogCount == 0) {
            m_highlightStartBlock = document()->blockCount();
        }
        verticalScrollBar()->setValue(savedPos);
        m_newLogCount++;
        updateNewLogHint();
    }
}
