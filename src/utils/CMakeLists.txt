set(SUB_NAME utils)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    *.c
    *.cpp
    *.h
)
# 添加库
add_library(${SUB_NAME} 
    ${SRC}
)
# 添加目录
target_include_directories(${SUB_NAME} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

