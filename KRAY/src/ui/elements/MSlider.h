#ifndef MSLIDER_H
#define MSLIDER_H

#include <Qt>
#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QWheelEvent>
#include <QMouseEvent>

class MSlider : public QSlider {
    Q_OBJECT
public:
    MSlider(QWidget *parent = nullptr);
    MSlider(Qt::Orientation orientation, QWidget *parent = nullptr);
private:  
    QLabel *m_scale;  
signals:
    void userReleased(int value);
protected:
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};


#endif // MSLIDER_H
