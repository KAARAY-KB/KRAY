#include "t2.h"
#include "./ui_t2.h"

#include "qtmaterialappbar.h"
#include "qtmaterialflatbutton.h"
#include "qtmaterialtextfield.h"
#include "qtmaterialtoggle.h"
#include "qtmaterialslider.h"
#include "qtmaterialcircularprogress.h"
#include "qtmaterialsnackbar.h"
#include "qtmaterialcheckbox.h"
#include "qtmaterialraisedbutton.h"
#include "qtmaterialiconbutton.h"
#include "qtmaterialtabs.h"
#include "qtmaterialprogress.h"
#include "qtmaterialtheme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

#ifdef OS_WINDOWS
#include <Windows.h>
#endif

T2::T2(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::T2)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowTitle("T2 Widget");
    resize(400, 500);
    setup_ui();
}

T2::~T2()
{
    delete ui;
}

void T2::setup_ui()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(16);

    QtMaterialAppBar *appBar = new QtMaterialAppBar(this);
    QLabel *titleLabel = new QLabel("Material Demo", appBar);
    titleLabel->setStyleSheet("color: white; font-size: 18px;");
    appBar->setLayout(new QHBoxLayout());
    appBar->layout()->addWidget(titleLabel);
    mainLayout->addWidget(appBar);

    QWidget *contentWidget = new QWidget(this);
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    contentLayout->setSpacing(16);

    QtMaterialTextField *textField = new QtMaterialTextField(contentWidget);
    textField->setLabel("Input Field");
    contentLayout->addWidget(textField);

    QtMaterialToggle *toggle = new QtMaterialToggle(contentWidget);
    toggle->setChecked(true);
    contentLayout->addWidget(toggle);

    QtMaterialSlider *slider = new QtMaterialSlider(contentWidget);
    slider->setRange(0, 100);
    slider->setValue(50);
    contentLayout->addWidget(slider);

    QtMaterialCircularProgress *circularProgress = new QtMaterialCircularProgress(contentWidget);
    circularProgress->setProgressType(QtMaterialCircularProgress::Determinate);
    circularProgress->setValue(75);
    contentLayout->addWidget(circularProgress);

    QtMaterialProgress *progress = new QtMaterialProgress(contentWidget);
    progress->setValue(60);
    contentLayout->addWidget(progress);

    QtMaterialCheckbox *checkbox = new QtMaterialCheckbox(contentWidget);
    checkbox->setText("Enable Feature");
    checkbox->setChecked(true);
    contentLayout->addWidget(checkbox);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QtMaterialFlatButton *flatBtn = new QtMaterialFlatButton("Flat Button", contentWidget);
    QtMaterialRaisedButton *raisedBtn = new QtMaterialRaisedButton("Raised Button", contentWidget);
    btnLayout->addWidget(flatBtn);
    btnLayout->addWidget(raisedBtn);
    contentLayout->addLayout(btnLayout);

    QtMaterialTabs *tabs = new QtMaterialTabs(contentWidget);
    tabs->setTabs({"Tab 1", "Tab 2", "Tab 3"});
    contentLayout->addWidget(tabs);

    QtMaterialSnackbar *snackbar = new QtMaterialSnackbar(contentWidget);
    snackbar->addMessage("Hello from Material Design!");

    connect(raisedBtn, &QtMaterialRaisedButton::clicked, this, [snackbar]() {
        snackbar->addMessage("Button Clicked!");
    });

    mainLayout->addWidget(contentWidget);
    mainLayout->addStretch();
}

void T2::closeEvent(QCloseEvent *event)
{
    qDebug("t2 widget close event");
    event->accept();
    emit exitWindow();
    QWidget::closeEvent(event);

}

void T2::show_top(void)
{
    if (this->isMinimized())
    {
        this->showNormal();
    }
#ifdef OS_WINDOWS
    SetWindowPos(HWND(this->winId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetWindowPos(HWND(this->winId()), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif
    this->show();
    this->activateWindow();
}

