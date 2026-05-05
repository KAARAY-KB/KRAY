#include "MKeyboardKey.h"
#include <QDebug>

MKeyboardKey::MKeyboardKey(msg_t &msg, int base_w, int base_h, QWidget *parent)
    : QPushButton(parent)
    , m_msg(msg)
    , m_base_w(base_w)
    , m_base_h(base_h)
{
    int w = m_base_w * m_msg.w_unit;
    int h = m_base_h;
    
    setCheckable(true);
    setText(m_msg.text);
    setMinimumSize(w, h);
    setMaximumSize(w, h); 
    setFocusPolicy(Qt::NoFocus);
    connect(this, &MKeyboardKey::toggled, this, [this](bool checked) {
        emit clicked(m_msg.seq, checked);
    });

    QString style;
    QString qss_path(":/resources/style/kb_key.qss");
    QFile qss_file(qss_path);
    if (qss_file.open(QFile::ReadOnly)) {
        style = QLatin1String(qss_file.readAll());
        qss_file.close();
    }
    else {
        qDebug() << "error opening the file: " << qss_path << qss_file.errorString();
    }
    setStyleSheet(style);
    // updateStyle();
}

/*
 * #e79796,  
 * #c39cce 
 * 
 * #bcd9c8, rgb(188, 217, 200) 薄荷绿
 * 
 * #e1c4e7 粉紫色
 * #bad1f3 粉蓝色
 * 
 * #bebebe, rgb(190, 190, 190) 浅灰色
 * #f2ddc7, rgb(242, 221, 199) 浅杏色
 * #efc7c1, rgb(239, 199, 193) 淡粉色
 * #f9ebbc, rgb(249, 235, 188) 淡黄色
 * #cec3f3, rgb(206, 195, 243) 淡紫色
 * #a5b5d9, rgb(165, 181, 217) 烟紫色
 * 
 * 
 */
QString MKeyboardKey::getStyle() {
    QString res = QString(
        "QPushButton {"
            "color: #353535;"
            "font-family: Consolas;"
            "font-size: 14px;"
            "font-weight: normal;"
            "padding: 0px;"
            "border-width:  2px;"
            "border-radius: 8px;"
            "border-style:  outset;"
            "border-color:"+dft_border_color+";"
            "background-color:"+dft_background_color+";"
        "}"
        "QPushButton:hover {"
            "color: "+hover_font_color+";"
            "font-size: 13px;"
            "border-style:  inset;"
            "background-color:"+hover_background_color+";"
        "}"
        "QPushButton:checked {"
            "background-color:"+checked_background_color+";"
        "}"
        "QPushButton:pressed:checked {"
            "background-color:"+checked1_background_color+";"
        "}"
        "QPushButton:pressed:!checked {"
            "background-color:"+checked0_background_color+";"
        "}"
     );
     return res;
}
