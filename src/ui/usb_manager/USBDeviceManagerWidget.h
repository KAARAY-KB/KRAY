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
#include <QMouseEvent>
#include <QCloseEvent>
#include "usb_device_info.hpp"

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
signals:
    void enterRequested(UsbDeviceInfo &m_info);
protected:
    // 点击卡片发射进入信号
    void mousePressEvent(QMouseEvent *event) override;
    // 悬停时显示手型光标
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
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
