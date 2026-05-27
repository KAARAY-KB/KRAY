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

    // 加载 style 样式表文件到成员变量 m_style
    QString qss_path(":/styles/kb_key.qss");
    QFile qss_file(qss_path);
    if (qss_file.open(QFile::ReadOnly)) {
        m_style = QLatin1String(qss_file.readAll());
        qss_file.close();
        // 移除 QSS 中的 /* ... */ 注释
        // 避免正则替换颜色时匹配到注释中的同名属性
        QRegularExpression commentRe(R"(/\*.*?\*/)");
        commentRe.setPatternOptions(QRegularExpression::DotMatchesEverythingOption);
        m_style.remove(commentRe);
    }
    else {
        Console::out() << "error opening the file: " << qss_path.toStdString() << std::endl;
    }
    setStyleSheet(m_style);
    // Console::out() << m_style.toStdString() << std::endl;


    // 从 QSS 文件内容初始化颜色成员变量，保持与 QSS 文件同步
    // 这样后续 getStyle() 用成员变量替换 m_style 中的颜色时，初始值一致
    m_border_color = styleToColor("QPushButton", "border-color");
    m_font_color = styleToColor("QPushButton", "color");
    m_background_color = styleToColor("QPushButton", "background-color");
    m_hover_font_color = styleToColor("QPushButton:hover", "color");
    m_hover_background_color = styleToColor("QPushButton:hover", "background-color");
    m_checked_background_color = styleToColor("QPushButton:checked", "background-color");
    m_pressed_not_checked_background_color = styleToColor("QPushButton:pressed:!checked", "background-color");
    m_pressed_checked_background_color = styleToColor("QPushButton:pressed:checked", "background-color");

    m_border_width = styleToInt("QPushButton", "border-width");
    m_border_radius = styleToInt("QPushButton", "border-radius");
    m_font_size = styleToInt("QPushButton", "font-size");
    m_hover_font_size = styleToInt("QPushButton:hover", "font-size");
    m_padding = styleToInt("QPushButton", "padding");
    m_font_family = styleToStr("QPushButton", "font-family");

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
#if 1
    // 以 m_qss（QSS 文件内容）为基准，只替换颜色属性
    // 避免硬编码整个样式表，保证 QSS 文件中的其他属性不被覆盖
    // set_border_color(m_border_color);
    // set_font_color(m_font_color);
    // set_background_color(m_background_color);
    // set_hover_font_color(m_hover_font_color);
    // set_hover_background_color(m_hover_background_color);
    // set_checked_background_color(m_checked_background_color);
    // set_pressed_not_checked_background_color(m_pressed_not_checked_background_color);
    // set_pressed_checked_background_color(m_pressed_checked_background_color);

    // set_border_width(m_border_width);
    // set_border_radius(m_border_radius);
    // set_font_size(m_font_size);
    // set_hover_font_size(m_hover_font_size);
    // set_padding(m_padding);
    // set_font_family(m_font_family);
    
    if (getId() == 0)
        Console::out() << m_style.toStdString() << std::endl;
    return m_style;
#else
    QString res = QString(
        "QPushButton {"
            "color: "+m_font_color+";"
            "font-family: Consolas;"
            "font-size: 14px;"
            "font-weight: normal;"
            "padding: 0px;"
            "border-width:  2px;"
            "border-radius: 8px;"
            "border-style:  outset;"
            "border-color:"+m_border_color+";"
            "background-color:"+m_background_color+";"
        "}"
        "QPushButton:hover {"
            "color: "+m_hover_font_color+";"
            "font-size: 13px;"
            "border-style:  inset;"
            "background-color:"+m_hover_background_color+";"
        "}"
        "QPushButton:checked {"
            "background-color:"+m_checked_background_color+";"
        "}"
        "QPushButton:pressed:checked {"
            "background-color:"+m_pressed_checked_background_color+";"
        "}"
        "QPushButton:pressed:!checked {"
            "background-color:"+m_pressed_not_checked_background_color+";"
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
    int w = this->width();
    int h = this->height();

    // 解析样式表边框宽度和圆角半径
    int bw = m_border_width;
    int br = m_border_radius;

    if (m_distCur > 0) {
        int fh = static_cast<int>(m_distCur / m_distMax * h);
        // 定义内容区域（减去边框宽度）
        QRect contentRect(bw, bw, w - 2 * bw, h - 2 * bw);

        // 定义填充区域（从顶部向下填充）
        QRect fillRect(contentRect.x(), contentRect.y(), contentRect.width(), fh);

        // 定义圆角矩形路径
        QPainterPath roundedRectPath;
        roundedRectPath.addRoundedRect(contentRect, br, br);

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
}


// 颜色转换为字符串
QString MKeyboardKey::colorToStr(QColor color)
{
    if (color.alpha() < 255)
        return QString("rgba(%1, %2, %3, %4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
    return QString("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());
}

// 解析颜色字符串为 QColor
QColor MKeyboardKey::strToColor(const QString &str)
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
    // rgba(r, g, b, a) — a 可以是浮点数(0.0~1.0)或整数(0~255)
    QRegularExpression reRgba(R"(rgba\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*([0-9.]+)\s*\))");
    m = reRgba.match(s);
    if (m.hasMatch()) {
        double alpha = m.captured(4).toDouble();
        // CSS 规范：alpha ≤ 1.0 为 0.0~1.0 范围，需转为 0~255
        int alphaInt = (alpha <= 1.0) ? qRound(alpha * 255) : qRound(alpha);
        return QColor(m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt(), alphaInt);
    }
    return QColor(s);
}

// 从样式表字符串中提取指定选择器内的属性值
// 返回属性值字符串，未找到返回空
QString MKeyboardKey::extractStyleAttr(const QString &style, const QString &selector, const QString &prop)
{
    // 移除注释
    QString clean = style;
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

// 从样式表解析颜色
QColor MKeyboardKey::styleToColor(const QString &selector, const QString &prop)
{
    QString val = extractStyleAttr(m_style, selector, prop);
    return strToColor(val);
}

// 从样式表解析整数
int MKeyboardKey::styleToInt(const QString &selector, const QString &prop)
{
    QString val = extractStyleAttr(m_style, selector, prop);
    // 移除 "px" 后缀
    val.remove("px");
    bool ok = false;
    int result = val.toInt(&ok);
    return ok ? result : 0;
}

// 从样式表解析字符串
QString MKeyboardKey::styleToStr(const QString &selector, const QString &prop)
{
    return extractStyleAttr(m_style, selector, prop);
}

// --- 颜色 getter：从当前 styleSheet() 解析 ---
QColor MKeyboardKey::get_border_color()          { return styleToColor("QPushButton", "border-color"); }
QColor MKeyboardKey::get_font_color()            { return styleToColor("QPushButton", "color"); }
QColor MKeyboardKey::get_hover_font_color()          { return styleToColor("QPushButton:hover", "color"); }
QColor MKeyboardKey::get_background_color()      { return styleToColor("QPushButton", "background-color"); }
QColor MKeyboardKey::get_hover_background_color()    { return styleToColor("QPushButton:hover", "background-color"); }
QColor MKeyboardKey::get_checked_background_color()  { return styleToColor("QPushButton:checked", "background-color"); }
QColor MKeyboardKey::get_pressed_not_checked_background_color() { return styleToColor("QPushButton:pressed:!checked", "background-color"); }
QColor MKeyboardKey::get_pressed_checked_background_color() { return styleToColor("QPushButton:pressed:checked", "background-color"); }

// --- 布局 getter：从当前 styleSheet() 解析 ---
int MKeyboardKey::get_border_width()     { return styleToInt("QPushButton", "border-width"); }
int MKeyboardKey::get_border_radius()    { return styleToInt("QPushButton", "border-radius"); }
int MKeyboardKey::get_font_size()        { return styleToInt("QPushButton", "font-size"); }
int MKeyboardKey::get_hover_font_size()  { return styleToInt("QPushButton:hover", "font-size"); }
int MKeyboardKey::get_padding()          { return styleToInt("QPushButton", "padding"); }

// --- 字体 getter：从当前 styleSheet() 解析 ---
QString MKeyboardKey::get_font_family()  { return styleToStr("QPushButton", "font-family"); }

// --- 颜色 setter：修改样式表中对应属性值 ---
QString MKeyboardKey::set_border_color(QColor color) {
    m_border_color = color;
    QRegularExpression re("(QPushButton\\s*\\{[^}]*border-color\\s*:\\s*)[^;]+(;)");
    m_style.replace(re, "\\1" + colorToStr(color) + "\\2");
    // setStyleSheet(style);
    return m_style;
}

QString MKeyboardKey::set_font_color(QColor color) {
    m_font_color = color;
    QRegularExpression re("(QPushButton\\s*\\{[^}]*color\\s*:\\s*)[^;]+(;)");
    m_style.replace(re, "\\1" + colorToStr(color) + "\\2");
    return m_style;
}

QString MKeyboardKey::set_hover_font_color(QColor color) {
    m_hover_font_color = color;
    QRegularExpression re("(QPushButton:hover\\s*\\{[^}]*color\\s*:\\s*)[^;]+(;)");
    m_style.replace(re, "\\1" + colorToStr(color) + "\\2");
    return m_style;
}

QString MKeyboardKey::set_background_color(QColor color) {
    m_background_color = color;
    QRegularExpression re("(QPushButton\\s*\\{[^}]*background-color\\s*:\\s*)[^;]+(;)");
    m_style.replace(re, "\\1" + colorToStr(color) + "\\2");
    return m_style;
}

QString MKeyboardKey::set_hover_background_color(QColor color) {
    m_hover_background_color = color;
    QRegularExpression re("(QPushButton:hover\\s*\\{[^}]*background-color\\s*:\\s*)[^;]+(;)");
    m_style.replace(re, "\\1" + colorToStr(color) + "\\2");
    return m_style;
}

QString MKeyboardKey::set_checked_background_color(QColor color) {
    m_checked_background_color = color;
    QRegularExpression re("(QPushButton:checked\\s*\\{[^}]*background-color\\s*:\\s*)[^;]+(;)");
    m_style.replace(re, "\\1" + colorToStr(color) + "\\2");
    return m_style;
}

QString MKeyboardKey::set_pressed_not_checked_background_color(QColor color) {
    m_pressed_not_checked_background_color = color;
    QRegularExpression re("(QPushButton:pressed:!checked\\s*\\{[^}]*background-color\\s*:\\s*)[^;]+(;)");
    m_style.replace(re, "\\1" + colorToStr(color) + "\\2");
    return m_style;
}

QString MKeyboardKey::set_pressed_checked_background_color(QColor color) {
    m_pressed_checked_background_color = color;
    QRegularExpression re("(QPushButton:pressed:checked\\s*\\{[^}]*background-color\\s*:\\s*)[^;]+(;)");
    m_style.replace(re, "\\1" + colorToStr(color) + "\\2");
    return m_style;
}

// --- 布局 setter：修改样式表中对应属性值 ---
QString MKeyboardKey::set_border_width(int w) {
    m_border_width = w;
    QRegularExpression re("(QPushButton\\s*\\{[^}]*border-width\\s*:\\s*)[^;]+(;)");
    m_style.replace(re, "\\1" + QString::number(w) + "px\\2");
    return m_style;
}

QString MKeyboardKey::set_border_radius(int r) {
    m_border_radius = r;
    QRegularExpression re("(QPushButton\\s*\\{[^}]*border-radius\\s*:\\s*)[^;]+(;)");
    m_style.replace(re, "\\1" + QString::number(r) + "px\\2");
    return m_style;
}

QString MKeyboardKey::set_font_size(int s) {
    m_font_size = s;
    QRegularExpression re("(QPushButton\\s*\\{[^}]*font-size\\s*:\\s*)[^;]+(;)");
    m_style.replace(re, "\\1" + QString::number(s) + "px\\2");
    return m_style;
}

QString MKeyboardKey::set_hover_font_size(int s) {
    m_hover_font_size = s;
    QRegularExpression re("(QPushButton:hover\\s*\\{[^}]*font-size\\s*:\\s*)[^;]+(;)");
    m_style.replace(re, "\\1" + QString::number(s) + "px\\2");
    return m_style;
}

QString MKeyboardKey::set_padding(int p) {
    m_padding = p;
    QRegularExpression re("(QPushButton\\s*\\{[^}]*padding\\s*:\\s*)[^;]+(;)");
    m_style.replace(re, "\\1" + QString::number(p) + "px\\2");
    return m_style;
}

QString MKeyboardKey::set_font_family(const QString &f) {
    m_font_family = f;
    QRegularExpression re("(QPushButton\\s*\\{[^}]*font-family\\s*:\\s*)[^;]+(;)");
    m_style.replace(re, "\\1" + f + "\\2");
    return m_style;
}
