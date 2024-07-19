#######################################################
##        General
#######################################################
cmake_minimum_required(VERSION 3.15)
project(Comp3D_tests)
set(COMP3D_TARGET Comp3D_tests)

# strict C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

#######################################################
##        Options
#######################################################

set(ADD_PROJ_CC ON)
set(ADD_PROJ_NTF ON)
set(ADD_PROJ_UTM ON)
set(USE_QT ON)
set(USE_RES ON)

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
set (QT_COMPONENTS Test)
set (QT_COMPONENTS_FULL ${QT_COMPONENTS})
list (TRANSFORM QT_COMPONENTS_FULL PREPEND Qt5::)
find_package(Qt5 REQUIRED COMPONENTS ${QT_COMPONENTS})
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
            ${CMAKE_CURRENT_SOURCE_DIR}/../version.cmake)

#######################################################
##      Configuration
#######################################################
configure_file (
  "${PROJECT_SOURCE_DIR}/../config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
)

#######################################################
##        Sources
#######################################################

set(SOURCES
    ../src/fformat.cpp
    ../src/filerefjson.cpp
    ../src/leastsquares.cpp
    ../src/misc_tools.cpp
    ../src/obs.cpp
    ../src/point.cpp
    ../src/coord.cpp
    ../src/projection.cpp
    ../src/station.cpp
    ../src/station_eq.cpp
    ../src/station_simple.cpp
    ../src/parameter.cpp
    ../src/project.cpp
    ../src/mathtools.cpp
    ../src/station_hz.cpp
    ../src/jsoncpp.cpp
    ../src/project_config.cpp
    ../src/datafile.cpp
    ../src/ellipsoid.cpp
    ../src/station_axis.cpp
    ../src/axisobs.cpp
    ../src/station_bascule.cpp
    ../src/matrixordering.cpp
    ../src/info.cpp
    ../src/varcovarmatrix.cpp
    tests_coord.cpp
    tests_main.cpp
    tests_basc.cpp
    compcompare.cpp
    tests_nonreg.cpp
    tests_stab.cpp
)

set(HEADERS
    ../src/info.h
    ../src/fformat.h
    ../src/filerefjson.h
    ../src/leastsquares.h
    ../src/misc_tools.h
    ../src/obs.h
    ../src/point.h
    ../src/coord.h
    ../src/projection.h
    ../src/station.h
    ../src/station_eq.h
    ../src/station_simple.h
    ../src/parameter.h
    ../src/project.h
    ../src/mathtools.h
    ../src/station_hz.h
    ../src/json/json-forwards.h
    ../src/json/json.h
    ../src/compile.h
    ../src/project_config.h
    ../src/datafile.h
    ../src/ellipsoid.h
    ../src/station_axis.h
    ../src/axisobs.h
    ../src/station_bascule.h
    ../src/matrixordering.h
    ../src/uni_stream.h
    ../src/varcovarmatrix.h
    compcompare.h
    tests_basc.h
    tests_coord.h
    tests_nonreg.h
    tests_stab.h
)


#######################################################
##        Target
#######################################################

add_executable(${COMP3D_TARGET} ${SOURCES} ${HEADERS} ../ressource.qrc ${CMAKE_BINARY_DIR}/git_revision.cpp )

target_include_directories (${COMP3D_TARGET} PRIVATE ../src . ${PROJECT_BINARY_DIR} ${PROJ_INCLUDE_DIRS})
target_link_libraries (${COMP3D_TARGET} PRIVATE COMP3D_compiler_flags ${BOOST_COMPONENTS_FULL} ${QT_COMPONENTS_FULL} ${PROJ_LIBRARIES} Eigen3::Eigen)
