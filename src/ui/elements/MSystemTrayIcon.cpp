#include "MSystemTrayIcon.h"


MSystemTrayIcon::MSystemTrayIcon(QObject *parent, const QIcon &icon,  const QFont &font)
    : QObject(parent)
{
    // 检查系统是否支持托盘功能
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, "Error", "System tray not supported!");
        return;
    }

    // 创建系统托盘图标
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(icon);
    m_trayIcon->setToolTip("System Tray");

    m_trayMenu = new QMenu();
    m_trayMenu->setFont(font);
    
    m_actionShow  = new QAction("显示", m_trayMenu);
    m_actionQuit  = new QAction("退出", m_trayMenu);
    m_actionShow->setFont(font);
    m_actionQuit->setFont(font);

    m_trayMenu->addAction(m_actionShow);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_actionQuit);

    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_actionShow, &QAction::triggered, this, [this]() {
        emit actionTriggered("show");
    });
    connect(m_actionQuit, &QAction::triggered, this, [this]() {
        emit actionTriggered("exit");
    });

    connect(m_trayIcon, &QSystemTrayIcon::activated, [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            // 托盘图标点击事件（左键单击）
            // QMessageBox::information(nullptr, "Click", "Tray icon clicked!");
            emit actionTriggered("show");
        }
    });

}

MSystemTrayIcon::~MSystemTrayIcon()
{
    if (m_trayIcon) {
        delete m_trayIcon;
    }
    if (m_trayMenu) {
        delete m_trayMenu;
    }
}
// 显示系统托盘图标
void MSystemTrayIcon::showTrayIcon()
{
    if (m_trayIcon != nullptr) {
        m_trayIcon->show();
    }
}
// 设置系统托盘图标图标
void MSystemTrayIcon::setIcon(const QIcon &icon)
{
    if (m_trayIcon != nullptr) {
        m_trayIcon->setIcon(icon);
    }
}
// 设置系统托盘图标提示信息
void MSystemTrayIcon::setToolTip(const QString &tooltipTip)
{
    if (m_trayIcon != nullptr) {
        m_trayIcon->setToolTip(tooltipTip);
    }
}
// 设置系统托盘图标菜单字体
void MSystemTrayIcon::setFont(const QFont &font)
{
    if (m_trayMenu != nullptr) {
        m_trayMenu->setFont(font);
    }
}
