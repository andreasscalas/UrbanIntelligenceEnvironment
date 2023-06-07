cmake_minimum_required(VERSION 3.5)

project(UrIntEnv VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets LinguistTools REQUIRED)
find_package(VTK REQUIRED COMPONENTS
    CommonColor
    CommonComputationalGeometry
    CommonCore
    CommonDataModel
    FiltersCore
    FiltersExtraction
    FiltersGeneral
    FiltersSources
    GUISupportQt
    IOInfovis
    IOLegacy
    InfovisCore
    InfovisLayout
    InteractionStyle
    RenderingContextOpenGL2
    RenderingCore
    RenderingFreeType
    RenderingGL2PSOpenGL2
    RenderingLabel
    RenderingOpenGL2
    RenderingAnnotation
    ViewsInfovis
)

#find_package(OSMManager REQUIRED)
#find_package(GeoJSONParser REQUIRED)
#find_library(SHAPELIB shp Shp)
#find_path(SHAPELIB_H shapelib)
#find_package(GDAL CONFIG REQUIRED)
#find_package(RapidJSON)
#find_path(DATA_STRUCTURES_H DataStructures )
#find_library(DATA_STRUCTURES_LIB libDataStructures)
#find_package(PROJ)
#find_package(libLAS REQUIRED)
find_package(SemantisedTriangleMesh REQUIRED)
find_package(DrawableGeometries REQUIRED)
#find_package(CityDigitalTwin REQUIRED)

set(TS_FILES UrIntEnv_it_IT.ts)

set(PROJECT_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VTKInteractorStyles/measurestyle.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mainwindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/annotationdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/annotationrelationshipdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/measureslistwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/attributewidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/annotationselectiondialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/categorybutton.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VTKInteractorStyles/annotationselectioninteractorstyle.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VTKInteractorStyles/lineselectionstyle.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VTKInteractorStyles/measurestyle.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VTKInteractorStyles/triangleselectionstyle.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VTKInteractorStyles/verticesselectionstyle.cpp
        ${TS_FILES}
)

set(PROJECT_HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mainwindow.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/annotationdialog.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/annotationrelationshipdialog.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/measureslistwidget.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/attributewidget.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/annotationselectiondialog.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/categorybutton.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/VTKInteractorStyles/annotationselectioninteractorstyle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/VTKInteractorStyles/lineselectionstyle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/VTKInteractorStyles/measurestyle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/VTKInteractorStyles/triangleselectionstyle.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/VTKInteractorStyles/verticesselectionstyle.hpp
)

set(PROJECT_UI_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mainwindow.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/measureslistwidget.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/attributewidget.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/annotationrelationshipdialog.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/annotationselectiondialog.ui)

set( QRCs ${CMAKE_SOURCE_DIR}/icons/icons.qrc )
qt5_add_resources(QRC_Srcs ${QRCs} )

source_group("GUI Files" FILES ${PROJECT_UI_SRC})


source_group("Resources" FILES
    ${QRCs}
    ${EXE_ICON} # Not present
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(UrIntEnv
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${PROJECT_HDRS}
        ${PROJECT_UI_SRC}
        ${QRC_Srcs}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET UrIntEnv APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(UrIntEnv SHARED
            ${PROJECT_SOURCES}
            ${PROJECT_HDRS}
            ${PROJECT_UI_SRC}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(UrIntEnv
            ${PROJECT_SOURCES}
            ${PROJECT_HDRS}
            ${PROJECT_UI_SRC}
            ${QRC_Srcs}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(UrIntEnv PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    ${VTK_LIBRARIES}
    #GDAL::GDAL
    ${libLAS_LIBRARIES}
    ${PROJ_LIBRARIES}
    ${KDTree_LIB}
    ${DATA_STRUCTURES_LIB}
    ${SemantisedTriangleMesh_LIBRARIES}
    ${DrawableGeometries_LIBRARIES})
target_include_directories(UrIntEnv PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/VTKInteractorStyles/
    ${MAIN_FOLDER}/include/
    ${SHAPELIB_H}
    ${RAPIDJSON_H}
    ${PROJ_INCLUDE_DIRS}
    ${NANOFLANN}
    ${KDTree_H}
    ${DATA_STRUCTURES_H}
    ${SemantisedTriangleMesh_INCLUDE_DIRS}
    ${DrawableGeometries_INCLUDE_DIRS}
    ${OSMManager_INCLUDE_DIRS}
    ${GeoJSONParser_INCLUDE_DIRS})

set_target_properties(UrIntEnv PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(UrIntEnv)
endif()
