#ifndef USBWIDGET_H
#define USBWIDGET_H

#include <QDebug>
#include <QWidget>
#include <QCloseEvent>
#include <QStyleOption>
#include <QPainter>


#include "libusb.h"
#include "USBHelper.h"
#include "USBThread.h"
#include "USBHotplug.h"
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
    libusb_context *m_ctx = nullptr;
    USBThread *m_usbThread = nullptr;
    USBHotplug *m_usbHotplug = nullptr;

    GT64HeWidget *m_gt64heWidget = nullptr;

    void openUSBWidget(USBHelper::DevMsg_t &info);
    void closeUSBWidget(USBHelper::DevMsg_t &info);

    void dev_hotplug_init();
    void dev_insert(const USBHelper::DevMsg_t info);
    void dev_remove(const USBHelper::DevMsg_t info);

    
    void gt64heWidget(USBHelper::DevMsg_t &info);
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
    void on_usbDeviceManager_enterRequested(USBHelper::DevMsg_t &req_info);

signals:
    void exitWindow();

private:
    Ui::USBWidget *ui;
};

#endif // USBWIDGET_H
