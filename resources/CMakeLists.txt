include(../cmake/helpers.cmake)

if (NOT CMAKE_AUTORCC)
set(CMAKE_AUTORCC ON)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS LinguistTools)

set(SUB_NAME resources)

set(RCC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc)
file(GLOB TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/translations/*.ts)
set(SRC )


if(QT_VERSION_MAJOR EQUAL 6)
    # Qt6: AUTORCC 已自动处理 .qrc，无需显式调用
    # 确保 resources.qrc 在 SRC 或同目录下被 CMake 发现
    # 如果没被自动包含，可以手动加：
    # qt_add_resources(RES_RCC ${RCC_FILES})
    qt_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR}/translations ${TS_FILES})
    list(APPEND SRC 
        ${RES_RCC}
        ${QM_FILES})

else()
    # Qt5: 需要显式调用 qt5_add_resources 来处理 .qrc 文件
    qt5_add_resources(RES_RCC ${RCC_FILES})
    qt5_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR}/translations ${TS_FILES})
    list(APPEND SRC 
        ${RES_RCC}
        ${QM_FILES}
    )

endif()



# 添加库
add_library(${SUB_NAME}
    ${SRC}
)
# 链接库
target_link_libraries(${SUB_NAME} PUBLIC 
    Qt${QT_VERSION_MAJOR}::Widgets
)
# 添加目录
target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# # 导出资源初始化函数
# target_compile_definitions(${SUB_NAME} PUBLIC
#     RESOURCES_INIT_FUNC=Q_INIT_RESOURCE(${SUB_NAME})
# )