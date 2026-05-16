#ifndef USBDEVICE_MANAGER_WIDGET_H
#define USBDEVICE_MANAGER_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QFrame>
#include <QDebug>
#include <QLayoutItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QGridLayout>
#include <QPushButton>
#include <QCloseEvent>
#include "devices/usb_device_info.hpp"

class USBDeviceSubWidget : public QFrame {
    Q_OBJECT

public:
    explicit USBDeviceSubWidget(UsbDeviceInfo &info, QWidget *parent = nullptr);
    enum {
        WIDTH = 300,
        HEIGHT = 200
    };
    UsbDeviceInfo getInfo(void) const { return m_info; }
private:
    UsbDeviceInfo m_info;
    QVBoxLayout *layout;
    QLabel *label;
    QPushButton *btn_enter;
signals:
    void enterRequested(UsbDeviceInfo &m_info);
};

class USBDeviceManagerWidget : public QWidget 
{
    Q_OBJECT

public:
    USBDeviceManagerWidget(QWidget *parent = nullptr);

    void haveDevice(UsbDeviceInfo &info);
    void addDevice(UsbDeviceInfo &info);
    void delDevice(UsbDeviceInfo info);

private:
    QGridLayout* gridLayout;
    QWidget* scrollAreaWidgetContents;
    QGridLayout *gridLayoutScroll;
    QScrollArea *scrollArea;
    const int ROW_CNT_MAX = 1;
    int m_deviceCount = 0;
    void reorganizeLayout();
    int calculateColumns() const;
    void show_top(void);

    
signals:
    void exitWindow();
    void exitWidget();
    void enterRequested(UsbDeviceInfo &m_info);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};

#endif // USBDEVICE_MANAGER_WIDGET_H
