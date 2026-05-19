# ============================================================================
# src/CMakeLists.txt - USB 控制器核心库构建文件
#
# 构建静态库 usb_controller，仅包含核心模块：
#   usb_core/  - USB 核心模块（上下文、设备、设备管理器、事件线程、热插拔）
#   usb_io/    - USB 数据传输模块（同步传输、HID 设备、异步传输）
#   usb_controller.cpp - USB 控制器统一接口
#
# 依赖：
#   - libusb：USB 底层库（INTERFACE 库，通过 middlewares 提供）
#
# 注意：本库不依赖 Qt，可独立使用。
#       Qt 层设备封装（devices/）由上层 CMakeLists.txt 可选添加。
# ============================================================================

set(SUB_NAME usb_controller)

# 收集所有核心源文件
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    usb_core/*.cpp
    usb_io/*.cpp
    usb_controller.cpp
)

# 非Windows平台排除Win热插拔源文件
if(NOT WIN32)
    list(FILTER SRC EXCLUDE REGEX "usb_hotplug_win\\.cpp$")
endif()

# 构建静态库
add_library(${SUB_NAME} STATIC ${SRC})

# 启用位置无关代码（-fPIC），使静态库可被共享库链接
set_target_properties(${SUB_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# 链接依赖（仅 libusb，无 Qt）
target_link_libraries(${SUB_NAME} PUBLIC
    libusb
)

# 头文件搜索路径
# 当前目录（src/）：让 #include "usb_controller.hpp" 和 #include "usb_core/..." 能找到
target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
