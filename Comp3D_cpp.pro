QMAKE_PRE_LINK = 'echo "const char *GIT_VERSION=\\"`cd $$PWD; git describe --dirty`\\""\\; > git_revision.cpp ; $(CC) -c $(CFLAGS) -o git_revision.o git_revision.cpp'

LIBS += git_revision.o

DEFINES += USE_GUI USE_SIM USE_AUTO
DEFINES += ADD_PROJ_CC ADD_PROJ_NTF

# If COMP3D_DOC_DIR is not defined on command line, set it to [source_path]/doc_uni
# To define it on command line : qmake comp3d.pro COMP3D_DOC_DIR="/my/path"
# (see also mainwindow::showHelp() )
!defined(COMP3D_DOC_DIR, var) {
    COMP3D_DOC_DIR="$$PWD/doc_uni"
}
DEFINES += COMP3D_DOC_DIR=\\\"$$COMP3D_DOC_DIR\\\"

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Comp3D
TEMPLATE = app
win32 {
    RC_ICONS = gui/icon.ico
}

CONFIG -= console
CONFIG += c++14 -Wno-error=date-time

#test if ccache is installed
system("ccache -V"): CONFIG += ccache

QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS]

SOURCES += main.cpp \
    gui/customtablewidgetitem.cpp \
    gui/exportcoorddialog.cpp \
    gui/infinityasctobasdialog.cpp \
    gui/sinexdialog.cpp \
    gui/maintables.cpp \
    lib_ex.cpp \
    src/fformat.cpp \
    src/filerefjson.cpp \
    src/leastsquares.cpp \
    src/misc_tools.cpp \
    src/obs.cpp \
    src/point.cpp \
    src/coord.cpp \
    src/projection.cpp \
    src/station.cpp \
    src/station_eq.cpp \
    src/station_simple.cpp \
    src/parameter.cpp \
    src/project.cpp \
    src/mathtools.cpp \
    src/station_hz.cpp \
    src/jsoncpp.cpp \
    #src/confidenceinterval.cpp \
    src/project_config.cpp \
    src/datafile.cpp \
    src/ellipsoid.cpp \
    src/station_axis.cpp \
    src/axisobs.cpp \
    src/station_bascule.cpp \
    src/matrixordering.cpp \
    gui/mainwindow.cpp \
    gui/calcthread.cpp \
    gui/configdialog.cpp \
    gui/errordialog.cpp \
    gui/applytransfodialog.cpp \
    gui/sightmatrixdialog.cpp \
    gui/conversiondialog.cpp \
    src/info.cpp \
    gui/dialogpreferences.cpp \
    gui/framewidget.cpp \
    gui/multisorttable.cpp \
    gui/exportpointsdialog.cpp \
    src/varcovarmatrix.cpp


INCLUDEPATH += src

HEADERS += \
    gui/customtablewidgetitem.h \
    gui/exportcoorddialog.h \
    gui/infinityasctobasdialog.h \
    gui/sinexdialog.h \
    gui/maintables.h \
    src/fformat.h \
    src/filerefjson.h \
    src/leastsquares.h \
    src/misc_tools.h \
    src/obs.h \
    src/point.h \
    src/coord.h \
    src/projection.h \
    src/station.h \
    src/station_eq.h \
    src/station_simple.h \
    src/parameter.h \
    src/project.h \
    src/mathtools.h \
    src/station_hz.h \
    src/json/json-forwards.h \
    src/json/json.h \
    src/compile.h \
    #src/confidenceinterval.h \
    src/project_config.h \
    src/datafile.h \
    src/ellipsoid.h \
    src/station_axis.h \
    src/axisobs.h \
    src/station_bascule.h \
    src/matrixordering.h \
    gui/mainwindow.h \
    gui/calcthread.h \
    gui/configdialog.h \
    gui/errordialog.h \
    gui/applytransfodialog.h \
    gui/sightmatrixdialog.h \
    gui/conversiondialog.h \
    src/info.h \
    gui/dialogpreferences.h \
    gui/framewidget.h \
    gui/multisorttable.h \
    gui/exportpointsdialog.h \
    src/uni_stream.h \
    src/varcovarmatrix.h

LIBS += -lproj -lsqlite3

win32 {
    INCLUDEPATH += /usr/lib/mxe/usr/i686-w64-mingw32.static/include/
    LIBS += -L/usr/lib/mxe/usr/i686-w64-mingw32.static/lib/
    LIBS += -lboost_system-mt
    LIBS += -lboost_filesystem-mt
    LIBS += -lboost_date_time-mt
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
    LIBS += -lboost_graph
}

FORMS += \
    gui/exportcoorddialog.ui \
    gui/infinityasctobasdialog.ui \
    gui/maintables.ui \
    gui/mainwindow.ui \
    gui/configdialog.ui \
    gui/errordialog.ui \
    gui/applytransfodialog.ui \
    gui/sightmatrixdialog.ui \
    gui/conversiondialog.ui \
    gui/dialogpreferences.ui \
    gui/framewidget.ui \
    gui/exportpointsdialog.ui \
    gui/sinexdialog.ui

TRANSLATIONS = gui/translations/Comp3D_fr.ts
CONFIG += lrelease embed_translations

RESOURCES += \
    ressource.qrc

OTHER_FILES += \
    gui/translations/Comp3D_fr.ts \
    gui/html/visu_comp.js \
    gui/html/comp3d.css

DISTFILES += \
    README.md \
    changelog.txt

DEFINES += QT_DEPRECATED_WARNINGS

unix {
    projlib.path = /usr/lib/
    projlib.files = /usr/local/proj82/lib/libproj.a
    target.path = /usr/bin/
    INSTALLS += projlib target
}
