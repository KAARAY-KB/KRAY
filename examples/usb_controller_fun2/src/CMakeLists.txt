add_library(usb_controller_lib STATIC
    usb_core/usb_context.cpp
    usb_core/usb_device.cpp
    usb_core/usb_device_manager.cpp
    usb_transfer/usb_transfer.cpp
    usb_transfer/hid_device.cpp
    usb_transfer/async_transfer.cpp
    usb_controller.cpp
)

target_link_libraries(usb_controller_lib PUBLIC libusb)

target_include_directories(usb_controller_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
