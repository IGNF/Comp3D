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

#ifndef MISC_TOOLS_H
#define MISC_TOOLS_H
#include "compile.h"
#include "filesystem_compat.h"

#ifdef USE_QT
#include <QString>

void copyRes(const QString & res, const QString & filename);
void copyAllRes(const QString & resPath, const QString & outPath); //< ":/gui/html/", "/res"
#endif

fs::path selfExecPath();

#endif // MISC_TOOLS_H
