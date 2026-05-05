#ifndef T2_H
#define T2_H

#include <QWidget>
#include <QCloseEvent>

class QtMaterialAppBar;
class QtMaterialFlatButton;
class QtMaterialTextField;
class QtMaterialToggle;
class QtMaterialSlider;
class QtMaterialCircularProgress;
class QtMaterialSnackbar;

QT_BEGIN_NAMESPACE
namespace Ui { class T2; }
QT_END_NAMESPACE

class T2 : public QWidget
{
    Q_OBJECT

public:
    T2(QWidget *parent = nullptr);
    ~T2();

    void closeEvent(QCloseEvent *event) override;
    void show_top(void);

private:
    void setup_ui();

signals:
    void exitWindow();
private:
    Ui::T2 *ui;
};
#endif // T2_H
