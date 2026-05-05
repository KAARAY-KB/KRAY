#include "MSystemTrayIcon.h"


MSystemTrayIcon::MSystemTrayIcon(QObject *parent) 
    : QObject(parent)
{
    // 检查系统是否支持托盘功能
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, "Error", "System tray not supported!");
        return;
    }

    // 创建系统托盘图标
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/resources/images/pixel_parrot.png"));
    m_trayIcon->setToolTip("My Tray Application");

    m_trayMenu = new QMenu();
    
    m_actionShow  = new QAction("显示", m_trayMenu);
    m_actionQuit  = new QAction("退出", m_trayMenu);

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
    // // 可选：托盘图标点击事件（左键单击）
    // connect(m_trayIcon, &QSystemTrayIcon::activated, [&](QSystemTrayIcon::ActivationReason reason) {
    //     if (reason == QSystemTrayIcon::Trigger) {
    //         // 左键单击逻辑
    //         QMessageBox::information(nullptr, "Click", "Tray icon clicked!");
    //     }
    // });

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

void MSystemTrayIcon::showTrayIcon()
{
    if (m_trayIcon != nullptr) {
        m_trayIcon->show();
    }
}

void MSystemTrayIcon::setIcon(const QIcon &icon)
{
    if (m_trayIcon != nullptr) {
        m_trayIcon->setIcon(icon);
    }
}