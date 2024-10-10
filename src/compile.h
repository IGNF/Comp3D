/**
 * Copyright (c) Institut national de l'information géographique et forestière https://www.ign.fr/
 *
 * File main authors:
 *  - JM Muller
 *
 * This file is part of Comp3D: https://github.com/IGNF/Comp3D
 *
 * Comp3D is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Comp3D is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Comp3D. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef COMPILE_H
#define COMPILE_H

#include "config.h"

#include <string>

#ifdef USE_GUI
    #ifndef USE_RES
        #define USE_RES
    #endif
#endif

#ifdef USE_RES
    #ifndef USE_QT
        #define USE_QT
    #endif
#endif

#ifdef _WIN32
#  define COMPILED_FOR "win"
#elif __linux__
#  define COMPILED_FOR "linux"
#else
#  define COMPILED_FOR "xxx"
#endif


//---------------------------------- Settings ------------------------------------
#define COMP3D_VERSION "COMP3D v5.24dev" "-PROJ " PROJ_VERSION "-" COMPILED_FOR
#define COMP3D_COPYRIGHT "Copyright 1992-2024 IGN France www.ign.fr"
#define COMP3D_LICENSE "Provided with absolutely no warranty, under GPLv3 license"
#define COMP3D_CONTACT "comp3d@ign.fr"
#define COMP3D_REPO "https://github.com/IGNF/Comp3D"

#define COMP3D_CONFIG_FILENAME "comp3d_config.txt"

#define COMP3D_APPLICATION_NAME     "Comp3D"
#define COMP3D_ORGANIZATION_NAME    "IGN"
#define COMP3D_ORGANIZATION_DOMAIN  "ign.fr"

#define WRITEWIDTH 15
#define WRITEPREC 7
#define MAXFILEDEPTH 5
#define MAXOPENFILES 500
#define RANDOM_SEED

// Keep paired and use curly !
#define SUPPORTED_LANG_CODE {"en","fr"}
#define SUPPORTED_LANG_NAME {"English",u8"Fran\u00e7ais"}

//debug output
//#define SHOW_MATRICES
//#define SHOW_LS
//#define COR_INFO
//#define OBS_INFO
//#define SHOW_CAP
//#define INFO_PROJ
//#define INFO_AXE
//#define INFO_EQ
//#define CORCOV_INFO
//#define MATRIX_A_IMAGE
//#define INFO_DELETE //unsafe

//computation choices
//#define SPARSE_INVERT //does not work for now

#define REORDER
//#define REORDER_DEBUG

//#define USE_QR_SOLVER

//-------------------------------- End of settings --------------------------------

static std::string COMP3D_OPTIONS=""
#ifdef USE_QT
        "QT "
#endif
#ifdef USE_GUI
        "GUI "
#endif
#ifdef USE_RES
        "RES "
#endif
#ifdef USE_AUTO
        "AUTO "
#endif
        ;

#ifdef USE_QT
    #include <QObject>
#endif

#ifndef USE_QT
    #include <string>

    #define QT_TRANSLATE_NOOP(c,x)   x

    //dummy tricks to replace tr() and toStdString() when not using Qt

    struct TranslatedString
    {
        explicit TranslatedString(const std::string &_s):mStr(_s){}
        std::string mStr;
        std::string toStdString() const { return mStr;}
    };

    namespace QObject
    {
        inline TranslatedString tr(const std::string &s){return TranslatedString(s);}
    }
#endif

#define toCstr() toStdString().c_str()

//WEIGHT_FACTOR is just to have a P matrix with values closer to 1 and 0
//average weight is around 0.0003 => square
constexpr double WEIGHT_FACTOR = 0.0000001;

//internal constraints sigma
constexpr double  INTERNALCONSTR_SIGMA = 1e-7;

//max distance for CAP to consider points as at the same place
constexpr double  CAP_CLOSE = 0.1;

//comp3D floating point type
using tdouble = double;

//forbid some observations on points too close because divider will be too small
constexpr double MINIMAL_DIVIDER = 1e-6;

constexpr double MINIMAL_SIGMA = 1e-8;

//make a warning if biggest ellipsoid is bigger than ELLIPS_TOO_BIG
constexpr double ELLIPS_TOO_BIG = 1.;

//make error if a point is at more than MAX_HZ_SIZE_M meters of local center
constexpr double MAX_HZ_SIZE_M = 100000.;

#define INT_REGEX R"([+-]?[\d]+)"
#define FLOAT_REGEX R"([+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)"

#define SPACE_FLOAT_REGEX R"((?:\s+)" FLOAT_REGEX ")"
#define COMMENT_EOL_REGEX R"(\s*([*].*|)$)"

#ifndef __GNUC__
    //__attribute__ is defined only for GCC
    #define __attribute__(x)
#endif

extern const char * GIT_VERSION;

#if __GNUC__ > 6
  #define FALLTHROUGH __attribute__((fallthrough));
#else
  #define FALLTHROUGH
#endif

#endif // COMPILE_H
