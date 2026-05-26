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

# miniaudio 跨平台音频库，各平台需要链接的系统库
if(WIN32)
    target_link_libraries(${SUB_NAME} PUBLIC
        ole32
        winmm
        ksuser
    )
elseif(UNIX AND NOT APPLE)
    # Linux: miniaudio 使用 ALSA/PulseAudio 后端
    target_link_libraries(${SUB_NAME} PUBLIC
        dl
        pthread
        m
    )
elseif(APPLE)
    # macOS: miniaudio 使用 CoreAudio 后端
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
