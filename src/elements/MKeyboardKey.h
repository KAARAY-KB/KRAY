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
        int seq;
        float w_unit;
        QString text;
        uint16_t ch_ty; /* ch_type_t */
        light_type_t light_ty; /* light_type_t */
    } msg_t;
    explicit MKeyboardKey(msg_t &msg, int base_w = 20, int base_h = 20, QWidget *parent = nullptr);
    int getSeq() const { return m_msg.seq; }
    uint16_t getChTy() const { return m_msg.ch_ty; }
    QString getStyle();

    void updateStyle() {
        setStyleSheet(getStyle().toUtf8());
    }

    float getDistMax() const { return m_distMax; }
    void setDistMax(float dist) { m_distMax = dist; }
    void updateDistCur(float dist, dist_dir_t dir = DIR_D2U) {
        setDistCur(dist, dir); 
        update(); // 重绘
    }
    float getDistCur() const { return m_distCur; }
    void setDistCur(float dist, dist_dir_t dir = DIR_D2U) { m_distCur = dist; m_dist_dir = dir; }
    void updateDistMax(float dist) {
        setDistMax(dist); 
        update(); // 重绘
    }
private:
    int m_base_w;
    int m_base_h;
    msg_t m_msg;

    float m_distMax;
    float m_distCur;
    dist_dir_t m_dist_dir; //0:D2U, 1:U2D

    QString dft_border_color = "rgba(200, 200, 200, 0.5)";
    QString dft_font_color = "#353535";
    QString hover_font_color = "#555555";
    QString dft_background_color = "#f0ece8";
    QString hover_background_color = "#efcfca";
    QString checked_background_color = "#c6ded0";
    QString checked0_background_color = "#f0d6d2";
    QString checked1_background_color = "#cfe1d6";
    QString colorToStr(QColor color) { return QString("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue()); }

    // 从样式表中解析边框宽度
    int parseBorderWidthFromStyleSheet();
    int parseBorderRadiusFromStyleSheet();
public slots:
    void set_dft_border_color(QColor color)          { dft_border_color = colorToStr(color);}
    void set_dft_font_color(QColor color)            { dft_font_color = colorToStr(color);}
    void set_hover_font_color(QColor color)          { hover_font_color = colorToStr(color);}
    void set_dft_background_color(QColor color)      { dft_background_color = colorToStr(color);}
    void set_hover_background_color(QColor color)    { hover_background_color = colorToStr(color);}
    void set_checked_background_color(QColor color)  { checked_background_color = colorToStr(color);}
    void set_checked0_background_color(QColor color) { checked0_background_color = colorToStr(color);}
    void set_checked1_background_color(QColor color) { checked1_background_color = colorToStr(color);}
protected:
    void paintEvent(QPaintEvent *event) override;
signals:
    // void stateChanged(int idx, bool checked);
    void clicked(int idx, bool checked);
};


#endif // MKEYBOARDKEY_H
