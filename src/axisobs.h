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

#ifndef AXISOBS_H
#define AXISOBS_H

#include "point.h"
#include "compile.h"
#include <string>
#include <vector>

class Station_Axis;
class AxisTarget;

class AxisObs
{
public:
    explicit AxisObs(Station_Axis *from);
    std::string toString() const;
    Json::Value toJson() const;
    bool read_obs(const std::string& line, int line_number, DataFile * _file, const std::string& comment);
    bool set_obs(LeastSquares *lsquares, bool initialResidual, bool internalConstraints);
    std::string getTargetNum() const {return mTargetNum;}
    std::string getPosNum() const {return mPosNum;}
    const Point *getPt() const {return mPt;}
    void setTarget(AxisTarget *target) {mTarget=target;}
    Obs *obsR,*obsT;
    bool toBeDeleted=false;
protected:
    Station_Axis *mStation;
    std::string mTargetNum;//number of the target
    std::string mPosNum;//number of the position of the target
    tdouble mSigmaR;//radius stability
    tdouble mSigmaT;//transverse stability (orthogonality)
    AxisTarget *mTarget;
    Point *mPt;//ground point corresponding to the target in position mPosNum
};

#endif // AXISOBS_H
