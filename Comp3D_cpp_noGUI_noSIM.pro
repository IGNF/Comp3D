win32 {
    include(path_win.pri)
}

gittarget.target = $$relative_path($$PWD/src/genere/git_revision.cpp,$$OUT_PWD)
licensetarget.target = $$relative_path($$PWD/src/genere/license_crypted.cpp,$$OUT_PWD)
win32 {
    #gittarget.commands = powershell -NonInteractive -File $$PWD/src/genere/script_infos_git.sh $$PWD $$PWD/src/genere/git_revision.cpp #ne quitte pas!
    gittarget.commands = $$GIT_BASH $$PWD/src/genere/script_infos_git.sh $$PWD $$PWD/src/genere/git_revision.cpp
    licensetarget.commands = $$GIT_BASH $$PWD/src/genere/script_crypt_license.sh $$PWD/src/genere
}
unix {
    gittarget.commands = sh $$PWD/src/genere/script_infos_git.sh $$PWD $$PWD/src/genere/git_revision.cpp
    licensetarget.commands = sh $$PWD/src/genere/script_crypt_license.sh $$PWD/src/genere
}
gittarget.depends = FORCE
licensetarget.depends = FORCE

QMAKE_EXTRA_TARGETS += gittarget licensetarget
PRE_TARGETDEPS += $$gittarget.target $$licensetarget.target

DEFINES += USE_QT USE_AUTO

QT       += core
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Comp3D
TEMPLATE = app

#CONFIG += debug
CONFIG -= console
CONFIG += c++11

DEPENDPATH += $$gittarget.target $$licensetarget.target

SOURCES += main.cpp \
	lib_ex.cpp \
	src/leastsquares.cpp \
    src/obs.cpp \
    src/point.cpp \
    src/coord.cpp \
    src/projection.cpp \
    src/station.cpp \
    src/station_simple.cpp \
    src/parameter.cpp \
    src/project.cpp \
    src/mathtools.cpp \
    src/station_hz.cpp \
    src/jsoncpp.cpp \
    src/project_config.cpp \
    src/datafile.cpp \
    src/ellipsoid.cpp \
	src/station_axis.cpp \
	src/axisobs.cpp \
	src/station_bascule.cpp \
	src/matrixordering.cpp \
    src/info.cpp \
    $$PWD/src/genere/git_revision.cpp \
    $$PWD/src/genere/license_crypted.cpp \
    src/comppref.cpp


INCLUDEPATH += src

HEADERS += \
    src/leastsquares.h \
    src/obs.h \
    src/point.h \
    src/coord.h \
    src/projection.h \
    src/station.h \
    src/station_simple.h \
    src/parameter.h \
    src/project.h \
    src/mathtools.h \
    src/station_hz.h \
    src/json/json-forwards.h \
    src/json/json.h \
    src/compile.h \
    src/project_config.h \
    src/datafile.h \
    src/ellipsoid.h \
	src/station_axis.h \
	src/axisobs.h \
	src/station_bascule.h \
	src/matrixordering.h \
    src/info.h \
    src/comppref.h

unix {
    LIBS += -lboost_system
    LIBS += -lboost_filesystem
    LIBS += -lboost_date_time
    LIBS += -lboost_regex
    LIBS += -lboost_graph
    LIBS += -lproj
}

RESOURCES += \
    ressource.qrc

OTHER_FILES += \
	gui/translations/Comp3D_fr.ts \
	gui/html/visu_comp.js \
	gui/html/comp3d.css

DISTFILES += \
    INSTALL.txt \
    README.md \
    infos_comp_cpp.txt \
    TODO
