#include "MKeyboardLayout.h"
#include <QDebug>
#include <QFile>



MKeyboardLayout::MKeyboardLayout(MKeyboardPanel::type_t type, int base_w, int base_h, QWidget *parent)
    : QWidget(parent)
{
    m_layout = new QGridLayout(this);
    m_layout->setObjectName("m_layout");
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_panel = new MKeyboardPanel(type, base_w, base_h);
    m_panel->setObjectName("m_panel");
    for (int row = 0; row < m_panel->getRowNum(); ++row) {
        for (int key = 0; key < m_panel->getKeyNum(row); ++key) {
            connect(m_panel->getKey(row, key), &MKeyboardKey::clicked, this, [this](int idx, bool checked) {emit keyClicked(idx, checked);});
        }
    }
    m_layout->addWidget(m_panel, 0, 0, 1, 1, Qt::AlignCenter);
    setLayout(m_layout);
}
