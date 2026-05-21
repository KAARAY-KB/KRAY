#ifndef MTABWIDGET_H
#define MTABWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QTabBar>
#include <QResizeEvent>


class MTabWidget : public QTabWidget {
public:
    MTabWidget(QWidget *parent = nullptr);
protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif // MTABWIDGET_H
