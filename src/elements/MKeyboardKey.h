#ifndef MKEYBOARDKEY_H
#define MKEYBOARDKEY_H

#include <QWidget>
#include <QFile>
#include <QString>
#include <QPushButton>

#include <QPainterPath>
#include <QPainter>
#include <QPaintEvent>
#include <QRegularExpression>
#include <QDebug>


class MKeyboardKey : public QPushButton {
    Q_OBJECT
public:
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

    // 从成员变量生成样式表
    QString getStyle();
    // 刷新样式到控件，并同步更新 m_style
    void updateStyle() {
        m_style = getStyle();
        setStyleSheet(m_style.toUtf8());
    }

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
signals:
    // void stateChanged(int idx, bool checked);
    void clicked(int idx, bool checked);
};


#endif // MKEYBOARDKEY_H
