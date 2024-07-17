/**
 * Copyright (c) Institut national de l'information géographique et forestière https://www.ign.fr/
 *
 * File main authors:
 *  - JM Muller
 *  - C Meynard
 *
 * This file is part of Comp3D: https://github.com/IGNF/Comp3D
 *
 * Comp3D is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Comp3D is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Comp3D. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef INFO_H
#define INFO_H

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdarg>

//Info subjects (not used for now...):
static const unsigned INFO_COR=   0x0001;
static const unsigned INFO_OBS=   0x0002;
static const unsigned INFO_CAP=   0x0004;
static const unsigned INFO_LS=    0x0008;
static const unsigned INFO_CONF=  0x0010;

class Info
{
    enum class InfoLevel {Message, Error, Warning};//warning if a point or an obs can't be used, error if nothing possible

public:
    Info();

#ifndef _MSC_VER
# define CHECK_PRINTF_FORMAT __attribute__ ((format (printf,4,5)))
#else
# define CHECK_PRINTF_FORMAT
#endif
    void msg(unsigned subjects, unsigned depth, const char * format, ...) CHECK_PRINTF_FORMAT ;
    void error(unsigned subjects, unsigned depth, const char * format, ...) CHECK_PRINTF_FORMAT ;
    void warning(unsigned subjects, unsigned depth, const char * format, ...) CHECK_PRINTF_FORMAT ;
#undef CHECK_PRINTF_FORMAT

    bool isEmpty(){return mStreamRaw.str().size()==0;}
    void clear();

    std::string toStrFormat();
    std::string toStrRaw();
    void setDirect(bool direct){mDirect=direct;}
protected:
    void add(unsigned subjets, InfoLevel type, unsigned depth, const char *format, va_list args);
    std::ostringstream mStreamFormat;
    std::ostringstream mStreamRaw;
    bool mDirect;
    std::vector<char> mBuffer;
};

#endif // INFO_H
