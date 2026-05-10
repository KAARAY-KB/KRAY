#ifndef KRAY_H
#define KRAY_H

#include <QMainWindow>
#include "MSystemTrayIcon.h"

#include "t1.h"
#include "t2.h"
#include "USBWidget.h"

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
    T1 *t1 = nullptr;
    T2 *t2 = nullptr;
    USBWidget *usb_widget = nullptr;
    MSystemTrayIcon *m_system_tray_icon = nullptr;

    void closeEvent(QCloseEvent *event) override;
    void get_system_info(void *p_context);
    void get_font(QFont font, void *p_context);
    void get_available_fonts(void *p_context);
    void get_all_widgets_info(void *p_context);
    void get_widget_info(QWidget *widget, void *p_context);
    

private slots:
    void on_btn_console_clicked();
    void on_btn_usb_clicked();
    void on_btn_close_clicked();
    void on_btn_t1_clicked();
    void on_btn_t2_clicked();

    void on_font_default_triggered();
    void on_font_available_triggered();
    void on_system_info_triggered();
    void on_all_widget_info_triggered();

private:
    Ui::Kray *ui;
};
#endif // KRAY_H
