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
    void actionTriggered(const QString &actionName);

private slots:
    // void onActionTriggered();

private:
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;
    QAction *m_actionShow = nullptr;
    QAction *m_actionQuit = nullptr;
};

#endif // TRAY_ICON_H
