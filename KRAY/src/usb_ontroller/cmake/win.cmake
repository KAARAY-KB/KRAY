#[[
功能:
    添加库函数，并指定文件可见性
参数:
    1. TARGET_NAME:库名称
    2. _scope:文件可见性,可选 PUBLIC, PRIVATE, INTERFACE, 默认 PUBLIC
    3. ARGN:库文件
示例:
# ]]

function(sy_usb_controller_link TARGET_NAME)
    set(_scope PUBLIC) # 文件可见性,默认 PUBLIC
    if (ARGC GREATER 1) # 参数大于1
        list(GET ARGN 0 _maybe_scope) # 获取第一个参数
        if (_maybe_scope STREQUAL "PUBLIC" OR _maybe_scope STREQUAL "PRIVATE" OR _maybe_scope STREQUAL "INTERFACE") # 如果参数是 PUBLIC,PRIVATE 或 INTERFACE,则设置 _scope
            set(_scope ${_maybe_scope}) # 设置 _scope
            list(REMOVE_AT ARGN 0) # 移除第一个参数
        endif ()
    endif ()

    target_link_libraries(${TARGET_NAME} ${_scope} 
        setupapi #Windows SetupAPI library
        # winusb #Windows WinUSB library
        ${ARGN}
    ) # 添加文件可见性到库
endfunction()

