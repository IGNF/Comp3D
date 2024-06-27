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

#ifndef STATION_H
#define STATION_H

#include "obs.h"
#include "filerefjson.h"
#include "json/json.h"
#include "parameter.h"
#include <vector>
#include <list>


class Point;
class DataFile;

enum class STATION_TYPE{
    ST_NONE = 0,//undefined
    ST_SIMPLE, //no parameter
    ST_HZ, //unknown hz
    ST_BASCULE, //3d rotation unknown
    ST_AXIS,
    ST_EQ,
};

/**
  Abstract class for a Station.
  One station is linked to a point ("from"),
  and a set of parameters (unknowns)
  describing its orientation etc.
  **/
class Station
{
public:
    template <class T>
    static T *create(Point *origin);         // Factory: only way to create a station

    Station(const Station&)=delete;
    Station(Station&&)=delete;
    Station& operator=(const Station&)=delete;
    Station& operator=(Station&&)=delete;
    virtual ~Station();
    Point* origin() const {return mOrigin;}
    bool initOk() const {return mInitOk;}
    std::vector<Parameter> params;
    std::list<Obs> observations;//< Every obs on this station, used for json generation.

    virtual std::string toString() const;
    virtual std::string typeStr() const =0;

    virtual bool read_obs(std::string line, int line_number,DataFile * _file,
                          const std::string &current_absolute_path,const std::string &comment)=0;
    //add observations to least square system, returns false if problem
    virtual bool set_obs(LeastSquares *lsquares, bool initialResidual, bool internalConstraints)=0;
    virtual bool initialize(bool verbose)=0;//< compute initial values for stations unknown if possible. Can also init from point if possible.
    virtual void update()=0;//< update station data after each iteration

    virtual bool isInternal(Obs* obs)=0;//< if the obs is compatible with internal constraints
    virtual bool isOnlyLeveling(Obs* obs)=0;//< for internal constraints
    virtual bool isHz(Obs* obs)=0;//< for internal constraints
    virtual bool isDistance(Obs* obs)=0;//< for internal constraints
    virtual bool isBubbuled(Obs* obs)=0;//< for internal constraints
    virtual bool useVertDeflection(Obs* obs)=0;//< to check vertical deflection consistancy

    virtual int numberOfBasicObs(Obs* obs)=0;//< number of lines in the matrix
    virtual void changeObsActivation(Obs &obs, bool active);//< enventually desactivate other obs

    virtual Json::Value toJson(FileRefJson& filesRef) const;

    void removeObs(const std::vector<Obs *> &list_obs_to_remove);
    void removeObsToPointsToDelete();
    virtual void removeObsConnectedToPoint(const Point& point);
    int updateNumberOfActiveObs(bool internalOnly=false);
    int getNum() const { return num; }
    static void clearNumStationsPerPointPerType();

protected:
    Station(Point *origin, STATION_TYPE _type);

    static std::map<std::pair<Point*, STATION_TYPE>, int> numStationsPerPointPerType;
    STATION_TYPE type;
    int num; //number per point per type

    Point * mOrigin;
    bool mInitOk;//TODO(jmm): make this and many other members protected!

    int nbActiveObs;

    friend class Tests_Coord;
};


template<class T>
T *Station::create(Point *origin)
{
    return new T(origin); // NOLINT : This is a non owning pointer, owner is Project::stations (see Station::Station() )
}

#endif // STATION_H
