/********************************************************************************
** Form generated from reading UI file 'testErTAZI.ui'
**
** Created by: Qt User Interface Compiler version 5.12.11
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TESTERTAZI_H
#define TESTERTAZI_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Form
{
public:
    QGridLayout *gridLayout;
    QStackedWidget *stackedWidget;
    QWidget *page_main;
    QWidget *page_kb;
    QWidget *page_test;
    QComboBox *comboBox;
    QPushButton *keyboardBtn;
    QPushButton *mainBtn;
    QPushButton *testBtn;

    void setupUi(QWidget *Form)
    {
        if (Form->objectName().isEmpty())
            Form->setObjectName(QString::fromUtf8("Form"));
        Form->resize(511, 342);
        gridLayout = new QGridLayout(Form);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        stackedWidget = new QStackedWidget(Form);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        page_main = new QWidget();
        page_main->setObjectName(QString::fromUtf8("page_main"));
        mainBtn = new QPushButton(page_main);
        mainBtn->setObjectName(QString::fromUtf8("mainBtn"));
        mainBtn->setGeometry(QRect(0, 0, 111, 51));
        comboBox = new QComboBox(page_main);
        comboBox->setObjectName(QString::fromUtf8("comboBox"));
        comboBox->setGeometry(QRect(10, 60, 261, 61));
        stackedWidget->addWidget(page_main);
        page_kb = new QWidget();
        page_kb->setObjectName(QString::fromUtf8("page_kb"));
        keyboardBtn = new QPushButton(page_kb);
        keyboardBtn->setObjectName(QString::fromUtf8("keyboardBtn"));
        keyboardBtn->setGeometry(QRect(140, 70, 141, 91));
        stackedWidget->addWidget(page_kb);
        page_test = new QWidget();
        page_test->setObjectName(QString::fromUtf8("page_test"));
        testBtn = new QPushButton(page_test);
        testBtn->setObjectName(QString::fromUtf8("testBtn"));
        testBtn->setGeometry(QRect(350, 240, 131, 81));
        stackedWidget->addWidget(page_test);

        gridLayout->addWidget(stackedWidget, 0, 0, 1, 1);


        retranslateUi(Form);

        QMetaObject::connectSlotsByName(Form);
    } // setupUi

    void retranslateUi(QWidget *Form)
    {
        Form->setWindowTitle(QApplication::translate("Form", "Form", nullptr));
        mainBtn->setText(QApplication::translate("Form", "open", nullptr));
        keyboardBtn->setText(QApplication::translate("Form", "keyboad", nullptr));
        testBtn->setText(QApplication::translate("Form", "test", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Form: public Ui_Form {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TESTERTAZI_H
