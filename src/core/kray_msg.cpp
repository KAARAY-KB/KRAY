#include "kray.h"
#include "./ui_kray.h"
#include "console.h"
#include <QApplication>
#include <QFontDatabase>

static std::string _t = "    ";

void Kray::get_system_info(void *p_context)
{
    // 获取并输出系统信息
    std::string t = "";
    if (p_context != nullptr) {
        t = _t;
    }
    QString str = "";
    str.append(t + "系统信息:\n");

    //QSysInfo::productVersion();
    QString qt_version = QString::asprintf("    QT version: %d.%d.%d\n",QT_VERSION_MAJOR, QT_VERSION_MINOR, QT_VERSION_PATCH);
    str.append(QString(t.c_str()) + qt_version);
    str.append(QString(t.c_str()) + QString("    system name: "));

#if defined(__linux__)
    str.append(QString("Linux"));
#elif defined(__WIN32__)
    str.append(QString("Windows "));
    #if defined(Q_PROCESSOR_X86_64)
        str.append(QString("64-bit build (x64)"));
    #elif defined(Q_PROCESSOR_X86)
        str.append(QString("32-bit build (x86)"));
    #endif
#elif defined(__APPLE__)
    str.append(QString("MacOS"));
#endif
    str.append(QString("\n"));

    Console::out() << str.toStdString() << std::endl;
    //ui->textBrowser->setText(str);
}

void Kray::get_font(QFont font, void *p_context)
{
    std::string t = "";
    if (p_context != nullptr) {
        t = _t;
    }
    // 输出字体信息
    Console::out() << t << "字体信息:" << std::endl;
    Console::out() << t << "    字体族: " << font.family().toStdString() << std::endl;
    Console::out() << t << "    字体大小: " << font.pointSize() << " pt" << std::endl;
    Console::out() << t << "    字体大小(像素): " << font.pixelSize() << " px" << std::endl;
    Console::out() << t << "    字体粗细: " << font.weight() << std::endl;
    Console::out() << t << "    字体样式: " << (font.italic() ? "斜体" : "正常") << std::endl;
    Console::out() << t << "    是否加粗: " << (font.bold() ? "是" : "否") << std::endl;
}

void Kray::get_available_fonts(void *p_context)
{
    std::string t = "";
    if (p_context != nullptr) {
        t = _t;
    }
    // 获取系统可用字体列表
    QFontDatabase fontDatabase;
    QStringList fontFamilies = fontDatabase.families();
    Console::out() << t << "系统可用字体:" << std::endl;
    Console::out() << t << "    数量: " << fontFamilies.size() << std::endl;
    Console::out() << t << "    字体: " << std::endl;
    for (const QString &family : fontFamilies) {
        Console::out() << t << "        - " << family.toStdString() << std::endl;
    }
}

void Kray::get_widget_info(QWidget *widget, void *p_context)
{
    std::string t = "";
    if (p_context != nullptr) {
        t = _t;
    }
    // 输出窗口详细信息
    Console::out() << t << "窗口详细信息:" << std::endl;
    if (widget == nullptr) {
        Console::out() << t << "    窗口指针为空" << std::endl;
        return;
    }

    Console::out() << t << "    窗口标题: " << widget->windowTitle().toStdString() << std::endl;
    Console::out() << t << "    窗口类型: " << widget->metaObject()->className() << std::endl;
    Console::out() << t << "    窗口对象名: " << widget->objectName().toStdString() << std::endl;
    Console::out() << std::endl;
    // 子控件数量
    Console::out() << t << "    子控件数量: " << widget->children().size() << std::endl;
    // 窗口状态
    Console::out() << t << "    窗口状态:" << std::endl;
    Console::out() << t << "        是否可见: " << (widget->isVisible() ? "是" : "否") << std::endl;
    Console::out() << t << "        是否隐藏: " << (widget->isHidden() ? "是" : "否") << std::endl;
    Console::out() << t << "        是否启用: " << (widget->isEnabled() ? "是" : "否") << std::endl;
    Console::out() << t << "        是否激活: " << (widget->isActiveWindow() ? "是" : "否") << std::endl;
    Console::out() << t << "        是否最小化: " << (widget->isMinimized() ? "是" : "否") << std::endl;
    Console::out() << t << "        是否最大化: " << (widget->isMaximized() ? "是" : "否") << std::endl;
    Console::out() << t << "        是否全屏: " << (widget->isFullScreen() ? "是" : "否") << std::endl;
    Console::out() << std::endl;
    // 窗口几何信息
    Console::out() << t << "    几何信息:" << std::endl;
    Console::out() << t << "        位置: (" << widget->x() << ", " << widget->y() << ")" << std::endl;
    Console::out() << t << "        尺寸: " << widget->width() << " x " << widget->height() << std::endl;
    Console::out() << t << "        最小尺寸: " << widget->minimumWidth() << " x " << widget->minimumHeight() << std::endl;
    Console::out() << t << "        最大尺寸: " << widget->maximumWidth() << " x " << widget->maximumHeight() << std::endl;
    Console::out() << std::endl;
    // 字体信息
    get_font(widget->font(), &t);

}

void Kray::get_all_widgets_info(void *p_context)
{
    std::string t = "";
    if (p_context != nullptr) {
        t = _t;
    }
    int i = 0;
    QList<QWidget*> topLevelWidgets = QApplication::topLevelWidgets();
    Console::out() << t << "所有窗口信息(" << topLevelWidgets.size() << "个):" << std::endl;

    for (QWidget *child : topLevelWidgets) {
        Console::out() << t << "窗口序号: " << i++ << std::endl;
        get_widget_info(child, &t);
    }
}
