#ifndef MKEYBOARDKEY_H
#define MKEYBOARDKEY_H

#include <QWidget>
#include <QFile>
#include <QString>
#include <QPushButton>

#include <QPainterPath>
#include <QPainter>
#include <QPaintEvent>
#include <QPolygonF>
#include <QLinearGradient>
#include <QRegularExpression>
#include <QDebug>
#include <QEvent>


class MKeyboardKey : public QPushButton {
    Q_OBJECT
public:
    // 边框3D模式
    typedef enum {
        BORDER_3D_NONE = 0,     // 无3D效果，纯色边框
        BORDER_3D_DIAG,         // 对角线渐变（左上亮→右下暗）
        BORDER_3D_VERTICAL,     // 垂直渐变（上亮→下暗）
        BORDER_3D_SPLIT,        // 分割式（上/左亮，下/右暗，带渐变过渡）
    } border_3d_mode_t;

    typedef enum {
        CH_TY_OTHER  = 0x01 << 0,
        CH_TY_LETTER = 0x01 << 1,
        CH_TY_GAME   = 0x01 << 2,
        CH_TY_NUMBER = 0x01 << 3,
        CH_TY_MODIFY = 0x01 << 4,
        CH_TY_DIR    = 0x01 << 5,
        CH_TY_PUNCTU = 0x01 << 6,
    } ch_type_t;
    // 灯珠类型
    typedef enum {
        LIGHT_NONE = 0x00, // 无
        LIGHT_ONLY = 0x01, // 单一颜色
        LIGHT_RGB  = 0x02, // RGB颜色
        LIGHT_RGBW = 0x03, // RGBW颜色
    } light_type_t;

    typedef enum {
        DIR_D2U = 0,
        DIR_U2D = 1,
    } dist_dir_t;
    typedef struct {
        int id;
        float w_unit;
        QString text;
        uint16_t ch_ty; /* ch_type_t */
        light_type_t light_ty; /* light_type_t */
    } msg_t;
    explicit MKeyboardKey(msg_t &msg, int base_w = 20, int base_h = 20, QWidget *parent = nullptr);
    int getId() const { return m_msg.id; }
    uint16_t getChTy() const { return m_msg.ch_ty; }
    float get_w_unit() const { return m_msg.w_unit; }

    // 从成员变量生成样式表
    QString getStyle();
    // 刷新样式到控件，并同步更新 m_style
    // 刷新样式到控件（背景色设为 transparent，由 paintEvent 自绘抗锯齿圆角）
    void updateStyle();

    // 距离相关
    float getDistMax() const { return m_distMax; }
    void setDistMax(float dist) { m_distMax = dist; }
    void updateDistCur(float dist, dist_dir_t dir = DIR_D2U) {
        setDistCur(dist, dir);
        update();
    }
    float getDistCur() const { return m_distCur; }
    void setDistCur(float dist, dist_dir_t dir = DIR_D2U) { m_distCur = dist; m_dist_dir = dir; }
    void updateDistMax(float dist) {
        setDistMax(dist);
        update();
    }

    // --- 颜色 getter（从样式表解析） ---
    QColor get_border_color();
    QColor get_font_color();
    QColor get_hover_font_color();
    QColor get_background_color();
    QColor get_hover_background_color();
    QColor get_checked_background_color();
    QColor get_pressed_not_checked_background_color();
    QColor get_pressed_checked_background_color();

    // --- 布局 getter（从样式表解析） ---
    int get_border_width();
    int get_border_radius();
    int get_font_size();
    int get_hover_font_size();
    int get_padding();
    QString get_font_family();

// public slots:
    // --- 颜色 setter ---
    QString set_border_color(QColor color);
    QString set_font_color(QColor color);
    QString set_hover_font_color(QColor color);
    QString set_background_color(QColor color);
    QString set_hover_background_color(QColor color);
    QString set_checked_background_color(QColor color);
    QString set_pressed_not_checked_background_color(QColor color);
    QString set_pressed_checked_background_color(QColor color);
    // --- 布局 setter ---
    QString set_border_width(int w);
    QString set_border_radius(int r);
    QString set_font_size(int s);
    QString set_hover_font_size(int s);
    QString set_padding(int p);
    QString set_font_family(const QString &f);

    // --- 边框3D模式 ---
    border_3d_mode_t get_border_3d_mode() const { return m_border_3d_mode; }
    void set_border_3d_mode(border_3d_mode_t mode) { m_border_3d_mode = mode; update(); }
    // 亮色透明度 (0.0~1.0)
    float get_border_light_alpha() const { return m_border_light_alpha; }
    void set_border_light_alpha(float a) { m_border_light_alpha = a; update(); }
    // 暗色透明度 (0.0~1.0)
    float get_border_dark_alpha() const { return m_border_dark_alpha; }
    void set_border_dark_alpha(float a) { m_border_dark_alpha = a; update(); }

    // --- 多段颜色（宽按键如空格对应多颗LED） ---
    int get_light_count() const { return m_light_count; }
    void set_light_count(int count);
    void set_segment_color(int seg, const QColor &color);
    QColor get_segment_color(int seg) const;
    void clear_segment_colors();

    // 律动状态
    void set_rhythm_active(bool active) { m_rhythm_active = active; }
    bool is_rhythm_active() const { return m_rhythm_active; }

private:
    int m_base_w;
    int m_base_h;
    msg_t m_msg;
    QString m_style;

    float m_distMax;
    float m_distCur;
    dist_dir_t m_dist_dir;

    QColor m_border_color;
    QColor m_font_color;
    QColor m_hover_font_color;
    QColor m_background_color;
    QColor m_hover_background_color;
    QColor m_checked_background_color;
    QColor m_pressed_not_checked_background_color;
    QColor m_pressed_checked_background_color;

    int m_border_width;
    int m_border_radius;
    int m_font_size;
    int m_hover_font_size;
    int m_padding;
    QString m_font_family;

    // 边框3D相关
    border_3d_mode_t m_border_3d_mode = BORDER_3D_SPLIT;
    float m_border_light_alpha = 0.7;
    float m_border_dark_alpha = 0.7;

    // 多段颜色（宽按键如空格对应多颗LED）
    int m_light_count = 1;
    QVector<QColor> m_segment_colors;
    bool m_rhythm_active = false;


    // 从样式表字符串中提取指定选择器内的属性值
    // 返回属性值字符串，未找到返回空
    QString extractStyleAttr(const QString &style, const QString &selector, const QString &prop);
    // 从样式表字符串解析整数、颜色、字符串
    int styleToInt(const QString &selector, const QString &prop);
    // int styleToInt(const QString &style);
    QString styleToStr(const QString &selector, const QString &prop);
    // QString styleToStr(const QString &style);
    QColor styleToColor(const QString &selector, const QString &prop);
    // QColor styleToColor(const QString &style);
    // QColor → 样式表字符串
    QString colorToStr(QColor color);
    QColor strToColor(const QString &str);
    
protected:
    void paintEvent(QPaintEvent *event) override;
    // 鼠标进入/离开时触发重绘，更新 hover 状态
    void enterEvent(QEvent *event) override { update(); QPushButton::enterEvent(event); }
    void leaveEvent(QEvent *event) override { update(); QPushButton::leaveEvent(event); }
signals:
    // void stateChanged(int idx, bool checked);
    void clicked(int idx, bool checked);
};


#endif // MKEYBOARDKEY_H
