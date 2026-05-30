include(../../../cmake/helpers.cmake)

set(SUB_NAME audio_process)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    *.cpp
    *.h
)

add_library(${SUB_NAME}
    ${SRC}
)

target_link_libraries(${SUB_NAME} PUBLIC
    console
)

target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
