#ifndef MDOUBLESPINBOX_H
#define MDOUBLESPINBOX_H

#include <QWidget>
#include <QDoubleSpinBox>
#include <QFocusEvent>


class MDoubleSpinBox : public QDoubleSpinBox {
public:
    MDoubleSpinBox(QWidget *parent = nullptr);
protected:
    void focusOutEvent(QFocusEvent *event) override;
};

#endif // MDOUBLESPINBOX_H
