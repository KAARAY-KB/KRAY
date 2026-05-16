# ============================================================================
# test/CMakeLists.txt - C API 共享库构建文件
#
# 构建 libusb_ctrl_capi.so 共享库，供 Python ctypes 加载
# ============================================================================

# 构建共享库
add_library(usb_ctrl_capi SHARED
    usb_ctrl_capi.cpp
)

# 链接 USB 控制器静态库和 libusb
target_link_libraries(usb_ctrl_capi PRIVATE
    usb_controller_fun3_lib
    libusb_f3
)

# 头文件搜索路径（usb_controller_fun3_lib 的 PUBLIC include 会自动传递）
target_include_directories(usb_ctrl_capi PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 设置输出名称
set_target_properties(usb_ctrl_capi PROPERTIES
    PREFIX "lib"  # 输出前缀
    OUTPUT_NAME "usb_ctrl_capi"  # 输出名称
    # CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILD_TYPE_DIR}/lib/static
    # CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILD_TYPE_DIR}/lib/dynamic
)
