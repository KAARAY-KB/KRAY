#ifndef USBTHREAD_H
#define USBTHREAD_H

#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <sys/time.h>

class USBThread
{
public:
    enum run_type {
        RUN_TYPE_NONE = 0x00,
        RUN_TYPE_QUIT,
        RUN_TYPE_EVT,
    };

    using start_callback = std::function<void(run_type)>;
    using quit_callback = std::function<void()>;

    USBThread();
    ~USBThread();

    void set_start_callback(start_callback cb);
    void set_quit_callback(quit_callback cb);

    void begin(run_type type);
    void change_run(run_type type);
    void abort(void);
    void keep(void);
    void end(void);

private:
    void run();

    std::thread m_thread;
    std::atomic<run_type> run_ty{RUN_TYPE_NONE};
    std::atomic<bool> m_running{false};
    std::atomic<bool> is_abort{false};
    struct timeval tv = {.tv_sec = 0, .tv_usec = 125};

    start_callback m_start_cb;
    quit_callback m_quit_cb;
};

#endif // USBTHREAD_H
