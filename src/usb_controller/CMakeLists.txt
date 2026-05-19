# ============================================================================
# CMakeLists.txt - USB 控制器模块编排文件
#
# 本文件是 usb_controller 模块的顶层入口，负责按需组合各子模块：
#
#   src/      - 核心库（必选）：USB 上下文、设备管理、数据传输、热插拔
#   example/  - 示例程序（可选）：交互式演示和单元测试
#   test/     - C API 测试（可选）：Python ctypes 共享库
#
# 构建选项：
#   USB_CTRL_BUILD_EXAMPLE - 构建示例程序（默认 OFF）
#   USB_CTRL_BUILD_TEST    - 构建 C API 测试（默认 OFF）
#
# 用法：
#   最小构建（仅核心库，无 Qt 依赖）：
#     cmake ...
#   完整构建（核心 + 示例 + 测试）：
#     cmake -DUSB_CTRL_BUILD_EXAMPLE=ON -DUSB_CTRL_BUILD_TEST=ON ...
# ============================================================================

# 核心库（必选，仅依赖 libusb）
add_subdirectory(src)

# ============================================================================
# 示例程序（可选）
# ============================================================================
option(USB_CTRL_BUILD_EXAMPLE "Build USB controller examples" OFF)

if(USB_CTRL_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()

# ============================================================================
# C API 测试共享库（可选）
# ============================================================================
option(USB_CTRL_BUILD_TEST "Build USB controller C API test" OFF)

if(USB_CTRL_BUILD_TEST)
    add_subdirectory(test)
endif()
