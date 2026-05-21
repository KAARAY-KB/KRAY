# ============================================================================
# devices_ctrl/CMakeLists.txt - USB 设备封装库构建文件
#
# 构建静态库 devices_ctrl，提供 Qt 层的 USB 设备封装：
#   usb_device_info  - 设备信息结构体（QMetaType 注册）
#   usb_device_base  - 设备基类（封装 UsbController）
#   gt64he_device    - GT-64HE 键盘设备
#
# 依赖：
#   - usb_controller：USB 控制器核心库
#   - console：日志输出库
#   - Qt::Core：QMetaType 等
# ============================================================================

set(SUB_NAME devices_ctrl)

# 收集所有源文件
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    *.cpp
    *.h
)

# 构建静态库
add_library(${SUB_NAME} STATIC ${SRC})

# 启用位置无关代码
set_target_properties(${SUB_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# 链接依赖
target_link_libraries(${SUB_NAME} PUBLIC
    usb_controller
    console
    Qt${QT_VERSION_MAJOR}::Core
)

# 头文件搜索路径
target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
