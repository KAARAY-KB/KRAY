#ifndef MKEYBOARDROW_H
#define MKEYBOARDROW_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPainter>
#include <QStyleOption>
#include "MKeyboardKey.h"


class MKeyboardRow : public QWidget {
    Q_OBJECT
public:
    typedef struct {
        int key_spacing;
        QVector<MKeyboardKey::msg_t> keys;
    } row_t;
    explicit MKeyboardRow(row_t &row, int base_w = 20, int base_h = 20, QWidget *parent = nullptr);
    MKeyboardKey *getKey(int keyIdx) { return m_keys[keyIdx]; }
    int getKeyNum() const { return m_keys.size(); }
private:
    row_t m_rowMsg;
    QVector<MKeyboardKey *>m_keys;
    QHBoxLayout *m_hlayout = nullptr;
    QString row_color;
    QString colorToStr(QColor color) { return QString("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue()); }
private slots:
    void on_row_color(QColor color) {row_color = colorToStr(color);}
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
};


#endif // MKEYBOARDROW_H
