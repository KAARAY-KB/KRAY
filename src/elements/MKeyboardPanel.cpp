#include "MKeyboardPanel.h"
#include <QDebug>


static QString _lo[] = {
    "Esc",   "1",    "2",    "3",       "4",    "5",  "6", "7",  "8",  "9", "0",   "-", "=", "Backspace", 
    "Tab",  "Q",   "W" , "E",      "R",   "T",   "Y",    "U", "I", "O", "P",  "[", "]", "\\", 
    "Caps", "A",   "S",  "D",      "F",   "G", "H", "J",   "K", "L",  ";",  "'","Enter",
    "Shift","Z",   "X",  "C",      "V",   "B",  "N",    "M",  ",",  ".",  "/", "Shift",
    "Ctrl", "Win", "Alt", "Space", "Alt", "Fn", "Menu","Ctrl", 
};


MKeyboardPanel::MKeyboardPanel(type_t type, int base_w, int base_h, QWidget *parent)
    : QWidget(parent) 
    , m_type(type)
{
    if (get_panel(m_panelMsg, m_type) != true) {
        qDebug() << "panel type error";
        return;
    }
    setStyleSheet(
        "border-width:  2px;"
        "border-radius: 12px;"
        "border-style:  outset;"
        "border-color:rgba(200, 200, 200, 0.5);"
        "background-color: #f0ebe5;"
    );

    m_vlayout = new QVBoxLayout(this);
    m_vlayout->setObjectName("m_vlayout");
    m_vlayout->setSpacing(m_panelMsg.row_spacing); // 设置行间距
    m_vlayout->setContentsMargins(15, 15, 15, 15); // 设置布局的边距

    int row_num = m_panelMsg.rows.size();
    m_rows.clear();
    for (int i = 0; i < row_num; ++i) {
        m_rows.append(new MKeyboardRow(m_panelMsg.rows[i], base_w, base_h));
        m_rows[i]->setObjectName(QString("m_row%1").arg(i).toUtf8());
        m_vlayout->addWidget(m_rows[i]);
        // m_vlayout->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter | Qt::AlignJustify); // 设置每行按键的对齐方式
    }
}
void MKeyboardPanel::setCheckedType(checked_type_t ty) {
    for (int row = 0; row < getRowNum(); ++row) {
        for (int key = 0; key < getKeyNum(row); ++key) {
            switch (ty) {
                case CHECKED_TYPE_OTHER:    getKey(row, key)->setChecked((getKey(row, key)->getChTy() & MKeyboardKey::CH_TY_OTHER)  ? true : false); break;
                case CHECKED_TYPE_LETTER:   getKey(row, key)->setChecked((getKey(row, key)->getChTy() & MKeyboardKey::CH_TY_LETTER) ? true : false); break;
                case CHECKED_TYPE_GAME:     getKey(row, key)->setChecked((getKey(row, key)->getChTy() & MKeyboardKey::CH_TY_GAME)   ? true : false); break;
                case CHECKED_TYPE_NUMBER:   getKey(row, key)->setChecked((getKey(row, key)->getChTy() & MKeyboardKey::CH_TY_NUMBER) ? true : false); break;
                case CHECKED_TYPE_MODIFY:   getKey(row, key)->setChecked((getKey(row, key)->getChTy() & MKeyboardKey::CH_TY_MODIFY) ? true : false); break;
                case CHECKED_TYPE_DIR:      getKey(row, key)->setChecked((getKey(row, key)->getChTy() & MKeyboardKey::CH_TY_DIR)    ? true : false); break;
                case CHECKED_TYPE_PUNCTU:   getKey(row, key)->setChecked((getKey(row, key)->getChTy() & MKeyboardKey::CH_TY_PUNCTU) ? true : false); break;
                case CHECKED_TYPE_INVERT:   getKey(row, key)->setChecked(!getKey(row, key)->isChecked()); break;
                case CHECKED_TYPE_ALL:      getKey(row, key)->setChecked(true); break;
                case CHECKED_TYPE_CANCEL:   getKey(row, key)->setChecked(false); break;
            }
        }
    }
}
bool MKeyboardPanel::get_panel(panel_t &panel, type_t type) {
    bool ret = true;
    switch (type) {
        case TYPE_ANSI_104:             ansi_104(panel);            break;
        case TYPE_ANSI_104_BIG_ENTER:   ansi_104_big_enter(panel);  break;
        case TYPE_ISO_105:              iso_105(panel);             break;
        case TYPE_DEFAULT_60:           default_60(panel);          break;
        case TYPE_LS_64:                ls_64(panel);               break;
        case TYPE_ISO_60:               iso_60(panel);              break;
        case TYPE_JD_40:                jd_40(panel);               break;
        case TYPE_ERGO_DOX:             ergo_dox(panel);            break;
        case TYPE_ATREUS:               atreus(panel);              break;
        case TYPE_PLANCK:               planck(panel);              break;
        case TYPE_KINESIS_ADVANTAGE:    kinesis_advantage(panel);   break;
        case TYPE_KEYCOOL_84:           keycool_84(panel);          break;
        case TYPE_LEOPOLD_FC660M:       leopold_fc660m(panel);      break;
        default:                        ret = false;               break;
    }
    return ret;
}
void MKeyboardPanel::default_60(panel_t &panel) {
    MKeyboardRow::row_t row0 = {KEY_SPACING, {{0x00, U1_00, "Esc", MKeyboardKey::CH_TY_OTHER},    {0x01, U1_00, "1", MKeyboardKey::CH_TY_NUMBER},                          {0x02, U1_00, "2", MKeyboardKey::CH_TY_NUMBER},                          {0x03, U1_00, "3", MKeyboardKey::CH_TY_NUMBER},                          {0x04, U1_00, "4", MKeyboardKey::CH_TY_NUMBER},   {0x05, U1_00, "5", MKeyboardKey::CH_TY_NUMBER}, {0x06, U1_00, "6", MKeyboardKey::CH_TY_NUMBER},   {0x07, U1_00, "7", MKeyboardKey::CH_TY_NUMBER}, {0x08, U1_00, "8", MKeyboardKey::CH_TY_NUMBER}, {0x09, U1_00, "9", MKeyboardKey::CH_TY_NUMBER}, {0x0A, U1_00, "0", MKeyboardKey::CH_TY_NUMBER}, {0x0B, U1_00, "-", MKeyboardKey::CH_TY_OTHER}, {0x0C, U1_00, "=", MKeyboardKey::CH_TY_OTHER}, {0x0D, U2_00, "Backspace", MKeyboardKey::CH_TY_OTHER}}};
    MKeyboardRow::row_t row1 = {KEY_SPACING, {{0x0E, U1_50, "Tab", MKeyboardKey::CH_TY_OTHER},    {0x0F, U1_00, "Q", MKeyboardKey::CH_TY_LETTER},                          {0x10, U1_00, "W", MKeyboardKey::CH_TY_LETTER|MKeyboardKey::CH_TY_GAME}, {0x11, U1_00, "E", MKeyboardKey::CH_TY_LETTER},                          {0x12, U1_00, "R", MKeyboardKey::CH_TY_LETTER},   {0x13, U1_00, "T", MKeyboardKey::CH_TY_LETTER}, {0x14, U1_00, "Y", MKeyboardKey::CH_TY_LETTER},   {0x15, U1_00, "U", MKeyboardKey::CH_TY_LETTER}, {0x16, U1_00, "I", MKeyboardKey::CH_TY_LETTER}, {0x17, U1_00, "O", MKeyboardKey::CH_TY_LETTER}, {0x18, U1_00, "P", MKeyboardKey::CH_TY_LETTER}, {0x19, U1_00, "[", MKeyboardKey::CH_TY_OTHER}, {0x1A, U1_00, "]", MKeyboardKey::CH_TY_OTHER}, {0x1B, U1_50, "\\", MKeyboardKey::CH_TY_OTHER}}};
    MKeyboardRow::row_t row2 = {KEY_SPACING, {{0x1C, U1_75, "Caps", MKeyboardKey::CH_TY_OTHER},   {0x1D, U1_00, "A", MKeyboardKey::CH_TY_LETTER|MKeyboardKey::CH_TY_GAME}, {0x1E, U1_00, "S", MKeyboardKey::CH_TY_LETTER|MKeyboardKey::CH_TY_GAME}, {0x1F, U1_00, "D", MKeyboardKey::CH_TY_LETTER|MKeyboardKey::CH_TY_GAME}, {0x20, U1_00, "F", MKeyboardKey::CH_TY_LETTER},   {0x21, U1_00, "G", MKeyboardKey::CH_TY_LETTER}, {0x22, U1_00, "H", MKeyboardKey::CH_TY_LETTER},   {0x23, U1_00, "J", MKeyboardKey::CH_TY_LETTER}, {0x24, U1_00, "K", MKeyboardKey::CH_TY_LETTER}, {0x25, U1_00, "L", MKeyboardKey::CH_TY_LETTER}, {0x26, U1_00, ";", MKeyboardKey::CH_TY_OTHER},  {0x27, U1_00, "'", MKeyboardKey::CH_TY_OTHER}, {0x28, U2_25, "Enter", MKeyboardKey::CH_TY_OTHER}}};
    MKeyboardRow::row_t row3 = {KEY_SPACING, {{0x29, U2_25, "Shift", MKeyboardKey::CH_TY_MODIFY}, {0x2A, U1_00, "Z", MKeyboardKey::CH_TY_LETTER},                          {0x2B, U1_00, "X", MKeyboardKey::CH_TY_LETTER},                          {0x2C, U1_00, "C", MKeyboardKey::CH_TY_LETTER},                          {0x2D, U1_00, "V", MKeyboardKey::CH_TY_LETTER},   {0x2E, U1_00, "B", MKeyboardKey::CH_TY_LETTER}, {0x2F, U1_00, "N", MKeyboardKey::CH_TY_LETTER},   {0x30, U1_00, "M", MKeyboardKey::CH_TY_LETTER}, {0x31, U1_00, ",", MKeyboardKey::CH_TY_OTHER},  {0x32, U1_00, ".", MKeyboardKey::CH_TY_OTHER},  {0x33, U1_00, "/", MKeyboardKey::CH_TY_OTHER},  {0x34, U2_75, "Shift", MKeyboardKey::CH_TY_MODIFY}}};
    MKeyboardRow::row_t row4 = {KEY_SPACING, {{0x35, U1_25, "Ctrl", MKeyboardKey::CH_TY_MODIFY},  {0x36, U1_25, "Win", MKeyboardKey::CH_TY_MODIFY},                        {0x37, U1_25, "Alt", MKeyboardKey::CH_TY_MODIFY},                        {0x38, U6_25+U0_5, "Space", MKeyboardKey::CH_TY_OTHER},                       {0x39, U1_25, "Alt", MKeyboardKey::CH_TY_MODIFY}, {0x3A, U1_25, "Fn", MKeyboardKey::CH_TY_OTHER}, {0x3B, U1_25, "Menu", MKeyboardKey::CH_TY_OTHER}, {0x3C, U1_25, "Ctrl", MKeyboardKey::CH_TY_MODIFY}}};
    panel.rows.clear();
    panel = {ROW_SPACING, {row0, row1, row2, row3, row4}};
}
void MKeyboardPanel::ls_64(panel_t &panel) {
    MKeyboardRow::row_t row0 = {KEY_SPACING, {{0x00, U1_00, "Esc", MKeyboardKey::CH_TY_OTHER},    {0x01, U1_00, "1", MKeyboardKey::CH_TY_NUMBER},                          {0x02, U1_00, "2", MKeyboardKey::CH_TY_NUMBER},                          {0x03, U1_00, "3", MKeyboardKey::CH_TY_NUMBER},                          {0x04, U1_00, "4", MKeyboardKey::CH_TY_NUMBER},   {0x05, U1_00, "5", MKeyboardKey::CH_TY_NUMBER}, {0x06, U1_00, "6", MKeyboardKey::CH_TY_NUMBER}, {0x07, U1_00, "7", MKeyboardKey::CH_TY_NUMBER}, {0x08, U1_00, "8", MKeyboardKey::CH_TY_NUMBER}, {0x09, U1_00, "9", MKeyboardKey::CH_TY_NUMBER}, {0x0A, U1_00, "0", MKeyboardKey::CH_TY_NUMBER}, {0x0B, U1_00, "-", MKeyboardKey::CH_TY_OTHER},      {0x0C, U1_00, "=", MKeyboardKey::CH_TY_OTHER}, {0x0D, U2_00, "Backspace", MKeyboardKey::CH_TY_OTHER}}};
    MKeyboardRow::row_t row1 = {KEY_SPACING, {{0x0E, U1_50, "Tab", MKeyboardKey::CH_TY_OTHER},    {0x0F, U1_00, "Q", MKeyboardKey::CH_TY_LETTER},                          {0x10, U1_00, "W", MKeyboardKey::CH_TY_LETTER|MKeyboardKey::CH_TY_GAME}, {0x11, U1_00, "E", MKeyboardKey::CH_TY_LETTER},                          {0x12, U1_00, "R", MKeyboardKey::CH_TY_LETTER},   {0x13, U1_00, "T", MKeyboardKey::CH_TY_LETTER}, {0x14, U1_00, "Y", MKeyboardKey::CH_TY_LETTER}, {0x15, U1_00, "U", MKeyboardKey::CH_TY_LETTER}, {0x16, U1_00, "I", MKeyboardKey::CH_TY_LETTER}, {0x17, U1_00, "O", MKeyboardKey::CH_TY_LETTER}, {0x18, U1_00, "P", MKeyboardKey::CH_TY_LETTER}, {0x19, U1_00, "[", MKeyboardKey::CH_TY_OTHER},      {0x1A, U1_00, "]", MKeyboardKey::CH_TY_OTHER}, {0x1B, U1_50, "\\", MKeyboardKey::CH_TY_OTHER}}};
    MKeyboardRow::row_t row2 = {KEY_SPACING, {{0x1C, U1_75, "Caps", MKeyboardKey::CH_TY_OTHER},   {0x1D, U1_00, "A", MKeyboardKey::CH_TY_LETTER|MKeyboardKey::CH_TY_GAME}, {0x1E, U1_00, "S", MKeyboardKey::CH_TY_LETTER|MKeyboardKey::CH_TY_GAME}, {0x1F, U1_00, "D", MKeyboardKey::CH_TY_LETTER|MKeyboardKey::CH_TY_GAME}, {0x20, U1_00, "F", MKeyboardKey::CH_TY_LETTER},   {0x21, U1_00, "G", MKeyboardKey::CH_TY_LETTER}, {0x22, U1_00, "H", MKeyboardKey::CH_TY_LETTER}, {0x23, U1_00, "J", MKeyboardKey::CH_TY_LETTER}, {0x24, U1_00, "K", MKeyboardKey::CH_TY_LETTER}, {0x25, U1_00, "L", MKeyboardKey::CH_TY_LETTER}, {0x26, U1_00, ";", MKeyboardKey::CH_TY_OTHER},  {0x27, U1_00, "'", MKeyboardKey::CH_TY_OTHER},      {0x28, U2_25, "Enter", MKeyboardKey::CH_TY_OTHER}}};
    MKeyboardRow::row_t row3 = {KEY_SPACING, {{0x29, U2_00, "Shift", MKeyboardKey::CH_TY_MODIFY}, {0x2A, U1_00, "Z", MKeyboardKey::CH_TY_LETTER},                          {0x2B, U1_00, "X", MKeyboardKey::CH_TY_LETTER},                          {0x2C, U1_00, "C", MKeyboardKey::CH_TY_LETTER},                          {0x2D, U1_00, "V", MKeyboardKey::CH_TY_LETTER},   {0x2E, U1_00, "B", MKeyboardKey::CH_TY_LETTER}, {0x2F, U1_00, "N", MKeyboardKey::CH_TY_LETTER}, {0x30, U1_00, "M", MKeyboardKey::CH_TY_LETTER}, {0x31, U1_00, ",", MKeyboardKey::CH_TY_OTHER},  {0x32, U1_00, ".", MKeyboardKey::CH_TY_OTHER},  {0x33, U1_00, "/", MKeyboardKey::CH_TY_OTHER},  {0x34, U1_00, "Shift", MKeyboardKey::CH_TY_MODIFY}, {0x35, U1_00, "↑", MKeyboardKey::CH_TY_DIR},   {0x36, U1_00, "Del", MKeyboardKey::CH_TY_OTHER}}};
    MKeyboardRow::row_t row4 = {KEY_SPACING, {{0x37, U1_25, "Ctrl", MKeyboardKey::CH_TY_MODIFY},  {0x38, U1_25, "Win", MKeyboardKey::CH_TY_MODIFY},                        {0x39, U1_25, "Alt", MKeyboardKey::CH_TY_MODIFY},                        {0x3A, U6_25+U0_5, "Space", MKeyboardKey::CH_TY_OTHER},                       {0x3B, U1_00, "Alt", MKeyboardKey::CH_TY_MODIFY}, {0x3C, U1_00, "Fn", MKeyboardKey::CH_TY_OTHER}, {0x3D, U1_00, "←", MKeyboardKey::CH_TY_DIR},    {0x3E, U1_00, "↓", MKeyboardKey::CH_TY_DIR},    {0x3F, U1_00, "→", MKeyboardKey::CH_TY_DIR}}};
    panel.rows.clear();
    panel = {ROW_SPACING, {row0, row1, row2, row3, row4}};
}
void MKeyboardPanel::ansi_104(panel_t &panel){}
void MKeyboardPanel::ansi_104_big_enter(panel_t &panel){}
void MKeyboardPanel::iso_105(panel_t &panel){}
void MKeyboardPanel::iso_60(panel_t &panel){}
void MKeyboardPanel::jd_40(panel_t &panel){}
void MKeyboardPanel::ergo_dox(panel_t &panel){}
void MKeyboardPanel::atreus(panel_t &panel){}
void MKeyboardPanel::planck(panel_t &panel){}
void MKeyboardPanel::kinesis_advantage(panel_t &panel){}
void MKeyboardPanel::keycool_84(panel_t &panel){}
void MKeyboardPanel::leopold_fc660m(panel_t &panel){}

