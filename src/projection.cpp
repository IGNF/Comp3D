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

#include "projection.h"

#include <iostream>
#include <boost/filesystem.hpp>
#include <cstdlib>
#include "mathtools.h"
#include "project.h"

#ifdef USE_QT
    #include <QCoreApplication>
    #include <QFileInfo>
    #include <QDir>
    #include <QSettings>
#endif

std::string Projection::cmdLineProjPath="";
bool Projection::projDir_found=false;

static std::vector<CRSproj> build_allCRS();
std::vector<CRSproj> Projection::_allCRS=build_allCRS();

Projection::Projection()
    :latitude(0),radius(0),centerStereo(),centerGeoref(),centerGeographic(),centerCartGeocentric(),scale(1),
      projDef("?"),stereoDef("?"), latlongDef("?"),geocentDef("?"),type(PROJ_TYPE::PJ_UNINIT),
      pj_in2latlong(nullptr),pj_in2geocent(nullptr),pj_in2stereo(nullptr),pj_in(nullptr),projIndex(-1)
{
}

Projection::~Projection()
{
    clear();
}

void Projection::clear()
{
    type=PROJ_TYPE::PJ_UNINIT;
    if (pj_in2latlong)
    {
        proj_destroy(pj_in2latlong);
        pj_in2latlong=nullptr;
    }
    if (pj_in2geocent)
    {
        proj_destroy(pj_in2geocent);
        pj_in2geocent=nullptr;
    }
    if (pj_in2stereo)
    {
        proj_destroy(pj_in2stereo);
        pj_in2stereo=nullptr;
    }
    if (pj_in)
    {
        proj_destroy(pj_in);
        pj_in=nullptr;
    }
}


bool Projection::calcRadius(tdouble _latitude)
{
    latitude=_latitude;

    tdouble reduction_level=0;//useless?
    radius=semi_axis*sqrt(1-e2)/(1-e2*sqr(sin(toRad(latitude,ANGLE_UNIT::DEG)))) + reduction_level;//total curvature sphere

    return true;
}

bool Projection::initLocal(tdouble _latitude, const Coord &_center, tdouble _scale)
{
    clear();
    std::ostringstream oss;
    scale = _scale;
    oss<<"+proj=sterea +lat_0="<<_latitude<<" +lon_0=0 +k_0="<<scale<<" +x_0="<<_center.x()<<" +y_0="<<_center.y();
    std::cout<<"Local proj is: "<<oss.str()<<std::endl;
    return initGeo(_center, oss.str(), true);
}

void Projection::showAllProj()
{
    std::cout<<"List of authorized projections:\n";
    for (auto &proj:Projection::allCRS())
    {
        if (proj.EPSGcode>0)
            std::cout<<"    \""<<proj.def<<"\"\n";
    }
    std::cout<<"    User defined\n";
    std::cout<<"End of list of authorized projections."<<std::endl;
}

bool Projection::initGeo(const Coord &_center, const std::string &_projDef, bool forLocal)
{
    if (!init_proj_dir())
        return false;

    clear();

    if (!forLocal)
    {
        //test if projection autorized:
        projIndex=-1;
        for (unsigned int i=0;i<Projection::allCRS().size();i++)
        {
            CRSproj proj=Projection::allCRS()[i];
            if (proj.def==_projDef)
            {
                projIndex=(int)i;
                break;
            }
        }
    }


    projDef=_projDef;

    //local stereographic :
    //http://geotiff.maptools.org/proj_list/oblique_stereographic.html
    //  +proj=sterea +lat_0=Latitude of natural origin
    //        +lon_0=Longitude of natural origin
    //        +k_0=Scale factor at natural origin
    //        +x_0=False Easting
    //        +y_0=False Northing

    PJ_COORD p1,p2;
    latlongDef ="+proj=latlong";
    geocentDef="+proj=geocent";
    //TODO: add proj area

    if (!(pj_in = proj_create(nullptr, projDef.c_str())))
    {
        Project::theInfo()->error(INFO_CONF,1,QT_TRANSLATE_NOOP("QObject","Error init proj \"%s\": %d %s"),
                                  projDef.c_str(),proj_errno(pj_in),proj_errno_string(proj_errno(pj_in)));
        return false;
    }

    if (!(pj_in2latlong =proj_create_crs_to_crs(nullptr, projDef.c_str(), latlongDef.c_str(), nullptr)))
    {
        Project::theInfo()->error(INFO_CONF,1,
                                  QT_TRANSLATE_NOOP("QObject","Error: init proj \"%s\" to \"%s\": %d %s"),
                                  projDef.c_str(),latlongDef.c_str(),proj_errno(pj_in2latlong),
                                  proj_errno_string(proj_errno(pj_in2latlong)));
        return false;
    } 

    if (!(pj_in2geocent = proj_create_crs_to_crs(nullptr, projDef.c_str(), geocentDef.c_str(), nullptr)))
    {
        Project::theInfo()->error(INFO_CONF,1,
                                  QT_TRANSLATE_NOOP("QObject","Error: init proj \"%s\" to \"%s\": %d %s"),
                                  projDef.c_str(),geocentDef.c_str(),proj_errno(pj_in2geocent),
                                  proj_errno_string(proj_errno(pj_in2geocent)));
        return false;
    }

    centerGeoref=_center;

#ifdef INFO_PROJ
    Project::theInfo()->msg(INFO_CONF,1,QT_TRANSLATE_NOOP("QObject","projDef %s"),projDef.c_str());
    Project::theInfo()->msg(INFO_CONF,1,QT_TRANSLATE_NOOP("QObject","defLatLong %s"),latlongDef.c_str());
    Project::theInfo()->msg(INFO_CONF,1,QT_TRANSLATE_NOOP("QObject","defGeocentr %s"),geocentDef.c_str());
    Project::theInfo()->msg(INFO_CONF,1,QT_TRANSLATE_NOOP("QObject","Center input: %.6f %.6f"),_center.x(),_center.y());
#endif
    p1.xyzt.x=_center.x();
    p1.xyzt.y=_center.y();
    p1.xyzt.z=0;
    p1.xyzt.t=0;
    p2 = proj_trans(pj_in2latlong, PJ_FWD, p1);
    if (proj_errno(pj_in2latlong))
    {
        Project::theInfo()->error(INFO_CONF,1,
                                  QT_TRANSLATE_NOOP("QObject","Error in projecting center %s"),
                                  proj_errno_string(proj_errno(pj_in2latlong)));
        return false;
    }
    centerGeographic.set(p2.uv.u,p2.uv.v,0);
#ifdef INFO_PROJ
    Project::theInfo()->msg(INFO_CONF,1,
                            QT_TRANSLATE_NOOP("QObject","Center lat long (deg): %.6f %.6f"),
                            centerGeographic.y(),centerGeographic.x());
#else
    std::streamsize ss = std::cout.precision();
    std::cout<<"Center lat long (deg): "<<std::setprecision(15)<<centerGeographic.y()<<" "
             <<centerGeographic.x()<<std::setprecision(ss)<<std::endl;
#endif

    p1.xyzt.x=_center.x();
    p1.xyzt.y=_center.y();
    p1.xyzt.z=0;
    p1.xyzt.t=0;
    p2 = proj_trans(pj_in2geocent, PJ_FWD, p1);
    if (proj_errno(pj_in2geocent))
    {
        Project::theInfo()->error(INFO_CONF,1,QT_TRANSLATE_NOOP("QObject","Error projecting center: %s"),
                                  proj_errno_string(proj_errno(pj_in2geocent)));
        return false;
    }
    centerCartGeocentric.set(p2.xyz.x,p2.xyz.y,p2.xyz.z);
#ifdef INFO_PROJ
    Project::theInfo()->msg(INFO_CONF,1,
                            QT_TRANSLATE_NOOP("QObject","Center cartesian geocentric: %.6f %.6f %.6f"),
                            centerCartGeocentric.x(),centerCartGeocentric.y(),centerCartGeocentric.z());
#endif

    //compute rotation RotGlobal2Geocentric
    tdouble longitude=centerGeographic.x()*PI/180.0;
    tdouble latitude=centerGeographic.y()*PI/180.0;
    Mat3 Rz;
    Rz <<  cos(longitude), sin(longitude), 0,
         -sin(longitude), cos(longitude), 0,
            0,                          0,                      1;

    Mat3 Ry;
    Ry<< cos(PI/2-latitude), 0,  -sin(PI/2-latitude),
            0 ,         1,       0,
         sin(PI/2-latitude), 0,   cos(PI/2-latitude);

    Mat3 Rz2;
    Rz2<< 0,  1,  0,
         -1,  0,  0,
          0,  0,  1;

    RotGlobal2Geocentric=(Rz2*Ry*Rz).transpose();
    //std::cout<<"T="<<centerCartGeocentric.toString()<<std::endl;
    //std::cout<<"R=\n"<<RotLocal2Geocentric<<std::endl;

    if (!calcRadius(centerGeographic.y()))
        return false;

    std::ostringstream stereo_def_oss;
    //use international 1924  => sigma0=15   +a=6378388.0 +b=6356911.946
    //use GRS80               => sigma0=11   +a=6378137   +b=6356752.314
    //default                 => sigma0=11
    stereo_def_oss<<std::setprecision(15)<<"+proj=sterea +lat_0="<<centerGeographic.y()
                <<" +lon_0="<<centerGeographic.x()
                <<" +k_0="<<1.0
                <<" +x_0="<<0.0
                <<" +y_0="<<0.0;
    stereoDef=stereo_def_oss.str();

#ifdef INFO_PROJ
    Project::theInfo()->msg(INFO_CONF,1,
                            QT_TRANSLATE_NOOP("QObject","Local stereographic definition: %s"),
                            stereoDef.c_str());
#endif
    if (!forLocal)
    {
        if (!(pj_in2stereo = proj_create_crs_to_crs(nullptr, projDef.c_str(), stereoDef.c_str(), nullptr)))
        {
            Project::theInfo()->error(INFO_CONF,1,
                                      QT_TRANSLATE_NOOP("QObject","Error: init proj \"%s\" to \"%s\": %d %s"),
                                      projDef.c_str(),stereoDef.c_str(),proj_errno(pj_in2stereo),
                                      proj_errno_string(proj_errno(pj_in2stereo)));
            return false;
        }

        p1.xyzt.x=_center.x();
        p1.xyzt.y=_center.y();
        p1.xyzt.z=0;
        p1.xyzt.t=0;
        p2 = proj_trans(pj_in2stereo, PJ_FWD, p1);
        if (proj_errno(pj_in2stereo))
        {
            Project::theInfo()->error(INFO_CONF,1,
                                      QT_TRANSLATE_NOOP("QObject","Error in projecting center: %s"),
                                      proj_errno_string(proj_errno(pj_in2stereo)));
            return false;
        }
        centerStereo.set(p2.xyz.x,p2.xyz.y,0);
    #ifdef INFO_PROJ
        Project::theInfo()->msg(INFO_CONF,1,
                                QT_TRANSLATE_NOOP("QObject","Center in stereographic: %.6f %.6f %.6f"),
                                p2.xyz.x,p2.xyz.y,p2.xyz.z);
    #endif
        type=PROJ_TYPE::PJ_GEO;
    } else {
        centerStereo = _center;
        type=PROJ_TYPE::PJ_LOCAL;
    }

    auto factors = getInputProjFactors({0,0,0});
    std::cout<<"proj factors at origin:\n";
    std::cout<<" - meridional_scale: "<<factors.meridional_scale<<"\n";
    std::cout<<" - parallel_scale: "<<factors.parallel_scale<<"\n";
    std::cout<<" - meridian_convergence: "<<factors.meridian_convergence<<"\n";

    return true;
}


//converts from georef coords (.cor) to spherical coords (where Comp3D works)
bool Projection::georefToSpherical(const Coord &in,Coord &out) const
{
    if (type!=PROJ_TYPE::PJ_UNINIT)
    {
        PJ_COORD p1,p2;
        if (type==PROJ_TYPE::PJ_GEO)
        {
            p1.xyzt.x=in.x();
            p1.xyzt.y=in.y();
            p1.xyzt.z=in.z();
            p1.xyzt.t=0;
            p2 = proj_trans(pj_in2stereo, PJ_FWD, p1);
            if (proj_errno(pj_in2stereo))
            {
                Project::theInfo()->error(INFO_CONF,1,
                                          QT_TRANSLATE_NOOP("QObject","Error: projecting point %s: %s"),
                                          in.toString().c_str(),proj_errno_string(proj_errno(pj_in2stereo)));
                return false;
            }
        }else{
            p2.xyzt.x=in.x();
            p2.xyzt.y=in.y();
            p2.xyzt.z=in.z();
            p2.xyzt.t=0;
        }
    #ifdef INFO_PROJ
        Project::theInfo()->msg(INFO_CONF,1,
                                QT_TRANSLATE_NOOP("QObject","Point in stereographic: %.6f %.6f %.6f"),
                                p2.xyz.x,p2.xyz.y,p2.xyz.z);
    #endif

        tdouble alpha  = 1+(sqr(  (p2.xyz.x-centerStereo.x())/scale  )+sqr(  (p2.xyz.y-centerStereo.y())/scale  ))/sqr(2*radius);

        tdouble _x      = (p2.xyz.x-centerStereo.x())/scale/alpha;
        tdouble _y      = (p2.xyz.y-centerStereo.y())/scale/alpha;
        out.setx(_x);
        out.sety(_y);
        out.setz(p2.xyz.z);
        return true;
    }else{
        Project::theInfo()->error(INFO_CONF,1,
                                  QT_TRANSLATE_NOOP("QObject","Error: projection uninitialized!"));
        return false;
    }
}


//converts from georef coords (.cor in projection) to lat-long
bool Projection::georefToLatLong(const Coord &in,Coord &out) const
{
    PJ_COORD p1,p2;
    p1.xyzt.x=in.x();
    p1.xyzt.y=in.y();
    p1.xyzt.z=in.z();
    p1.xyzt.t=0;
    p2 = proj_trans(pj_in2latlong, PJ_FWD, p1);
    if (proj_errno(pj_in2latlong))
    {
        Project::theInfo()->error(INFO_CONF,1,
                                  QT_TRANSLATE_NOOP("QObject","Error: projecting point %s: %s"),
                                  in.toString().c_str(),proj_errno_string(proj_errno(pj_in2latlong)));
        return false;
    }
#ifdef INFO_PROJ
    Project::theInfo()->msg(INFO_CONF,1,
                            QT_TRANSLATE_NOOP("QObject","Point in latlong: %.6f %.6f %.6f"),
                            p2.xyz.x,p2.xyz.y,p2.xyz.z);
#endif
    out.set(p2.xyz.x,p2.xyz.y,p2.xyz.z);
    return true;
}

bool Projection::sphericalToGeoref(const Coord &in, Coord &out) const
{
    if (type!=PROJ_TYPE::PJ_UNINIT)
    {
        tdouble alpha  = sqr(2*radius)/(sqr(in.x())+sqr(in.y())+sqr(sqrt(sqr(radius)-sqr(in.x())-sqr(in.y()))+radius));
        tdouble _x      = in.x()*alpha*scale+centerStereo.x();
        tdouble _y      = in.y()*alpha*scale+centerStereo.y();
        PJ_COORD p1,p2;
        if (type==PROJ_TYPE::PJ_GEO)
        {
            p1.xyzt.x=_x;
            p1.xyzt.y=_y;
            p1.xyzt.z=in.z();
            p1.xyzt.t=0;
            p2 = proj_trans(pj_in2stereo, PJ_INV, p1);
            if (proj_errno(pj_in2stereo))
            {
                Project::theInfo()->error(INFO_CONF,1,
                                          QT_TRANSLATE_NOOP("QObject","Error: projecting point %s: %s"),
                                          in.toString().c_str(),proj_errno_string(proj_errno(pj_in2stereo)));
                return false;
            }
        }else{
            p2.xyzt.x=_x;
            p2.xyzt.y=_y;
            p2.xyzt.z=in.z();
            p2.xyzt.t=0;
        }
    #ifdef INFO_PROJ
        Project::theInfo()->msg(INFO_CONF,1,
                                QT_TRANSLATE_NOOP("QObject","Point in stereographic: %.6f %.6f %.6f"),
                                p2.xyz.x,p2.xyz.y,p2.xyz.z);
    #endif
        out.set(p2);

        return true;
    }else{
        Project::theInfo()->error(INFO_CONF,1,
                                  QT_TRANSLATE_NOOP("QObject","Error: projection uninitialized!"));
        return false;
    }
}

void Projection::sphericalToCartGeocentric(const Coord &in, Coord &out) const
{
    Coord in_georef;
    PJ_COORD p_catgeo;
    sphericalToGeoref(in, in_georef);
    p_catgeo = proj_trans(pj_in2geocent, PJ_FWD, in_georef.toPJ());
    out.set(p_catgeo);
}


void Projection::sphericalToGlobalCartesian(const Coord &in, Coord &out) const
{
    Coord in_catgeo;
    sphericalToCartGeocentric(in, in_catgeo);
    cartGeocentricToglobalCartesian(in_catgeo, out);
}

void Projection::globalCartesianToSpherical(const Coord &in, Coord &out) const
{
    Coord in_catgeo, in_georef;
    PJ_COORD p_georef;
    globalCartesianToCartGeocentric(in, in_catgeo);
    p_georef = proj_trans(pj_in2geocent, PJ_INV, in_catgeo.toPJ());
    in_georef.set(p_georef);
    georefToSpherical(in_georef, out);
}

void Projection::globalCartesianToVertCartesian(const Coord &in, Coord &out,const Coord &vert_ref_spher,
                                                tdouble eta, tdouble xi) const
{
    Coord center_catglob;
    sphericalToGlobalCartesian(vert_ref_spher, center_catglob);
    out.set(global2vertRot(vert_ref_spher, eta, xi)*(in.toVect()-center_catglob.toVect()));
}

void Projection::vertCartesianToGlobalCartesian(const Coord &in, Coord &out, const Coord &vert_ref_spher,
                                                tdouble eta, tdouble xi) const
{
    Coord center_catglob;
    sphericalToGlobalCartesian(vert_ref_spher, center_catglob);
    out.set(global2vertRot(vert_ref_spher, eta, xi).transpose()*in.toVect()+center_catglob.toVect());
}

void Projection::sphericalToLatLong(const Coord &in, Coord &out) const
{
    Coord tmp;
    sphericalToGeoref(in, tmp);
    PJ_COORD p1, p2;
    p1.xyzt.x=tmp.x();
    p1.xyzt.y=tmp.y();
    p1.xyzt.z=0;
    p1.xyzt.t=0;
    p2 = proj_trans(pj_in2latlong, PJ_FWD, p1);
    out.set(p2.uv.u,p2.uv.v,in.z());
}

void Projection::globalCartesianToCartGeocentric(const Coord &cartesian,Coord &out) const
{
    out.set(RotGlobal2Geocentric*cartesian.toVect()+centerCartGeocentric.toVect());
}

void Projection::cartGeocentricToglobalCartesian(const Coord &cartgeo, Coord &out) const
{
    out.set(RotGlobal2Geocentric.transpose()*(cartgeo.toVect()-centerCartGeocentric.toVect()));
}

PJ_FACTORS Projection::getInputProjFactors(const Coord & coord_comp)
{
    // example parallel_scale: 0.999885
    // example meridian_convergence: -0.00556984 (rad)
    Coord coord_compensated_georef, latlong;
    sphericalToGeoref(coord_comp, coord_compensated_georef);
    Project::theone()->projection.georefToLatLong(coord_compensated_georef,latlong);
    PJ_COORD p1;
    p1.lp.lam=latlong.x()*PI/180.;
    p1.lp.phi=latlong.y()*PI/180.;
    PJ_FACTORS factors = proj_factors(pj_in, p1);
    if (proj_errno(pj_in))
    {
        Project::theInfo()->error(INFO_CONF,1,QT_TRANSLATE_NOOP("QObject","Error getting factors for input proj with point %s: %s"),
                                  latlong.toString().c_str(),proj_errno_string(proj_errno(pj_in)));
        proj_errno_reset(pj_in);
        return {};
    }
    return factors;
}

Json::Value Projection::toJson() const
{
    Json::Value val;
    val["center_latitude"]=(double)latitude;
    val["earth_model_radius"]=(double)radius;
    val["local_center_stereo"]=centerStereo.toJson();
    val["local_scale"]=(double)scale;
    val["is_georef"]=(type==PROJ_TYPE::PJ_GEO);
    val["input_proj_def"]=projDef;
    val["input_proj_name"]=projIndex>=0?allCRS().at(projIndex).name:std::string("?");
    val["input_proj_EPSG"]=projIndex>=0?allCRS().at(projIndex).EPSGcode:-1;
    val["stereo_def"]=stereoDef;
    val["latlong_def"]=latlongDef;
    val["geocent_def"]=geocentDef;
    if (type==PROJ_TYPE::PJ_GEO)
    {
        val["center_geographic"]=centerGeographic.toJson();
        val["center_cartgeocentric"]=centerCartGeocentric.toJson();
        val["rot_global2geocentric"]=R2json(RotGlobal2Geocentric);
    }
    return val;
}

bool Projection::init_proj_dir()
{
    if (projDir_found)
        return true;

    static const std::vector<std::string> proj_files_to_test={"proj.db","ntf_r93.gsb"};
    //TODO: replace QT with c++14 filesystem current_path()
    enum SearchDir{CMDLINE=0,ABS,
           #ifdef USE_QT
                   EXE,LOCAL,
           #endif
                   STOP};
    #ifdef USE_QT
        QDir newDir;
    #endif
    std::string dir;

    if (Project::theone())
    {
        for (int search=0;search<STOP && !projDir_found;search++)
        {
            switch(search)
            {
            case CMDLINE:
                if (cmdLineProjPath.size() == 0)
                    continue;
                dir = cmdLineProjPath;
                break;
            case ABS:
                dir = std::string(std::getenv("APPDIR")?std::getenv("APPDIR"):"")+"/usr/local/proj82/share/proj/";//$APPDIR to work with AppImage
                break;
    #ifdef USE_QT
            case EXE:
                newDir = QDir(QCoreApplication::applicationDirPath());
                dir = (newDir.absolutePath()+"/proj").toStdString();
                break;
            case LOCAL:
                newDir = QDir(QCoreApplication::applicationDirPath());
                newDir.cdUp();
                dir = (newDir.absolutePath()+"/local/comp3d5_proj").toStdString();
                break;
    #endif
            }
            std::cout<<QObject::tr("Testing Proj files in ").toCstr()<<dir.c_str()<<std::endl;
            projDir_found =true;
            for (auto &filename:proj_files_to_test)
            {
                if (!boost::filesystem::exists(dir+"/"+filename))
                {
                    projDir_found=false;
                    break;
                }
            }
        }

        if (projDir_found)
        {
            std::cout<<QObject::tr("Proj dir found: ").toCstr()<<dir<<"\n";
            const char* paths[1]= {dir.c_str()};
            proj_context_set_search_paths(nullptr,1,paths);
        } else {
            Project::theInfo()->error(INFO_CONF,1,
                                      QT_TRANSLATE_NOOP("QObject","Proj directory not found, please make sure proj grids"
                                                                  " are correctly installed (see documentation)."));
        }
    }
    return projDir_found;
}

static std::vector<CRSproj> build_allCRS()
{
    std::vector<CRSproj> allCRS={
        {0000,QObject::tr("User CRS").toCstr(),QObject::tr("???").toCstr()},
        {2154,"RGF93 / Lambert-93","IGNF:LAMB93"},
    #ifdef ADD_PROJ_CC
        {3942,"RGF93 / CC42","IGNF:RGF93CC42"},
        {3943,"RGF93 / CC43","IGNF:RGF93CC43"},
        {3944,"RGF93 / CC44","IGNF:RGF93CC44"},
        {3945,"RGF93 / CC45","IGNF:RGF93CC45"},
        {3946,"RGF93 / CC46","IGNF:RGF93CC46"},
        {3947,"RGF93 / CC47","IGNF:RGF93CC47"},
        {3948,"RGF93 / CC48","IGNF:RGF93CC48"},
        {3949,"RGF93 / CC49","IGNF:RGF93CC49"},
        {3950,"RGF93 / CC50","IGNF:RGF93CC50"},
    #endif
    #ifdef ADD_PROJ_NTF
        {27571,"NTF (Paris) / Lambert zone I","IGNF:LAMB1"},
        {27572,"NTF (Paris) / Lambert zone II","IGNF:LAMB2"},
        {27573,"NTF (Paris) / Lambert zone III","IGNF:LAMB3"},
        {27574,"NTF (Paris) / Lambert zone IV","IGNF:LAMB4"},
    #endif
    };

    #ifdef ADD_PROJ_UTM
    for (int n=1;n<=60;n++)
    {
        std::ostringstream oss1,oss2;
        oss1<<"WGS 84 / UTM zone "<<n<<"N";
        oss2<<"epsg:"<<32600+n;
        allCRS.push_back({32600+n,oss1.str(),oss2.str()});
    }
    for (int n=1;n<=60;n++)
    {
        std::ostringstream oss1,oss2;
        oss1<<"WGS 84 / UTM zone "<<n<<"S";
        oss2<<"epsg:"<<32700+n;
        allCRS.push_back({32700+n,oss1.str(),oss2.str()});
    }
    #endif
    return allCRS;
}
