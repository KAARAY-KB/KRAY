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
        emit clicked(m_msg.id, checked);
    });

    // 加载 qss 样式表文件到成员变量 m_qss
    QString qss_path(":/styles/kb_key.qss");
    QFile qss_file(qss_path);
    if (qss_file.open(QFile::ReadOnly)) {
        m_qss = QLatin1String(qss_file.readAll());
        qss_file.close();
    }
    else {
        Console::out() << "error opening the file: " << qss_path.toStdString() << std::endl;
    }
    setStyleSheet(m_qss);
    // Console::out() << m_qss.toStdString() << std::endl;



    // dft_border_color = get_dft_border_color();
    // dft_font_color = get_dft_font_color();
    // hover_font_color = get_hover_font_color();
    // dft_background_color = get_dft_background_color();
    // hover_background_color = get_hover_background_color();
    // checked_background_color = get_checked_background_color();
    // checked0_background_color = get_checked0_background_color();
    // checked1_background_color = get_checked1_background_color();



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
#if 0
    // QString qss = m_qss; // 生成样式表（从当前 styleSheet 重新生成，保持一致性）

    m_qss = set_dft_border_color(m_qss, dft_border_color);
    m_qss = set_dft_background_color(m_qss, dft_background_color);
    m_qss = set_hover_font_color(m_qss, hover_font_color);
    m_qss = set_hover_background_color(m_qss, hover_background_color);
    m_qss = set_checked_background_color(m_qss, checked_background_color);
    m_qss = set_checked0_background_color(m_qss, checked0_background_color);
    m_qss = set_checked1_background_color(m_qss, checked1_background_color);

    Console::out() << m_qss.toStdString() << std::endl;
    return m_qss;
#else
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
#endif
}

void MKeyboardKey::paintEvent(QPaintEvent *event) {
    QPushButton::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 获取按钮的高度和宽度
    int height = this->height();
    int width = this->width();
    int fillHeight = m_distCur / m_distMax * height;

    // 解析样式表边框宽度和圆角半径
    int borderRadius = parseStyleInt(this->styleSheet(), "QPushButton", "border-radius") - 2; // 微调圆角半径
    int borderWidth = parseStyleInt(this->styleSheet(), "QPushButton", "border-width");

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



// 从样式表字符串中提取指定选择器内的属性值
// 返回属性值字符串，未找到返回空
static QString extractStyleProp(const QString &qss, const QString &selector, const QString &prop)
{
    // 移除注释
    QString clean = qss;
    clean.remove(QRegularExpression(R"(/\*.*?\*/)"));

    // 匹配选择器块内容
    QRegularExpression selRe(QRegularExpression::escape(selector) + R"(\s*\{([^}]*)\})");
    QRegularExpressionMatch selMatch = selRe.match(clean);
    if (!selMatch.hasMatch()) return QString();

    QString block = selMatch.captured(1);

    // 匹配属性值
    QRegularExpression propRe(QRegularExpression::escape(prop) + R"(\s*:\s*([^;]+);)");
    QRegularExpressionMatch propMatch = propRe.match(block);
    if (!propMatch.hasMatch()) return QString();

    return propMatch.captured(1).trimmed();
}

// 解析颜色字符串为 QColor
static QColor strToColor(const QString &str)
{
    if (str.isEmpty()) return QColor();

    QString s = str.trimmed();

    // #RGB / #RRGGBB / #RRGGBBAA
    if (s.startsWith('#')) {
        return QColor(s);
    }

    // rgb(r, g, b)
    QRegularExpression reRgb(R"(rgb\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\))");
    QRegularExpressionMatch m = reRgb.match(s);
    if (m.hasMatch()) {
        return QColor(m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt());
    }

    // rgba(r, g, b, a)
    QRegularExpression reRgba(R"(rgba\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\))");
    m = reRgba.match(s);
    if (m.hasMatch()) {
        return QColor(m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt(), m.captured(4).toInt());
    }

    return QColor(s);
}

// 从样式表解析颜色
QColor MKeyboardKey::parseStyleColor(const QString &qss, const QString &selector, const QString &prop)
{
    QString val = extractStyleProp(qss, selector, prop);
    return strToColor(val);
}

// 从样式表解析整数
int MKeyboardKey::parseStyleInt(const QString &qss, const QString &selector, const QString &prop)
{
    QString val = extractStyleProp(qss, selector, prop);
    // 移除 "px" 后缀
    val.remove("px");
    bool ok = false;
    int result = val.toInt(&ok);
    return ok ? result : 0;
}

// 从样式表解析字符串
QString MKeyboardKey::parseStyleString(const QString &qss, const QString &selector, const QString &prop)
{
    return extractStyleProp(qss, selector, prop);
}
// 颜色转换为字符串
QString MKeyboardKey::colorToStr(QColor color)
{
    if (color.alpha() < 255)
        return QString("rgba(%1, %2, %3, %4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
    return QString("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());
}

// --- 颜色 getter：从当前 styleSheet() 解析 ---
QColor MKeyboardKey::get_dft_border_color()          { return parseStyleColor(m_qss, "QPushButton", "border-color"); }
QColor MKeyboardKey::get_dft_font_color()            { return parseStyleColor(m_qss, "QPushButton", "color"); }
QColor MKeyboardKey::get_hover_font_color()          { return parseStyleColor(m_qss, "QPushButton:hover", "color"); }
QColor MKeyboardKey::get_dft_background_color()      { return parseStyleColor(m_qss, "QPushButton", "background-color"); }
QColor MKeyboardKey::get_hover_background_color()    { return parseStyleColor(m_qss, "QPushButton:hover", "background-color"); }
QColor MKeyboardKey::get_checked_background_color()  { return parseStyleColor(m_qss, "QPushButton:checked", "background-color"); }
QColor MKeyboardKey::get_checked0_background_color() { return parseStyleColor(m_qss, "QPushButton:pressed:!checked", "background-color"); }
QColor MKeyboardKey::get_checked1_background_color() { return parseStyleColor(m_qss, "QPushButton:pressed:checked", "background-color"); }

// --- 布局 getter：从当前 styleSheet() 解析 ---
int MKeyboardKey::get_border_width()     { return parseStyleInt(m_qss, "QPushButton", "border-width"); }
int MKeyboardKey::get_border_radius()    { return parseStyleInt(m_qss, "QPushButton", "border-radius"); }
int MKeyboardKey::get_font_size()        { return parseStyleInt(m_qss, "QPushButton", "font-size"); }
int MKeyboardKey::get_hover_font_size()  { return parseStyleInt(m_qss, "QPushButton:hover", "font-size"); }
int MKeyboardKey::get_padding()          { return parseStyleInt(m_qss, "QPushButton", "padding"); }

// --- 字体 getter：从当前 styleSheet() 解析 ---
QString MKeyboardKey::get_font_family()  { return parseStyleString(m_qss, "QPushButton", "font-family"); }

// --- 颜色 setter：修改样式表中对应属性值 ---
QString MKeyboardKey::set_dft_border_color(QString qss, QColor color)
{
    QRegularExpression re("(QPushButton\\s*\\{[^}]*border-color\\s*:\\s*)[^;]+(;)");
    qss.replace(re, "\\1" + colorToStr(color) + "\\2");
    // setStyleSheet(qss);
    return qss;
}

QString MKeyboardKey::set_dft_font_color(QString qss, QColor color)
{
    QRegularExpression re("(QPushButton\\s*\\{[^}]*color\\s*:\\s*)[^;]+(;)");
    qss.replace(re, "\\1" + colorToStr(color) + "\\2");
    return qss;
}

QString MKeyboardKey::set_hover_font_color(QString qss, QColor color)
{
    QRegularExpression re("(QPushButton:hover\\s*\\{[^}]*color\\s*:\\s*)[^;]+(;)");
    qss.replace(re, "\\1" + colorToStr(color) + "\\2");
    return qss;
}

QString MKeyboardKey::set_dft_background_color(QString qss, QColor color)
{
    QRegularExpression re("(QPushButton\\s*\\{[^}]*background-color\\s*:\\s*)[^;]+(;)");
    qss.replace(re, "\\1" + colorToStr(color) + "\\2");
    return qss;
}

QString MKeyboardKey::set_hover_background_color(QString qss, QColor color)
{
    QRegularExpression re("(QPushButton:hover\\s*\\{[^}]*background-color\\s*:\\s*)[^;]+(;)");
    qss.replace(re, "\\1" + colorToStr(color) + "\\2");
    return qss;
}

QString MKeyboardKey::set_checked_background_color(QString qss, QColor color)
{
    QRegularExpression re("(QPushButton:checked\\s*\\{[^}]*background-color\\s*:\\s*)[^;]+(;)");
    qss.replace(re, "\\1" + colorToStr(color) + "\\2");
    return qss;
}

QString MKeyboardKey::set_checked0_background_color(QString qss, QColor color)
{
    QRegularExpression re("(QPushButton:pressed:!checked\\s*\\{[^}]*background-color\\s*:\\s*)[^;]+(;)");
    qss.replace(re, "\\1" + colorToStr(color) + "\\2");
    return qss;
}

QString MKeyboardKey::set_checked1_background_color(QString qss, QColor color)
{
    QRegularExpression re("(QPushButton:pressed:checked\\s*\\{[^}]*background-color\\s*:\\s*)[^;]+(;)");
    qss.replace(re, "\\1" + colorToStr(color) + "\\2");
    return qss;
}

// --- 布局 setter：修改样式表中对应属性值 ---
QString MKeyboardKey::set_border_width(QString qss, int w)
{
    QRegularExpression re("(QPushButton\\s*\\{[^}]*border-width\\s*:\\s*)[^;]+(;)");
    qss.replace(re, "\\1" + QString::number(w) + "px\\2");
    return qss;
}

QString MKeyboardKey::set_border_radius(QString qss, int r)
{
    QRegularExpression re("(QPushButton\\s*\\{[^}]*border-radius\\s*:\\s*)[^;]+(;)");
    qss.replace(re, "\\1" + QString::number(r) + "px\\2");
    return qss;
}

QString MKeyboardKey::set_font_size(QString qss, int s)
{
    QRegularExpression re("(QPushButton\\s*\\{[^}]*font-size\\s*:\\s*)[^;]+(;)");
    qss.replace(re, "\\1" + QString::number(s) + "px\\2");
    return qss;
}

QString MKeyboardKey::set_hover_font_size(QString qss, int s)
{
    QRegularExpression re("(QPushButton:hover\\s*\\{[^}]*font-size\\s*:\\s*)[^;]+(;)");
    qss.replace(re, "\\1" + QString::number(s) + "px\\2");
    return qss;
}

QString MKeyboardKey::set_padding(QString qss, int p)
{
    QRegularExpression re("(QPushButton\\s*\\{[^}]*padding\\s*:\\s*)[^;]+(;)");
    qss.replace(re, "\\1" + QString::number(p) + "px\\2");
    return qss;
}

QString MKeyboardKey::set_font_family(QString qss, const QString &f)
{
    QRegularExpression re("(QPushButton\\s*\\{[^}]*font-family\\s*:\\s*)[^;]+(;)");
    qss.replace(re, "\\1" + f + "\\2");
    return qss;
}
