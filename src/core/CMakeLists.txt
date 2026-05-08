include(../../cmake/helpers.cmake)

set(SUB_NAME core)

# target_sources(${PROJECT_NAME} PRIVATE
#     main.cpp
#     kray.cpp
#     kray.h
#     kray.ui
# )


set(SRC
    main.cpp
    kray.cpp
    kray.h
    kray.ui
    t1.h
    t1.cpp
    t1.ui
    t2.h
    t2.cpp
    t2.ui
)

# 添加库
add_library(${SUB_NAME} STATIC
    ${SRC}
)
# 链接库
target_link_libraries(${SUB_NAME} PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    usb_manager
    console
)
# 添加目录
target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
