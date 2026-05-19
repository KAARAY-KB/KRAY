#ifndef USBWIDGET_H
#define USBWIDGET_H

#include <QDebug>
#include <QWidget>
#include <QCloseEvent>
#include <QStyleOption>
#include <QPainter>

#include "usb_controller.hpp"
#include "usb_device_info.hpp"
#include "GT64HeWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class USBWidget; }
QT_END_NAMESPACE

class USBWidget : public QWidget
{
    Q_OBJECT

public:
    explicit USBWidget(QWidget *parent = nullptr);
    ~USBWidget();

    void closeEvent(QCloseEvent *event) override;
    void show_top(void);

private:
    usb_ctrl::UsbController m_ctrl;         // USB 控制器（设备管理和热插拔）
    GT64HeWidget *m_gt64heWidget = nullptr;

    void openUSBWidget(UsbDeviceInfo &info);
    void closeUSBWidget(UsbDeviceInfo &info);

    void dev_hotplug_init();
    void dev_insert(UsbDeviceInfo info);
    void dev_remove(UsbDeviceInfo info);

    void gt64heWidget(UsbDeviceInfo &info);
protected:
    void paintEvent(QPaintEvent *) {
        QStyleOption opt;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        opt.rect = rect();
        opt.palette = palette();
        opt.state = QStyle::State_Enabled;
#else
        opt.init(this);
#endif
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    }
private slots:
    void on_stackedWidget_widgetRemoved(int idx);
    void on_stackedWidget_currentChanged(int idx);
    void on_usbDeviceManager_enterRequested(UsbDeviceInfo &req_info);

signals:
    void exitWindow();
    void sig_dev_insert(UsbDeviceInfo info);
    void sig_dev_remove(UsbDeviceInfo info);

private:
    Ui::USBWidget *ui;
};

#endif // USBWIDGET_H
