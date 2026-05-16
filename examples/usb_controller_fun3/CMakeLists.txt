# ============================================================================
# CMakeLists.txt - USB 控制器演示项目根构建文件
#
# 功能说明：
#   定义项目名称、C++ 标准、平台相关的 libusb 库配置。
#   构建可执行文件 ex_usb_controller_fun3。
#
# 构建目标：
#   - ex_usb_controller_fun3：USB 控制器演示可执行文件
#   - usb_controller_fun3_lib：USB 控制器静态库（在 src/CMakeLists.txt 中定义）
#
# 平台支持：
#   - Linux：通过 pkg-config 查找 libusb-1.0
#   - Windows (MSVC)：使用预编译的 libusb-1.0.lib
#   - Windows (MinGW)：使用预编译的 libusb-1.0.a
# ============================================================================

cmake_minimum_required(VERSION 3.16)

set(LIB_NAME ex_usb_controller_fun3)

project(${LIB_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)        # 禁用编译器扩展（保证可移植性）

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)  # 目标 Windows 版本（Vista+）
    add_definitions(-DWIN32_LEAN_AND_MEAN)  # 精简 Windows 头文件
    add_definitions(-DNOMINMAX)             # 禁用 min/max 宏（避免与 std::min/max 冲突）
endif()

# ============================================================================
# 输出目录设置
# 所有可执行文件输出到 ${CMAKE_BINARY_DIR}/bin
# ============================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(middlewares/libusb)
add_subdirectory(src)

# ============================================================================
# 构建可执行文件
# ============================================================================
add_executable(${LIB_NAME} main.cpp) # 可执行文件源文件

# 链接依赖库
if(MINGW OR MSVC)
    # Windows：直接使用静态库文件路径，避免 -l 查找失败
    target_link_libraries(${LIB_NAME} PRIVATE 
        libusb_f3
        usb_controller_fun3_lib                  # USB 控制器静态库                  # 平台相关库（Windows: setupapi）
    )
else()
    # Linux：使用 pkg-config 返回的库名
    target_link_libraries(${LIB_NAME} PRIVATE 
        libusb_f3
        usb_controller_fun3_lib # USB 控制器静态库
    )
endif()
