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

#ifndef PROJECTION_H
#define PROJECTION_H

#include <json/json.h>
#include <proj.h>
#include <eigen3/Eigen/Dense>
#include "coord.h"
#include "compile.h"

enum class PROJ_TYPE{
    PJ_UNINIT,
    PJ_LOCAL,
    PJ_GEO,
};

struct CRSproj
{
    int EPSGcode; //only for information
    std::string name;
    std::string def;
};

//GRS80
const tdouble semi_axis = 6378137;
const tdouble e2        = 0.00669438;

/********
 * The Stereographic projection is the one used in local COR file
 * it has a local scale, to be equivalent to any conform projection
 *
 * the spherical frame is the one used for computations
 *
 * cartesian coordinates are used for export and pure 3D obs
 *
 * here cartesian means local cartesian (0,0,0) for center, z=vertical at center
 *
 * Cartesian frames:
 *   - geocentric : from Earth center, Z = Earth rotation axis
 *   - global : from project origin, Z = ellips normal at origin
 *   - vertical : from one point, Z = vertical at this point (using vert dev)
 *   - instrument : from one point, Z = Z of instrument frame
 *
 * */
class Projection
{
public:
    Projection();
    ~Projection();
    void clear();
    bool initLocal(tdouble _latitude, const Coord &_center, tdouble _scale);
    bool initGeo(const Coord &_center, const std::string &_projDef, bool forLocal=false);
    tdouble getStereoMeridianConvergence(const Coord &coord_comp);
    bool calcRadius(tdouble _latitude);

    bool georefToSpherical(const Coord &in,Coord &out) const;

    bool sphericalToGeoref(const Coord &in, Coord &out) const;

    void sphericalToCartGeocentric(const Coord &in, Coord &out) const;

    void sphericalToGlobalCartesian(const Coord &in, Coord &out) const;
    void globalCartesianToSpherical(const Coord &in, Coord &out) const;

    void globalCartesianToVertCartesian(const Coord &in, Coord &out, const Coord &vert_ref_spher,
                                        tdouble eta, tdouble xi) const;
    void vertCartesianToGlobalCartesian(const Coord &in, Coord &out, const Coord &vert_ref_spher,
                                        tdouble eta, tdouble xi) const;

    void sphericalToLatLong(const Coord &in, Coord &out) const;

    void globalCartesianToCartGeocentric(const Coord &cartesian, Coord &out) const;
    void cartGeocentricToglobalCartesian(const Coord &cartgeo, Coord &out) const;

    bool georefToLatLong(const Coord &in, Coord &out) const;
    PJ_FACTORS getInputProjFactors(const Coord & coord_comp);

    //void setGeoParam(tdouble _longitude);//set geographical coords parameters
    Json::Value toJson() const;

    tdouble latitude;//,longitude;
    tdouble radius;
    Coord centerStereo;
    Coord centerGeoref;
    Coord centerGeographic;
    Coord centerCartGeocentric;
    Mat3 RotGlobal2Geocentric;
    tdouble scale;

    std::string projDef, stereoDef, latlongDef, geocentDef;//<proj4 definitions (ex: "IGNF:LAMB93")

    PROJ_TYPE type;
    PJ *stereoProj;
    PJ *pj_in2latlong, *pj_in2geocent, *pj_in2stereo; //pj_in2stereo only for georef
    PJ *pj_in;
    int projIndex;//in allEPSG

    static const std::vector<CRSproj>& allCRS(void) { return _allCRS; };

    static void showAllProj();
    static void setCmdLineProjPath(const char* path) { cmdLineProjPath = path ;}
private:
    static bool init_proj_dir();
    static std::vector<CRSproj> _allCRS;
    static bool projDir_found;
    static std::string cmdLineProjPath;
};

#endif // PROJECTION_H
