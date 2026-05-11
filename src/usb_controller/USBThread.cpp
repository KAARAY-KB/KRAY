#include "USBThread.h"
#include "USBHelper.h"
#include "console.h"
#include <libusb.h>

USBThread::USBThread()
{
}

USBThread::~USBThread()
{
    end();
}

void USBThread::set_start_callback(start_callback cb)
{
    m_start_cb = cb;
}

void USBThread::set_quit_callback(quit_callback cb)
{
    m_quit_cb = cb;
}

void USBThread::begin(run_type type)
{
    run_ty = type;
    if (!m_running) {
        m_running = true;
        m_thread = std::thread(&USBThread::run, this);
    }
}

void USBThread::end(void)
{
    Console::out() << "end" << std::endl;
    run_ty = RUN_TYPE_QUIT;
    if (m_running && m_thread.joinable()) {
        m_thread.join();
        m_running = false;
    }
    if (m_quit_cb) {
        m_quit_cb();
    }
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

void USBThread::run()
{
    if (m_start_cb) {
        m_start_cb(run_ty);
    }

    for (;;) {
        if (is_abort) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        switch (run_ty) {
            case RUN_TYPE_QUIT:
                Console::out() << "thread end" << std::endl;
                return;

            case RUN_TYPE_NONE:
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                break;

            case RUN_TYPE_EVT:
                // int r = libusb_handle_events(nullptr);
                int r = libusb_handle_events_timeout(nullptr, &tv);
                if (r != LIBUSB_SUCCESS) {
                    Console::out() << "libusb_handle_events_timeout error: " << libusb_error_name(r) << std::endl;
                }
                break;
        }
    }
}
