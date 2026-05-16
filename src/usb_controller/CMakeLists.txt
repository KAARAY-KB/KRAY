# ============================================================================
# CMakeLists.txt - USB 控制器库构建文件
#
# 构建静态库 usb_controller，包含新模块的所有源文件：
#   usb_core/  - USB 核心模块（上下文、设备、设备管理器、事件线程、热插拔）
#   usb_io/    - USB 数据传输模块（同步传输、HID 设备、异步传输）
#   usb_controller.cpp - USB 控制器统一接口
# ============================================================================

set(SUB_NAME usb_controller)

# 核心源文件
add_library(${SUB_NAME}
    usb_core/usb_context.cpp
    usb_core/usb_device.cpp
    usb_core/usb_device_manager.cpp
    usb_core/usb_event_thread.cpp
    usb_core/usb_hotplug_base.cpp
    usb_core/usb_hotplug_linux.cpp
    usb_io/sync_transfer.cpp
    usb_io/hid_device.cpp
    usb_io/async_transfer.cpp
    usb_controller.cpp
    devices/usb_device_info.cpp
    devices/usb_device_base.cpp
    devices/gt64he_device.cpp
)

# 启用位置无关代码
set_target_properties(${SUB_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# Windows 平台添加热插拔回退源文件
if(WIN32)
    target_sources(${SUB_NAME} PRIVATE
        usb_core/usb_hotplug_win.cpp
    )
endif()

# 链接依赖
target_link_libraries(${SUB_NAME} PUBLIC
    libusb
    console
)

# 头文件搜索路径
target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
