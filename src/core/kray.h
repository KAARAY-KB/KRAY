#ifndef KRAY_H
#define KRAY_H

#include <QMainWindow>
#include "MSystemTrayIcon.h"

#include "t1.h"
#include "t2.h"

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
    T2 *t2 = nullptr;
    
private slots:
    void on_btn_usb_clicked();
    void on_btn_close_clicked();
    void on_btn_t1_clicked();
    void on_btn_t2_clicked();

private:
    Ui::Kray *ui;
};
#endif // KRAY_H
