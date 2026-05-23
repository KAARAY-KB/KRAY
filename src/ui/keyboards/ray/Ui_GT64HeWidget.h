#ifndef UI_GT64HEWIDGET_H
#define UI_GT64HEWIDGET_H
#include <QApplication>
#include <QWidget>

#include <QLabel>
#include <QFrame>
#include <QCheckBox>
#include <QSplitter>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>

#include <QDoubleSpinBox>
#include <QButtonGroup>
#include <QRadioButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>

#include "MSlider.h"
#include "MTabWidget.h"
#include "MKeyboardLayout.h"
#include "MDoubleSpinBox.h"


QT_BEGIN_NAMESPACE

class GT64HeWidgetUiSidebar {
public:
    QWidget *m_widget;
    QGridLayout *m_gridLayout;
    QPushButton *btn;
    void setupUi(QWidget *form = nullptr) {
        m_widget = new QWidget(form);
        m_widget->setObjectName(QString::fromUtf8(objHead.toUtf8()+"widgm_widgetet"));
        m_widget->setContentsMargins(0, 0, 20, 0); /* int aleft, int atop, int aright, int abottom */

        m_gridLayout = new QGridLayout(m_widget);
        m_gridLayout->setObjectName(QString::fromUtf8(objHead.toUtf8()+"m_gridLayout"));

        btn = new QPushButton();
        btn->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn"));
        btn->setText("Side");
        m_gridLayout->addWidget(btn);
    }
    QString objectNameHead() const { return objHead; };
    void setObjectNameHead(const QString &name) { objHead = name + "_"; }
private:
    QString objHead = nullptr;
};
class GT64HeWidgetUiStatebar {
public:
    QHBoxLayout *m_layout;
    QLabel *label_cfgFile;
    QPushButton *btn_save;
    QPushButton *btn_exitWidget;
    void setupUi(QWidget *form = nullptr) {
        m_layout = new QHBoxLayout(form);
        m_layout->setObjectName(QString::fromUtf8(objHead.toUtf8()+"m_layout"));

        label_cfgFile = new QLabel();
        label_cfgFile->setObjectName(QString::fromUtf8(objHead.toUtf8()+"label_cfgFile"));
        label_cfgFile->setText("Config: A Mode");
        QFont font;
        font.setFamily(QString::fromUtf8("Consolas"));
        font.setPointSize(14);
        font.setBold(false);
        label_cfgFile->setFont(font);
        
        btn_save = new QPushButton();
        btn_save->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_save"));
        btn_save->setText("Save");
        
        btn_exitWidget = new QPushButton();
        btn_exitWidget->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_exitWidget"));
        btn_exitWidget->setText("Exit Widget");
        
        m_layout->addWidget(label_cfgFile);
        m_layout->addWidget(btn_save);
        m_layout->addWidget(btn_exitWidget);
    }
    QString objectNameHead() const { return objHead; };
    void setObjectNameHead(const QString &name) { objHead = name + "_"; }
private:
    QString objHead = nullptr;
};
class GT64HeWidgetUiKeyboard {
public:
    QWidget *m_widget;
    MKeyboardLayout *layout;
    void setupUi(QWidget *form = nullptr) {
        m_widget = new QWidget(form);
        m_widget->setObjectName(QString::fromUtf8(objHead.toUtf8()+"m_widget"));
        m_widget->setContentsMargins(20, 40, 0, 40); /* int aleft, int atop, int aright, int abottom */
        
        layout = new MKeyboardLayout(MKeyboardPanel::TYPE_DEFAULT_60);
        layout->setObjectName(QString::fromUtf8(objHead.toUtf8()+"layout"));

        m_widget->setLayout(layout->layout());
    }
    QString objectNameHead() const { return objHead; };
    void setObjectNameHead(const QString &name) { objHead = name + "_"; }
private:
    QString objHead = nullptr;
};
class GT64HeWidgetUiFastLocation {
public:
    QVBoxLayout *m_layout;
    QLabel *label;
    QVector<QPushButton *> btn;
    void setupUi(QWidget *form = nullptr) {
        m_layout = new QVBoxLayout(form);
        m_layout->setObjectName(QString::fromUtf8(objHead.toUtf8()+"m_layout"));
        m_layout->setContentsMargins(20, 0, 0, 0);

        QString btn_qss;
        QFile qss_file(":/styles/kb_key.qss");
        if (qss_file.open(QFile::ReadOnly)) {
            btn_qss = QLatin1String(qss_file.readAll());
            qss_file.close();
        }
        QStringList str_fast = {"game", "letter", "number", "invert", "all", "cancel"};

        label = new QLabel();
        label->setObjectName(QString::fromUtf8(objHead.toUtf8()+"label"));
        label->setAlignment(Qt::AlignCenter);
        label->setText("fast location");

        m_layout->addStretch();
        m_layout->addWidget(label);
        btn.clear();
        for (int i = 0; i < str_fast.size(); i++) {
            btn.append(new QPushButton());
            btn[i]->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_"+str_fast[i].toUtf8()));
            btn[i]->setText(str_fast[i]);
            btn[i]->setStyleSheet(btn_qss);
            m_layout->addSpacing(10);
            m_layout->addWidget(btn[i]);
        }
        m_layout->addStretch();
    }
    QString objectNameHead() const { return objHead; };
    void setObjectNameHead(const QString &name) { objHead = name + "_"; }
private:
    QString objHead = nullptr;
};
class GT64HeWidgetUiParamProperty {
public:
    QHBoxLayout *m_layout;

    QGroupBox *groupBoxLeft;
    QGridLayout *propertyGridLayout;
    QButtonGroup *radioButtonGroup;
    QVector<QRadioButton *> radioButton;

    QGroupBox *groupBoxMiddle;
    QGridLayout *layoutMiddle;
    QStackedWidget *stackedWidget;
    QVector<QLabel *>label_param;
    QVector<MSlider *>slider_param;
    QVector<MDoubleSpinBox *>spinBox_param;
    
    QGroupBox *groupBoxRight;
    QGridLayout *noteGridLayout;


    void setupUi(QWidget *form = nullptr) {
        m_layout = new QHBoxLayout(form);
        m_layout->setObjectName(QString::fromUtf8(objHead.toUtf8()+"m_layout"));
        
        QString qss_text;
        QFile qss_file(":/styles/group_box.qss");
        if (qss_file.open(QFile::ReadOnly)) {
            qss_text = QLatin1String(qss_file.readAll());
            qss_file.close();
        }
        groupBoxLeftSetupUi(qss_text);
        groupBoxMiddleSetupUi(qss_text);
        groupBoxRightSetupUi(qss_text);

        m_layout->addWidget(groupBoxLeft, 1);
        m_layout->addWidget(groupBoxMiddle, 1);
        m_layout->addWidget(groupBoxRight, 1);
        
    }

    void groupBoxLeftSetupUi(QString style = nullptr, QWidget *form = nullptr) {
        groupBoxLeft = new QGroupBox(form);
        groupBoxLeft->setObjectName(QString::fromUtf8(objHead.toUtf8()+"groupBoxLeft"));
        // groupBoxLeft->setAlignment(Qt::AlignCenter);
        groupBoxLeft->setTitle("Property");
        groupBoxLeft->setStyleSheet(style);

        propertyGridLayout = new QGridLayout(groupBoxLeft);
        propertyGridLayout->setObjectName(QString::fromUtf8(objHead.toUtf8()+"propertyGridLayout"));
        
        radioButtonGroup = new QButtonGroup(propertyGridLayout);
        radioButtonGroup->setObjectName(QString::fromUtf8(objHead.toUtf8()+"radioButtonGroup"));

        QStringList str_obj  = {"dft", "rt", "rtp"};
        QStringList str_text = {"default", "rapid trigger", "rapid trigger pro"};
        for (int i = 0; i < str_obj.size(); i++) {
            radioButton.append(new QRadioButton());
            radioButton[i]->setObjectName(QString::fromUtf8(objHead.toUtf8()+"radioButton_"+str_obj[i].toUtf8()));
            radioButton[i]->setText(str_text[i]);
            radioButtonGroup->addButton(radioButton[i], i);
            propertyGridLayout->addWidget(radioButton[i], i, 0);
        }
        radioButtonGroup->button(0)->setChecked(true);
    }


    void groupBoxMiddleSetupUi(QString style = nullptr, QWidget *form = nullptr) {
        groupBoxMiddle = new QGroupBox(form);
        groupBoxMiddle->setObjectName(QString::fromUtf8(objHead.toUtf8()+"groupBoxMiddle"));
        // groupBoxMiddle->setAlignment(Qt::AlignCenter);
        groupBoxMiddle->setTitle("Paam");
        groupBoxMiddle->setStyleSheet(style);
        
        stackedWidget = new QStackedWidget();
        stackedWidget->setObjectName(QString::fromUtf8(objHead.toUtf8()+"stackedWidget"));

        layoutMiddle = new QGridLayout(groupBoxMiddle);
        layoutMiddle->setObjectName(QString::fromUtf8(objHead.toUtf8()+"layoutMiddle"));
        layoutMiddle->addWidget(stackedWidget);

        for (int i = 0; i < radioButton.size(); i++) {
            QWidget *page = new QWidget();
            stackedWidget->addWidget(page);
            switch (i) {
                case 0: middlePage0SetupUi(page);   break;
                case 1: middlePage1SetupUi(page);   break;
                case 2: middlePage2SetupUi(page);   break;
            }
        }
        stackedWidget->setCurrentIndex(radioButtonGroup->checkedId());
    }

    void middlePage0SetupUi(QWidget *form = nullptr) {
        QGridLayout *pageLayout = new QGridLayout(form);

        QFont font;
        font.setPointSize(12);
        //trigger
        QGridLayout *trgLayout = new QGridLayout();
        label_param.append(new QLabel());
        label_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"label_param"+QString::number(label_param.size() - 1).toUtf8()));
        label_param.last()->setText(QString("trigger point"));
        label_param.last()->setFont(font);

        slider_param.append(new MSlider(Qt::Horizontal));
        slider_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"slider_param"+QString::number(slider_param.size() - 1).toUtf8()));
        slider_param.last()->setTracking(false); //仅在用户释放滑块时才发出 valueChanged 信号
        slider_param.last()->setFocusPolicy(Qt::NoFocus);
        slider_param.last()->setMouseTracking(false);
        slider_param.last()->setTabletTracking(false);
        slider_param.last()->setRange(10, 4000);
        slider_param.last()->setTickPosition(QSlider::TicksBothSides);
        slider_param.last()->setTickInterval(500);
        
        
        spinBox_param.append(new MDoubleSpinBox());
        spinBox_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"spinBox_param"+QString::number(spinBox_param.size() - 1).toUtf8()));
        spinBox_param.last()->setDecimals(3);
        spinBox_param.last()->setRange(0.01, 4.0);
        spinBox_param.last()->setSingleStep(0.01);
        spinBox_param.last()->setKeyboardTracking(false);

        trgLayout->addWidget(label_param.last(), 0, 0, 1, 1);
        trgLayout->addWidget(slider_param.last(), 1, 0, 1, 1);
        trgLayout->addWidget(spinBox_param.last(), 1, 1, 1, 1);
        
        //release
        QGridLayout *relLayout = new QGridLayout();
        label_param.append(new QLabel());
        label_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"label_param"+QString::number(label_param.size() - 1).toUtf8()));
        label_param.last()->setText(QString("release point"));
        label_param.last()->setFont(font);

        slider_param.append(new MSlider(Qt::Horizontal));
        slider_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"slider_param"+QString::number(slider_param.size() - 1).toUtf8()));
        slider_param.last()->setTracking(false);
        slider_param.last()->setRange(10, 4000);
        
        
        spinBox_param.append(new MDoubleSpinBox());
        spinBox_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"spinBox_param"+QString::number(spinBox_param.size() - 1).toUtf8()));
        spinBox_param.last()->setDecimals(3);
        spinBox_param.last()->setRange(0.01, 4.0);
        spinBox_param.last()->setSingleStep(0.01);
        spinBox_param.last()->setKeyboardTracking(false);

        relLayout->addWidget(label_param.last(), 0, 0, 1, 1);
        relLayout->addWidget(slider_param.last(), 1, 0, 1, 1);
        relLayout->addWidget(spinBox_param.last(), 1, 1, 1, 1);

        pageLayout->addLayout(trgLayout, 0, 0, 1, 1);
        pageLayout->addLayout(relLayout, 1, 0, 1, 1);

    }
    void middlePage1SetupUi(QWidget *form = nullptr) {
        QGridLayout *pageLayout = new QGridLayout(form);

        QFont font;
        font.setPointSize(12);
        //trigger
        QGridLayout *trgLayout = new QGridLayout();
        label_param.append(new QLabel());
        label_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"label_param"+QString::number(label_param.size() - 1).toUtf8()));
        label_param.last()->setText(QString("trigger point"));
        label_param.last()->setFont(font);

        slider_param.append(new MSlider(Qt::Horizontal));
        slider_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"slider_param"+QString::number(slider_param.size() - 1).toUtf8()));
        slider_param.last()->setTracking(false);
        slider_param.last()->setRange(10, 4000);

        spinBox_param.append(new MDoubleSpinBox());
        spinBox_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"spinBox_param"+QString::number(spinBox_param.size() - 1).toUtf8()));
        spinBox_param.last()->setDecimals(3);
        spinBox_param.last()->setRange(0.01, 4.0);
        spinBox_param.last()->setSingleStep(0.01);
        spinBox_param.last()->setKeyboardTracking(false);

        trgLayout->addWidget(label_param.last(), 0, 0, 1, 1);
        trgLayout->addWidget(slider_param.last(), 1, 0, 1, 1);
        trgLayout->addWidget(spinBox_param.last(), 1, 1, 1, 1);
        
        //release
        QGridLayout *relLayout = new QGridLayout();
        label_param.append(new QLabel());
        label_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"label_param"+QString::number(label_param.size() - 1).toUtf8()));
        label_param.last()->setText(QString("release point"));
        label_param.last()->setFont(font);

        slider_param.append(new MSlider(Qt::Horizontal));
        slider_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"slider_param"+QString::number(slider_param.size() - 1).toUtf8()));
        slider_param.last()->setTracking(false);
        slider_param.last()->setRange(10, 4000);
        slider_param.last()->setValue(1000); //default
        
        spinBox_param.append(new MDoubleSpinBox());
        spinBox_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"spinBox_param"+QString::number(spinBox_param.size() - 1).toUtf8()));
        spinBox_param.last()->setDecimals(3);
        spinBox_param.last()->setRange(0.01, 4.0);
        spinBox_param.last()->setSingleStep(0.01);
        spinBox_param.last()->setKeyboardTracking(false);

        relLayout->addWidget(label_param.last(), 0, 0, 1, 1);
        relLayout->addWidget(slider_param.last(), 1, 0, 1, 1);
        relLayout->addWidget(spinBox_param.last(), 1, 1, 1, 1);

        //rt down
        QGridLayout *rtdLayout = new QGridLayout();
        label_param.append(new QLabel());
        label_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"label_param"+QString::number(label_param.size() - 1).toUtf8()));
        label_param.last()->setText(QString("rt down"));
        label_param.last()->setFont(font);

        slider_param.append(new MSlider(Qt::Horizontal));
        slider_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"slider_param"+QString::number(slider_param.size() - 1).toUtf8()));
        slider_param.last()->setTracking(false);
        slider_param.last()->setRange(10, 4000);
        slider_param.last()->setValue(1000); //default
        
        spinBox_param.append(new MDoubleSpinBox());
        spinBox_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"spinBox_param"+QString::number(spinBox_param.size() - 1).toUtf8()));
        spinBox_param.last()->setDecimals(3);
        spinBox_param.last()->setRange(0.01, 4.0);
        spinBox_param.last()->setSingleStep(0.01);
        spinBox_param.last()->setKeyboardTracking(false);

        rtdLayout->addWidget(label_param.last(), 0, 0, 1, 1);
        rtdLayout->addWidget(slider_param.last(), 1, 0, 1, 1);
        rtdLayout->addWidget(spinBox_param.last(), 1, 1, 1, 1);

        //rt up
        QGridLayout *rtuLayout = new QGridLayout();
        label_param.append(new QLabel());
        label_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"label_param"+QString::number(label_param.size() - 1).toUtf8()));
        label_param.last()->setText(QString("rt up"));
        label_param.last()->setFont(font);

        slider_param.append(new MSlider(Qt::Horizontal));
        slider_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"slider_param"+QString::number(slider_param.size() - 1).toUtf8()));
        slider_param.last()->setTracking(false);
        slider_param.last()->setRange(10, 4000);
        slider_param.last()->setValue(1000); //default
        
        spinBox_param.append(new MDoubleSpinBox());
        spinBox_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"spinBox_param"+QString::number(spinBox_param.size() - 1).toUtf8()));
        spinBox_param.last()->setDecimals(3);
        spinBox_param.last()->setRange(0.01, 4.0);
        spinBox_param.last()->setSingleStep(0.01);
        spinBox_param.last()->setKeyboardTracking(false);

        rtuLayout->addWidget(label_param.last(), 0, 0, 1, 1);
        rtuLayout->addWidget(slider_param.last(), 1, 0, 1, 1);
        rtuLayout->addWidget(spinBox_param.last(), 1, 1, 1, 1);

        pageLayout->addLayout(trgLayout, 0, 0, 1, 1);
        pageLayout->addLayout(relLayout, 1, 0, 1, 1);
        pageLayout->addLayout(rtdLayout, 2, 0, 1, 1);
        pageLayout->addLayout(rtuLayout, 3, 0, 1, 1);

    }
    void middlePage2SetupUi(QWidget *form = nullptr) {
        QHBoxLayout *pageLayout = new QHBoxLayout(form);
        label_param.append(new QLabel());
        label_param.last()->setObjectName(QString::fromUtf8(objHead.toUtf8()+"label_param"+QString::number(label_param.size() - 1).toUtf8()));
        label_param.last()->setText(QString("Page %1 Content").arg(label_param.size() - 1));
        pageLayout->addWidget(label_param.last());
    }
    




    void groupBoxRightSetupUi(QString style = nullptr, QWidget *form = nullptr) {
        groupBoxRight = new QGroupBox(form);
        groupBoxRight->setObjectName(QString::fromUtf8(objHead.toUtf8()+"groupBoxRight"));
        // groupBoxRight->setAlignment(Qt::AlignCenter);
        groupBoxRight->setTitle("Note");
        groupBoxRight->setStyleSheet(style);
        
        noteGridLayout = new QGridLayout(groupBoxRight);
        noteGridLayout->setObjectName(QString::fromUtf8(objHead.toUtf8()+"noteGridLayout"));

        QLabel *label_param0 = new QLabel();
        label_param0->setObjectName(QString::fromUtf8(objHead.toUtf8()+"label_param0"));
        label_param0->setText("param0");
        
        noteGridLayout->addWidget(label_param0, 0, 0);
    }


    QString objectNameHead() const { return objHead; };
    void setObjectNameHead(const QString &name) { objHead = name + "_"; }
private:
    QString objHead = nullptr;
};
class GT64HeWidgetUiParamDbg {
public:
    // QVector<QFrame *> lines;
    QCheckBox *panel;
    QCheckBox *dft_border;
    QCheckBox *dft_font;
    QCheckBox *hover_font;
    QCheckBox *dft_background;
    QCheckBox *hover_background;
    QCheckBox *checked_background;
    QCheckBox *checked0_background;
    QCheckBox *checked1_background;
    QPushButton *btn_setColor;

    QPushButton *btn_usbTx;
    QPushButton *btn_usbRx;
    QPushButton *btn_usbRxStop;

    QPushButton *btn_color_panel;
    QPushButton *btn_color_dft_border;
    QPushButton *btn_color_dft_font;
    QPushButton *btn_color_hover_font;
    QPushButton *btn_color_dft_background;
    QPushButton *btn_color_hover_background;
    QPushButton *btn_color_checked_background;
    QPushButton *btn_color_checked0_background;
    QPushButton *btn_color_checked1_background;

    QGridLayout *gridLayout;
    void setupUi(QWidget *parent = nullptr) {
        gridLayout = new QGridLayout(parent);
        gridLayout->setObjectName(QString::fromUtf8(objHead.toUtf8()+"gridLayout"));

        dft_border = new QCheckBox(parent);
        dft_border->setObjectName(QString::fromUtf8(objHead.toUtf8()+"dft_border"));
        dft_border->setText("Dft Border");

        dft_font = new QCheckBox(parent);
        dft_font->setObjectName(QString::fromUtf8(objHead.toUtf8()+"dft_font"));
        dft_font->setText("Dft Font");

        hover_font = new QCheckBox(parent);
        hover_font->setObjectName(QString::fromUtf8(objHead.toUtf8()+"hover_font"));
        hover_font->setText("Hover Font");

        dft_background = new QCheckBox(parent);
        dft_background->setObjectName(QString::fromUtf8(objHead.toUtf8()+"dft_background"));
        dft_background->setText("Dft Background");

        hover_background = new QCheckBox(parent);
        hover_background->setObjectName(QString::fromUtf8(objHead.toUtf8()+"hover_background"));
        hover_background->setText("Hover Background");

        checked_background = new QCheckBox(parent);
        checked_background->setObjectName(QString::fromUtf8(objHead.toUtf8()+"checked_background"));
        checked_background->setText("Checked Background");

        checked0_background = new QCheckBox(parent);
        checked0_background->setObjectName(QString::fromUtf8(objHead.toUtf8()+"checked0_background"));
        checked0_background->setText("Checked0 Background");

        checked1_background = new QCheckBox(parent);
        checked1_background->setObjectName(QString::fromUtf8(objHead.toUtf8()+"checked1_background"));
        checked1_background->setText("checked1 background");

        panel = new QCheckBox(parent);
        panel->setObjectName(QString::fromUtf8(objHead.toUtf8()+"panel"));
        panel->setText("panel");

        btn_setColor = new QPushButton();
        btn_setColor->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_setColor"));
        btn_setColor->setText("setColor");

        btn_usbTx = new QPushButton();
        btn_usbTx->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_usbTx"));
        btn_usbTx->setText("usbTx");

        btn_usbRx = new QPushButton();
        btn_usbRx->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_usbRx"));
        btn_usbRx->setText("usbRx");

        btn_usbRxStop = new QPushButton();
        btn_usbRxStop->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_usbRxStop"));
        btn_usbRxStop->setText("usbRxStop");

        btn_color_panel = new QPushButton();
        btn_color_panel->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_color_panel"));
        btn_color_panel->setFixedSize(30, 20);
        btn_color_panel->setStyleSheet("background-color: #000000; border: 1px solid #888888;");

        btn_color_dft_border = new QPushButton();
        btn_color_dft_border->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_color_dft_border"));
        btn_color_dft_border->setFixedSize(30, 20);
        btn_color_dft_border->setStyleSheet("background-color: #000000; border: 1px solid #888888;");

        btn_color_dft_font = new QPushButton();
        btn_color_dft_font->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_color_dft_font"));
        btn_color_dft_font->setFixedSize(30, 20);
        btn_color_dft_font->setStyleSheet("background-color: #000000; border: 1px solid #888888;");

        btn_color_hover_font = new QPushButton();
        btn_color_hover_font->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_color_hover_font"));
        btn_color_hover_font->setFixedSize(30, 20);
        btn_color_hover_font->setStyleSheet("background-color: #000000; border: 1px solid #888888;");

        btn_color_dft_background = new QPushButton();
        btn_color_dft_background->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_color_dft_background"));
        btn_color_dft_background->setFixedSize(30, 20);
        btn_color_dft_background->setStyleSheet("background-color: #000000; border: 1px solid #888888;");

        btn_color_hover_background = new QPushButton();
        btn_color_hover_background->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_color_hover_background"));
        btn_color_hover_background->setFixedSize(30, 20);
        btn_color_hover_background->setStyleSheet("background-color: #000000; border: 1px solid #888888;");

        btn_color_checked_background = new QPushButton();
        btn_color_checked_background->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_color_checked_background"));
        btn_color_checked_background->setFixedSize(30, 20);
        btn_color_checked_background->setStyleSheet("background-color: #000000; border: 1px solid #888888;");

        btn_color_checked0_background = new QPushButton();
        btn_color_checked0_background->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_color_checked0_background"));
        btn_color_checked0_background->setFixedSize(30, 20);
        btn_color_checked0_background->setStyleSheet("background-color: #000000; border: 1px solid #888888;");

        btn_color_checked1_background = new QPushButton();
        btn_color_checked1_background->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_color_checked1_background"));
        btn_color_checked1_background->setFixedSize(30, 20);
        btn_color_checked1_background->setStyleSheet("background-color: #000000; border: 1px solid #888888;");

        
        gridLayout->addWidget(panel,                0, 0, 1, 1);
        gridLayout->addWidget(btn_color_panel,      0, 1, 1, 1);
        gridLayout->addWidget(dft_border,           1, 0, 1, 1);
        gridLayout->addWidget(btn_color_dft_border, 1, 1, 1, 1);
        gridLayout->addWidget(dft_font,             2, 0, 1, 1);
        gridLayout->addWidget(btn_color_dft_font,   2, 1, 1, 1);
        gridLayout->addWidget(hover_font,           3, 0, 1, 1);
        gridLayout->addWidget(btn_color_hover_font, 3, 1, 1, 1);
        gridLayout->addWidget(dft_background,       4, 0, 1, 1);
        gridLayout->addWidget(btn_color_dft_background, 4, 1, 1, 1);
        gridLayout->addWidget(hover_background,     5, 0, 1, 1);
        gridLayout->addWidget(btn_color_hover_background, 5, 1, 1, 1);
        gridLayout->addWidget(checked_background,   6, 0, 1, 1);
        gridLayout->addWidget(btn_color_checked_background, 6, 1, 1, 1);
        gridLayout->addWidget(checked0_background,  7, 0, 1, 1);
        gridLayout->addWidget(btn_color_checked0_background, 7, 1, 1, 1);
        gridLayout->addWidget(checked1_background,  8, 0, 1, 1);
        gridLayout->addWidget(btn_color_checked1_background, 8, 1, 1, 1);
        gridLayout->addWidget(btn_setColor,         9, 0, 1, 1);
        gridLayout->addWidget(btn_usbTx,            9, 1, 1, 1);
        gridLayout->addWidget(btn_usbRx,            10, 0, 1, 1);
        gridLayout->addWidget(btn_usbRxStop,        10, 1, 1, 1);
    }
    QString objectNameHead() const { return objHead; };
    void setObjectNameHead(const QString &name) { objHead = name + "_"; }
private:
    QString objHead = nullptr;
};

class GT64HeWidgetUiParam {
public: 
    QHBoxLayout *m_layout;
    MTabWidget *tabWidget;
    QVector<QWidget *> tabs;
    
    QPushButton *btn_prev;
    QPushButton *btn_next;

    // // 在底部GroupBox添加按钮
    // bottomLayout->addWidget(prevButton);
    // bottomLayout->addWidget(nextButton);

    // // 连接按钮信号
    // QObject::connect(prevButton, &QPushButton::clicked, [stack](){
    // stack->setCurrentIndex((stack->currentIndex() - 1 + 3) % 3);
    // });
    // QObject::connect(nextButton, &QPushButton::clicked, [stack](){
    // stack->setCurrentIndex((stack->currentIndex() + 1) % 3);
    // });


    GT64HeWidgetUiParamDbg *dbg;
    GT64HeWidgetUiParamProperty *property;
    void setupUi(QWidget *form = nullptr) {
        m_layout = new QHBoxLayout(form);
        m_layout->setObjectName(QString::fromUtf8(objHead.toUtf8()+"m_layout"));

        tabs.clear();
        QString tabName[TABWIDGET_MAX] = {"RGB", "KEY", "ADVANCED", "PROPERTY", "SWITCH", "CALIB", "DBG"};

        tabWidget = new MTabWidget(form);
        tabWidget->setObjectName(QString::fromUtf8(objHead.toUtf8()+"tabWidget"));
        tabWidget->setTabPosition(QTabWidget::South); //Tab底部
        for (uint8_t i = 0; i < TABWIDGET_MAX; i++) {
            tabs.append(new QWidget());
            tabWidget->addTab(tabs[i], tabName[i]);
        }
            
        btn_prev = new QPushButton(form);
        btn_prev->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_prev"));
        btn_prev->setText("Prev");

        btn_next = new QPushButton(form);
        btn_next->setObjectName(QString::fromUtf8(objHead.toUtf8()+"btn_next"));
        btn_next->setText("Next");
        
        m_layout->addWidget(btn_prev);
        m_layout->addWidget(tabWidget);
        m_layout->addWidget(btn_next);

        dbg = new GT64HeWidgetUiParamDbg;
        dbg->setObjectNameHead(QString::fromUtf8(objHead.toUtf8()+"dbg"));
        dbg->setupUi(tabs[TABWIDGET_DBG]);

        property = new GT64HeWidgetUiParamProperty;
        property->setObjectNameHead(QString::fromUtf8(objHead.toUtf8()+"property"));
        property->setupUi(tabs[TABWIDGET_RGB]);

        updateTabWidgetStyle(tabWidget);
    }
    QString objectNameHead() const { return objHead; };
    void setObjectNameHead(const QString &name) { objHead = name + "_"; }
    enum {
        TABWIDGET_RGB,
        TABWIDGET_KEY,
        TABWIDGET_ADV,
        TABWIDGET_PROPERTY,
        TABWIDGET_SW,
        TABWIDGET_CALIB,
        TABWIDGET_DBG,
        TABWIDGET_MAX,
    };
private:
    QString objHead = nullptr;
    void updateTabWidgetStyle(QTabWidget *tabWidget) {
        QString qss_text;
        QFile qss_file(":/styles/kb_tab_widget.qss");
        if (qss_file.open(QFile::ReadOnly)) {
            qss_text = QLatin1String(qss_file.readAll());
            qss_file.close();
        }
        tabWidget->setStyleSheet(qss_text);
    }
};



class Ui_GT64HeWidget
{
public:
    QWidget *ui;
    QWidget *widget;
    QVBoxLayout *verticalLayout;
    QGridLayout *gridLayout;

    GT64HeWidgetUiParam        *param;
    GT64HeWidgetUiSidebar      *sidebar;
    GT64HeWidgetUiStatebar     *statebar;
    GT64HeWidgetUiKeyboard     *keyboard;
    GT64HeWidgetUiFastLocation *fastLocation;

    QWidget *paramWidget;
    QGridLayout *paramGridLayout;


    /* Splitter */
    QSplitter *keyAndParamSplitter;
    QSplitter *sideAndParamSplitter;


    void setupUi(QWidget *form) {
        if (form->objectName().isEmpty()) {
            form->setObjectName(QString::fromUtf8("form"));
        }
        ui = form;
        ui->setWindowTitle("Keyboard GT64He");
        ui->setWindowIcon(QIcon(":/images/applet.png"));

        gridLayout = new QGridLayout(ui);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));

        sidebar = new GT64HeWidgetUiSidebar;
        sidebar->setObjectNameHead(QString::fromUtf8("sidebar"));
        sidebar->setupUi();

        statebar = new GT64HeWidgetUiStatebar;
        statebar->setObjectNameHead(QString::fromUtf8("statebar"));
        statebar->setupUi();

        keyboard = new GT64HeWidgetUiKeyboard;
        keyboard->setObjectNameHead(QString::fromUtf8("keyboard"));
        keyboard->setupUi();

        fastLocation = new GT64HeWidgetUiFastLocation;
        fastLocation->setObjectNameHead(QString::fromUtf8("fastLocation"));
        fastLocation->setupUi();

        param = new GT64HeWidgetUiParam;
        param->setObjectNameHead(QString::fromUtf8("param"));
        param->setupUi();

        /* param area */
        paramWidget = new QWidget();
        paramWidget->setObjectName(QString::fromUtf8("paramWidget"));
        
        paramGridLayout = new QGridLayout(paramWidget);
        paramGridLayout->setObjectName(QString::fromUtf8("paramGridLayout"));
        paramGridLayout->setContentsMargins(0, 0, 0, 0);
        paramGridLayout->setSpacing(0);

        paramGridLayout->addLayout(param->m_layout, 0, 0, 1, 1);
        paramGridLayout->addLayout(fastLocation->m_layout, 0, 1, 1, 1);
        paramWidget->setLayout(paramGridLayout);
        

        /* state/keyboard/param/fast */
        widget = new QWidget();
        widget->setObjectName(QString::fromUtf8("widget"));

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));

        keyAndParamSplitter = new QSplitter(Qt::Vertical);
        keyAndParamSplitter->setObjectName(QString::fromUtf8("keyAndParamSplitter"));
        keyAndParamSplitter->setChildrenCollapsible(false);
        keyAndParamSplitter->setHandleWidth(1);

        keyAndParamSplitter->addWidget(keyboard->m_widget);
        keyAndParamSplitter->addWidget(paramWidget);

        verticalLayout->addLayout(statebar->m_layout);
        verticalLayout->addWidget(keyAndParamSplitter);

        widget->setLayout(verticalLayout);

        /**/
        sideAndParamSplitter = new QSplitter(Qt::Horizontal);
        sideAndParamSplitter->setObjectName(QString::fromUtf8("sideAndParamSplitter"));
        sideAndParamSplitter->setChildrenCollapsible(false);
        sideAndParamSplitter->setHandleWidth(1);

        sideAndParamSplitter->addWidget(sidebar->m_widget);
        sideAndParamSplitter->addWidget(widget);
        
        QString str = "rgb(223, 216, 205)";
        keyAndParamSplitter->setStyleSheet("QSplitter::handle { background-color: "+str+"; }");
        sideAndParamSplitter->setStyleSheet("QSplitter::handle { background-color:"+str+"; }");

        gridLayout->addWidget(sideAndParamSplitter, 0, 0, 1, 1);

        QMetaObject::connectSlotsByName(ui);
    }

private:

    
    QString m_title;
};
namespace Ui {
   class GT64HeWidget: public Ui_GT64HeWidget {};
}
QT_END_NAMESPACE

#endif // UI_GT64HEWIDGET_H
