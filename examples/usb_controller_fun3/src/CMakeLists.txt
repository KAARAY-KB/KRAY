# ============================================================================
# src/CMakeLists.txt - USB 控制器静态库构建文件
#
# 功能说明：
#   将 USB 控制器库的所有源文件编译为静态库 usb_controller_fun3_lib。
#   该静态库被根 CMakeLists.txt 中的可执行文件链接使用。
#
# 源文件组织：
#   usb_core/ - USB 核心模块（上下文、设备、设备管理器、事件线程）
#   usb_io/   - USB 数据传输模块（同步传输、HID 设备、异步传输）
#   usb_controller.cpp - USB 控制器统一接口
#
# 依赖：
#   - libusb：USB 底层库（通过 pkg-config 或预编译库提供）
# ============================================================================

# ============================================================================
# 构建静态库 usb_controller_fun3_lib
#
# 包含所有 USB 控制器库的源文件：
#   1. usb_core/usb_context.cpp      - USB 上下文管理（libusb 初始化）
#   2. usb_core/usb_device.cpp       - USB 设备信息封装
#   3. usb_core/usb_device_manager.cpp - USB 设备管理器（枚举和查询）
#   4. usb_core/usb_event_thread.cpp   - libusb 事件处理线程
#   5. usb_io/sync_transfer.cpp      - 同步传输（控制/批量/中断）
#   6. usb_io/hid_device.cpp         - HID 设备操作
#   7. usb_io/async_transfer.cpp     - 异步传输
#   8. usb_controller.cpp            - USB 控制器统一接口
# ============================================================================
add_library(usb_controller_fun3_lib STATIC
    usb_core/usb_context.cpp
    usb_core/usb_device.cpp
    usb_core/usb_device_manager.cpp
    usb_core/usb_event_thread.cpp
    usb_core/usb_hotplug.cpp
    usb_io/sync_transfer.cpp
    usb_io/hid_device.cpp
    usb_io/async_transfer.cpp
    usb_controller.cpp
)

# ============================================================================
# 链接依赖
# PUBLIC 表示使用此库的目标也会自动链接 libusb
# ============================================================================
target_link_libraries(usb_controller_fun3_lib PUBLIC libusb)

# ============================================================================
# 头文件搜索路径
# PUBLIC 表示使用此库的目标也会自动包含此路径
# ${CMAKE_CURRENT_SOURCE_DIR} 指向 src/ 目录，
# 这样可以使用 #include "usb_core/usb_context.hpp" 等相对路径
# ============================================================================
target_include_directories(usb_controller_fun3_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBUSB_INCLUDE_DIR}
    ${LIBUSB_INCLUDE_DIRS}
)
