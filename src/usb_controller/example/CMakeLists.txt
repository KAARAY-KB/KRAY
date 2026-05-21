# ============================================================================
# example/CMakeLists.txt - 单元测试构建文件
#
# 构建所有独立的单元测试可执行文件：
#   - test_device_enum  ：设备枚举测试
#   - test_hid_open     ：HID 设备打开/关闭测试
#   - test_hid_io       ：HID 数据读写测试
#   - test_async        ：异步传输测试
#   - test_bulk         ：批量传输测试
#   - test_hotplug      ：热插拔监听测试
#   - test_auto_all     ：自动运行所有测试
# ============================================================================

# 单元测试名称列表
set(EXAMPLE_TESTS
    test_device_enum
    test_hid_open
    test_hid_io
    test_async
    test_bulk
    test_hotplug
    test_auto_all
    main
)

# 遍历并构建每个测试可执行文件
foreach(test_name ${EXAMPLE_TESTS})
    add_executable(usb_controller_${test_name} ${test_name}.cpp) # 构建可执行文件
    # 链接 USB 控制器静态库
    target_link_libraries(usb_controller_${test_name} PRIVATE
        usb_controller
        libusb
    )
endforeach()
