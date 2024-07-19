#######################################################
##        General
#######################################################
cmake_minimum_required(VERSION 3.15)
project(Comp3D)
set(COMP3D_TARGET Comp3D)

set(TRANSLATIONS fr)

# strict C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

#######################################################
##        Options
#######################################################

option(ADD_PROJ_CC  "Add proj CC" ON)
option(ADD_PROJ_NTF "Add proj NTF" ON)
option(ADD_PROJ_UTM "Add Proj UTM" OFF)

option(USE_AUTO "Automatic mode" ON)

option(USE_GUI "GUI" ON)
if (NOT ${USE_GUI})
  option(USE_RES "ressources" ON)
else()
  set(USE_RES ON CACHE BOOL "ressources" FORCE)
endif()

if (NOT ${USE_RES})
  option(USE_QT "QT" ON)
else()
  set(USE_QT ON CACHE BOOL "QT" FORCE)
endif()

set(COMP3D_DOC_DIR "${CMAKE_SOURCE_DIR}/doc_uni" CACHE STRING "Path to doc")
if ("${COMP3D_DOC_DIR}" STREQUAL "")
  set(COMP3D_DOC_DIR "${CMAKE_SOURCE_DIR}/doc_uni" CACHE STRING  "Path to doc" FORCE)
endif()

set(PROJ_DATA_LOCAL_PATH "" CACHE STRING "Path to proj data")

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
# build type. Default: Release
if(NOT is_multi_config)
  set(default_build_type "Release")
  if(NOT CMAKE_BUILD_TYPE)
    message(NOTICE "-- Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
         STRING "Choose the type of build, options are: Release RelWithDebInfo Debug" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Release RelWithDebInfo Debug)
  endif()
endif()

#######################################################
##        Dependencies
#######################################################

# Eigen
find_package(Eigen3 REQUIRED NO_MODULE)

# Boost
set (BOOST_VERSION_MIN 1.60.0)
set (BOOST_COMPONENTS system date_time graph)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9")
  list(APPEND BOOST_COMPONENTS filesystem)
endif()
set (BOOST_COMPONENTS_FULL ${BOOST_COMPONENTS})
list (TRANSFORM BOOST_COMPONENTS_FULL PREPEND Boost::)
find_package(Boost ${BOOST_VERSION_MIN} REQUIRED COMPONENTS ${BOOST_COMPONENTS})

# Proj
find_package(PROJ QUIET)
if (NOT  PROJ_FOUND)
  # needed on Ubuntu
  find_package(PkgConfig)
  pkg_search_module(PROJ REQUIRED proj)
endif()
if (PROJ_FOUND)
  message(STATUS "PROJ ${PROJ_VERSION} FOUND: ${PROJ_BINARY_DIRS}")
endif()
if (CMAKE_HOST_WIN32)
  find_file(PROJ_PROJ_DB_FILE proj.db PATHS ${PROJ_DIR} ${PROJ_BINARY_DIRS}/../share/proj NO_DEFAULT_PATH)
  file(COPY ${PROJ_PROJ_DB_FILE} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/proj)
endif()

# Qt
set (QT_COMPONENTS Widgets)
set (QT_COMPONENTS_FULL ${QT_COMPONENTS})
list (TRANSFORM QT_COMPONENTS_FULL PREPEND Qt5::)
find_package(Qt5 REQUIRED COMPONENTS ${QT_COMPONENTS} LinguistTools)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Windows: install PROJ grids
if (CMAKE_HOST_WIN32)
  include(FetchContent)
  if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
     cmake_policy(SET CMP0135 OLD)
  endif()

  FetchContent_Declare(
    proj-data
    URL      https://download.osgeo.org/proj/proj-datumgrid-1.8.zip
  )
  FetchContent_MakeAvailable(proj-data)
  file(COPY ${proj-data_SOURCE_DIR}/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/proj)
endif()


#######################################################
##        Compilation toolchain
#######################################################

# ccache
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif(CCACHE_FOUND)

# compilers specific options : warnings, MSVC specific defines
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
set(gnu_cxx  "$<COMPILE_LANG_AND_ID:CXX,GNU>")

add_library(COMP3D_compiler_flags INTERFACE)
target_compile_options(COMP3D_compiler_flags INTERFACE
    "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Werror>>"
    "$<${msvc_cxx}:$<BUILD_INTERFACE:/W3;/D_USE_MATH_DEFINES;/D_HAS_STD_BYTE=0;/wd4244;/wd4267;/wd4305;/wd4661;/D_CRT_SECURE_NO_WARNINGS>>"
    "$<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:MSVC>:/bigobj>>"
    "$<$<CONFIG:RelWithDebInfo>:$<$<CXX_COMPILER_ID:MSVC>:/bigobj>>"
)

# we really want -O3 instead of -O2 when available (GNU, Clang) in RelWithDebInfo mode
string(REPLACE "-O2" "-O3" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

#######################################################
##     Versionning
#######################################################
ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp
    COMMAND ${CMAKE_COMMAND} -P
            ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake)

#######################################################
##      Configuration
#######################################################
configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
)

#######################################################
##        Sources
#######################################################

set(SOURCES
    main.cpp
    lib_ex.cpp
    src/fformat.cpp
    src/filerefjson.cpp
    src/leastsquares.cpp
    src/misc_tools.cpp
    src/obs.cpp
    src/point.cpp
    src/coord.cpp
    src/projection.cpp
    src/station.cpp
    src/station_eq.cpp
    src/station_simple.cpp
    src/parameter.cpp
    src/project.cpp
    src/mathtools.cpp
    src/station_hz.cpp
    src/jsoncpp.cpp
    src/project_config.cpp
    src/datafile.cpp
    src/ellipsoid.cpp
    src/station_axis.cpp
    src/axisobs.cpp
    src/station_bascule.cpp
    src/matrixordering.cpp
    src/info.cpp
    src/varcovarmatrix.cpp
)

set(SOURCES_GUI
    gui/customtablewidgetitem.cpp
    gui/exportcoorddialog.cpp
    gui/infinityasctobasdialog.cpp
    gui/sinexdialog.cpp
    gui/maintables.cpp
    gui/mainwindow.cpp
    gui/calcthread.cpp
    gui/configdialog.cpp
    gui/errordialog.cpp
    gui/applytransfodialog.cpp
    gui/sightmatrixdialog.cpp
    gui/conversiondialog.cpp
    gui/dialogpreferences.cpp
    gui/framewidget.cpp
    gui/multisorttable.cpp
    gui/exportpointsdialog.cpp
)

set(HEADERS
    src/info.h
    src/fformat.h
    src/filerefjson.h
    src/leastsquares.h
    src/misc_tools.h
    src/obs.h
    src/point.h
    src/coord.h
    src/projection.h
    src/station.h
    src/station_eq.h
    src/station_simple.h
    src/parameter.h
    src/project.h
    src/mathtools.h
    src/station_hz.h
    src/json/json-forwards.h
    src/json/json.h
    src/compile.h
    src/project_config.h
    src/datafile.h
    src/ellipsoid.h
    src/station_axis.h
    src/axisobs.h
    src/station_bascule.h
    src/matrixordering.h
    src/uni_stream.h
    src/varcovarmatrix.h
    src/filesystem_compat.h
    gui/customtablewidgetitem.h
    gui/exportcoorddialog.h
    gui/infinityasctobasdialog.h
    gui/sinexdialog.h
    gui/maintables.h
    gui/mainwindow.h
    gui/calcthread.h
    gui/configdialog.h
    gui/errordialog.h
    gui/applytransfodialog.h
    gui/sightmatrixdialog.h
    gui/conversiondialog.h
    gui/dialogpreferences.h
    gui/framewidget.h
    gui/multisorttable.h
    gui/exportpointsdialog.h
)

set(UI_FILES
    gui/applytransfodialog.ui
    gui/configdialog.ui
    gui/conversiondialog.ui
    gui/dialogpreferences.ui
    gui/errordialog.ui
    gui/exportcoorddialog.ui
    gui/exportpointsdialog.ui
    gui/framewidget.ui
    gui/infinityasctobasdialog.ui
    gui/maintables.ui
    gui/mainwindow.ui
    gui/sightmatrixdialog.ui
    gui/sinexdialog.ui
)


#######################################################
##        Translation
#######################################################
list (TRANSFORM TRANSLATIONS PREPEND "Comp3D_")
set(TS_FILES ${TRANSLATIONS})
list (TRANSFORM TS_FILES PREPEND "gui/translations/")
list (TRANSFORM TS_FILES APPEND ".ts")

set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/i18n")
qt5_add_translation(QM_FILES ${TS_FILES})

set (QM_FILES_QRC ${TRANSLATIONS})
list (TRANSFORM QM_FILES_QRC PREPEND "<file>i18n/")
list (TRANSFORM QM_FILES_QRC APPEND ".qm</file>\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cmake_qm.qrc"
"
<RCC>
  <qresource>
  ${QM_FILES_QRC}
  </qresource>
</RCC>
")

#######################################################
##        Target
#######################################################

add_executable(${COMP3D_TARGET}
    ${SOURCES} ${SOURCES_GUI} ${HEADERS} ${UI_FILES}
    comp3d.rc ressource.qrc ${CMAKE_BINARY_DIR}/cmake_qm.qrc ${CMAKE_BINARY_DIR}/git_revision.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)

target_include_directories (${COMP3D_TARGET} PRIVATE src gui . ${PROJECT_BINARY_DIR} ${PROJ_INCLUDE_DIRS})
target_link_libraries (${COMP3D_TARGET} PRIVATE COMP3D_compiler_flags ${BOOST_COMPONENTS_FULL} ${QT_COMPONENTS_FULL} ${PROJ_LIBRARIES} Eigen3::Eigen)



#######################################################
##        Install
#######################################################
set(PROJLIB_PATH "/usr/lib/")
set(PROJLIB_FILE "${PROJ_LIBRARY_DIRS}/libproj.a")
set(TARGET_PATH "/usr/bin/")
set(INSTALL_PREFIX "" CACHE PATH "Path prefix for installation")


install(FILES ${PROJLIB_FILE} DESTINATION ${INSTALL_PREFIX}/${PROJLIB_PATH})
install(TARGETS ${COMP3D_TARGET} DESTINATION ${INSTALL_PREFIX}/${TARGET_PATH})

