set(SUB_NAME libusb)

set(LIBUSB_LIBRARIES    )
set(LIBUSB_INCLUDE_DIRS )
set(LIBUSB_EXAMPLES_LISTDEVS OFF)

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
    message(FATAL_ERROR "${SUB_NAME} This platform has not yet undergone compatibility processing.")
    # macOS 使用 Homebrew 安装的 libusb
    list(APPEND LIBUSB_LIBRARIES    /opt/homebrew/lib/libusb-1.0.dylib)
    list(APPEND LIBUSB_INCLUDE_DIRS /opt/homebrew/include/libusb-1.0)

else()
    message(FATAL_ERROR "${SUB_NAME} not supported on this platform.")
endif()


# libusb examples
if (LIBUSB_EXAMPLES_LISTDEVS)
    set(EXAMPLES_LISTDEVS ./examples/source/listdevs.c)
else()
    set(EXAMPLES_LISTDEVS )
endif()

# 添加库
add_library(${SUB_NAME} STATIC
    ${EXAMPLES_LISTDEVS}
)
# 链接库
target_link_libraries(${SUB_NAME} PRIVATE 
    ${LIBUSB_LIBRARIES}
)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(${SUB_NAME} PRIVATE 
        setupapi  # Windows SetupAPI
        winusb    # Windows WinUSB
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(Threads REQUIRED)
    target_link_libraries(${SUB_NAME} PRIVATE 
        pthread  # libusb on Linux requires pthread
        udev     # sometimes needed; if link error, try adding or removing
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(${SUB_NAME} PRIVATE 
        "-framework IOKit"       # macOS I/O Kit
        "-framework CoreFoundation"  # macOS Core Foundation
    )
endif()

# 添加目录
target_include_directories(${SUB_NAME} PUBLIC
    ${LIBUSB_INCLUDE_DIRS}
)
