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

#ifndef PARAMETER_H
#define PARAMETER_H

#include <string>
#include "compile.h"
#include "json/json.h"

class Point;

/**
  Parameter class represents unknown in leastsquare
  For one given point the first parameter must be X, the second Y then Z. (unless one is missing)
  There is no check for that
  **/
class Parameter
{
public:
    Parameter(const std::string &_name, tdouble *ptr_to_val, tdouble _unitFactor, const std::string &_unit_str, Point* _on_point, int _rank=0);
    Json::Value toJson() const;
    tdouble *value;
    tdouble unitFactor;
    std::string unitStr;
    int rank;//where on the matrix
    std::string name;
    Point *on_point;//just for information, null if not linked to a point
    static Parameter cst;//fake parameter to represent constants
};

#endif // PARAMETER_H
