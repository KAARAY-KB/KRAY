#include "MTabWidget.h"

MTabWidget::MTabWidget(QWidget *parent)
    : QTabWidget(parent) 
{
}

void MTabWidget::resizeEvent(QResizeEvent *event) {
    QTabWidget::resizeEvent(event);
    this->tabBar()->setFixedWidth(event->size().width());
}
