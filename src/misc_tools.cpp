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

#include "misc_tools.h"
#ifdef __linux__
#  include <unistd.h>
#elif _WIN32
#  include <windows.h>
#  include <process.h>
#elif __APPLE__
#  include <unistd.h>
#  include <mach-o/dyld.h>
#endif


#ifdef USE_QT
#include <QFile>
#include <QDirIterator>

void copyRes(const QString & res, const QString & filename)
{
    QFile tmp(filename);
    tmp.setPermissions((QFileDevice::Permission)0x6666);
    QFile::remove(filename);
    QFile::copy(res, filename);
    QFile tmp2(filename);
    tmp2.setPermissions((QFileDevice::Permission)0x6666);
}

void copyAllRes(const QString & resPath, const QString & outPath)
{
    QDirIterator it(resPath, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        QString filePathOut = filePath;
        filePathOut.replace(resPath,outPath+"/");
        copyRes(filePath, filePathOut);
    }
}
#endif


#ifdef __linux__
fs::path selfExecPath()
{
    char buf[4096];

    *buf = 0;
    ssize_t result = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (result >= 0 && (size_t)result < sizeof(buf) - 1)
        buf[result] = 0;
    else
        buf[0] = 0;
    return fs::path(buf).parent_path();
}
#elif _WIN32
fs::path selfExecPath()
{
    wchar_t buffer[MAX_PATH];
    *buffer = L'0';
    DWORD size = GetModuleFileNameW(nullptr, buffer,(DWORD)sizeof(buffer));
    if (size <0 || size == (DWORD)sizeof(buffer))
        *buffer = L'0';
    return fs::path(buffer).parent_path();
}
#elif __APPLE__
fs::path selfExecPath()
{
    // Ch.M: Not tested
    fs::path path;

    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    char *buffer = new char[size + 1];
    if (_NSGetExecutablePath(buffer, &size) >= 0) {
        buffer[size] = '\0';
        path = buffer;
    }
    delete[] buffer;
    return path;
#else
#  error Please implement selfExecPath() for your OS
#endif
