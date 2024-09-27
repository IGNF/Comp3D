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

#ifndef STATION_AXIS_H
#define STATION_AXIS_H

#include "compile.h"
#include "axisobs.h"
#include "point.h"
#include <string>
#include <vector>


class Station_Axis;

enum MainDir{
    MainDir_unknown,
    MainDir_X,
    MainDir_Y,
    MainDir_Z
};

/**
 * @brief The AxisTarget class
 */
class AxisTarget
{
    friend class Station_Axis;
public:
    AxisTarget(Station_Axis* station, const std::string &targetNum);
    bool initialize(bool verbose);//<compute l r when axis is init
    Json::Value toJson() const;
    std::string getTargetNum() const {return mTargetNum;}
    std::vector<const Point *> get3BestPos();//<using coord_read, 2 most distant, one far from the 2 first
    tdouble getL() const {return l;}
    tdouble getR() const {return r;}
    unsigned getParamLindex() const {return param_l_index;}
    unsigned getParamRindex() const {return param_r_index;}
    std::list <AxisObs> allAxisObs;
    void removeObstoBeDeleted();
protected:
    std::string mTargetNum;
    bool active;
    Station_Axis* mStation;
    tdouble l,r; //parameters
    tdouble mWingspan;//<wingspan = diagonal length of the AABB with coord_init_spher, computed by get3BestPos;
    unsigned param_l_index,param_r_index;//<to have access to the params in mStation.params
};


class Station_Axis : public Station
{
#ifdef USE_QT
    Q_OBJECT
#endif
public:
    std::string typeStr() const override;
    AxisTarget* updateTarget(std::string target_num, AxisObs &axisObs);

    bool read_obs(std::string line, int line_number, DataFile * _file, const std::string &current_absolute_path, const std::string &comment) override;
    bool set_obs(LeastSquares *lsquares, bool initialResidual, bool internalConstraint) override;
    bool initialize(bool verbose) override;
    void update() override;

    Json::Value toJson(FileRefJson& fileRef) const override;

    bool isInternal(Obs* obs) override;//if the obs is compatible with internal constraints
    bool isOnlyLeveling(Obs* obs) override;//for internal constraints
    bool isHz(Obs* obs) override;//for internal constraints
    bool isDistance(Obs* obs) override;//for internal constraints
    bool isBubbuled(Obs* obs) override;//for internal constraints
    bool useVertDeflection(Obs* obs) override;//to check vertical deflection consistancy
    int numberOfBasicObs(Obs* obs) override;//number of lines in the matrix
    void removeObsConnectedToPoint(const Point& point) override;
    const AxisFile& getFile() const {return *file;}
    Coord getN() const{return n;}
    std::list <AxisTarget>& getTargets(){return targets;}
    MainDir getMainDir() const {return mMainDir;}
    bool read_constr(const std::string &line, int line_number, DataFile *_file, const std::string &comment);
    Obs * getAxisCombi() const {return axisCombi;}
protected:
    template <typename T>
    friend T* Station::create(Point *origin);
    explicit Station_Axis(Point *origin);

    std::list <AxisTarget> targets; //target num 1 is targets[0]
    std::unique_ptr<AxisFile> file;
    MainDir mMainDir;
    //only 2 of those variables are parameters (depending on axis orientation)
    Coord n;//parameters: value of axis orientation
    Obs *axisOrientFix;//<The observation to fix one parameter of orientation
    Obs *axisCombi;//if point must be perp to an othe point.
};

#endif // STATION_AXIS_H
