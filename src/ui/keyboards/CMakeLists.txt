include(../../../cmake/helpers.cmake)

set(SUB_NAME keyboards)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    *.cpp
    *.h
)

# 添加库
add_library(${SUB_NAME}
    ${SRC}
)
# 链接库
target_link_libraries(${SUB_NAME} PUBLIC 
    elements
    usb_controller
    ctrl_device
    protocol
)
# 添加目录
target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ray
)
