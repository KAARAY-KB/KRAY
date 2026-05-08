set(SUB_NAME console)

set(SRC
    console.h
    console_sink.h
    console_widget.h
    console.cpp
    console_widget.cpp
)

# 添加库
add_library(${SUB_NAME} STATIC
    ${SRC}
)

# 启用 Qt 的 MOC/UIC/RCC 自动处理（用于信号槽机制）
set_target_properties(${SUB_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# 链接库
target_link_libraries(${SUB_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Widgets
)

# 添加目录
target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
