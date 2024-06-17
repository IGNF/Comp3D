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

#ifndef COORD_H
#define COORD_H

#include "compile.h"
#include "json/json.h"
#include <cmath>
#include <eigen3/Eigen/Dense>
#include <iostream>
#include <proj.h>
#include <string>

using Mat3 = Eigen::Matrix<tdouble, 3, 3>;
using Vect3 = Eigen::Matrix<tdouble, 3, 1>;


class Coord
{
public:
    Coord():m_x(0.0),m_y(0.0),m_z(0.0){}
    Coord(tdouble _x,tdouble _y, tdouble _z):m_x(_x),m_y(_y),m_z(_z){}
    explicit Coord(const Vect3 &vect):m_x(vect[0]),m_y(vect[1]),m_z(vect[2]){}
    Coord operator+( const Coord &other ) const;
    Coord& operator+=( const Coord &other );
    Coord operator-( const  Coord &other ) const;
    Coord& operator-=( const Coord &other );
    Coord operator*( tdouble factor ) const;
    Coord& operator*=( tdouble factor );
    Coord operator/( tdouble factor ) const;
    Coord& operator/=( tdouble factor );
    Coord operator-() const;
    tdouble& operator[](std::size_t idx);
    tdouble scalaire( const  Coord &other ) const;
    Coord dotProduct( const  Coord &other ) const;
    Json::Value toJson() const;
    std::string toString(int nbDigits=-1) const;
    tdouble x() const {return m_x;}
    tdouble y() const {return m_y;}
    tdouble z() const {return m_z;}
    void set(tdouble _x,tdouble _y,tdouble _z){m_x=_x;m_y=_y;m_z=_z;}
    void set(const Vect3 &vect){m_x=vect[0];m_y=vect[1];m_z=vect[2];}
    void set(const PJ_COORD &p){m_x=p.xyz.x;m_y=p.xyz.y;m_z=p.xyz.z;}
    void setx(tdouble _x){m_x=_x;}
    void sety(tdouble _y){m_y=_y;}
    void setz(tdouble _z){m_z=_z;}
    Vect3 toVect() const;
    PJ_COORD toPJ() const;
    Mat3 sigma2varCovar() const;
    void varCovar2sigma(Mat3 m);
    tdouble norm2() const{return m_x*m_x+m_y*m_y+m_z*m_z;}

protected:
    tdouble m_x,m_y,m_z;
};




#endif // COORD_H
