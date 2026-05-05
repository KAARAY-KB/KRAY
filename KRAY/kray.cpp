#include "kray.h"
#include "./ui_kray.h"

Kray::Kray(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Kray)
{
    ui->setupUi(this);
}

Kray::~Kray()
{
    delete ui;
}
