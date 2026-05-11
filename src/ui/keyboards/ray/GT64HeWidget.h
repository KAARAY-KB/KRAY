#ifndef GT64HEWIDGET_H
#define GT64HEWIDGET_H

#include <QTimer>
#include <QWidget>
#include <QCloseEvent>
#include <QStyleOption>
#include <QColorDialog>
#include <QPainter>


#include "USBHelper.h"
#include "GT64HeController.h"



QT_BEGIN_NAMESPACE
namespace Ui { class GT64HeWidget; }
QT_END_NAMESPACE

class GT64HeWidget : public QWidget
                //    , public GT64HeController
{
    Q_OBJECT

public:
    explicit GT64HeWidget(USBHelper::DevMsg_t info, QWidget *parent = nullptr);
    ~GT64HeWidget();

    void closeEvent(QCloseEvent *event) override;
    void closeWidget(void);

    GT64HeController *controller = nullptr;

private:
    libusb_context *m_ctx = nullptr;
    bool m_activeWindow = false;
    QTimer *m_activeWindowTimer = nullptr;
    void updateParamValue();

    
    USBTransferCallback::TransferAction read_done(libusb_transfer *transfer);
    USBTransferCallback::TransferAction write_done(libusb_transfer *transfer);
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

private slots:

    void on_statebar_btn_exitWidget_clicked();

    void on_fastLocation_btn_game_clicked();
    void on_fastLocation_btn_letter_clicked();
    void on_fastLocation_btn_number_clicked();
    void on_fastLocation_btn_invert_clicked();
    void on_fastLocation_btn_all_clicked();
    void on_fastLocation_btn_cancel_clicked();
    
    void on_param_btn_prev_clicked();
    void on_param_btn_next_clicked();
    
    void on_param_dbg_btn_setColor_clicked();
    void on_param_dbg_btn_usbTx_clicked();
    void on_param_dbg_btn_usbRx_clicked();
    void on_param_dbg_btn_usbRxStop_clicked();
    void on_param_dbg_btn_color_panel_clicked();
    void on_param_dbg_btn_color_dft_border_clicked();
    void on_param_dbg_btn_color_dft_font_clicked();
    void on_param_dbg_btn_color_hover_font_clicked();
    void on_param_dbg_btn_color_dft_background_clicked();
    void on_param_dbg_btn_color_hover_background_clicked();
    void on_param_dbg_btn_color_checked_background_clicked();
    void on_param_dbg_btn_color_checked0_background_clicked();
    void on_param_dbg_btn_color_checked1_background_clicked();
    
    
    void on_param_property_slider_param0_userReleased(int value);
    void on_param_property_slider_param1_userReleased(int value);
    void on_param_property_slider_param2_userReleased(int value);
    void on_param_property_slider_param3_userReleased(int value);
    void on_param_property_slider_param4_userReleased(int value);
    void on_param_property_slider_param5_userReleased(int value);
    void on_param_property_spinBox_param0_valueChanged(double value);
    void on_param_property_spinBox_param1_valueChanged(double value);
    void on_param_property_spinBox_param2_valueChanged(double value);
    void on_param_property_spinBox_param3_valueChanged(double value);
    void on_param_property_spinBox_param4_valueChanged(double value);
    void on_param_property_spinBox_param5_valueChanged(double value);
    void on_param_property_radioButtonGroup_buttonToggled(int id, bool checked);

    void slot_keyboard_key_clicked(int idx, bool checked);
    void slot_activeWindowChanged(bool active);

signals:
    void exitWindow();
    void exitWidget();
    void activeWindowChanged(bool active);
private:
    Ui::GT64HeWidget *ui;
};

#endif // GT64HEWIDGET_H
