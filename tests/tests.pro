QMAKE_PRE_LINK = 'echo "const char *GIT_VERSION=\\"`cd $$PWD/..; git describe --dirty`\\""\\; > git_revision.cpp ; $(CC) -c $(CFLAGS) -o git_revision.o git_revision.cpp'
LIBS += git_revision.o

DEFINES += USE_QT USE_SIM
DEFINES += ADD_PROJ_CC ADD_PROJ_NTF ADD_PROJ_UTM

QT += testlib core

TARGET = Comp3D_tests
TEMPLATE = app

CONFIG += c++14 -Wno-error=date-time
CONFIG += testcase

#test if ccache is installed
system("ccache -V"): CONFIG += ccache

QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS]

# Input
SOURCES += \
    ../src/station_eq.cpp \
    ../src/varcovarmatrix.cpp \
    tests_coord.cpp \
    tests_main.cpp \
    ../src/leastsquares.cpp \
    ../src/obs.cpp \
    ../src/point.cpp \
    ../src/coord.cpp \
    ../src/projection.cpp \
    ../src/station.cpp \
    ../src/station_simple.cpp \
    ../src/parameter.cpp \
    ../src/fformat.cpp \
    ../src/project.cpp \
    ../src/mathtools.cpp \
    ../src/misc_tools.cpp \
    ../src/station_hz.cpp \
    ../src/jsoncpp.cpp \
    ../src/project_config.cpp \
    ../src/datafile.cpp \
    ../src/ellipsoid.cpp \
    ../src/station_axis.cpp \
    ../src/axisobs.cpp \
    ../src/station_bascule.cpp \
    ../src/matrixordering.cpp \
    ../src/info.cpp \
    ../src/filerefjson.cpp \
    tests_basc.cpp \
    compcompare.cpp \
    tests_nonreg.cpp \
    tests_stab.cpp


INCLUDEPATH += . ../src

HEADERS += \
    ../src/leastsquares.h \
    ../src/obs.h \
    ../src/point.h \
    ../src/coord.h \
    ../src/projection.h \
    ../src/station.h \
    ../src/station_eq.h \
    ../src/station_simple.h \
    ../src/parameter.h \
    ../src/fformat.h \
    ../src/project.h \
    ../src/mathtools.h \
    ../src/misc_tools.h \
    ../src/station_hz.h \
    ../src/json/json-forwards.h \
    ../src/json/json.h \
    ../src/compile.h \
    ../src/project_config.h \
    ../src/datafile.h \
    ../src/ellipsoid.h \
    ../src/station_axis.h \
    ../src/axisobs.h \
    ../src/station_bascule.h \
    ../src/matrixordering.h \
    ../src/varcovarmatrix.h \
    tests_coord.h \
    ../src/info.h \
    ../src/filerefjson.h \
    tests_basc.h \
    compcompare.h \
    tests_nonreg.h \
    tests_stab.h

LIBS += -lproj -lsqlite3

win32 {
    INCLUDEPATH += /usr/lib/mxe/usr/i686-w64-mingw32.static/include/
    LIBS += /usr/lib/mxe/usr/x86_64-w64-mingw32.static/lib/libsqlite3.a
    LIBS += -L/usr/lib/mxe/usr/i686-w64-mingw32.static/lib/
    LIBS += -lboost_system-mt
    LIBS += -lboost_filesystem-mt
    LIBS += -lboost_date_time-mt
    LIBS += -lboost_regex-mt
    LIBS += -lboost_graph-mt
}

unix {
    QMAKE_LFLAGS += -no-pie
    INCLUDEPATH += /usr/local/proj82/include/
    LIBS += -L/usr/local/proj82/lib/
    LIBS +=  -ldl
    LIBS += -lboost_system
    LIBS += -lboost_filesystem
    LIBS += -lboost_date_time
    LIBS += -lboost_regex
    LIBS += -lboost_graph
}

DEFINES += QT_DEPRECATED_WARNINGS

