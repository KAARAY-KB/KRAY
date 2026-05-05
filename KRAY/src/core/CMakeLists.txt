include(../../cmake/helpers.cmake)


set(SUB_NAME core)


# target_sources(${PROJECT_NAME} PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.h
#         ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp
#         ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.ui
# )

# if(PF_BUILD_TEST_WIDGET)
#     target_sources(${PROJECT_NAME} PRIVATE
#             ${CMAKE_CURRENT_SOURCE_DIR}/test.h
#             ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
#             ${CMAKE_CURRENT_SOURCE_DIR}/test.ui
#     )
# endif()


set(SRC
    main.cpp
    kray.cpp
    kray.h
    kray.ui
)

# 添加库
add_library(${SUB_NAME} STATIC
    ${SRC}
)
# 链接库
target_link_libraries(${SUB_NAME} PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets
)
# 添加目录
target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
