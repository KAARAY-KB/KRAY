#ifndef MKEYBOARDPANEL_H
#define MKEYBOARDPANEL_H

#include "MKeyboardRow.h"
#include <functional>

class MKeyboardPanel : public QWidget {
    Q_OBJECT
public:
    typedef enum {
        TYPE_ANSI_104 = 0x00,
        TYPE_ANSI_104_BIG_ENTER,
        TYPE_ISO_105,
        TYPE_DEFAULT_60,
        TYPE_LS_64,
        TYPE_ISO_60,
        TYPE_JD_40,
        TYPE_ERGO_DOX,
        TYPE_ATREUS,
        TYPE_PLANCK,
        TYPE_KINESIS_ADVANTAGE,
        TYPE_KEYCOOL_84,
        TYPE_LEOPOLD_FC660M,
        TYPE_DFT_60_SPLIT,
        TYPE_LS_64_SPLIT,
        TYPE_MAX,
    } type_t;
    typedef enum {
        CHECKED_TYPE_OTHER = 0,
        CHECKED_TYPE_LETTER,
        CHECKED_TYPE_GAME,
        CHECKED_TYPE_NUMBER,
        CHECKED_TYPE_MODIFY,
        CHECKED_TYPE_DIR,
        CHECKED_TYPE_PUNCTU,
        CHECKED_TYPE_INVERT,
        CHECKED_TYPE_ALL,
        CHECKED_TYPE_CANCEL,
    } checked_type_t;

    explicit MKeyboardPanel(type_t type, int base_w = KEY_W_BASE, int base_h = KEY_H_BASE, QWidget *parent = nullptr);
    
    int getRowNum() const { return m_rows.size(); }
    int getKeyNum(int row) const { return m_rows[row]->getKeyNum(); }
    void getAllKeyNum(std::function<void(MKeyboardKey *key_data, void *user)> cb, void *context_data) {
         for (int row = 0; row < getRowNum(); ++row) {
             for (int cnt = 0; cnt < getKeyNum(row); ++cnt) {
                MKeyboardKey *key = getKey(row, cnt);
                cb(key, context_data);
             }
         }
    }

    MKeyboardRow *getRow(int rowIdx) { return m_rows[rowIdx]; }
    MKeyboardKey *getKey(int rowIdx, int keyIdx) { return getRow(rowIdx)->getKey(keyIdx); }

    void setCheckedType(checked_type_t ty);

    type_t m_type;
    static constexpr int KEY_W_BASE   = 42;
    static constexpr int KEY_H_BASE   = 42;
    static constexpr int KEY_W_EXPAND = 60;
    static constexpr int KEY_H_EXPAND = 60;
    static constexpr int ROW_SPACING  = 3;
    static constexpr int KEY_SPACING  = 3;
    static constexpr float U1_00 = 1.0;  // 1u
    static constexpr float U1_25 = 1.25; // 1.25u
    static constexpr float U1_50 = 1.5;  // 1.5u
    static constexpr float U1_75 = 1.75; // 1.75u
    static constexpr float U2_00 = 2.0;  // 2u
    static constexpr float U2_25 = 2.25; // 2.25u
    static constexpr float U2_75 = 2.75; // 2.75u
    static constexpr float U6_25 = 6.25; // 6.25u
    static constexpr float U0_5 = 0.5;

private:
    typedef struct  {
        int row_spacing;
        QVector<MKeyboardRow::row_t> rows;
    } panel_t;
    
    panel_t m_panelMsg;
    QVector<MKeyboardRow *>m_rows;
    QString panel_color;
    QVBoxLayout *m_vlayout = nullptr;
    bool get_panel(panel_t &panel, type_t type);
    void ansi_104(panel_t &panel);
    void ansi_104_big_enter(panel_t &panel);
    void iso_105(panel_t &panel);
    void default_60(panel_t &panel);
    void ls_64(panel_t &panel);
    void iso_60(panel_t &panel);
    void jd_40(panel_t &panel);
    void ergo_dox(panel_t &panel);
    void atreus(panel_t &panel);
    void planck(panel_t &panel);
    void kinesis_advantage(panel_t &panel);
    void keycool_84(panel_t &panel);
    void leopold_fc660m(panel_t &panel);
    QString colorToStr(QColor color) { return QString("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue()); }
private slots:
    void on_panel_color(QColor color) {panel_color = colorToStr(color);}
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
};


#endif // MKEYBOARDPANEL_H
