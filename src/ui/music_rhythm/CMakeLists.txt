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
    # 添加 mmdevapi 导入库搜索路径（部分 MinGW 版本缺少该库）
    # target_link_directories(${SUB_NAME} PUBLIC
    #     ${CMAKE_CURRENT_SOURCE_DIR}/lib
    # )
    target_link_libraries(${SUB_NAME} PUBLIC
        ole32
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libmmdevapi.a
    )
elseif(UNIX AND NOT APPLE)
    # Linux 平台链接 PulseAudio
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PULSE REQUIRED libpulse)
    target_include_directories(${SUB_NAME} PUBLIC ${PULSE_INCLUDE_DIRS})
    target_link_libraries(${SUB_NAME} PUBLIC ${PULSE_LIBRARIES})
elseif(APPLE)
    # macOS 平台链接 CoreAudio
    target_link_libraries(${SUB_NAME} PUBLIC
        "-framework CoreAudio"
        "-framework AudioUnit"
        "-framework CoreFoundation"
    )
endif()

# 添加目录
target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
