# 指定CMake的最低版本要求
cmake_minimum_required(VERSION 3.16)

# 定义项目名称、版本和使用的编程语言
project(KRAY VERSION 0.1 LANGUAGES CXX)

# AUTOUIC: 自动生成UI代码（从.ui文件生成C++代码）
set(CMAKE_AUTOUIC ON)
# AUTOMOC: 自动处理Qt的元对象编译器（处理signals/slots等）
set(CMAKE_AUTOMOC ON)
# AUTORCC: 自动处理Qt的资源编译器（处理.qrc资源文件）
set(CMAKE_AUTORCC ON)

# 设置C++标准为C++17，并且要求必须支持
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Qt库，优先找Qt6，反之则找Qt5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(TS_FILES ${CMAKE_SOURCE_DIR}/resources/translations/KRAY_zh_CN.ts)

# 定义项目的所有源文件
set(PROJECT_SOURCES
    # src/core/main.cpp
)

add_subdirectory(src/core)

# 根据Qt版本使用不同的构建方式
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    # Qt 6 使用新的可执行文件创建方式
    # MANUAL_FINALIZATION: 手动完成最终化，需要在后面调用qt_finalize_executable
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    # 创建翻译文件（Qt6方式）
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    # Qt5 的构建方式
    if(ANDROID)
        # Android平台构建为共享库
        add_library(${PROJECT_NAME} SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        # 其他平台构建为可执行文件
        add_executable(${PROJECT_NAME}
            ${PROJECT_SOURCES}
        )
    endif()

    # 创建翻译文件（Qt5方式）
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

# 链接Qt Widgets库到项目
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets
    core
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.${PROJECT_NAME})
endif()

# 设置目标属性的各种配置
set_target_properties(${PROJECT_NAME} PROPERTIES
    ${BUNDLE_ID_OPTION}                                    # Bundle标识符（仅Qt < 6.1.0）
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}        # Bundle版本号
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}  # 短版本字符串
    MACOSX_BUNDLE TRUE                                     # 启用macOS Bundle
    WIN32_EXECUTABLE TRUE                                  # Windows下使用GUI子系统（不显示控制台窗口）
)

# 包含GNU标准的安装目录变量
include(GNUInstallDirs)

# 配置安装规则
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .                                   # macOS Bundle安装到根目录
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}            # 库文件安装到lib目录
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}            # 可执行文件安装到bin目录
)

# Qt6需要调用finalize来完成可执行文件的构建
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
