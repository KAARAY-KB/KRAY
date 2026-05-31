include(../../../cmake/helpers.cmake)

set(SUB_NAME audio_fft)

# 只编译 AudioFft.h/cpp，排除 fftw 子目录
add_library(${SUB_NAME}
    AudioFft.cpp
    AudioFft.h
)

target_link_libraries(${SUB_NAME} PUBLIC
    console
)

target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# FFTW3 编译期可选：cmake -DUSE_FFTW3=ON .. 
option(USE_FFTW3 "Use FFTW3 for FFT computation instead of built-in Cooley-Tukey" ON)

if(USE_FFTW3)
    # Windows: 使用本地 FFTW3 库
    if(WIN32)
        # 根据编译器位数选择对应目录：32位编译用dll32，64位编译用dll64
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fftw-3.3.5-dll64/fftw3.h")
                set(FFTW3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fftw-3.3.5-dll64")
            endif()
        else()
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fftw-3.3.5-dll32/fftw3.h")
                set(FFTW3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fftw-3.3.5-dll32")
            endif()
        endif()

        if(FFTW3_DIR)
            set(FFTW3_INCLUDE_DIR "${FFTW3_DIR}")
            set(FFTW3_DLL "${FFTW3_DIR}/libfftw3f-3.dll")
            # MSVC: 使用 .lib 导入库；MinGW: 使用 .a 导入库；都没有则链接 .dll
            if(EXISTS "${FFTW3_DIR}/libfftw3f-3.lib")
                set(FFTW3_LIBRARY "${FFTW3_DIR}/libfftw3f-3.lib")
            elseif(EXISTS "${FFTW3_DIR}/libfftw3f-3.a")
                set(FFTW3_LIBRARY "${FFTW3_DIR}/libfftw3f-3.a")
            elseif(EXISTS "${FFTW3_DIR}/libfftw3f-3.dll")
                set(FFTW3_LIBRARY "${FFTW3_DIR}/libfftw3f-3.dll")
            endif()
            if(FFTW3_LIBRARY)
                set(FFTW3_FOUND TRUE)
            endif()
        endif()
    endif()

    # Linux/macOS: 使用系统安装的 FFTW3
    if(NOT FFTW3_FOUND)
        # Linux:  sudo apt install libfftw3-dev
        # macOS:  brew install fftw
        find_package(PkgConfig)
        if(PkgConfig_FOUND)
            pkg_check_modules(FFTW3 QUIET fftw3f)
        endif()

        if(NOT FFTW3_FOUND)
            find_path(FFTW3_INCLUDE_DIR NAMES fftw3.h)
            find_library(FFTW3_LIBRARY NAMES fftw3f fftw3f-3)
            if(FFTW3_INCLUDE_DIR AND FFTW3_LIBRARY)
                set(FFTW3_FOUND TRUE)
            endif()
        endif()
    endif()

    if(FFTW3_FOUND)
        message(STATUS "FFTW3 found: ${FFTW3_LIBRARY}")
        target_compile_definitions(${SUB_NAME} PUBLIC USE_FFTW3)
        target_include_directories(${SUB_NAME} PUBLIC ${FFTW3_INCLUDE_DIR})
        target_link_libraries(${SUB_NAME} PUBLIC ${FFTW3_LIBRARY})

        # Windows: 运行时需要 dll，复制到输出目录
        if(WIN32 AND EXISTS "${FFTW3_DLL}")
            add_custom_command(TARGET ${SUB_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${FFTW3_DLL}"
                    "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin/"
                COMMENT "Copying FFTW3 DLL to output directory"
            )
        endif()
    else()
        message(WARNING "FFTW3 not found, falling back to built-in Cooley-Tukey FFT")
        set(USE_FFTW3 OFF CACHE BOOL "" FORCE)
    endif()
endif()
