#ifndef KRAY_H
#define KRAY_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class Kray;
}
QT_END_NAMESPACE

class Kray : public QMainWindow
{
    Q_OBJECT

public:
    Kray(QWidget *parent = nullptr);
    ~Kray();

private:
    Ui::Kray *ui;
};
#endif // KRAY_H
