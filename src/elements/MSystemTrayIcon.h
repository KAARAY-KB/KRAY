#ifndef TRAY_ICON_H
#define TRAY_ICON_H

#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMessageBox>


class MSystemTrayIcon : public QObject
{
    Q_OBJECT
public:
    explicit MSystemTrayIcon(QObject *parent = nullptr, const QIcon &icon = QIcon(), const QFont &font = QFont());
    ~MSystemTrayIcon();

    void setToolTip(const QString &tooltipTip);
    void setFont(const QFont &font);
    void showTrayIcon();
    void setIcon(const QIcon &icon);

signals:
    // 托盘菜单动作触发信号
    void actionTriggered(const QString &actionName);
    // "退出时关闭程序"勾选状态变更信号
    void closeToQuitToggled(bool checked);

private slots:
    // void onActionTriggered();

private:
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;
    QAction *m_actionShow = nullptr;
    QAction *m_actionQuit = nullptr;
    QAction *m_actionCloseToQuit = nullptr; // "退出时关闭程序"勾选项
};

#endif // TRAY_ICON_H
