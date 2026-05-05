#ifndef USBTHREAD_H
#define USBTHREAD_H

#include <QObject>
#include <QThread>
//#include <QEventLoop>
//#include <QTimer>


class USBThread : public QThread
{
    Q_OBJECT

public:
    USBThread(QObject *parent = nullptr);
//    explicit ~USBThread();

    enum run_type {
        RUN_TYPE_NONE = 0x00,
        RUN_TYPE_QUIT,
        RUN_TYPE_EVT,
    };


    void begin(run_type type);
    void change_run(run_type type);
    void abort(void);
    void keep(void);
    void end(void);


private:
    run_type run_ty = RUN_TYPE_NONE;
    struct timeval tv = {.tv_sec = 0, .tv_usec = 125};
    int isCmp = false;
    bool is_abort = false;

protected:
    void run();

signals:
    void sg_start(run_type type);
    void sg_quit(void);
};





#endif // USBTHREAD_H
