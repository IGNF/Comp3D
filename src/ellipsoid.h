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

#ifndef ELLIPSOID_H
#define ELLIPSOID_H

#include <json/json.h>
#include <cmath>
#include <iostream>
#include <string>
#include "compile.h"
#include "mathtools.h"

//TODO: where to apply sigma0 factor?
class Ellipsoid
{
public:
    Ellipsoid();
    inline void set(int i,int j, tdouble val);//<Use it before compute_eigenvalues
    tdouble get_variance(int i) const;
    tdouble get_ellipsAxe(int i) const;
    std::string sigmasToString(tdouble sigma0);
    void compute_eigenvalues(tdouble sigma0);

    Json::Value toJson(tdouble sigma0) const;
    std::string toString() const;
    bool isSet() const {return isset;}
    MatX get_matrix() const {return matrix;}

protected:
    MatX matrix;
    tdouble eigen_values[3];
    tdouble azimuts[3];
    tdouble sites[3];//TODO: translate ?
    bool isset;
};

inline void Ellipsoid::set(int i,int j, tdouble val)
{
    if ((i<0)||(i>2)||(j<0)||(j>2))
    {
        std::cerr<<"Ellipsoid::set: dimension must be 0<=d<3!"<<std::endl;
        return;
    }else{
        matrix(i,j)=val;
    }
}

#endif // ELLIPSOID_H
