# ============================================================================
# usb_server/CMakeLists.txt - USB TCP 服务器构建文件
#
# 构建静态库 usb_server，提供进程内 TCP 服务器：
#   UsbServer   - TCP 监听、连接管理、消息分发
#   UsbHandler  - 请求处理（协议 → UsbController API）
#   UsbProtocol - 二进制协议编解码
#
# 依赖：
#   - usb_controller：USB 控制器核心库
#   - console：日志输出库
# ============================================================================

set(SUB_NAME usb_server)

# 收集所有源文件
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    *.cpp
    *.h
)

# 构建静态库
add_library(${SUB_NAME} STATIC ${SRC})

# 链接依赖
target_link_libraries(${SUB_NAME} PUBLIC
    usb_controller
    console
)

# Windows 平台需要 ws2_32 库
if(WIN32)
    target_link_libraries(${SUB_NAME} PUBLIC ws2_32)
endif()

# 头文件搜索路径
target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
