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

#ifndef POINT_H
#define POINT_H

#include "compile.h"
#include "coord.h"
#include "datafile.h"
#include "ellipsoid.h"
#include "json/json.h"
#include "parameter.h"
#include "project.h"
#include "station.h"
#include <eigen3/Eigen/Dense>
#include <list>
#include <sstream>
#include <string>
#include <vector>


//constraints in .cor file
//update create_coordinates_constraits() according to this
enum class CR_CODE{
    FORBIDDEN=-1,
    FREE=0,
    CR_XYZ=1,
    CR_XY=2,
    CR_Z=3,
    NIV_FREE=4, //a 1d point
    NIV_CR=5, //a constrained 1d point
    PLANI_FREE=6, //a 2d point
    PLANI_CR=7, //a constrained 2d point
    //from there, not used in internal constraints
    PLANI_FAR_FREE=8, //a far 2d point (not used in internal constraints)
    PLANI_FAR_CR=9, //a constrained far 2d point (not used in internal constraints)
    //last one for tests
    UNDEFINED
};

enum class STATION_CREATION_MODE{
    ST_CREAT_FORBIDDEN,
    ST_CREAT_ALLOWED,
    ST_CREAT_WARNING
};

static const int DIM_PT_UNINIT=-1;

class Point
{
public:
    Point();
    Point(CORFile * _file, int _line_number);
    Point(const Point&) = delete;
    Point(Point&&) = delete;
    Point operator=(const Point&) = delete;
    Point& operator=(Point&&) = delete;
    ~Point();

    Json::Value toJson(FileRefJson &fileRef) const;
    int getPointNumber() const {return pointNumber;}
    bool isInit() const {return dimension>DIM_PT_UNINIT;}//TODO(jmm): use code==UNDEFINED for uninit and set default dim to 0

    std::string name;
    std::string comment;
    CR_CODE code;///known coordinates?
    int dimension;//number of unknowns (DIM_PT_UNINIT if point not init)
    //int index;///in points list
    int lineNumber;///in cor file, to create .new file with comments

    std::list<Station*> stations_;//every station starting on this point, including coordinates constraints (stations belong to Project)
    std::vector<Parameter> params;//always 3 params, with Parameter::cst for lower dim or fixed coords = equivalent to coord_compensated

    //bool coord_read_is_geographical;
    Coord coord_read;//from file
    Coord coord_init_spher;//read transformed into comp local system (spherical)

    Coord coord_comp;//< coord spherical during compensation = *(params[0..2].value), in local ref

    Coord coord_compensated_georef;//compensated in input projection
    Coord sigmas_init;
    Coord shift;//displacement between read and compensated in input projection

    //Monte-Carlo simulation data
    Coord MC_shift_max,MC_shift_sq_average;

    //vertical deflection, NaN is not set, in rad
    //E part, N part, azim and norm
    tdouble dev_eta,dev_xi,dev_azim,dev_norm;

    CORFile * file;

    static std::map<CR_CODE, std::string> all_code_names;
    Ellipsoid ellipsoid;

    bool isFree() const;//< if point code is free for its dimension
    bool isForbidden() const { return code == CR_CODE::FORBIDDEN;}//< if point code is forbidden

    bool read_point(std::string line, bool fromNEW=false);
    bool set_point(bool setParam) { return set_point(coord_read, sigmas_init, code, setParam);}
    bool set_point(const Coord &_coord_read, const Coord &_sigmas_init, CR_CODE new_code, bool setParam);
    void create_coordinates_constraits();
    template<class T> T *getLastStation(unsigned fileDepth,
                            STATION_CREATION_MODE creation_mode=STATION_CREATION_MODE::ST_CREAT_ALLOWED);//where to add new observations

    bool initCoordinates();
    bool set_posteriori_variance(const MatX &Qxx); //< return false if error
    std::string toString() const;

    std::vector<Point *> pointsMeasured() const;//<list of measured point from this

    void update_MonteCarlo();
    void finish_MonteCarlo(int niter);
    FileComment point2comment() const;

    int nbActiveObs;//< nb obs connected to the point
    std::string code_name;//< set from all_code_names when setParam

    Obs *obsX, *obsY, *obsZ; //if constraints
    bool isXfixed, isYfixed, isZfixed;
    static unsigned int pointCounter;
    int pointNumber;
    bool toBeDeleted;
protected:
    unsigned getFileDepth()   const {return file ? file->get_fileDepth()+1 : 1;}
    std::string getFileName() const {return file ? file->get_name() : std::string();}
};


/********
find the last station of a given type (to add new observations)
or creates the first station of this type
*******/
template<class T>
T * Point::getLastStation(unsigned fileDepth, STATION_CREATION_MODE creation_mode)
{
    for (auto st = stations_.rbegin(); st != stations_.rend(); ++st)
    {
        T* casted_station = dynamic_cast<T*>(*st);
        if (casted_station)
            return casted_station;
    }
    //if no station found, create it (if allowed)
    switch (creation_mode)
    {
      case STATION_CREATION_MODE::ST_CREAT_FORBIDDEN:
        Project::theInfo()->warning(INFO_COR,fileDepth+1,
                                    QT_TRANSLATE_NOOP("QObject","Error: not allowed to create station"
                                                                " on point %s."),
                                    name.c_str());
        return nullptr;
      case STATION_CREATION_MODE::ST_CREAT_WARNING:
        Project::theInfo()->warning(INFO_COR,fileDepth+1,
                                    QT_TRANSLATE_NOOP("QObject","Warning: try to create a new hz station"
                                                                " on point %s without opening."),
                                    name.c_str());
        FALLTHROUGH
      default:
        return Station::create<T>(this);
    }
}

#endif // POINT_H
