#include "t2.h"
#include "./ui_t2.h"
#include "console.h"

#ifdef _WIN32
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
    Console::out() << "t2 widget close event" << std::endl;
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
#ifdef _WIN32
    SetWindowPos(HWND(this->winId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetWindowPos(HWND(this->winId()), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif
    this->show();
    this->activateWindow();
}

