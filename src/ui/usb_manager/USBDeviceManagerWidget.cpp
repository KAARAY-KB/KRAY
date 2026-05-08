#include "USBDeviceManagerWidget.h"
#include "console.h"

#ifdef __WIN32__
#include <Windows.h>
#endif

// USBDeviceWidget 类实现
USBDeviceSubWidget::USBDeviceSubWidget(USBHelper::DevMsg_t &info, QWidget *parent) 
    : QFrame(parent)
    , m_info(info)
{
    setFrameStyle(QFrame::Box | QFrame::Raised); // 设置边框样式
    setLineWidth(1);
    setFixedSize(WIDTH, HEIGHT);

    layout = new QVBoxLayout(this);
    label = new QLabel();
    label->setWordWrap(true);

    if (m_info.id.ty != USBHelper::DevType::DEV_UNKNOWN) {
        label->setAlignment(Qt::AlignCenter);
    }
    QString mfr  = m_info.ch.mfr.empty()  ? "NULL" : QString(m_info.ch.mfr.c_str());
    QString prod = m_info.ch.prod.empty() ? "NULL" : QString(m_info.ch.prod.c_str());
    QString sn   = m_info.ch.sn.empty()   ? "NULL" : QString(m_info.ch.sn.c_str());
    
    QString str = QString("%1-%2").arg(mfr).arg(prod);
    label->setText(str); // 显示制造商和产品名称

    QString tooltip = QString::fromStdString(USBHelper::msg(&m_info, true));
    Console::out() << tooltip.toStdString() << std::endl;
    label->setToolTip(tooltip); // 鼠标悬停显示提示信息
    
    btn_enter = new QPushButton("Enter");
    connect(btn_enter, &QPushButton::clicked, this, [this](){ emit enterRequested(m_info); });

    layout->addWidget(label);
    layout->addWidget(btn_enter);

}

USBDeviceManagerWidget::USBDeviceManagerWidget(QWidget *parent) 
    : QWidget(parent)
{
    gridLayout = new QGridLayout(this);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    
    // // 添加按钮
    // QPushButton* addButton = new QPushButton("Add Device", this);
    // connect(addButton, &QPushButton::clicked, this, [this](){
    //     USBHelper::DevMsg_t info;
    //     addDevice(info);
    // });
    // gridLayout->addWidget(addButton);

    // 创建滚动区域
    scrollArea = new QScrollArea(this);
    scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
    scrollArea->setFrameShadow(QFrame::Raised);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidgetResizable(true);

    scrollAreaWidgetContents = new QWidget();
    scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));

    gridLayoutScroll = new QGridLayout(scrollAreaWidgetContents);
    gridLayoutScroll->setObjectName(QString::fromUtf8("gridLayoutScroll"));

    const int spacing = 20; // 设置间距大小
    gridLayoutScroll->setHorizontalSpacing(spacing);
    gridLayoutScroll->setVerticalSpacing(spacing);
    gridLayoutScroll->setContentsMargins(spacing, spacing, spacing, spacing); // 设置边距
    
    scrollArea->setWidget(scrollAreaWidgetContents);
    gridLayout->addWidget(scrollArea);

    int width = 700, height = 400;
    width  = (width > USBDeviceSubWidget::WIDTH) ? width : USBDeviceSubWidget::WIDTH;
    height = (height > USBDeviceSubWidget::HEIGHT) ? height : USBDeviceSubWidget::HEIGHT;

    setMinimumSize(width, height);
}

void USBDeviceManagerWidget::haveDevice(USBHelper::DevMsg_t &info) {
    USBDeviceSubWidget * device = new USBDeviceSubWidget(info);
    gridLayoutScroll->addWidget(device);
    connect(device, &USBDeviceSubWidget::enterRequested, this, [this](USBHelper::DevMsg_t &req_info){ emit enterRequested(req_info); });
    reorganizeLayout(); // 调整布局以适应新添加的设备
}
void USBDeviceManagerWidget::addDevice(USBHelper::DevMsg_t &info) {
    haveDevice(info);
    // ======== 动态布局优化 ========
    // 1. 强制立即更新布局计算
    gridLayoutScroll->activate();
    // 2. 调整容器尺寸以适应内容
    scrollAreaWidgetContents->adjustSize();
    // 3. 确保滚动区域更新显示
    QMetaObject::invokeMethod(this, [this](){
        scrollArea->ensureVisible(0, scrollAreaWidgetContents->height());
    }, Qt::QueuedConnection);
}
void USBDeviceManagerWidget::delDevice(USBHelper::DevMsg_t info) {
    // 收集所有现存的设备小部件
    for(int i = 0; i < gridLayoutScroll->count(); ++i) {
        QWidget* widget = gridLayoutScroll->itemAt(i)->widget();
        if(widget && qobject_cast<USBDeviceSubWidget*>(widget)) {
            USBDeviceSubWidget *device = static_cast<USBDeviceSubWidget*>(widget); 
            if (USBHelper::dev_id_compare(device->getInfo(), info)) {
                for(int j = 0; j < gridLayoutScroll->count(); ++j) {
                    if(gridLayoutScroll->itemAt(j)->widget() == widget) {
                        gridLayoutScroll->removeWidget(widget);
                        widget->deleteLater();
                        break;
                    }
                }
                break;
            }
        }
    }
    reorganizeLayout();
}
int USBDeviceManagerWidget::calculateColumns() const {
    // 假设每个设备的宽度是固定的200像素加上水平间距的一半
    int widgetWidth = USBDeviceSubWidget::WIDTH + (gridLayoutScroll->horizontalSpacing() / 2) * 2;
    return std::max(1, (scrollArea->width() - gridLayoutScroll->contentsMargins().left() - gridLayoutScroll->contentsMargins().right()) / widgetWidth);
}
void USBDeviceManagerWidget::reorganizeLayout() {
    QList<USBDeviceSubWidget*> devices;
    // 收集所有现存的设备小部件
    for(int i = 0; i < gridLayoutScroll->count(); ++i) {
        QWidget* widget = gridLayoutScroll->itemAt(i)->widget();
        if(widget && qobject_cast<USBDeviceSubWidget*>(widget)) {
            devices.append(static_cast<USBDeviceSubWidget*>(widget));
        }
    }

    // 清除现有布局
    QLayoutItem* item;
    while ((item = gridLayoutScroll->takeAt(0)) != nullptr) {
        delete item;
    }

    int cols = calculateColumns();
    int row = 0, col = 0;
    m_deviceCount = 0;
    foreach(USBDeviceSubWidget* device, devices) {
        m_deviceCount++;
        gridLayoutScroll->addWidget(device, row, col);
        if (++col >= cols) {
            col = 0;
            ++row;
        }
    }
    
}
void USBDeviceManagerWidget::show_top(void) {
    if (this->isMinimized())
    {
        this->showNormal();
    }
#ifdef __WIN32__
    SetWindowPos(HWND(this->winId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetWindowPos(HWND(this->winId()), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif
    this->show();
    this->activateWindow();
}

void USBDeviceManagerWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    reorganizeLayout();
}
void USBDeviceManagerWidget::closeEvent(QCloseEvent *event) {
    Console::out() << "widget close event" << std::endl;
    event->accept();
    emit exitWindow();
    QWidget::closeEvent(event);
}

