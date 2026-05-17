#include "GT64HeWidget.h"
#include "Ui_GT64HeWidget.h"
#include "hphpt.h"
#include "console.h"
#include <QDebug>


GT64HeWidget::GT64HeWidget(UsbDeviceInfo info, QWidget *parent) 
    : QWidget(parent)
    , ui(new Ui::GT64HeWidget)
{
    ui->setupUi(this);

    // 创建并打开 GT-64HE 设备
    device = new GT64HeDevice(info);
    if (device->open()) {
        Console::out() << "GT64HeWidget: device opened" << std::endl;
    } else {
        Console::out() << "GT64HeWidget: device open failed" << std::endl;
    }

    connect(this, &GT64HeWidget::activeWindowChanged, this, &GT64HeWidget::slot_activeWindowChanged);
    m_activeWindowTimer = new QTimer(this);
    connect(m_activeWindowTimer, &QTimer::timeout, this, [this](){
        if (m_activeWindow != this->isActiveWindow()) {
            m_activeWindow = this->isActiveWindow();
            emit activeWindowChanged(m_activeWindow);
        }
        if (m_activeWindowTimer->interval() != 1000) {
            m_activeWindowTimer->setInterval(1000);
        }
    });
    connect(ui->keyboard->layout, &MKeyboardLayout::keyClicked, this, &GT64HeWidget::slot_keyboard_key_clicked);

    m_activeWindowTimer->start(50);
}

GT64HeWidget::~GT64HeWidget() {
    if (m_activeWindow) {
        emit activeWindowChanged(false);
    }
    if (m_activeWindowTimer) {
        Console::out() << "GT64HeWidget::~m_activeWindowTimer" << std::endl;
        m_activeWindowTimer->stop();
        delete m_activeWindowTimer;
    }
    if (device) {
        Console::out() << "GT64HeWidget::~device" << std::endl;
        device->close();
        delete device;
    }
    delete ui;
}

void GT64HeWidget::closeEvent(QCloseEvent *event) {
    Console::out() << "GT64HeWidget: close event" << std::endl;
    event->accept();
    emit exitWindow();
    QWidget::closeEvent(event);
}
void GT64HeWidget::closeWidget(void) {
    Console::out() << "GT64HeWidget: close widget" << std::endl;
    emit exitWidget();
}

void GT64HeWidget::updateParamValue() {
    double val = 2.0;
    ui->param->property->spinBox_param[0]->setValue(val);
}

// 读取完成回调
void GT64HeWidget::on_read_done(const std::vector<uint8_t>& data) {
    std::string str = "usb rx[" + std::to_string(data.size()) + "]: ";
    for (size_t i = 0; i < 10 && i < data.size(); ++i) {
        str += std::to_string(data[i]) + ",";
    }
    if (!data.empty()) str.pop_back();
    Console::out() << str << std::endl;
}
