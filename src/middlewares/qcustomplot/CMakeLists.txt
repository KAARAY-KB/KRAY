set(SUB_NAME qcustomplot)

set(CUSTOMPLOT_DEMO OFF)

set(SRC 
    qcustomplot.cpp
    qcustomplot.h
)

if (CUSTOMPLOT_DEMO)
    set(DEMO 
        examples/text-document-integration/mainwindow.cpp
        examples/text-document-integration/mainwindow.h
        examples/text-document-integration/mainwindow.ui
        examples/text-document-integration/qcpdocumentobject.cpp
        examples/text-document-integration/qcpdocumentobject.h
    )
else()
    set(DEMO )
endif()

add_library(${SUB_NAME} 
    ${SRC}
    ${DEMO}
)

target_link_libraries(${SUB_NAME} PUBLIC 
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

target_include_directories(${SUB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
# target_compile_definitions(${SUB_NAME} PRIVATE QCUSTOMPLOT_COMPILE_LIBRARY)
# target_compile_definitions(${SUB_NAME} PRIVATE QT_NO_PRINTER)  #禁用打印
