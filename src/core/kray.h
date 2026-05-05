#ifndef KRAY_H
#define KRAY_H

#include <QMainWindow>

#include "t1.h"
#include "MSystemTrayIcon.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Kray;
}
QT_END_NAMESPACE

class Kray : public QMainWindow
{
    Q_OBJECT

public:
    Kray(QWidget *parent = nullptr);
    ~Kray();

private:
    MSystemTrayIcon *m_system_tray_icon = nullptr;
    T1 *t1 = nullptr;
    
private slots:
    void on_btn_usb_clicked();

    void on_btn_close_clicked();

    void on_btn_test_clicked();

private:
    Ui::Kray *ui;
};
#endif // KRAY_H
