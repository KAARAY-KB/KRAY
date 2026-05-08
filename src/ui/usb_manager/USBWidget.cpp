#include "USBWidget.h"
#include "Ui_USBWidget.h"
#include "console.h"


USBWidget::USBWidget(QWidget *parent) 
    : QWidget(parent)
    , ui(new Ui::USBWidget)
    , m_gt64heWidget(nullptr)
{
    ui->setupUi(this);

    int ret = libusb_init(nullptr);
    // int ret = libusb_init_context(&ctx, nullptr, 0);
    if (ret != LIBUSB_SUCCESS) {
        Console::out() << "libusb init failed" << libusb_error_name(ret) << std::endl;
    }

    dev_hotplug_init();
    Console::out() << "size " << m_usbHotplug->handle->get_decice_info().size() << std::endl;
    for (int i = 0; i < m_usbHotplug->handle->get_decice_info().size(); i++) {
        USBHelper::DevMsg_t info = USBHelper::find_device_support(m_usbHotplug->handle->get_decice_info()[i], USBHelper::DevFindType::FIND_DEV_TYPE_VID_PID);
        if (info.id.ty != USBHelper::DEV_UNKNOWN) {
            ui->usbDeviceManager->haveDevice(info);
        }
        else {
            ui->usbDeviceManager->haveDevice(info);
        }
    }

    // demo
    if (m_usbHotplug->handle->get_decice_info().size() == 0) {
        USBHelper::DevMsg_t info;
        USBHelper::debug_msg(info);
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

    m_usbThread = new USBThread();
}

USBWidget::~USBWidget()
{
    Console::out() << "USBWidget::~USBWidget()" << std::endl;

    if (m_usbHotplug) {
        delete m_usbHotplug;
    }
    if (m_usbThread) {
        m_usbThread->end();
    }
    libusb_exit(nullptr);
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
    m_usbHotplug = new USBHotplug();
    if (!m_usbHotplug->handle->start_detector()) {
        Console::out() << "Failed to start detector for USB hotplug events" << std::endl;
        delete m_usbHotplug;
        m_usbHotplug = nullptr;
    }
    else {
        m_usbHotplug->handle->register_cb(std::bind(&USBWidget::dev_insert, this, std::placeholders::_1),
                                std::bind(&USBWidget::dev_remove, this, std::placeholders::_1));
        // m_usbHotplug->handle->refresh_device_evt();
    }
}

void USBWidget::openUSBWidget(USBHelper::DevMsg_t &info) {
    // if (info.id.ty == USBHelper::DEV_KB_GT64HE) { 
    gt64heWidget(info);
    // }
}
void USBWidget::closeUSBWidget(USBHelper::DevMsg_t &info) {
    
}

void USBWidget::gt64heWidget(USBHelper::DevMsg_t &info)
{
    if (m_gt64heWidget == nullptr)
    {
        m_usbThread->begin(USBThread::RUN_TYPE_EVT);
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
                m_usbThread->begin(USBThread::RUN_TYPE_NONE);
            }
        });
        connect(m_gt64heWidget, &GT64HeWidget::exitWindow, this, [=](){
            if (m_gt64heWidget) {
                delete m_gt64heWidget;
                m_gt64heWidget = nullptr;
                m_usbThread->begin(USBThread::RUN_TYPE_NONE);
            }
        });
        ui->showSubWidget(pt->type, pt->idx);
    }
}

