#include "kray.h"
#include "./ui_kray.h"

#include <QApplication>
Kray::Kray(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Kray)
{
    ui->setupUi(this);
    setWindowTitle("KRAY-main");
    setWindowIcon(QIcon(":/images/pixel_pizza.png"));

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


    QString str = QString::asprintf("基本信息\n"
    "QT version: %d.%d.%d\n",QT_VERSION_MAJOR, QT_VERSION_MINOR, QT_VERSION_PATCH);

    str.append(QString("SYSTEM name: "));
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
    ui->textBrowser->setText(str);
}

Kray::~Kray()
{
    if (t1 != nullptr) {
        delete t1;
    }
    if (t2 != nullptr) {
        delete t2;
    }
    if (usb_widget != nullptr) {
        delete usb_widget;
    }
    if (m_system_tray_icon != nullptr) {
        delete m_system_tray_icon;
    }

    delete ui;
}


void Kray::on_btn_usb_clicked()
{
    if (usb_widget == nullptr)
    {
        usb_widget = new USBWidget();
        connect(usb_widget, &USBWidget::exitWindow, this, [=]() {
            delete usb_widget;
            usb_widget = nullptr;
        });
    }
    usb_widget->show_top();
}

void Kray::on_btn_t1_clicked()
{
    if (t1 == nullptr)
    {
        t1 = new T1();
        connect(t1, &T1::exitWindow, this, [=]() {
            delete t1;
            t1 = nullptr;
        });
    }
    t1->show_top();

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
    }
    t2->show();
}

void Kray::on_btn_close_clicked()
{
    this->close();
}



