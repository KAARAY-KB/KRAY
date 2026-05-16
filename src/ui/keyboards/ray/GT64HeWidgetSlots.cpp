#include "GT64HeWidget.h"
#include "Ui_GT64HeWidget.h"
#include "console.h"


void GT64HeWidget::slot_activeWindowChanged(bool active) {
    Console::out() << "activeWindowChanged:" << active << std::endl;
}

void GT64HeWidget::slot_keyboard_key_clicked(int idx, bool checked) {
    Console::out() << "key id" << idx << "checked:" << checked << std::endl;
}
/*********** slots ***********/


void GT64HeWidget::on_param_dbg_btn_usbTx_clicked() {
    if (!device || !device->is_open()) return;
    // 构造测试数据包
    static uint8_t cnt = 0;
    std::vector<uint8_t> buf(GT64HeDevice::EP_SIZE, 0);
    memset(buf.data(), cnt++, GT64HeDevice::EP_SIZE);
    for (uint8_t i = 0; i < 8; i++) {
        buf[i] = 9;
        buf[GT64HeDevice::EP_SIZE - 1 - i] = 9;
    }
    // 通过设备层写入
    device->write_async(buf);
}
void GT64HeWidget::on_param_dbg_btn_usbRx_clicked() {
    if (!device || !device->is_open()) return;
    Console::out() << "usb rx start" << std::endl;
    // 启动连续异步读取
    device->start_read(GT64HeDevice::EP_SIZE,
        [this](const std::vector<uint8_t>& data) {
            on_read_done(data);
        });
}
void GT64HeWidget::on_param_dbg_btn_usbRxStop_clicked() {
    if (!device) return;
    Console::out() << "usb rx stop" << std::endl;
    device->stop_read();
}


void GT64HeWidget::on_param_dbg_btn_setColor_clicked() {
    QColor color = QColorDialog::getColor(Qt::white, nullptr, "pick a color", QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
        QString colorStr = QString("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());
        Console::out() << colorStr.toStdString() << std::endl;
        if (ui->param->dbg->panel->isChecked()) {
            ui->keyboard->layout->m_panel->setStyleSheet("background-color: " + colorStr + ";");
        }
        for (int row = 0; row < ui->keyboard->layout->m_panel->getRowNum(); ++row) {
            for (int key = 0; key < ui->keyboard->layout->m_panel->getKeyNum(row); ++key) {
                MKeyboardKey *kbKey = ui->keyboard->layout->m_panel->getKey(row, key);
                if (kbKey->isChecked()) {
                    if (ui->param->dbg->dft_border->isChecked())            kbKey->set_dft_border_color(color);
                    if (ui->param->dbg->dft_font->isChecked())              kbKey->set_dft_font_color(color);
                    if (ui->param->dbg->hover_font->isChecked())            kbKey->set_hover_font_color(color);
                    if (ui->param->dbg->dft_background->isChecked())        kbKey->set_dft_background_color(color);
                    if (ui->param->dbg->hover_background->isChecked())      kbKey->set_hover_background_color(color);
                    if (ui->param->dbg->checked_background->isChecked())    kbKey->set_checked_background_color(color);
                    if (ui->param->dbg->checked0_background->isChecked())   kbKey->set_checked0_background_color(color);
                    if (ui->param->dbg->checked1_background->isChecked())   kbKey->set_checked1_background_color(color);
                    kbKey->updateStyle();
                }
            }
        }
    }
}

void GT64HeWidget::on_param_dbg_btn_color_panel_clicked() {
    QColor color = QColorDialog::getColor(Qt::white, nullptr, "Panel Background Color", QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
        QString colorStr = QString("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());
        ui->param->dbg->btn_color_panel->setStyleSheet(QString("background-color: %1; border: 1px solid #888888;").arg(colorStr));
        ui->keyboard->layout->m_panel->setStyleSheet("background-color: " + colorStr + ";");
    }
}
void GT64HeWidget::on_param_dbg_btn_color_dft_border_clicked() {
    QColor color = QColorDialog::getColor(Qt::white, nullptr, "Dft Border Color", QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
        QString colorStr = QString("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());
        ui->param->dbg->btn_color_dft_border->setStyleSheet(QString("background-color: %1; border: 1px solid #888888;").arg(colorStr));
        for (int row = 0; row < ui->keyboard->layout->m_panel->getRowNum(); ++row) {
            for (int key = 0; key < ui->keyboard->layout->m_panel->getKeyNum(row); ++key) {
                MKeyboardKey *kbKey = ui->keyboard->layout->m_panel->getKey(row, key);
                if (kbKey->isChecked()) {
                    kbKey->set_dft_border_color(color);
                    kbKey->updateStyle();
                }
            }
        }
    }
}
void GT64HeWidget::on_param_dbg_btn_color_dft_font_clicked() {
    QColor color = QColorDialog::getColor(Qt::white, nullptr, "Dft Font Color", QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
        QString colorStr = QString("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());
        ui->param->dbg->btn_color_dft_font->setStyleSheet(QString("background-color: %1; border: 1px solid #888888;").arg(colorStr));
        for (int row = 0; row < ui->keyboard->layout->m_panel->getRowNum(); ++row) {
            for (int key = 0; key < ui->keyboard->layout->m_panel->getKeyNum(row); ++key) {
                MKeyboardKey *kbKey = ui->keyboard->layout->m_panel->getKey(row, key);
                if (kbKey->isChecked()) {
                    kbKey->set_dft_font_color(color);
                    kbKey->updateStyle();
                }
            }
        }
    }
}
void GT64HeWidget::on_param_dbg_btn_color_hover_font_clicked() {
    QColor color = QColorDialog::getColor(Qt::white, nullptr, "Hover Font Color", QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
        QString colorStr = QString("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());
        ui->param->dbg->btn_color_hover_font->setStyleSheet(QString("background-color: %1; border: 1px solid #888888;").arg(colorStr));
        for (int row = 0; row < ui->keyboard->layout->m_panel->getRowNum(); ++row) {
            for (int key = 0; key < ui->keyboard->layout->m_panel->getKeyNum(row); ++key) {
                MKeyboardKey *kbKey = ui->keyboard->layout->m_panel->getKey(row, key);
                if (kbKey->isChecked()) {
                    kbKey->set_hover_font_color(color);
                    kbKey->updateStyle();
                }
            }
        }
    }
}
void GT64HeWidget::on_param_dbg_btn_color_dft_background_clicked() {
    QColor color = QColorDialog::getColor(Qt::white, nullptr, "Dft Background Color", QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
        QString colorStr = QString("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());
        ui->param->dbg->btn_color_dft_background->setStyleSheet(QString("background-color: %1; border: 1px solid #888888;").arg(colorStr));
        for (int row = 0; row < ui->keyboard->layout->m_panel->getRowNum(); ++row) {
            for (int key = 0; key < ui->keyboard->layout->m_panel->getKeyNum(row); ++key) {
                MKeyboardKey *kbKey = ui->keyboard->layout->m_panel->getKey(row, key);
                if (kbKey->isChecked()) {
                    kbKey->set_dft_background_color(color);
                    kbKey->updateStyle();
                }
            }
        }
    }
}
void GT64HeWidget::on_param_dbg_btn_color_hover_background_clicked() {
    QColor color = QColorDialog::getColor(Qt::white, nullptr, "Hover Background Color", QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
        QString colorStr = QString("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());
        ui->param->dbg->btn_color_hover_background->setStyleSheet(QString("background-color: %1; border: 1px solid #888888;").arg(colorStr));
        for (int row = 0; row < ui->keyboard->layout->m_panel->getRowNum(); ++row) {
            for (int key = 0; key < ui->keyboard->layout->m_panel->getKeyNum(row); ++key) {
                MKeyboardKey *kbKey = ui->keyboard->layout->m_panel->getKey(row, key);
                if (kbKey->isChecked()) {
                    kbKey->set_hover_background_color(color);
                    kbKey->updateStyle();
                }
            }
        }
    }
}
void GT64HeWidget::on_param_dbg_btn_color_checked_background_clicked() {
    QColor color = QColorDialog::getColor(Qt::white, nullptr, "Checked Background Color", QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
        QString colorStr = QString("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());
        ui->param->dbg->btn_color_checked_background->setStyleSheet(QString("background-color: %1; border: 1px solid #888888;").arg(colorStr));
        for (int row = 0; row < ui->keyboard->layout->m_panel->getRowNum(); ++row) {
            for (int key = 0; key < ui->keyboard->layout->m_panel->getKeyNum(row); ++key) {
                MKeyboardKey *kbKey = ui->keyboard->layout->m_panel->getKey(row, key);
                if (kbKey->isChecked()) {
                    kbKey->set_checked_background_color(color);
                    kbKey->updateStyle();
                }
            }
        }
    }
}
void GT64HeWidget::on_param_dbg_btn_color_checked0_background_clicked() {
    QColor color = QColorDialog::getColor(Qt::white, nullptr, "Checked0 Background Color", QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
        QString colorStr = QString("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());
        ui->param->dbg->btn_color_checked0_background->setStyleSheet(QString("background-color: %1; border: 1px solid #888888;").arg(colorStr));
        for (int row = 0; row < ui->keyboard->layout->m_panel->getRowNum(); ++row) {
            for (int key = 0; key < ui->keyboard->layout->m_panel->getKeyNum(row); ++key) {
                MKeyboardKey *kbKey = ui->keyboard->layout->m_panel->getKey(row, key);
                if (kbKey->isChecked()) {
                    kbKey->set_checked0_background_color(color);
                    kbKey->updateStyle();
                }
            }
        }
    }
}
void GT64HeWidget::on_param_dbg_btn_color_checked1_background_clicked() {
    QColor color = QColorDialog::getColor(Qt::white, nullptr, "Checked1 Background Color", QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
        QString colorStr = QString("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());
        ui->param->dbg->btn_color_checked1_background->setStyleSheet(QString("background-color: %1; border: 1px solid #888888;").arg(colorStr));
        for (int row = 0; row < ui->keyboard->layout->m_panel->getRowNum(); ++row) {
            for (int key = 0; key < ui->keyboard->layout->m_panel->getKeyNum(row); ++key) {
                MKeyboardKey *kbKey = ui->keyboard->layout->m_panel->getKey(row, key);
                if (kbKey->isChecked()) {
                    kbKey->set_checked1_background_color(color);
                    kbKey->updateStyle();
                }
            }
        }
    }
}
void GT64HeWidget::on_statebar_btn_exitWidget_clicked() {
    closeWidget();
}
void GT64HeWidget::on_fastLocation_btn_game_clicked() {
    ui->keyboard->layout->m_panel->setCheckedType(MKeyboardPanel::CHECKED_TYPE_GAME);
}
void GT64HeWidget::on_fastLocation_btn_letter_clicked() {
    ui->keyboard->layout->m_panel->setCheckedType(MKeyboardPanel::CHECKED_TYPE_LETTER);
}
void GT64HeWidget::on_fastLocation_btn_number_clicked() {
    ui->keyboard->layout->m_panel->setCheckedType(MKeyboardPanel::CHECKED_TYPE_NUMBER);
}
void GT64HeWidget::on_fastLocation_btn_invert_clicked() {
    ui->keyboard->layout->m_panel->setCheckedType(MKeyboardPanel::CHECKED_TYPE_INVERT);
}
void GT64HeWidget::on_fastLocation_btn_all_clicked() {
    ui->keyboard->layout->m_panel->setCheckedType(MKeyboardPanel::CHECKED_TYPE_ALL);
}
void GT64HeWidget::on_fastLocation_btn_cancel_clicked() {
    ui->keyboard->layout->m_panel->setCheckedType(MKeyboardPanel::CHECKED_TYPE_CANCEL);
}
void GT64HeWidget::on_param_property_radioButtonGroup_buttonToggled(int id, bool checked) {
    Console::out() << "编号为 " << id << "选择状态变更为 " << checked << std::endl;
    if (id < ui->param->property->stackedWidget->count()) {
        ui->param->property->stackedWidget->setCurrentIndex(id);
    }
}
void GT64HeWidget::on_param_btn_prev_clicked() {
    int idx = (ui->param->tabWidget->currentIndex() - 1 + ui->param->tabWidget->count()) % ui->param->tabWidget->count();
    ui->param->tabWidget->setCurrentIndex(idx);
}
void GT64HeWidget::on_param_btn_next_clicked() {
    int idx = (ui->param->tabWidget->currentIndex() + 1) % ui->param->tabWidget->count();
    ui->param->tabWidget->setCurrentIndex(idx);
}

void GT64HeWidget::on_param_property_slider_param0_userReleased(int value) {
    ui->param->property->spinBox_param[0]->setValue(value / 1000.0);
}
void GT64HeWidget::on_param_property_slider_param1_userReleased(int value) {
    ui->param->property->spinBox_param[1]->setValue(value / 1000.0);
}
void GT64HeWidget::on_param_property_slider_param2_userReleased(int value) {
    ui->param->property->spinBox_param[2]->setValue(value / 1000.0);
}
void GT64HeWidget::on_param_property_slider_param3_userReleased(int value) {
    ui->param->property->spinBox_param[3]->setValue(value / 1000.0);
}
void GT64HeWidget::on_param_property_slider_param4_userReleased(int value) {
    ui->param->property->spinBox_param[4]->setValue(value / 1000.0);
}
void GT64HeWidget::on_param_property_slider_param5_userReleased(int value) {
    ui->param->property->spinBox_param[5]->setValue(value / 1000.0);
}

void GT64HeWidget::on_param_property_spinBox_param0_valueChanged(double value) {
    ui->param->property->slider_param[0]->setSliderPosition(value * 1000.0);
    Console::out() << "TODO: 0 send usb message" << value << std::endl;
}
void GT64HeWidget::on_param_property_spinBox_param1_valueChanged(double value) {
    ui->param->property->slider_param[1]->setSliderPosition(value * 1000.0);
    Console::out() << "TODO: 1 send usb message" << value << std::endl;
}
void GT64HeWidget::on_param_property_spinBox_param2_valueChanged(double value) {
    ui->param->property->slider_param[2]->setSliderPosition(value * 1000.0);
    Console::out() << "TODO: 2 send usb message" << value << std::endl;
}
void GT64HeWidget::on_param_property_spinBox_param3_valueChanged(double value) {
    ui->param->property->slider_param[3]->setSliderPosition(value * 1000.0);
    Console::out() << "TODO: 3 send usb message" << value << std::endl;
}
void GT64HeWidget::on_param_property_spinBox_param4_valueChanged(double value) {
    ui->param->property->slider_param[4]->setSliderPosition(value * 1000.0);
    Console::out() << "TODO: 4 send usb message" << value << std::endl;
}
void GT64HeWidget::on_param_property_spinBox_param5_valueChanged(double value) {
    ui->param->property->slider_param[5]->setSliderPosition(value * 1000.0);
    Console::out() << "TODO: 5 send usb message" << value << std::endl;
}
