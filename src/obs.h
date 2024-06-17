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

#ifndef OBS_H
#define OBS_H

#include <string>
#include "json/json.h"

#include "compile.h"
#include "filerefjson.h"

/*************
  General observation
To know the meaning of "code", you need to know the station type.
 *************/

class Point;
class Station;
class DataFile;
class LeastSquares;
class Obs3D;

enum class STATION_CODE{
    ST_UNDEF = -1,
    BASC_XYZ_CART=11, //1,2,3 are coordinates (3D scanner style) in cartesian
    BASC_ANG_CART=12, //1,2,3 are hz, zen, dist (tracker style) in cartesian
    AXIS_OBS=18,
    BASELINE_GEO_XYZ=19, //1,2,3 are dX dY dZ in cartesian geocentric (baseline)
    OBS_EQ_DH=21,
    OBS_EQ_DIST=22,
};

enum class OBS_CODE{
    UNKNOWN=0,

    //only created by points
    COORD_X=-1,
    COORD_Y=-2,
    COORD_Z=-3,

    //in .obs
    DIST1=1,
    DIST_HZ0=2,
    DIST=3,
    DH=4,
    HZ_ANG=5,
    ZEN=6,
    HZ_REF=7,
    AZIMUTH=8,
    CENTER_META=9,//used only to create DE and DN obs

    DX_SPHER=14,//diff in x spherical coords
    DY_SPHER=15, //diff in y spherical coords

    EQ_DH=21, //dh equals 1 param
    EQ_DIST=22, //dist equals 1 param

    //only created by basc
    BASCULE_X=101,
    BASCULE_Y=102,
    BASCULE_Z=103,
    BASCULE_HZ=104,
    BASCULE_ZEN=105,
    BASCULE_DIST=106,
    BASELINE_X=107, //gnss baselines
    BASELINE_Y=108,
    BASELINE_Z=109,

    //only created by axis
    AXIS_R=181,
    AXIS_T=182,
    AXIS_COMBI=184, //for axis combination
    AXIS_FIX_X=187,
    AXIS_FIX_Y=188,
    AXIS_FIX_Z=189,

    //for internal constraints
    INTCONST_DX=201,//sum(dx)=0
    INTCONST_DY=202,//sum(dy)=0
    INTCONST_DZ=203,//sum(dz)=0
    INTCONST_RX=204,//drot(x)=0
    INTCONST_RY=205,//drot(y)=0
    INTCONST_RZ=206,//drot(z)=0
    INTCONST_SC=207,//dscale=0

};


class Obs
{
    friend class LeastSquares;
    friend class Obs3D;
public:
    Obs(Point *_from, Point *_to, Station *_station, OBS_CODE _code, bool _active, tdouble _value_original,
        tdouble _sigma_abs_original, tdouble _sigma_rel, tdouble _instrument_height,
        tdouble _target_height, int _line, tdouble _unit_factor, const std::string &_unit_str,
        DataFile * _file, const std::string &_comment);
    Obs(const Obs &other);

    std::string toString() const;
    std::string toObsFile(bool withComputedValue=false) const;
    Json::Value toJson(FileRefJson &filesRef) const;

    int getObsNumber(){return obsNumber;}
    int getObsRank(){return obsRank;}

    void reset();
    bool accept1D();//< check if obs type will work with 1D points
    bool accept2D();//< check if obs type will work with 2D points
    bool isInternal();//< if the obs is compatible with internal constraints
    bool isOnlyLeveling();//< for internal constraints
    bool isHz();//< for internal constraints
    bool isDistance();//< for internal constraints
    bool isBubbuled();//< for internal constraints
    bool useVertDeflection();//< to check vertical deflection consistancy
    int numberOfBasicObs();//< number of lines in the matrix (the obs is supposed to be active)
    bool checkPointsDimension();//< returns ok if points dimensions are correct for this obs
    bool computeInfos(bool isInitialResidual);

    //setEquation only for simple type of obs. Other are handled by Stations classes
    bool setEquation(LeastSquares *lsquares,
                     bool isInitialResidual, bool internalConstraints);

    int lineNumber;//line number
    Point *from;
    Point *to;
    Station *station;
    OBS_CODE code;
    tdouble unitFactor;//< factor from original unit to least squares unit
    tdouble value,originalValue;//< read value is converted in computation unit, original is in the unit of the file
    tdouble sigmaAbs,sigmaAbsOriginal;
    tdouble sigmaRel,sigmaTotal;//< sigma total = absolute + relative*/distance
    tdouble sigmaAposteriori;//< from Qll
    tdouble instrumentHeight,targetHeight;
    tdouble computedValue;
    tdouble deflectionCorrection;//< vertical deflection correction
    tdouble residual,normalizedResidual;//< after compensation
    tdouble residualStd;//< compared to sigmaAposteriori
    tdouble initialResidual,initialNormalizedResidual;//< before compensation
    tdouble sigmaResidual;
    tdouble obsRedondancy;//< percent: 0% very useful, 100% useless
    tdouble standardizedResidual;//< undefined if obsRedondancy<1% (set to nan, not exported in json)
    tdouble appliedNormalisation;//< only for CI observations that have very big values
    std::string comment;
    bool active;
    bool activeRead; //< save original active to know if needs to be changed in obs file
    bool varianceFromMatrix;
    std::string unitStr;
    DataFile * file;
    tdouble getAzim(){return azim;}
    tdouble getZen(){return zen;}

    static std::string getTypeName(OBS_CODE _code);

    static std::map<OBS_CODE,std::string> obsTypeName;

    static int obsCounter;
protected:
    int obsNumber;//among all obs
    int obsRank;//only active obs, rank in A
    tdouble C,S;//sin and cos from earth center
public:
    tdouble D,D0,Dhz;//length of the obs (to compute mm residual for angles), hz distance level 0, hz distance (on origin level)
    tdouble azim, zen;//azimuth = absolute breaing of the observation, zenithal of obs (with heights)
};

#endif // OBS_H
