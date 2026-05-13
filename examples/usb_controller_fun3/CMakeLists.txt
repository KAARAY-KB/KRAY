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

# ============================================================================
# libusb 库配置
#
# 根据平台和编译器选择不同的 libusb 库路径和名称：
#   - MSVC：使用 VS2022 编译的静态库
#   - MinGW：使用 MinGW 编译的静态库
#   - Linux：通过 pkg-config 查找系统安装的 libusb
# ============================================================================

# libusb 头文件路径（所有平台通用）
set(LIBUSB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library/libusb/include)

if(MSVC)
    # ========================================================================
    # MSVC (Visual Studio) 编译器配置
    # ========================================================================
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(LIBUSB_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library/libusb/VS2022/MS64/static) # 64 位
    else()
        set(LIBUSB_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library/libusb/VS2022/MS32/static) # 32 位
    endif()
    set(LIBUSB_LIB_NAME libusb-1.0.lib) # MSVC 静态库文件名
    set(PLATFORM_LIBS setupapi)         # Windows 需要链接 setupapi
elseif(MINGW)
    # ========================================================================
    # MinGW 编译器配置
    # ========================================================================
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(LIBUSB_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library/libusb/MinGW64/static) # 64 位
        set(LIBUSB_LIB_NAME libusb-1.0.a) # MinGW 64 位静态库
    else()
        set(LIBUSB_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library/libusb/MinGW32/static) # 32 位
        set(LIBUSB_LIB_NAME libusb-1.0.a) # MinGW 32 位静态库
    endif()
    set(PLATFORM_LIBS setupapi) # Windows 需要链接 setupapi
else()
    # ========================================================================
    # Linux 配置（通过 pkg-config 查找 libusb）
    # ========================================================================
    find_package(PkgConfig REQUIRED)                    # 查找 pkg-config 工具
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)       # 查找 libusb-1.0 包
    set(LIBUSB_LIB_NAME ${LIBUSB_LIBRARIES})            # 使用 pkg-config 返回的库
    set(PLATFORM_LIBS "")                               # Linux 不需要额外平台库
endif()

add_subdirectory(src)

# ============================================================================
# 构建可执行文件
# ============================================================================
add_executable(${LIB_NAME} main.cpp) # 可执行文件源文件

# 链接依赖库
target_link_libraries(${LIB_NAME} PRIVATE 
    usb_controller_fun3_lib # USB 控制器静态库
    ${LIBUSB_LIB_NAME}      # libusb 库
    ${PLATFORM_LIBS}        # 平台相关库（Windows: setupapi）
)

# 头文件搜索路径
target_include_directories(${LIB_NAME} PRIVATE
    ${LIBUSB_INCLUDE_DIR}   # libusb 头文件路径
    ${LIBUSB_INCLUDE_DIRS}  # pkg-config 返回的额外头文件路径
)

# 库文件搜索路径
target_link_directories(${LIB_NAME} PRIVATE
    ${LIBUSB_LIB_DIR}       # libusb 库文件路径
)
