include(../../../cmake/helpers.cmake)

set(SUB_NAME music_rhythm)

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
    console
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
)

# Windows 平台链接音频相关系统库
if(WIN32)
    target_link_libraries(${SUB_NAME} PUBLIC
        ole32
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libmmdevapi.a
    )
endif()

# 添加目录
target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
