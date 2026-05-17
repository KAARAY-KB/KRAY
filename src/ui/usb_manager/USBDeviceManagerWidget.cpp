#include "USBDeviceManagerWidget.h"
#include "console.h"

#ifdef _WIN32
#include <Windows.h>
#endif

// USBDeviceSubWidget 构造函数
USBDeviceSubWidget::USBDeviceSubWidget(UsbDeviceInfo &info, QWidget *parent) 
    : QFrame(parent)
    , m_info(info)
{
    setFrameStyle(QFrame::Box | QFrame::Raised); // 设置边框样式
    setLineWidth(1);
    setFixedSize(WIDTH, HEIGHT);

    layout = new QVBoxLayout(this);
    label = new QLabel();
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter); // 居中对齐

    // 显示制造商和产品名称
    QString mfr  = m_info.di.manufacturer.empty()  ? "NULL" : QString(m_info.di.manufacturer.c_str());
    QString prod = m_info.di.product.empty() ? "NULL" : QString(m_info.di.product.c_str());
    QString str = QString("%1-%2").arg(mfr).arg(prod);
    label->setText(str);

    // 鼠标悬停显示详细提示信息
    QString tooltip = QString::fromStdString(m_info.to_string());
    // Console::out() << tooltip.toStdString() << std::endl;
    label->setToolTip(tooltip);
    
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
    gridLayoutScroll->setContentsMargins(spacing, spacing, spacing, spacing);
    
    scrollArea->setWidget(scrollAreaWidgetContents);
    gridLayout->addWidget(scrollArea);

    int width = 700, height = 400;
    width  = (width > USBDeviceSubWidget::WIDTH) ? width : USBDeviceSubWidget::WIDTH;
    height = (height > USBDeviceSubWidget::HEIGHT) ? height : USBDeviceSubWidget::HEIGHT;

    setMinimumSize(width, height);
}

void USBDeviceManagerWidget::haveDevice(UsbDeviceInfo &info) {
    USBDeviceSubWidget *device = new USBDeviceSubWidget(info);
    gridLayoutScroll->addWidget(device);
    connect(device, &USBDeviceSubWidget::enterRequested, this, [this](UsbDeviceInfo &req_info){ emit enterRequested(req_info); });
    reorganizeLayout();
}
void USBDeviceManagerWidget::addDevice(UsbDeviceInfo &info) {
    haveDevice(info);
    // 动态布局优化
    gridLayoutScroll->activate();
    scrollAreaWidgetContents->adjustSize();
    QMetaObject::invokeMethod(this, [this](){
        scrollArea->ensureVisible(0, scrollAreaWidgetContents->height());
    }, Qt::QueuedConnection);
}
void USBDeviceManagerWidget::delDevice(UsbDeviceInfo info) {
    // 遍历查找匹配设备并删除
    for(int i = 0; i < gridLayoutScroll->count(); ++i) {
        QWidget* widget = gridLayoutScroll->itemAt(i)->widget();
        if(widget && qobject_cast<USBDeviceSubWidget*>(widget)) {
            USBDeviceSubWidget *device = static_cast<USBDeviceSubWidget*>(widget); 
            if (device->getInfo() == info) { // 使用 UsbDeviceInfo::operator== 比较
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
#ifdef _WIN32
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
    Console::out() << "USBDeviceManagerWidget: close event" << std::endl;
    event->accept();
    emit exitWindow();
    QWidget::closeEvent(event);
}
