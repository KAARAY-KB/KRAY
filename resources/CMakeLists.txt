include(../cmake/helpers.cmake)

if (NOT CMAKE_AUTORCC)
set(CMAKE_AUTORCC ON)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS)

if(QT_VERSION_MAJOR EQUAL 5)
    qt5_add_resources(RES_RCC 
        ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc
    )

    list(APPEND SRC 
        ${RES_RCC}
    )
else()
    # Qt6: AUTORCC 已自动处理 .qrc，无需显式调用
    # 确保 resources.qrc 在 SRC 或同目录下被 CMake 发现
    # 如果没被自动包含，可以手动加：
    list(APPEND SRC 
        ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc
    )
endif()


set(LIB_NAME resources)
#file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
#    *.qrc
#)

# 添加库
add_library(${LIB_NAME}
    ${SRC}
)
# 链接库
target_link_libraries(${LIB_NAME} PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
)
# 添加目录
target_include_directories(${LIB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)



# set(CMAKE_AUTORCC ON)

# set(LIB_NAME resources)

# # 正确获取当前目录下的qrc
# set(RESOURCES_RCC ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc)

# # 创建OBJECT库
# add_library(resources_obj OBJECT ${RESOURCES_RCC})
# target_link_libraries(resources_obj PRIVATE Qt${QT_VERSION_MAJOR}::Core)

# # 创建静态库（包含OBJECT库）
# add_library(${LIB_NAME} STATIC $<TARGET_OBJECTS:resources_obj>)
# target_include_directories(resources PUBLIC 
#     ${CMAKE_CURRENT_SOURCE_DIR}
# )

# # 导出资源初始化函数
# target_compile_definitions(${LIB_NAME} PUBLIC
#     RESOURCES_INIT_FUNC=Q_INIT_RESOURCE(${LIB_NAME})
# )