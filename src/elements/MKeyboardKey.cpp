#include "MKeyboardKey.h"
#include "console.h"

MKeyboardKey::MKeyboardKey(msg_t &msg, int base_w, int base_h, QWidget *parent)
    : QPushButton(parent)
    , m_msg(msg)
    , m_base_w(base_w)
    , m_base_h(base_h)
    , m_distMax(4.0)
    , m_distCur(0)
    , m_dist_dir(DIR_D2U)
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
    QString qss_path(":/styles/kb_key.qss");
    QFile qss_file(qss_path);
    if (qss_file.open(QFile::ReadOnly)) {
        style = QLatin1String(qss_file.readAll());
        qss_file.close();
    }
    else {
        Console::out() << "error opening the file: " << qss_path.toStdString() << std::endl;
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



void MKeyboardKey::paintEvent(QPaintEvent *event) {
    QPushButton::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 获取按钮的高度和宽度
    int height = this->height();
    int width = this->width();
    int fillHeight = m_distCur / m_distMax * height;

    // 解析样式表获取边框宽度和圆角半径
    int borderRadius = parseBorderRadiusFromStyleSheet() - 2; // 微调圆角半径
    int borderWidth = parseBorderWidthFromStyleSheet();

    // 定义内容区域（减去边框宽度）
    QRect contentRect(borderWidth, borderWidth, width - 2 * borderWidth, height - 2 * borderWidth);

    // 定义填充区域（从顶部向下填充）
    QRect fillRect(contentRect.x(), contentRect.y(), contentRect.width(), fillHeight);
    // QRect fillRect(0, 0, width, fillHeight); //底开始填充
    // QRect fillRect(0, height - fillHeight, width, fillHeight); //顶开始填充

    // 定义圆角矩形路径
    QPainterPath roundedRectPath;
    roundedRectPath.addRoundedRect(contentRect, borderRadius, borderRadius);

    // 限制绘制范围到内容区域
    painter.setClipPath(roundedRectPath);
    if (m_dist_dir == DIR_D2U) {
        // rgba(242, 223, 201, 0.5)
        painter.fillRect(fillRect, QColor(242, 221, 199, 150));
    }
    else {
        // rgba(186, 209, 243, 0.5)
        painter.fillRect(fillRect, QColor(186, 209, 243, 150));
    }

}
// 从样式表中解析边框宽度
int MKeyboardKey::parseBorderWidthFromStyleSheet() {
    QString styleSheet = this->styleSheet();
    QRegularExpression regex(R"(border-width:\s*(\d+)px)");
    QRegularExpressionMatch match = regex.match(styleSheet);
    if (match.hasMatch()) {
        return match.captured(1).toInt();
    }
    return 1; // 默认值
}

// 从样式表中解析圆角半径
int MKeyboardKey::parseBorderRadiusFromStyleSheet() {
    QString styleSheet = this->styleSheet();
    QRegularExpression regex(R"(border-radius:\s*(\d+)px)");
    QRegularExpressionMatch match = regex.match(styleSheet);
    if (match.hasMatch()) {
        return match.captured(1).toInt();
    }
    return 0; // 默认值
}