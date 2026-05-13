cmake_minimum_required(VERSION 3.15)

set (LIB_NAME ex_usb_controller)

project(${LIB_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# libusb配置
set(LIBUSB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library/libusb/include)

# 根据平台选择libusb库
if(MSVC)
    # Visual Studio
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(LIBUSB_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library/libusb/VS2022/MS64/static)
    else()
        set(LIBUSB_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library/libusb/VS2022/MS32/static)
    endif()
    set(LIBUSB_LIB_NAME libusb-1.0.lib)
    set(PLATFORM_LIBS setupapi)
elseif(MINGW)
    # MinGW
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(LIBUSB_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library/libusb/MinGW64/static)
        set(LIBUSB_LIB_NAME libusb-1.0.a)
    else()
        set(LIBUSB_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library/libusb/MinGW32/static)
        set(LIBUSB_LIB_NAME libusb-1.0.a)
    endif()
    set(PLATFORM_LIBS setupapi)
else()
    # Linux - 使用系统libusb
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    set(LIBUSB_LIB_NAME ${LIBUSB_LIBRARIES})
    set(PLATFORM_LIBS "")
endif()

# 源文件
set(SOURCES
    main.cpp
    src/core/UsbContext.cpp
    src/core/UsbDevice.cpp
    src/device/UsbDeviceInfo.cpp
    src/hid/HidInterface.cpp
    src/async/AsyncTransfer.cpp
)

set(HEADERS
    include/core/UsbContext.h
    include/core/UsbDevice.h
    include/device/UsbDeviceInfo.h
    include/hid/HidInterface.h
    include/async/AsyncTransfer.h
)

add_executable(${LIB_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${LIB_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LIBUSB_INCLUDE_DIR}
    ${LIBUSB_INCLUDE_DIRS}
)

target_link_directories(${LIB_NAME} PRIVATE
    ${LIBUSB_LIB_DIR}
)

target_link_libraries(${LIB_NAME} PRIVATE
    ${LIBUSB_LIB_NAME}
    ${PLATFORM_LIBS}
)
