#include "kray.h"
#include "./ui_kray.h"

Kray::Kray(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Kray)
{
    ui->setupUi(this);
    QString str = QString::asprintf(""
    "QT version: %d.%d.%d\n",QT_VERSION_MAJOR, QT_VERSION_MINOR, QT_VERSION_PATCH);

    str.append(QString("SYSTEM name: "));
#if defined(__linux__)
    str.append(QString("Linux"));

#elif defined(Q_OS_WINDOWS)
    str.append(QString("Windows "));
    #if defined(Q_PROCESSOR_X86_64)
        str.append(QString("64-bit build (x64)"));
    #elif defined(Q_PROCESSOR_X86)
        str.append(QString("32-bit build (x86)"));
    #endif
#elif defined(__APPLE__)
    str.append(QString("MacOS"));
#endif
    str.append(QString("\n"));
    ui->textBrowser->setText(str);
}

Kray::~Kray()
{
    delete ui;
}
