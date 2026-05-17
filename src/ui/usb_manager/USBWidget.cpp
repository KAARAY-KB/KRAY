#include "USBWidget.h"
#include "Ui_USBWidget.h"
#include "console.h"


USBWidget::USBWidget(QWidget *parent) 
    : QWidget(parent)
    , ui(new Ui::USBWidget)
    , m_gt64heWidget(nullptr)
{
    ui->setupUi(this);

    // 刷新设备列表
    m_ctrl.refresh_devices();

    // 初始化热插拔监听
    dev_hotplug_init();

    // 遍历当前设备，添加到设备管理器
    auto& devs = m_ctrl.devices();
    Console::out() << "USBWidget: found " << devs.size() << " devices" << std::endl << std::endl;
    size_t i = 0;
    for (auto& dev : devs) {
        Console::out() << "#"<< i++ << ":" << std::endl;
        UsbDeviceInfo info = UsbDeviceInfo::from_usb_device(dev);
        ui->usbDeviceManager->haveDevice(info);
    }

    connect(this, &USBWidget::exitWindow, this, [=]() {
        Console::out() << " gt exitWindow" << std::endl;
        if (m_gt64heWidget)
        {
            delete m_gt64heWidget;
            m_gt64heWidget = nullptr;
        }
    });
}

USBWidget::~USBWidget()
{
    Console::out() << "USBWidget::~USBWidget()" << std::endl;
    m_ctrl.hotplug_stop(); // 停止热插拔监听
    delete ui;
}

void USBWidget::closeEvent(QCloseEvent *event)
{
    Console::out() << "widget close event" << std::endl;
    event->accept();
    emit exitWindow();
    QWidget::closeEvent(event);
}

void USBWidget::show_top(void)
{
    ui->show_top();
}


void USBWidget::dev_hotplug_init(void)
{
    // 启动热插拔监听，注册插入/拔出回调
    bool ok = m_ctrl.hotplug_start(
        [this](usb_ctrl::core::HotplugEvent event, usb_ctrl::core::UsbDevice& device) {
            Console::out() << "USBWidget: 热插拔事件接收: " << (event == usb_ctrl::core::HotplugEvent::Arrived ? "插入" : "拔出") << std::endl;
            UsbDeviceInfo info = UsbDeviceInfo::from_usb_device(device);
            if (event == usb_ctrl::core::HotplugEvent::Arrived) {
                dev_insert(info);   // 设备插入
            } else {
                dev_remove(info);   // 设备拔出
            }
        }
    );
    if (!ok) {
        Console::out() << "Failed to start hotplug listener" << std::endl;
    }
}

void USBWidget::openUSBWidget(UsbDeviceInfo &info) {
    gt64heWidget(info);
}
void USBWidget::closeUSBWidget(UsbDeviceInfo &info) {
    
}

void USBWidget::gt64heWidget(UsbDeviceInfo &info)
{
    if (m_gt64heWidget == nullptr)
    {
        Console::out() << "open keyboard Widget" << std::endl;
        m_gt64heWidget = new GT64HeWidget(info);
        Ui::USBWidget::page_t *pt = ui->addSubWidget(Ui::USBWidget::PAGE_TY_KB, m_gt64heWidget);
        connect(m_gt64heWidget, &GT64HeWidget::exitWidget, this, [=](){
            int cur_idx = ui->stackedWidget->currentIndex();
            ui->showSubWidget(Ui::USBWidget::PAGE_TY_MAIN, 0);
            ui->delSubWidget(Ui::USBWidget::PAGE_TY_KB, cur_idx);
            if (m_gt64heWidget) {
                delete m_gt64heWidget;
                m_gt64heWidget = nullptr;
            }
        });
        connect(m_gt64heWidget, &GT64HeWidget::exitWindow, this, [=](){
            if (m_gt64heWidget) {
                delete m_gt64heWidget;
                m_gt64heWidget = nullptr;
            }
        });
        ui->showSubWidget(pt->type, pt->idx);
    }
}
