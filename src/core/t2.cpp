#include "t2.h"
#include "./ui_t2.h"

#ifdef OS_WINDOWS
#include <Windows.h>
#endif

T2::T2(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::T2)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowTitle("T2 Widget");
}

T2::~T2()
{
    delete ui;
}

void T2::closeEvent(QCloseEvent *event)
{
    qDebug("t2 widget close event");
    event->accept();
    emit exitWindow();
    QWidget::closeEvent(event);

}

void T2::show_top(void)
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

