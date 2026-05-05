#ifndef T1_H
#define T1_H

#include <QWidget>
#include <QCloseEvent>

QT_BEGIN_NAMESPACE
namespace Ui {
class T1;
}
QT_END_NAMESPACE

class T1 : public QWidget
{
    Q_OBJECT

public:
    T1(QWidget *parent = nullptr);
    ~T1();

    void closeEvent(QCloseEvent *event) override;
    void show_top(void);

protected:

private:

private slots:

signals:
    void exitWindow();

private:
    Ui::T1 *ui;
};
#endif // T1_H
