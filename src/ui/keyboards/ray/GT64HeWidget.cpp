#include "GT64HeWidget.h"
#include "Ui_GT64HeWidget.h"
#include "hphpt.h"


GT64HeWidget::GT64HeWidget(USBHelper::DevMsg_t info, QWidget *parent) 
    : QWidget(parent)
    , ui(new Ui::GT64HeWidget)
{
    ui->setupUi(this);
    // updateParamValue();

    libusb_device *dev = USBHelper::find_device(info);
    if ( dev != nullptr ) {
        qDebug() << "Found device";
        controller = new GT64HeController(dev);
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

    // char ch[64];
    // int len = sprintf(ch, "%s / %s", info.ch.mfr.c_str(), info.ch.prod.c_str());

    // QString str;
    // str.append(ch);
    // qDebug() << "test: " << str;
    // ui->statebar->label_cfgFile->setText(str);
    m_activeWindowTimer->start(50);
}

GT64HeWidget::~GT64HeWidget() {
    if (m_activeWindow) {
        emit activeWindowChanged(false);
    }
    if (m_activeWindowTimer) {
        qDebug() << "GT64HeWidget::~m_activeWindowTimer";
        m_activeWindowTimer->stop();
        delete m_activeWindowTimer;
    }
    if (controller) {
        qDebug() << "GT64HeWidget::~controller";
        delete controller;
    }
    delete ui;
}

void GT64HeWidget::closeEvent(QCloseEvent *event) {
    qDebug() << "GT64HeWidget::closeEvent()";
    event->accept();
    emit exitWindow();
    QWidget::closeEvent(event);
}
void GT64HeWidget::closeWidget(void) {
    qDebug() << "GT64HeWidget::closeWidget()";
    emit exitWidget();
}

void GT64HeWidget::updateParamValue() {
    double val = 2.0;
    ui->param->property->spinBox_param[0]->setValue(val);
}


USBTransferCallback::TransferAction GT64HeWidget::read_done(libusb_transfer *transfer) {
    qDebug("usb rx[%d][%d]%d,%d,%d\n", transfer->length, transfer->actual_length, transfer->buffer[0], transfer->buffer[1], transfer->buffer[transfer->actual_length-1]);
    
    hpt_decode(NULL, 0);
    
    return USBTransferCallback::TransferAction::TRANSFER_CONTINUE;
}
USBTransferCallback::TransferAction GT64HeWidget::write_done(libusb_transfer *transfer) {
    return USBTransferCallback::TransferAction::TRANSFER_ONCE;
}

