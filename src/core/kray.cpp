#include "kray.h"
#include "./ui_kray.h"
#include "console.h"
#include "console_widget.h"
#include <QApplication>

Kray::Kray(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Kray)
{
    ui->setupUi(this);
    setWindowTitle("KRAY");
    setWindowIcon(QIcon(":/images/pixel_pizza.png"));

    // 创建控制台窗口并注册为日志 sink（与 Kray 同生命周期）
    m_console_widget = new ConsoleWidget();
    Console::registerSink(m_console_widget->sink());
    Console::abort_msg();

    m_system_tray_icon = new MSystemTrayIcon(this, QIcon(":/images/pixel_parrot.png"), QApplication::font());
    m_system_tray_icon->showTrayIcon();

    // 连接系统托盘图标点击事件
    connect(m_system_tray_icon, &MSystemTrayIcon::actionTriggered, this, [this](const QString &actionName) {
        if (actionName == "show") {
            this->show();
            this->raise();
            this->activateWindow();
        } else if (actionName == "exit") {
            this->close();
        }
    });
    // 连接托盘"退出时关闭程序"勾选信号
    connect(m_system_tray_icon, &MSystemTrayIcon::closeToQuitToggled, this, [this](bool checked) {
        m_close_to_quit = checked;
    });

}

void Kray::closeEvent(QCloseEvent *event)
{
    qDebug() << "Kray::closeEvent()";
    Console::out() << "Kray::closeEvent()" << std::endl;
    // 关闭所有子窗口（除了 console 窗口）
    if (t1 != nullptr) {
        qDebug() << "Kray::closeEvent() t1";
        Console::out() << "Kray::closeEvent() t1" << std::endl;
        t1->close();
        delete t1;
        t1 = nullptr;
    }
    if (t2 != nullptr) {
        qDebug() << "Kray::closeEvent() t2";
        Console::out() << "Kray::closeEvent() t2" << std::endl;
        t2->close();
        delete t2;
        t2 = nullptr;
    }
    if (m_system_tray_icon != nullptr) {
        qDebug() << "Kray::closeEvent() m_system_tray_icon";
        Console::out() << "Kray::closeEvent() m_system_tray_icon" << std::endl;
        delete m_system_tray_icon;
        m_system_tray_icon = nullptr;
    }
    if (usb_widget != nullptr) {
        qDebug() << "Kray::closeEvent() usb_widget";
        Console::out() << "Kray::closeEvent() usb_widget" << std::endl;
        usb_widget->close();
        delete usb_widget;
        usb_widget = nullptr;
    }
    if (m_music_widget != nullptr) {
        qDebug() << "Kray::closeEvent() m_music_widget";
        Console::out() << "Kray::closeEvent() m_music_widget" << std::endl;
        m_music_widget->close();
        delete m_music_widget;
        m_music_widget = nullptr;
    }
    // 不关闭 console 窗口：日志在主窗口隐藏后仍可见
    if (m_console_widget != nullptr)
    {
        qDebug() << "Kray::closeEvent() m_console_widget not close";
        Console::out() << "Kray::closeEvent() m_console_widget not close" << std::endl;
    }
    // 勾选"退出时关闭程序"则彻底退出应用
    if (m_close_to_quit) {
        qApp->quit();
    }

    QMainWindow::closeEvent(event);
}

Kray::~Kray()
{
    qDebug() << "Kray::~Kray()";
    Console::out() << "Kray::~Kray()" << std::endl;
    if (t1 != nullptr) {
        qDebug() << "Kray::~Kray() t1";
        Console::out() << "Kray::~Kray() t1";
        delete t1;
    }
    if (t2 != nullptr) {
        qDebug() << "Kray::~Kray() t2";
        Console::out() << "Kray::~Kray() t2";
        delete t2;
    }
    if (usb_widget != nullptr) {
        qDebug() << "Kray::~Kray() usb_widget";
        Console::out() << "Kray::~Kray() usb_widget";
        delete usb_widget;
    }
    if (m_music_widget != nullptr) {
        qDebug() << "Kray::~Kray() m_music_widget";
        Console::out() << "Kray::~Kray() m_music_widget";
        delete m_music_widget;
    }
    if (m_system_tray_icon != nullptr) {
        qDebug() << "Kray::~Kray() m_system_tray_icon";
        Console::out() << "Kray::~Kray() m_system_tray_icon";
        delete m_system_tray_icon;
    }
    if (m_console_widget != nullptr) {
        qDebug() << "Kray::~Kray() m_console_widget";
        Console::out() << "Kray::~Kray() m_console_widget" << std::endl;
        // ConsoleWidget 析构内部会自动调用 Console::unregisterSink，
        // 此处只负责释放窗口对象，保持调用顺序：先注销、再删除（由析构函数完成）
        delete m_console_widget;
        m_console_widget = nullptr;
    }
    delete ui;
}

void Kray::on_btn_console_clicked()
{
    if (m_console_widget != nullptr) {
        m_console_widget->show_top();
    }
}

void Kray::on_btn_usb_clicked()
{
    if (usb_widget == nullptr)
    {
        usb_widget = new USBWidget();
        connect(usb_widget, &USBWidget::exitWindow, this, [=]() {
            usb_widget->close();
            delete usb_widget;
            usb_widget = nullptr;
        });
    }
    if (usb_widget != nullptr)
    {
        usb_widget->show_top();
    }
}

void Kray::on_btn_t1_clicked()
{
    if (t1 == nullptr)
    {
        t1 = new T1();
        connect(t1, &T1::exitWindow, this, [=]() {
            t1->close();
            delete t1;
            t1 = nullptr;
        });
    }
    if (t1 != nullptr)
    {
        t1->show_top();
    }

    // QColor color = QColorDialog::getColor(Qt::white, this, "pick a color", QColorDialog::ShowAlphaChannel);
    // if (color.isValid()) {
    //     QString styleSheet = QString(
    //         "QPushButton#btn_test{"
    //             "background-color: rgba(%1, %2, %3, %4);"
    //         "}"
    //     ).arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alphaF());
    //     setStyleSheet(styleSheet);
    // }

    // if (USBDeviceManager == nullptr)
    // {
    //     USBDeviceManager = new USBDeviceManagerWidget();
    //     connect(USBDeviceManager, &USBDeviceManagerWidget::exitWindow, this, [=]() {
    //         delete USBDeviceManager;
    //         USBDeviceManager = nullptr;
    //     });
    // }
    // USBDeviceManager->show_top();

    // if (usbDeviceWidget == nullptr)
    // {
    //     usbDeviceWidget = new USBDeviceWidget();
    //     connect(usbDeviceWidget, &USBDeviceWidget::exitWindow, this, [=]() {
    //         delete usbDeviceWidget;
    //         usbDeviceWidget = nullptr;
    //     });
    // }
    // usbDeviceWidget->show_top();
}

void Kray::on_btn_t2_clicked()
{
    if (t2 == nullptr)
    {
        t2 = new T2();
        connect(t2, &T2::exitWindow, this, [=]() {
            t2->close();
            delete t2;
            t2 = nullptr;
        });
    }
    if (t2 != nullptr)
    {
        t2->show_top();
    }   
}

void Kray::on_btn_music_clicked()
{
    if (m_music_widget == nullptr)
    {
        m_music_widget = new MusicRhythmWidget();
        connect(m_music_widget, &MusicRhythmWidget::exitWindow, this, [=]() {
            m_music_widget->close();
            delete m_music_widget;
            m_music_widget = nullptr;
        });
    }
    if (m_music_widget != nullptr)
    {
        m_music_widget->show_top();
    }
}

void Kray::on_btn_audio_clicked()
{

}

void Kray::on_btn_close_clicked()
{
    this->close();
}
void Kray::on_font_default_triggered()
{
    // 获取并输出应用默认字体信息
    get_font(QApplication::font(), nullptr);
}
void Kray::on_font_available_triggered()
{
    get_available_fonts(nullptr);
}
void Kray::on_system_info_triggered()
{
    get_system_info(nullptr);
}
void Kray::on_all_widget_info_triggered()
{
    get_all_widgets_info(nullptr);
}
void Kray::on_action_close_toggled(bool arg1)
{
    m_close_to_quit = arg1;
}



