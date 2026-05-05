#include "MDoubleSpinBox.h"

MDoubleSpinBox::MDoubleSpinBox(QWidget *parent)
    : QDoubleSpinBox(parent) 
{
    
}
void MDoubleSpinBox::focusOutEvent(QFocusEvent *event) {
    QDoubleSpinBox::focusOutEvent(event);
    // 在这里可以添加失去焦点时的自定义逻辑
}

