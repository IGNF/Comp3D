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

#include "info.h"

#include <cstring>
#include <iostream>
#include "compile.h"                // IWYU pragma: keep  (may be used in non QT mode)

Info::Info():mDirect(true),mBuffer(2048,'\0')
{
}

void Info::add(unsigned subjects, InfoLevel type, unsigned depth, const char *format, va_list args)
{
    (void) subjects;
    std::string trFormat = QObject::tr(format).toStdString();

    va_list args_copy;
    va_copy (args_copy, args);
    int sizeNeeded=vsnprintf(mBuffer.data(),mBuffer.size(),trFormat.c_str(), args);
    if (sizeNeeded<0)
    {
        std::cerr<<"Error converting format \""<<trFormat<<"\"!\n";

    }
    if (sizeNeeded>=(int)mBuffer.size())
    {
        mBuffer.resize(((sizeNeeded + 1 + 1023) / 1024) * 1024);  // at least sizeNeeded+1, rounded to upper %1024
        vsnprintf(mBuffer.data(),mBuffer.size(),trFormat.c_str(), args_copy);
    }

    switch (type) {
    case InfoLevel::Message: mStreamFormat<<""; break;
    case InfoLevel::Warning: mStreamFormat<<"<font color=#FF8000>"; mStreamRaw<<"Warning: "; break;
    case InfoLevel::Error:   mStreamFormat<<"<font color=red>"; mStreamRaw<<"Error: "; break;
    }

    for (unsigned i=0;i<depth;i++)
    {
        mStreamFormat<<"&nbsp;&nbsp;&nbsp;&nbsp;";
        mStreamRaw<<"                ";
    }

    mStreamFormat<<mBuffer.data();
    mStreamRaw<<mBuffer.data();

    switch (type) {
    case InfoLevel::Message: mStreamFormat<<"<br/>"; break;
    case InfoLevel::Warning: mStreamFormat<<"</font><br/>"; break;
    case InfoLevel::Error:   mStreamFormat<<"</font><br/>"; break;
    }

    mStreamFormat<<"\n";
    mStreamRaw<<"\n";

    if (mDirect||(type==InfoLevel::Error))
        std::cout<<mBuffer.data()<<std::endl;

    va_end(args_copy);
}

#define MSG(type)   \
    va_list args;   \
    va_start(args, format); \
    add(subjects,type,depth,format,args); \
    va_end(args);

void Info::msg(unsigned subjects, unsigned depth, const char *format, ...)
{
    MSG(Info::InfoLevel::Message);
}

void Info::error(unsigned subjects, unsigned depth, const char *format, ...)
{
    MSG(Info::InfoLevel::Error);
}

void Info::warning(unsigned subjects, unsigned depth, const char *format, ...)
{
    MSG(Info::InfoLevel::Warning);
}

void Info::clear()
{
    mStreamFormat.str("");
    mStreamRaw.str("");
}

std::string Info::toStrFormat()
{
    return mStreamFormat.str();
}

std::string Info::toStrRaw()
{
    return mStreamRaw.str();
}
