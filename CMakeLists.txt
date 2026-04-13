cmake_minimum_required(VERSION 3.16)

project(l4d2-mod-manager VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Sql)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

# 包含路径（解决提升控件找不到头文件的问题）
INCLUDE_DIRECTORIES(
    ${PROJECT_SOURCE_DIR}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)

    qt_add_executable(l4d2-mod-manager
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        gamemanager.h gamemanager.cpp
        sqliteobj.h sqliteobj.cpp
        header.h
        comp/modcard.h comp/modcard.cpp comp/modcard.ui
        comp/categorydialog.h comp/categorydialog.cpp  comp/categorydialog.ui
        comp/categorybutton.h comp/categorybutton.cpp comp/categorybutton.ui
        comp/checkboxlistwidget.h comp/checkboxlistwidget.cpp comp/checkboxlistwidget.ui
        utils/imageloader.h utils/imageloader.cpp

        widget/cardcontainer.h widget/cardcontainer.cpp widget/cardcontainer.ui
        widget/moddetailwidget.h widget/moddetailwidget.cpp widget/moddetailwidget.ui
        README.md
        Resources.qrc

        comp/paramcheckwidget.h comp/paramcheckwidget.cpp comp/paramcheckwidget.ui
        widget/categorylistwidget.h widget/categorylistwidget.cpp widget/categorylistwidget.ui
        utils/vpkfileparser.h utils/vpkfileparser.cpp



    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET l4d2-mod-manager APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(l4d2-mod-manager SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(l4d2-mod-manager
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(l4d2-mod-manager PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(l4d2-mod-manager PRIVATE Qt6::Core)
target_link_libraries(l4d2-mod-manager PRIVATE Qt6::Sql)
target_link_libraries(l4d2-mod-manager PRIVATE Qt6::Core)
target_link_libraries(l4d2-mod-manager PRIVATE Qt6::Core)
target_link_libraries(l4d2-mod-manager PRIVATE Qt6::Core)
target_link_libraries(l4d2-mod-manager PRIVATE Qt6::Core)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.l4d2-mod-manager)
endif()
set_target_properties(l4d2-mod-manager PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS l4d2-mod-manager
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(l4d2-mod-manager)
endif()
