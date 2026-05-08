include(../../cmake/helpers.cmake)


set(SUB_NAME usb_controller)

file(GLOB_RECURSE LIB_SRC CONFIGURE_DEPENDS
    USB*.cpp
    USB*.h
    devices/*.cpp
    devices/*.h
)

# 排除所有平台特定文件
list(FILTER LIB_SRC EXCLUDE REGEX "main\\.cpp$")
list(FILTER LIB_SRC EXCLUDE REGEX "hotplug/USBHotplugWindows")
list(FILTER LIB_SRC EXCLUDE REGEX "hotplug/USBHotplugLinux")
list(FILTER LIB_SRC EXCLUDE REGEX "hotplug/USBHotplugMac")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "${SUB_NAME} running on: windows")
    list(APPEND LIB_SRC
        hotplug/USBHotplugWindows.cpp
        hotplug/USBHotplugWindows.h
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "${SUB_NAME} running on: linux")
    list(APPEND LIB_SRC
        hotplug/USBHotplugLinux.cpp
        hotplug/USBHotplugLinux.h
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "${SUB_NAME} running on: macos")
    list(APPEND LIB_SRC
        hotplug/USBHotplugMac.cpp
        hotplug/USBHotplugMac.h
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    message(FATAL_ERROR "${SUB_NAME} This platform has not yet undergone compatibility processing.")
else()
    message(FATAL_ERROR "${SUB_NAME} not supported on this platform.")
endif()


# 添加库
add_library(${SUB_NAME} 
    ${LIB_SRC}
)
# 链接库
target_link_libraries(${SUB_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Widgets
    libusb
    console
)
# 添加目录
target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/devices
    ${CMAKE_CURRENT_SOURCE_DIR}/hotplug
)
