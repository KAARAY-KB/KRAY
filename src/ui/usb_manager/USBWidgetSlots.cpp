#include "USBWidget.h"
#include "Ui_USBWidget.h"
#include "console.h"


void USBWidget::dev_insert(USBHelper::DevMsg_t info) {
    Console::out() << "device insert:" << std::endl;
    ui->usbDeviceManager->addDevice(info);
}

void USBWidget::dev_remove(USBHelper::DevMsg_t info) {
    Console::out() << "device remove:" << std::endl;
    ui->usbDeviceManager->delDevice(info);
    
}


void USBWidget::on_usbDeviceManager_enterRequested(USBHelper::DevMsg_t &req_info) {
    openUSBWidget(req_info);
}
void USBWidget::on_stackedWidget_widgetRemoved(int idx) {
    Console::out() << "页面被移除:" << idx << std::endl;
}
void USBWidget::on_stackedWidget_currentChanged(int idx) {
    Console::out() << "页面发生变化:" << idx << std::endl;
    Ui::USBWidget::page_t *pt = ui->getSubWidget(idx);
    
    // ui->widgetSizeChanged(pt);
    
    switch (pt->type) {
        case Ui::USBWidget::PAGE_TY_MAIN:
            setStyleSheet("QWidget#form {background-color: #f0f0f0;}");
            resize(400, 200);
            Console::out() << "main" << std::endl;
            break;
        case Ui::USBWidget::PAGE_TY_KB:
            setStyleSheet("QWidget#form {background-color: #f0edea;}");
            resize(1000, 700);
            Console::out() << "kb" << std::endl;
            break;
    }
}
