#ifndef MKEYBOARDLAYOUT_H
#define MKEYBOARDLAYOUT_H

#include <QWidget>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QStyleOption>
#include <QPainter>


#include "MKeyboardPanel.h"



class MKeyboardLayout : public QWidget {
    Q_OBJECT
public:
    MKeyboardLayout(MKeyboardPanel::type_t type, int base_w = MKeyboardPanel::KEY_W_BASE, int base_h = MKeyboardPanel::KEY_H_BASE, QWidget *parent = nullptr);
    MKeyboardPanel *m_panel = nullptr;
private:
    QGridLayout *m_layout = nullptr;
protected:

    void paintEvent(QPaintEvent *) {
        QStyleOption opt;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        opt.rect = rect();
        opt.palette = palette();
        opt.state = QStyle::State_Enabled;
#else
        opt.init(this);
#endif
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    }
signals:
    void keyClicked(int idx, bool checked);
};



#endif // MKEYBOARDLAYOUT_H
