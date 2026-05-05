#include "MSlider.h"
#include <QToolTip>
#include <QPoint>
#include <QDebug>

MSlider::MSlider(QWidget *parent) 
    : QSlider(parent)
{
}
MSlider::MSlider(Qt::Orientation orientation, QWidget *parent)
    : QSlider(orientation, parent)
{
}

void MSlider::wheelEvent(QWheelEvent *event) {
    if (event->type() == QEvent::Wheel) {
        event->ignore();
    }
    else {
        QSlider::wheelEvent(event);
    }
}
void MSlider::mousePressEvent(QMouseEvent *event) {
    // Move to the mouse position
    int pos = (this->orientation() == Qt::Horizontal) ? event->pos().x() : event->pos().y();
    double per = pos * 1.0 / this->width();
    int value = per * (this->maximum() - this->minimum()) + this->minimum();
    this->setSliderPosition(value);

    QSlider::mousePressEvent(event);
}
void MSlider::mouseReleaseEvent(QMouseEvent *event) {
    QSlider::mouseReleaseEvent(event);
    emit userReleased(this->sliderPosition());
}
void MSlider::mouseMoveEvent(QMouseEvent *event) {
    QSlider::mouseMoveEvent(event);  
}
