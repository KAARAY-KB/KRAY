#include "MKeyboardKey.h"
#include "console.h"
#include <cmath>

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
    // 禁止 Qt 自动填充背景，由 paintEvent 自绘
    setAutoFillBackground(false);
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
        Console::error("MKeyboardKey") << "opening the file: " << qss_path.toStdString() << std::endl;
    }
    // 从 QSS 文件内容初始化颜色成员变量，保持与 QSS 文件同步
    // 必须在 updateStyle() 之前初始化，因为 paintEvent 使用这些成员变量
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

    // 用 updateStyle() 设置样式（自动将 background-color 替换为 transparent）
    updateStyle();
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
        if (Console::shouldLog("MKeyboardKey", 1000))
            Console::info("MKeyboardKey") << m_style.toStdString() << std::endl;
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

// 刷新样式到控件，并同步更新 m_style
// 将 background-color 和 border 替换，避免 Qt stylesheet 自绘锯齿圆角
// 背景和边框由 paintEvent 中 QPainter + Antialiasing 自绘
void MKeyboardKey::updateStyle() {
    m_style = getStyle();
    QString ts = m_style;
    // 替换所有 background-color 为 transparent
    QRegularExpression re_bg("(background-color\\s*:\\s*)[^;]+(;)");
    ts.replace(re_bg, "\\1transparent\\2");
    // 替换所有 border-style 为 none
    QRegularExpression re_bs("(border-style\\s*:\\s*)[^;]+(;)");
    ts.replace(re_bs, "\\1none\\2");
    // 替换所有 border-width 为 0px
    QRegularExpression re_bw("(border-width\\s*:\\s*)[^;]+(;)");
    ts.replace(re_bw, "\\10px\\2");
    setStyleSheet(ts.toUtf8());
}

void MKeyboardKey::paintEvent(QPaintEvent *event) {
    // 完全自绘：用 QPainter + Antialiasing 绘制抗锯齿圆角
    // 避免 Qt stylesheet border-radius 的锯齿问题

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    int w = width();
    int h = height();
    int bw = m_border_width;
    int br = m_border_radius;
    int inner_br = qMax(0, br - bw);

    // 根据交互状态选择颜色和字体大小
    // 优先级：pressed > checked > hover > normal
    QColor bg = m_background_color;
    QColor fc = m_font_color;
    int fs = m_font_size;
    // 计算 outset/inset 边框的亮色和暗色
    // outset: 凸起，inset: 凹陷
    QColor borderLight = m_border_color.lighter(200); // base:100
    QColor borderDark = m_border_color.darker(95); // base:100
    // 应用透明度
    borderLight.setAlphaF(0.7);
    borderDark.setAlphaF(0.7);

    bool isHover = underMouse(); // 悬停或按下状态

    if (isDown()) { // 按下状态
        bg = isChecked() ? m_pressed_checked_background_color
                         : m_pressed_not_checked_background_color;
        fc = m_hover_font_color;
        fs = m_hover_font_size;
    }
    else if (isChecked()) { // 选中状态
        bg = m_checked_background_color;
        if (isHover) { // 悬停状态
            fc = m_hover_font_color;
            fs = m_hover_font_size;
        }
    }
    else if (isHover) { // 悬停状态
        bg = m_hover_background_color;
        fc = m_hover_font_color;
        fs = m_hover_font_size;
    }

    // 悬停或按下时使用 inset 边框，与原始 QSS 的 border-style 对应
    if (isHover) {
        std::swap(borderLight, borderDark);
    }

    // 绘制边框 + 背景
    // 方法：先填充整个控件为边框色（外层圆角），再填充内部为背景色（内层圆角）
    // 避免描边方式导致边框被控件矩形裁切只显示一半的问题
    QPainterPath outer_path;
    outer_path.addRoundedRect(0, 0, w, h, br, br);

    if (bw > 0) {
        QLinearGradient grad;
        switch (m_border_3d_mode) {
        case BORDER_3D_NONE:
            // 纯色边框，无3D效果
            painter.fillPath(outer_path, borderLight);
            break;
        case BORDER_3D_DIAG:
            // 对角线渐变：左上亮→右下暗
                grad = QLinearGradient(0, 0, w, h);
                grad.setColorAt(0, borderLight);
                grad.setColorAt(1, borderDark);
                painter.fillPath(outer_path, grad);
            break;
        case BORDER_3D_VERTICAL:
            // 垂直渐变：上亮→下暗
                grad = QLinearGradient(0, 0, 0, h);
                grad.setColorAt(0, borderLight);
                grad.setColorAt(1, borderDark);
                painter.fillPath(outer_path, grad);
            break;

        case BORDER_3D_SPLIT:
            // 分割式：上/左亮，下/右暗，对角线渐变过渡
            // 先用暗色填充整个边框区域（下边+右边+右下圆角）
            painter.fillPath(outer_path, borderDark);
            // 用对角线渐变裁剪覆盖亮色，在对角线处产生柔和过渡
            if (0)
            {
                // 渐变方向：从右上(0,h)到左下(w,0)的对角线
                // 过渡宽度为边框宽度的2倍，避免太窄
                float halfLen = qMax(bw * 2.0f, 4.0f);
                float diagLen = sqrtf(float(w * w + h * h));
                float centerT = 1.0f - float(h) / diagLen;
                float spreadT = halfLen / diagLen;

                QLinearGradient grad(w, 0, 0, h);
                grad.setColorAt(0, borderLight);
                grad.setColorAt(qMax(0.0f, centerT - spreadT), borderLight);
                grad.setColorAt(qMin(1.0f, centerT + spreadT), borderDark);
                grad.setColorAt(1, borderDark);
            }
            // 用多边形裁剪，上边+左边+左上圆角+右上圆角覆盖亮色
            // 对角线从右上到左下，与 Qt outset/inset 边框效果一致
            /*
                (0,0)→(w,0)→(w,br)→(0,h)

                (0,0) ─────────────── (w,0)
                │                         │
                │     area                │
                │                         │
                (0,h) ─────────────── (w,h)
            */
            QPainterPath clip;
            clip.addPolygon(QPolygonF()
                << QPointF(0, h - br * 0.05)
                << QPointF(0, 0)
                << QPointF(w, 0)
                << QPointF(w, br * 0.2));
            clip.closeSubpath();
            painter.save();
            painter.setClipPath(clip);
            painter.fillPath(outer_path, borderLight);
            painter.restore();
            break;
        }

        // 填充内部背景
        QPainterPath inner_path;
        inner_path.addRoundedRect(bw, bw, w - 2 * bw, h - 2 * bw, inner_br, inner_br);
        painter.fillPath(inner_path, bg);
    }
    else {
        painter.fillPath(outer_path, bg);
    }

    // 距离填充效果（按键按压深度可视化）
    if (m_distCur > 0) {
        int fh = static_cast<int>(m_distCur / m_distMax * h);
        // 定义内容区域（减去边框宽度）
        QRect contentRect(bw, bw, w - 2 * bw, h - 2 * bw);
        // 定义填充区域（从顶部向下填充）
        QRect fillRect(contentRect.x(), contentRect.y(), contentRect.width(), fh);

        // 定义圆角矩形路径
        QPainterPath clip_path;
        clip_path.addRoundedRect(contentRect, inner_br, inner_br);

        // 限制绘制范围到内容区域
        painter.setClipPath(clip_path);
        if (m_dist_dir == DIR_D2U) {
            painter.fillRect(fillRect, QColor(242, 221, 199, 150));
        }
        else {
            painter.fillRect(fillRect, QColor(186, 209, 243, 150));
        }
        painter.setClipping(false);
    }

    // 从 QSS 解析的属性构建字体，而非 this->font()
    // 因为 QSS 字体属性由样式系统渲染时应用，不会写入 QWidget::font()
    QFont f;
    if (!m_font_family.isEmpty()) {
        f.setFamily(m_font_family);
    }
    f.setPixelSize(fs);
    f.setWeight(QFont::Normal);
    painter.setFont(f);

    // 绘制文字（带抗锯齿）
    painter.setPen(fc);
    painter.drawText(rect(), Qt::AlignCenter, text());
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
