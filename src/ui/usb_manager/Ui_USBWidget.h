#ifndef UI_USBWIDGET_H
#define UI_USBWIDGET_H
#include <QApplication>
#include <QWidget>
#include <QStackedWidget>
#include <QComboBox>
#include <QPushButton>
#include <QGridLayout>

#include "USBDeviceManagerWidget.h"
#include "console.h"

QT_BEGIN_NAMESPACE

class Ui_USBWidget
{
public:
    enum PAGE_TY {
        PAGE_TY_MAIN = 0,
        PAGE_TY_KB,
        PAGE_TY_TEST,
        PAGE_TY_MAX
    };
    typedef struct {
        int idx;
        PAGE_TY type;
        QString name;
        QWidget *widget;
    } page_t;
    QVector<page_t *> pages;


    QWidget *ui;
    QGridLayout *gridLayout;
    QStackedWidget *stackedWidget;
    
    QWidget *p0_main;
    QGridLayout *p0_gridLayout;
    USBDeviceManagerWidget *usbDeviceManager;
    QComboBox *p0_comboBox;
    QPushButton *p0_openBtn;

    void setupUi(QWidget *form) {
        if (form->objectName().isEmpty()) {
            form->setObjectName(QString::fromUtf8("form"));
        }
        ui = form;
        ui->resize(400, 200);
        ui->setAttribute(Qt::WA_QuitOnClose, false);
        ui->setWindowTitle("USB Keyboard");
        ui->setWindowIcon(QIcon(":/images/applet.png"));

        gridLayout = new QGridLayout(ui);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        
        initStackedWidget();
        initPageMain();

        gridLayout->addWidget(stackedWidget, 0, 0, 1, 1);

        showSubWidget(PAGE_TY_MAIN, 0);
        
        QMetaObject::connectSlotsByName(ui);
    }
    
    void show_top(void) {
        if (ui->isMinimized()) {
            ui->showNormal();
        }
    #ifdef __WIN32__
        SetWindowPos(HWND(ui->winId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        SetWindowPos(HWND(ui->winId()), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    #endif
        ui->show();
        ui->activateWindow();
    }

    page_t *addSubWidget(PAGE_TY ty, QWidget *widget = nullptr) {
        page_t *pt = nullptr;
        if (ty >= PAGE_TY_MAX) {
            return pt;
        }
        pages.append(new page_t);
        pt = pages.last();
        pt->type   = ty;
        pt->widget = (widget == nullptr) ? new QWidget() : widget;
        stackedWidget->addWidget(pt->widget);
        pt->idx  = stackedWidget->count() - 1;
        pt->name = (ty == PAGE_TY_MAIN) ? QString("page%1_main").arg(pt->idx) :
                   (ty == PAGE_TY_KB)   ? QString("page%1_kb").arg(pt->idx) :
                   (ty == PAGE_TY_TEST) ? QString("page%1_test").arg(pt->idx) : QString("page%1_unknown").arg(pt->idx);
        pt->widget->setObjectName(QString::fromUtf8(pt->name.toUtf8()));
        return pt;
    }
    page_t *getSubWidget(int idx) {
        page_t *pt = nullptr;
        if (idx >= stackedWidget->count()) {
            return pt;
        }
        for (int i = 0; i < pages.size(); i++) {
            pt = pages[i];
            if (pt->idx == idx) {
                return pt;
            }
        }
        return pt;
    }
    void delSubWidget(PAGE_TY ty, int idx) {
        if (ty >= PAGE_TY_MAX) {
            return;
        }
        for (int i = 0; i < pages.size(); i++) {
            page_t *pt = pages[i];
            if ((pt->type == ty) && (pt->idx == idx)) {
                stackedWidget->removeWidget(pt->widget);
                // if (pt->widget) {
                //     delete pt->widget;
                //     pt->widget = nullptr;
                // }
                pages.remove(i);
                break;
            }
        }
    }
    void showSubWidget(PAGE_TY ty, int idx) {
        if (ty >= PAGE_TY_MAX) {
            return;
        }
        bool find = false;
        for (int i = 0; i < pages.size(); i++) {
            page_t *pt = pages[i];
            if ((pt->type == ty) && (pt->idx == idx)) {
                find = true;
                break;
            }
        }
        if (find) {
            stackedWidget->setCurrentIndex(idx);
        }
    }

    void widgetSizeChanged(page_t *pt) {
        if (pt == nullptr) {
            return;
        }
        switch (pt->type) {
            case PAGE_TY_MAIN:
                ui->resize(400, 200);
                ui->setStyleSheet("QWidget#form {background-color: #f0f0f0;}");
                Console::out() << "main" << std::endl;
                break;
            case PAGE_TY_KB:
                ui->resize(1000, 700);
                ui->setStyleSheet("QWidget#form {background-color: #f0edea;}");
                Console::out() << "kb" << std::endl;
                break;
        }
    }


private:
    void initStackedWidget(QWidget *parent = nullptr) {
        pages.clear();
        stackedWidget = new QStackedWidget(parent);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
    }

    void initPageMain() {
        QWidget *widget = addSubWidget(PAGE_TY_MAIN)->widget;
        if (widget == nullptr) {
            return;
        }

        p0_gridLayout = new QGridLayout(widget);

        usbDeviceManager = new USBDeviceManagerWidget();
        usbDeviceManager->setObjectName(QString::fromUtf8("usbDeviceManager"));
        
        p0_gridLayout->addWidget(usbDeviceManager);
    }
    QString m_title;
};
namespace Ui {
   class USBWidget: public Ui_USBWidget {};
}
QT_END_NAMESPACE

#endif // UI_USBWIDGET_H
