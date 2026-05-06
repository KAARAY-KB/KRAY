include(../../../cmake/helpers.cmake)

set(SUB_NAME usb_manager)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    *.cpp
    *.h
)

# 添加库
add_library(${SUB_NAME} STATIC
    ${SRC}
)
# 链接库
target_link_libraries(${SUB_NAME} PUBLIC 
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    usb_controller
)
# 添加目录
target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
