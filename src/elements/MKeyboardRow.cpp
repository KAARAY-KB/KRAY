#include "MKeyboardRow.h"


MKeyboardRow::MKeyboardRow(row_t &row, int base_w, int base_h, QWidget *parent)
    : QWidget(parent) 
    , m_rowMsg(row)
{
    setStyleSheet(
        // "border-style:  normal;"
        "background-color: #f0ebe5;"
    );
    m_hlayout = new QHBoxLayout(this);
    m_hlayout->setObjectName("m_hlayout");
    m_hlayout->setSpacing(m_rowMsg.key_spacing); // 设置按键之间的间距
    m_hlayout->setContentsMargins(0, 0, 0, 0); // 设置布局的边距

    int key_num = m_rowMsg.keys.size();
    m_keys.clear();
    for (int i = 0; i < key_num; ++i) {
        m_keys.append(new MKeyboardKey(m_rowMsg.keys[i], base_w, base_h));
        m_keys[i]->setObjectName(QString("m_key%1").arg(m_keys[i]->getSeq()).toUtf8());
        m_hlayout->addWidget(m_keys[i]);
        if (i != (key_num - 1)) { //每个控件之间都添加伸缩
            m_hlayout->addStretch();
        }
    }
    // setStyleSheet("background-color: rgba(0, 17, 255, 0.95);");
}

