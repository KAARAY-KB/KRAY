#include "test.h"
#include "ui_test.h"

#include <QDebug>
#ifdef OS_WINDOWS
#include <Windows.h>
#endif

#define TEST_LIBUSB 0
#define TEST_QCUSTOMPLOT 0

#if TEST_QCUSTOMPLOT
#include "qcustomplot.h"
#endif

#if TEST_LIBUSB
#include "libusb.h"
#endif


Test::Test(QWidget *parent) 
    : QWidget(parent)
    , ui(new Ui::Test)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowTitle("Test Widget");

#if TEST_LIBUSB
    const struct libusb_version *libusb_ver = libusb_get_version();
    qDebug("libusb: %d.%d.%d.%d.%s, %s", libusb_ver->major, libusb_ver->minor, libusb_ver->micro, libusb_ver->nano, libusb_ver->rc, libusb_ver->describe);
#endif

#if TEST_QCUSTOMPLOT
    QCustomPlot* plot = new QCustomPlot(this);
    plot->setGeometry(10, 10, 780, 580); // 或使用布局

    // 示例数据
    QVector<double> x(100), y(100);
    for (int i = 0; i < 100; ++i) {
        x[i] = i * 0.1;
        y[i] = qSin(x[i]) * qCos(x[i]/2.0);
    }

    plot->addGraph();
    plot->graph(0)->setData(x, y);
    plot->graph(0)->setPen(QPen(QColor(30, 144, 255), 2)); // DodgerBlue

    plot->xAxis->setLabel("X Axis");
    plot->yAxis->setLabel("Y = sin(x)·cos(x/2)");

    plot->rescaleAxes();
    plot->replot();

    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom); // 拖动和缩放
#endif
}

Test::~Test()
{
    delete ui;
}

void Test::closeEvent(QCloseEvent *event)
{
    qDebug("widget close event");
    event->accept();
    emit exitWindow();
    QWidget::closeEvent(event);
}

void Test::show_top(void)
{
    if (this->isMinimized())
    {
        this->showNormal();
    }
#ifdef OS_WINDOWS
    SetWindowPos(HWND(this->winId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetWindowPos(HWND(this->winId()), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif
    this->show();
    this->activateWindow();
}
