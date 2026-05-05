# include(../../cmake/helpers.cmake)

if (NOT CMAKE_AUTORCC)
set(CMAKE_AUTORCC ON)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core Gui StateMachine)

set(LIB_NAME qt-material-widgets)

set(SRC
    lib/qtmaterialstyle.cpp
    lib/qtmaterialstyle.h
    lib/qtmaterialstyle_p.h
    lib/qtmaterialtheme.cpp
    lib/qtmaterialtheme.h
    lib/qtmaterialtheme_p.h
    lib/qtmaterialoverlaywidget.cpp
    lib/qtmaterialoverlaywidget.h
    lib/qtmaterialcheckable.cpp
    lib/qtmaterialcheckable.h
    lib/qtmaterialcheckable_p.h
    lib/qtmaterialcheckable_internal.cpp
    lib/qtmaterialcheckable_internal.h
    lib/qtmaterialripple.cpp
    lib/qtmaterialripple.h
    lib/qtmaterialrippleoverlay.cpp
    lib/qtmaterialrippleoverlay.h
    lib/qtmaterialstatetransition.cpp
    lib/qtmaterialstatetransition.h
    lib/qtmaterialstatetransitionevent.h
    qtmaterialappbar.cpp
    qtmaterialappbar.h
    qtmaterialappbar_p.h
    qtmaterialautocomplete.cpp
    qtmaterialautocomplete.h
    qtmaterialautocomplete_p.h
    qtmaterialautocomplete_internal.cpp
    qtmaterialautocomplete_internal.h
    qtmaterialavatar.cpp
    qtmaterialavatar.h
    qtmaterialavatar_p.h
    qtmaterialbadge.cpp
    qtmaterialbadge.h
    qtmaterialbadge_p.h
    qtmaterialcheckbox.cpp
    qtmaterialcheckbox.h
    qtmaterialcheckbox_p.h
    qtmaterialcircularprogress.cpp
    qtmaterialcircularprogress.h
    qtmaterialcircularprogress_p.h
    qtmaterialcircularprogress_internal.cpp
    qtmaterialcircularprogress_internal.h
    qtmaterialdialog.cpp
    qtmaterialdialog.h
    qtmaterialdialog_p.h
    qtmaterialdialog_internal.cpp
    qtmaterialdialog_internal.h
    qtmaterialdrawer.cpp
    qtmaterialdrawer.h
    qtmaterialdrawer_p.h
    qtmaterialdrawer_internal.cpp
    qtmaterialdrawer_internal.h
    qtmaterialfab.cpp
    qtmaterialfab.h
    qtmaterialfab_p.h
    qtmaterialflatbutton.cpp
    qtmaterialflatbutton.h
    qtmaterialflatbutton_p.h
    qtmaterialflatbutton_internal.cpp
    qtmaterialflatbutton_internal.h
    qtmaterialiconbutton.cpp
    qtmaterialiconbutton.h
    qtmaterialiconbutton_p.h
    qtmateriallist.cpp
    qtmateriallist.h
    qtmateriallist_p.h
    qtmateriallistitem.cpp
    qtmateriallistitem.h
    qtmateriallistitem_p.h
    qtmaterialmenu.cpp
    qtmaterialmenu.h
    qtmaterialmenu_p.h
    qtmaterialmenu_internal.cpp
    qtmaterialmenu_internal.h
    qtmaterialpaper.cpp
    qtmaterialpaper.h
    qtmaterialpaper_p.h
    qtmaterialprogress.cpp
    qtmaterialprogress.h
    qtmaterialprogress_p.h
    qtmaterialprogress_internal.cpp
    qtmaterialprogress_internal.h
    qtmaterialradiobutton.cpp
    qtmaterialradiobutton.h
    qtmaterialradiobutton_p.h
    qtmaterialraisedbutton.cpp
    qtmaterialraisedbutton.h
    qtmaterialraisedbutton_p.h
    qtmaterialscrollbar.cpp
    qtmaterialscrollbar.h
    qtmaterialscrollbar_p.h
    qtmaterialscrollbar_internal.cpp
    qtmaterialscrollbar_internal.h
    qtmaterialslider.cpp
    qtmaterialslider.h
    qtmaterialslider_p.h
    qtmaterialslider_internal.cpp
    qtmaterialslider_internal.h
    qtmaterialsnackbar.cpp
    qtmaterialsnackbar.h
    qtmaterialsnackbar_p.h
    qtmaterialsnackbar_internal.cpp
    qtmaterialsnackbar_internal.h
    layouts/qtmaterialsnackbarlayout.cpp
    layouts/qtmaterialsnackbarlayout.h
    layouts/qtmaterialsnackbarlayout_p.h
    qtmaterialtabs.cpp
    qtmaterialtabs.h
    qtmaterialtabs_p.h
    qtmaterialtabs_internal.cpp
    qtmaterialtabs_internal.h
    qtmaterialtable.cpp
    qtmaterialtable.h
    qtmaterialtable_p.h
    qtmaterialtextfield.cpp
    qtmaterialtextfield.h
    qtmaterialtextfield_p.h
    qtmaterialtextfield_internal.cpp
    qtmaterialtextfield_internal.h
    qtmaterialtoggle.cpp
    qtmaterialtoggle.h
    qtmaterialtoggle_p.h
    qtmaterialtoggle_internal.cpp
    qtmaterialtoggle_internal.h
    resources.qrc
)

add_library(${LIB_NAME} STATIC 
    ${SRC}
)

target_include_directories(${LIB_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${LIB_NAME} PUBLIC 
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::StateMachine
)
