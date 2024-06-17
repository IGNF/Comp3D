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

#include "coord.h"

#include "project.h"
#include <sstream>


Coord Coord::operator+( const Coord &other ) const
{
    return {this->m_x+other.m_x,this->m_y+other.m_y,this->m_z+other.m_z};
}

Coord& Coord::operator+=( const Coord &other )
{
    this->m_x+=other.m_x;
    this->m_y+=other.m_y;
    this->m_z+=other.m_z;
    return *this;
}

Coord Coord::operator-( const Coord &other ) const
{
    return {this->m_x-other.m_x,this->m_y-other.m_y,this->m_z-other.m_z};
}


Coord& Coord::operator-=( const Coord &other )
{
    this->m_x-=other.m_x;
    this->m_y-=other.m_y;
    this->m_z-=other.m_z;
    return *this;
}

Coord Coord::operator*( tdouble factor ) const
{
    return {this->m_x*factor,this->m_y*factor,this->m_z*factor};
}

Coord& Coord::operator*=( tdouble factor )
{
    this->m_x*=factor;
    this->m_y*=factor;
    this->m_z*=factor;
    return *this;
}

Coord Coord::operator/( tdouble factor ) const
{
    return {this->m_x/factor,this->m_y/factor,this->m_z/factor};
}

Coord& Coord::operator/=( tdouble factor )
{
    this->m_x/=factor;
    this->m_y/=factor;
    this->m_z/=factor;
    return *this;
}

Coord Coord::operator-() const
{
    return {-this->m_x,-this->m_y,-this->m_z};
}

tdouble& Coord::operator[](std::size_t idx)
{
    switch (idx) {
        case 0:
            return m_x;
        case 1:
            return m_y;
        case 2:
            return m_z;
        default:
            throw std::out_of_range("Coord dim must be [0-2]");
    }
}

tdouble Coord::scalaire( const  Coord &other ) const
{
    return this->m_x*other.m_x+this->m_y*other.m_y+this->m_z*other.m_z;
}

Coord Coord::dotProduct( const  Coord &other ) const
{
    return { y() * other.z() - z() * other.y(),
             z() * other.x() - x() * other.z(),
             x() * other.y() - y() * other.x() };
}

Json::Value Coord::toJson() const
{
    Json::Value val;
    val.append(m_x);
    val.append(m_y);
    val.append(m_z);
    return val;
}

std::string Coord::toString(int nbDigits) const
{
    std::ostringstream oss;
    if (nbDigits>0)
        oss.precision(nbDigits);
    else
        oss.precision(Project::theone()?Project::theone()->config.nbDigits:8);
    oss<<std::fixed;
    oss.width(15);
    oss<<x()<<" "<<y()<<" "<<z();
    return oss.str();
}

Vect3 Coord::toVect() const
{
    Vect3 vect(m_x,m_y,m_z);
    return vect;
}

PJ_COORD Coord::toPJ() const
{
    PJ_COORD p;
    p.xyzt.x = m_x;
    p.xyzt.y = m_y;
    p.xyzt.z = m_z;
    p.xyzt.t = 0.0;
    return p;
}

Mat3 Coord::sigma2varCovar() const
{
    Mat3 m;
    m<<m_x*m_x,0.0,0.0,0.0,m_y*m_y,0.0,0.0,0.0,m_z*m_z;
    return m;
}

void Coord::varCovar2sigma(Mat3 m)
{
    m_x=sqrt(m(0,0));
    m_y=sqrt(m(1,1));
    m_z=sqrt(m(2,2));
}
