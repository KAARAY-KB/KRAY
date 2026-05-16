set(SUB_NAME libusb)

set(LIBUSB_LIBRARIES    )
set(LIBUSB_INCLUDE_DIRS )

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "${SUB_NAME} running on: windows")
    # 根据编译器架构选择正确的静态库
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # 64位编译器
        list(APPEND LIBUSB_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/MinGW64/static/libusb-1.0.a)
    else()
        # 32位编译器
        list(APPEND LIBUSB_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/MinGW32/static/libusb-1.0.a)
    endif()
    list(APPEND LIBUSB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "${SUB_NAME} running on: linux")
    list(APPEND LIBUSB_LIBRARIES    /usr/local/lib/libusb-1.0.so)
    list(APPEND LIBUSB_INCLUDE_DIRS /usr/local/include/libusb-1.0)

elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "${SUB_NAME} This platform has not yet undergone compatibility processing.")
    # macOS 使用 Homebrew 安装的 libusb
    list(APPEND LIBUSB_LIBRARIES    /opt/homebrew/lib/libusb-1.0.dylib)
    list(APPEND LIBUSB_INCLUDE_DIRS /opt/homebrew/include/libusb-1.0)

else()
    message(FATAL_ERROR "${SUB_NAME} not supported on this platform.")
endif()

add_library(${SUB_NAME} INTERFACE)
# 添加目录
target_include_directories(${SUB_NAME} INTERFACE ${LIBUSB_INCLUDE_DIRS})
# 链接库
target_link_libraries(${SUB_NAME} INTERFACE ${LIBUSB_LIBRARIES})

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(${SUB_NAME} INTERFACE setupapi winusb)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(Threads REQUIRED)
    target_link_libraries(${SUB_NAME} INTERFACE 
        pthread  # libusb on Linux requires pthread
        udev     # sometimes needed; if link error, try adding or removing
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(${SUB_NAME} INTERFACE
        "-framework IOKit"           # I/O Kit
        "-framework CoreFoundation"  # Core Foundation
    )
endif()
