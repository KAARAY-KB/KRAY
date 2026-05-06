set(SUB_NAME protocol)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    *.c
    *.cpp
    *.h
)

# 添加库
add_library(${SUB_NAME}
    ${SRC}
)
# 添加库
target_link_libraries(${SUB_NAME} PUBLIC 
    utils
)
# 添加目录
target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

