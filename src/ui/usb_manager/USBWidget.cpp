#include "USBWidget.h"
#include "Ui_USBWidget.h"
#include "console.h"


USBWidget::USBWidget(QWidget *parent) 
    : QWidget(parent)
    , ui(new Ui::USBWidget)
    , m_gt64heWidget(nullptr)
{
    ui->setupUi(this);

    qRegisterMetaType<UsbDeviceInfo>("UsbDeviceInfo");
    connect(this, &USBWidget::sig_dev_insert, this, &USBWidget::dev_insert);
    connect(this, &USBWidget::sig_dev_remove, this, &USBWidget::dev_remove);

    // 刷新设备列表
    m_ctrl.refresh_devices();

    // 初始化热插拔监听
    dev_hotplug_init();

    // 遍历当前设备，添加到设备管理器
    auto& devs = m_ctrl.devices();
    Console::info("USBWidget") << "found " << devs.size() << " devices" << std::endl << std::endl;
    size_t i = 0;
    for (auto& dev : devs) {
        Console::info("USBWidget") << std::endl << "#"<< i++ << ":" << std::endl;
        UsbDeviceInfo info = UsbDeviceInfo::from_usb_device(dev);
        ui->usbDeviceManager->haveDevice(info);
    }

    connect(this, &USBWidget::exitWindow, this, [=]() {
        if (m_gt64heWidget)
        {
            Console::info("USBWidget") << " gt exitWindow" << std::endl;
            delete m_gt64heWidget;
            m_gt64heWidget = nullptr;
        }
    });
}

USBWidget::~USBWidget()
{
    Console::info("USBWidget") << "~USBWidget()" << std::endl;
    m_ctrl.hotplug_stop(); // 停止热插拔监听
    delete ui;
}

void USBWidget::closeEvent(QCloseEvent *event)
{
    Console::info("USBWidget") << "closeEvent" << std::endl;
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
            Console::info("USBWidget") << std::endl << "热插拔事件: " << (event == usb_ctrl::core::HotplugEvent::Arrived ? "插入" : "拔出") << std::endl;
            UsbDeviceInfo info = UsbDeviceInfo::from_usb_device(device);
            if (event == usb_ctrl::core::HotplugEvent::Arrived) {
                emit sig_dev_insert(info);
            } else {
                emit sig_dev_remove(info);
            }
        }
    );
    if (!ok) {
        Console::error("USBWidget") << "Failed to start hotplug listener" << std::endl;
    }
}

void USBWidget::openUSBWidget(UsbDeviceInfo &info) {
    if (!info.is_hid) {
        Console::warn("USBWidget") << "skip non-HID device: " << info.to_string() << std::endl;
        return;
    }
    gt64heWidget(info);
}
void USBWidget::closeUSBWidget(UsbDeviceInfo &info) {
    
}

void USBWidget::gt64heWidget(UsbDeviceInfo &info)
{
    if (m_gt64heWidget == nullptr)
    {
        Console::info("USBWidget") << "open keyboard Widget" << std::endl;
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
