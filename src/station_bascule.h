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

#ifndef STATION_BASCULE_H
#define STATION_BASCULE_H

#include "coord.h"
#include "compile.h"
#include "obs.h"
#include "point.h"
#include "station.h"
#include <eigen3/Eigen/Dense>
#include <string>
#include <vector>


class Station_Bascule;

class Obs3D
{
public:
    explicit Obs3D(Station_Bascule* _station);
    std::string toString() const;
    Json::Value toJson() const;
    bool read_obs(const std::string& line, int line_number, DataFile * _file, const std::string& comment);
    bool set_obs(LeastSquares *lsquares, bool initialResidual, bool internalConstraints);
    Coord currVectorToInstrumentFrameCoords(); //< get vector between coord_comp in instrument frame
    Coord obsToInstrumentFrameCoords(bool simul);///<transform observation into coordinates in instrument frame
    Vect3 obsToVectGlobalCart(bool simul); //< normalized sight vector in global cartesian frame
    Vect3 baselineXYZ;//< only for baselines
    Mat3 baselineVarCovar;//< only for baselines

    Obs *obs1,*obs2,*obs3;
protected:
    Station_Bascule* station;
};


/**
  Class for a scanner Station, with 3 orientation unknowns
  Used for : laser scanner, laser tracker
  **/
class Station_Bascule : public Station
{
public:
    std::string typeStr() const override;
    bool read_obs(std::string line, int line_number, DataFile * _file, const std::string &current_absolute_path, const std::string &comment) override;

    bool set_obs(LeastSquares *lsquares, bool initialResidual, bool internalConstraints) override;
    Json::Value toJson(FileRefJson& fileRef) const override;
    bool initialize(bool verbose) override;
    void update() override;

    bool isInternal(Obs* obs) override;//if the obs is compatible with internal constraints
    bool isOnlyLeveling(Obs* obs) override;//for internal constraints
    bool isHz(Obs* obs) override;//for internal constraints
    bool isDistance(Obs* obs) override;//for internal constraints
    bool isBubbuled(Obs* obs) override;//for internal constraints
    bool useVertDeflection(Obs* obs) override;//to check vertical deflection consistancy
    int numberOfBasicObs(Obs* obs) override;//number of lines in the matrix
    void changeObsActivation(Obs& obs, bool active) override;//enventually desactivate other obs

    bool readXYZrotation(const std::string &filename);

    //transformations between instrument and ground frames (fromFile to know if use compensated or .xyz params)
    Coord ptCartGlobal2Instr(const Coord &pt_cart_global, bool fromFile);
    Mat3 sigmaGlobal2Instr(const Mat3 &varCovarGlobal);//we suppose that global sherical and cartesian is the same for sigmas
    Coord mes2GlobalCart(const Coord &target_in_instr_coords, bool fromFile);//where the point measured by the station is on global cart frame
    Mat3 sigmaInstr2Global(const Mat3 &varCovarInstr);//we suppose that global sherical and cartesian is the same for sigmas

    tdouble angleInstr2Vert() const;

    //all orientation parameters are in vertical cartesian frame (where z = vertical at station position)
    bool isVertical;
    bool isGeocentric;//used for GNSS baselines: no parameters, observations are in cartesian geocentric
    tdouble da,db,dc;//dR unknowns, params equivalent to *(params[1,2,3].value,)
    tdouble a,b,c;//value of orientations unknowns
    Mat3 R_vert2instr;//Matrix between vertical on station point and instrument z (should be I if bubbuled)

    tdouble stationHeight, varianceFactor; //only for GNSS baselines

    //only when reading transfo
    std::string readStationName;
    Coord readStationCoordCartGlobal;

    std::list<Obs3D> observations3D;
    STATION_CODE triplet_type;

    void removeObsConnectedToPoint(const Point& point) override;
    const XYZFile& getFile(){return *file;}

protected:
    template <typename T>
    friend T* Station::create(Point *origin);
    explicit Station_Bascule(Point *origin);
    std::unique_ptr<XYZFile> file;
};

#endif // STATION_BASCULE_H
