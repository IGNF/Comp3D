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

#include "parameter.h"
#include "leastsquares.h"
#include "project.h"

Parameter Parameter::cst("const", nullptr, 0, "", nullptr, LeastSquares::no_param_index);

Parameter::Parameter(const std::string &_name, tdouble *ptr_to_val, tdouble _unitFactor,
                     const std::string &_unit_str, Point* _on_point, int _rank)
    :value(ptr_to_val),unitFactor(_unitFactor),unitStr(_unit_str),rank(_rank),name(_name),on_point(_on_point)
{
    if (value) *value = NAN;
}

Json::Value Parameter::toJson() const
{
    Json::Value jsparam;
    jsparam["rank"]=rank;
    if (value)
        jsparam["value"]=*value;
    if (rank!=LeastSquares::no_param_index)
        if (Project::theone()->invertedMatrix)
            jsparam["sigma"]=(double)(sqrt(Project::theone()->lsquares.Qxx(rank,rank)*WEIGHT_FACTOR)*Project::theone()->lsquares.sigma_0);
    jsparam["unit_factor"]=(double)unitFactor;
    jsparam["unit_str"]=unitStr;
    return jsparam;
}
