#ifndef TEST_H
#define TEST_H

#include <QWidget>
#include <QCloseEvent>

QT_BEGIN_NAMESPACE
class Ui_Test
{
public:
    void setupUi(QWidget *main)
    {
        main->resize(400, 300);
    }
};
namespace Ui {
   class Test: public Ui_Test {};
}
QT_END_NAMESPACE

class Test : public QWidget
{
    Q_OBJECT

public:
    explicit Test(QWidget *parent = nullptr);
    ~Test();

    void closeEvent(QCloseEvent *event) override;
    void show_top(void);

protected:

private:

private slots:

signals:
    void exitWindow();

private:
    Ui::Test *ui;
};

#endif // TEST_H
