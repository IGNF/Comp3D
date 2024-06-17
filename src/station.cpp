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

#include "station.h"

#include "point.h"

std::map<std::pair<Point*, STATION_TYPE>, int> Station::numStationsPerPointPerType={};

void Station::clearNumStationsPerPointPerType()
{
    numStationsPerPointPerType.clear();
}

Station::Station(Point *origin, STATION_TYPE _type) :
    type(_type),num(-1),mOrigin(origin),mInitOk(false),nbActiveObs(-1)
{
    if (origin)
        origin->stations_.push_back(this);
    Project::theone()->stations.push_back(std::unique_ptr<Station>(this));

    if (numStationsPerPointPerType.find({origin, type})==numStationsPerPointPerType.end())
        numStationsPerPointPerType[{origin, type}] = 0;
    num = ++numStationsPerPointPerType[{origin, type}];
}

Station::~Station()
{
    if (origin())
        origin()->stations_.remove(this);
    observations.clear();
#ifdef INFO_DELETE
    if (origin())
        std::cout<<"Station "<<(origin()?origin()->name:"?")<<" deleted"<<std::endl;
    else
        std::cout<<"Station ? deleted"<<std::endl;
#endif
}

void Station::removeObsToPointsToDelete()
{
#ifdef INFO_DELETE
    for (auto & obs : observations)
    {
        if (obs.to->toBeDeleted)
            std::cout<<obs.toString()<<std::endl;
    }
#endif
    auto obsToDelete = [](auto const& obs) { return (obs.from &&  obs.from->toBeDeleted) || ( obs.to && obs.to->toBeDeleted ); };
    observations.remove_if(obsToDelete);
}


void Station::removeObs(const std::vector<Obs *> &list_obs_to_remove)
{
#ifdef INFO_DELETE
    if (!list_obs_to_remove.empty())
    {
        std::cout<<"removeObs from "<<toString()<<":\n";
        for (const auto &obs_to_remove:list_obs_to_remove)
            std::cout<<obs_to_remove<<std::endl;
    }
#endif
    for (const auto & obs_to_remove : list_obs_to_remove)
    {
        auto isToDelete = [&obs_to_remove](auto const& obs) { return &obs==obs_to_remove; };
        observations.remove_if(isToDelete);
    }
}

void Station::removeObsConnectedToPoint(const Point &point)
{
    observations.remove_if([&point](const auto& obs){ return obs.to==&point || obs.from==&point;});
}

int Station::updateNumberOfActiveObs(bool internalOnly)
{
    //first, check if other obs have to be desactivated
    for (auto & obs : observations)
        changeObsActivation(obs, obs.active);
    //count active obs
    nbActiveObs=0;
    for (auto & obs : observations)
    {
        if (obs.active && (!internalOnly || obs.isInternal()))
            nbActiveObs+=obs.numberOfBasicObs();
    }
    return nbActiveObs;
}

Json::Value Station::toJson(FileRefJson &filesRef) const
{
    Json::Value val;
    val["type"] = typeStr();
    val["observations"] = Json::arrayValue;
    for (const auto & obs: observations)
        val["observations"].append(obs.toJson(filesRef));
    val["nbr_active_obs"] = nbActiveObs;
    Json::Value jsparams;
    for (const auto &param:params)
        jsparams[param.name] = param.toJson();
    val["params"]=jsparams;
    val["num"]=num;
    return val;
}

void Station::changeObsActivation(Obs& obs, bool active)
{
    obs.active = active;
}

std::string Station::toString() const
{
    std::ostringstream oss;
    oss<<"station type "<<typeStr()<<" number "<<getNum();
    if (origin())
        oss<<" on point "<<origin()->name;
    return oss.str();
}
