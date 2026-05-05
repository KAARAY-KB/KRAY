#include "USBThread.h"

USBThread::USBThread(QObject *parent)
    : QThread(parent)
{
    connect(this, &USBThread::sg_quit, this, [this](){
        run_ty = RUN_TYPE_QUIT;
        qDebug("on_quit");
        if (isRunning() == true) {
            requestInterruption();
            quit();
            wait();
        }
    });

    connect(this, &USBThread::sg_start, this, [this](run_type type){
        run_ty = type;
        if (isRunning() != true) {
            start();
        }
    });
}

void USBThread::run()
{
    for (;;) {
        if (is_abort) {
            continue;
        }
        switch (run_ty) {
            case RUN_TYPE_QUIT:
                goto QUIT;
            break;

            case RUN_TYPE_NONE:
            break;

            case RUN_TYPE_EVT:
                // int r = libusb_handle_events(nullptr);
                int r = libusb_handle_events_timeout(nullptr, &tv);
                if (r != LIBUSB_SUCCESS) {
                    qDebug("%s", libusb_error_name(r));
                }
            break;
        }
    }
QUIT:
    qDebug("thread end");
}

void USBThread::end(void)
{
    qDebug("end");
    emit sg_quit();
}
void USBThread::begin(run_type type)
{
    emit sg_start(type);
}
void USBThread::change_run(run_type type)
{
    run_ty = type;
}
void USBThread::abort(void)
{
    is_abort = true;
}
void USBThread::keep(void)
{
    is_abort = false;
}

