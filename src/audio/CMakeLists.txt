include(../../cmake/helpers.cmake)

set(SUB_NAME audio)

# 添加子目录：FFT、采集层和处理层
add_subdirectory(fft)
add_subdirectory(core)
add_subdirectory(process)
