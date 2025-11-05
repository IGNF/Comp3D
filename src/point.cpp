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

#include "point.h"
#include "compile.h"
#include "project.h"
#include "station.h"
#include "station_simple.h"
#include <regex>
#include <sstream>



std::map<CR_CODE, std::string> Point::all_code_names{ //always 4 chars
    {CR_CODE::FORBIDDEN,"????" },
    {CR_CODE::FREE,"3---"},{CR_CODE::CR_XYZ,"3xyz"},{CR_CODE::CR_XY,"3xy-"},{CR_CODE::CR_Z,"3--z"},
    {CR_CODE::NIV_FREE,"1  -"},{CR_CODE::NIV_CR,"1  z"},{CR_CODE::PLANI_FREE,"2-- "},
    {CR_CODE::PLANI_CR,"2xy "},{CR_CODE::PLANI_FAR_FREE,"2--R"},{CR_CODE::PLANI_FAR_CR,"2xyR"},
    {CR_CODE::UNDEFINED,"????" }
};

// TODO: constexpr check that every entry in all_code_names corresponds to a CR_CODE


Point::Point():
    code(CR_CODE::UNDEFINED),dimension(DIM_PT_UNINIT),lineNumber(0),
    //coord_read_is_geographical(false),
    dev_eta(NAN),dev_xi(NAN),dev_azim(NAN),dev_norm(NAN),
    file(nullptr),nbActiveObs(0),code_name("?"),obsX(nullptr),obsY(nullptr),obsZ(nullptr),
    isXfixed(false),isYfixed(false),isZfixed(false),
    pointNumber(-1),toBeDeleted(false)
{

}

Point::Point(CORFile * _file,int _line_number):
    code(CR_CODE::UNDEFINED),dimension(-1),lineNumber(_line_number),
    //coord_read_is_geographical(false),
    dev_eta(NAN),dev_xi(NAN),dev_azim(NAN),dev_norm(NAN),
    file(_file),nbActiveObs(0),code_name("?"),obsX(nullptr),obsY(nullptr),obsZ(nullptr),
    isXfixed(false),isYfixed(false),isZfixed(false),
    pointNumber(-1),toBeDeleted(false)
{
  #ifdef COR_INFO
    std::cout<<"Create point."<<std::endl;
  #endif
}

Point::~Point()
{
#ifdef INFO_DELETE
  std::cout<<"Delete point "<<this<<" "<<toString()<<std::endl;
  std::cout<<"With "<<stations_.size()<<" station(s)\n";
  for (auto &st:stations_)
  {
    std::cout<<" - "<<st<<"\n";
  }
  std::cout<<std::flush;
#endif
  stations_.clear();
  if (file) file->removePoint(this);
}

std::string Point::toString() const
{
    std::ostringstream oss;
    oss<<code_name<<" "<<name<<" "<<coord_comp.toString();
    if (ellipsoid.isSet())
    {
        oss<<"   var:";
        for (int i=0;i<3;i++)
            oss<<" "<<ellipsoid.get_variance(i);
    }
    return oss.str();
}

FileComment Point::point2comment() const
{
    std::ostringstream oss;
    oss<<"*"<<static_cast<int>(code)<<" "<<name<<" "<<coord_read.toString()<<" "<<sigmas_init.toString()<<" *"<<comment;
    return {oss.str(),lineNumber};
}

bool Point::set_point(const Coord &_coord_read, const Coord &_sigmas_init, CR_CODE new_code, bool setParam)
{
    bool ok=true;
    code = new_code;

    switch(code)
    {
        case CR_CODE::FREE:
        case CR_CODE::CR_XYZ:
        case CR_CODE::CR_XY:
        case CR_CODE::CR_Z:
            dimension=3;
            break;
        case CR_CODE::NIV_FREE:
        case CR_CODE::NIV_CR:
            dimension=1;
            break;
        case CR_CODE::PLANI_FREE:
        case CR_CODE::PLANI_CR:
        case CR_CODE::PLANI_FAR_FREE:
        case CR_CODE::PLANI_FAR_CR:
            dimension=2;
            break;
        case CR_CODE::UNDEFINED:
            Project::theInfo()->error(INFO_COR,1,QT_TRANSLATE_NOOP("QObject","Can't set point UNDEFINED."));
            dimension=0;
            return false;
            break;
        case CR_CODE::FORBIDDEN:
            Project::theInfo()->error(INFO_COR,1,QT_TRANSLATE_NOOP("QObject","Can't set point FORBIDDEN."));
            dimension=0;
            return false;
            break;
    }
    coord_read=_coord_read;
    sigmas_init=_sigmas_init;

    if (setParam)
    {
        code_name = all_code_names.at(code); //will be adjusted if fixed params
        params.clear();
        params.reserve(3);
        std::ostringstream oss;
        switch (dimension)
        {
            case 1:
                params.push_back(Parameter::cst);
                params.push_back(Parameter::cst);
                if ((code==CR_CODE::NIV_CR) && (sigmas_init.z()==0))
                {
                    params.push_back(Parameter::cst);
                    code_name.at(3)='Z';
                } else {
                    oss<<name<<"_z";
                    params.emplace_back(oss.str(),&coord_comp[2],1.0,"m",this);
                }
                break;
            case 2:
                if ((code==CR_CODE::PLANI_CR) && (sigmas_init.x()==0))
                {
                    params.push_back(Parameter::cst);
                    code_name.at(1)='X';
                } else {
                    oss<<name<<"_x";
                    params.emplace_back(oss.str(),&coord_comp[0],1.0,"m",this);
                    oss.str("");
                }
                if ((code==CR_CODE::PLANI_CR) && (sigmas_init.y()==0))
                {
                    params.push_back(Parameter::cst);
                    code_name.at(2)='Y';
                } else {
                    oss<<name<<"_y";
                    params.emplace_back(oss.str(),&coord_comp[1],1.0,"m",this);
                }
                params.push_back(Parameter::cst);
                break;
            case 3:
                if ((code==CR_CODE::CR_XYZ||(code==CR_CODE::CR_XY)) && (sigmas_init.x()==0))
                {
                    params.push_back(Parameter::cst);
                    code_name.at(1)='X';
                } else {
                    oss<<name<<"_x";
                    params.emplace_back(oss.str(),&coord_comp[0],1.0,"m",this);
                    oss.str("");
                }
                if ((code==CR_CODE::CR_XYZ||(code==CR_CODE::CR_XY)) && (sigmas_init.y()==0))
                {
                    params.push_back(Parameter::cst);
                    code_name.at(2)='Y';
                } else {
                    oss<<name<<"_y";
                    params.emplace_back(oss.str(),&coord_comp[1],1.0,"m",this);
                    oss.str("");
                }
                if ((code==CR_CODE::CR_XYZ||(code==CR_CODE::CR_Z)) && (sigmas_init.z()==0))
                {
                    params.push_back(Parameter::cst);
                    code_name.at(3)='Z';
                } else {
                    oss<<name<<"_z";
                    params.emplace_back(oss.str(),&coord_comp[2],1.0,"m",this);
                }
                break;
        }
        isXfixed = (params[0].rank==LeastSquares::no_param_index);
        isYfixed = (params[1].rank==LeastSquares::no_param_index);
        isZfixed = (params[2].rank==LeastSquares::no_param_index);
        ok=initCoordinates();
        create_coordinates_constraits();
    }

    if (ok)
    {
        //Project::theInfo()->msg(INFO_CAP,1,QT_TRANSLATE_NOOP("QObject","Set point %s to %s."),name.c_str(),coord_read.toString(3).c_str());
        coord_comp=coord_init_spher;
    }

    return ok;
}

bool Point::isFree() const
{
    return (code==CR_CODE::FREE) || (code==CR_CODE::NIV_FREE) || (code==CR_CODE::PLANI_FREE) || (code==CR_CODE::PLANI_FAR_FREE);
}

bool Point::read_point(std::string line, bool fromNEW)
{

    bool ok=true;
  #ifdef COR_INFO
    std::cout<<"Try to read point: "<<line<<std::endl;
  #endif

    if (fromNEW) //add a fake code if new
    {
        std::ostringstream oss;
        oss<<static_cast<int>(CR_CODE::UNDEFINED)<<" "<<line;
        line=oss.str();
    }

    //test full line structure
    //  "\\s*([*].*|)$": end of line is optional spaces when optional comment till end of line
    static std::vector<std::regex> allFomsRegex={
        std::regex(R"(^\s*-1\s+\S*\s*.*$)"),//forbidden point
        std::regex(R"(^\s*\d+\s+\S+)" SPACE_FLOAT_REGEX "{3}" COMMENT_EOL_REGEX),//no sigma
        std::regex(R"(^\s*\d+\s+\S+)" SPACE_FLOAT_REGEX "{6}" COMMENT_EOL_REGEX),//sigma x y z
        std::regex(R"(^\s*\d+\s+\S+)" SPACE_FLOAT_REGEX "{8}" COMMENT_EOL_REGEX),//sigma x y z eta xi
        std::regex(R"(^\s*\d+\s+\S+)" SPACE_FLOAT_REGEX "{5}" COMMENT_EOL_REGEX),//eta xi
    };
    bool formOk=false;
    for (auto &e: allFomsRegex)
        if (std::regex_search(line, e))
        {
            formOk=true;
            break;
        }
    if (!formOk)
    {
        Project::theInfo()->warning(INFO_COR,getFileDepth(),
                                    QT_TRANSLATE_NOOP("QObject","In %s:%d: Wrong format in: %s"),
                                    getFileName().c_str(),lineNumber,line.c_str());
        return false;
    }

    //get final comment
    std::regex regex_line("^(.*)\\*([^\r]*)(\r)?$");//is there a * somewhere? (remove ending \r if using win file on unix)
    std::smatch what;
    if(std::regex_match(line, what, regex_line))
    {
        comment=what[2];
        line=what[1];
    }

    Coord _coord_read;
    Coord _sigmas_init;
    bool eta_xi_given=false;//if 2 more values
    bool sigmas_given=false;//if 3 more values
    tdouble _eta = NAN;
    tdouble _xi = NAN;

    std::istringstream iss(line);

    {
        int code_int=0; // temp
        if (!(iss >> code_int))
        {
            if (!fromNEW)
            {
                Project::theInfo()->warning(INFO_COR,getFileDepth(),
                                            QT_TRANSLATE_NOOP("QObject","In %s:%d: Can't convert point code."),
                                            getFileName().c_str(),lineNumber);
                ok=false;
            }
        }
        code=static_cast<CR_CODE>(code_int);
    }

    if (code>=CR_CODE::UNDEFINED)
    {
        Project::theInfo()->warning(INFO_COR,getFileDepth(),
                                    QT_TRANSLATE_NOOP("QObject","In %s:%d: Unknown point code %d."),
                                    getFileName().c_str(),lineNumber,static_cast<int>(code));
        ok=false;
    }

    std::string name_newpt;
    if (!(iss >> name_newpt))
    {
        Project::theInfo()->warning(INFO_COR,getFileDepth(),
                                    QT_TRANSLATE_NOOP("QObject","In %s:%d: Can't convert point name."),
                                    getFileName().c_str(),lineNumber);
        ok=false;
    }

    if (name_newpt[0]=='@')
    {
        Project::theInfo()->warning(INFO_COR,getFileDepth(),
                                    QT_TRANSLATE_NOOP("QObject","In %s:%d: Point name starting with "
                                                                "'@' is forbidden."),
                                    getFileName().c_str(),lineNumber);
        ok=false;
    }

    for (auto & point : Project::theone()->forbidden_points)
        if (point == name_newpt)
        {
            Project::theInfo()->warning(INFO_COR,getFileDepth(),
                                        QT_TRANSLATE_NOOP("QObject","In %s:%d: There is already a forbidden "
                                                                    "point named \"%s\"."),
                                        getFileName().c_str(),lineNumber,name_newpt.c_str());
            return false;
        }

    if (code<=CR_CODE::FORBIDDEN) //forbidden point
    {
        Project::theone()->forbidden_points.push_back(name_newpt);
        return false;
    }

    if (Project::theone()->getPoint(name_newpt))
    {
        Project::theInfo()->warning(INFO_COR,getFileDepth(),
                                    QT_TRANSLATE_NOOP("QObject","In %s:%d: There is already a point named \"%s\"."),
                                    getFileName().c_str(),lineNumber,name_newpt.c_str());
        ok=false;
    }

    name = name_newpt;
    if (!(iss >> _coord_read[0]))
    {
        Project::theInfo()->warning(INFO_COR,getFileDepth(),
                                    QT_TRANSLATE_NOOP("QObject","In %s:%d: Can't convert point x."),
                                    getFileName().c_str(),lineNumber);
        ok=false;
    }

    if (!(iss >> _coord_read[1]))
    {
        Project::theInfo()->warning(INFO_COR,getFileDepth(),
                                    QT_TRANSLATE_NOOP("QObject","In %s:%d: Can't convert point y."),
                                    getFileName().c_str(),lineNumber);
        ok=false;
    }

    if (!(iss >> _coord_read[2]))
    {
        if (!fromNEW)
        {
            Project::theInfo()->warning(INFO_COR,getFileDepth(),
                                        QT_TRANSLATE_NOOP("QObject","In %s:%d: Can't convert point z."),
                                        getFileName().c_str(),lineNumber);
            ok=false;
        }
    }

    if (iss >> _sigmas_init[0])
    {
        if (!(iss >> _sigmas_init[1]))
        {
            if (!fromNEW)
            {
                Project::theInfo()->warning(INFO_COR,getFileDepth(),
                                            QT_TRANSLATE_NOOP("QObject","In %s:%d: Can't convert point sigma_y."),
                                            getFileName().c_str(),lineNumber);
                ok=false;
            }
        }else if (!(iss >> _sigmas_init[2]))
        {
            eta_xi_given=true;
        }else if (ok)
            sigmas_given=true;
    }

    if (iss >> _eta)
    {
        if (!(iss >> _xi))
        {
            Project::theInfo()->warning(INFO_COR,getFileDepth(),
                                        QT_TRANSLATE_NOOP("QObject","In %s:%d: Can't convert point xi."),
                                        getFileName().c_str(),lineNumber);
            ok=false;
        }else{
            eta_xi_given=true;
        }
    }

    if (eta_xi_given && !sigmas_given)
    {
        _eta = _sigmas_init[0];
        _xi = _sigmas_init[1];
        _sigmas_init[0] = 0;
        _sigmas_init[1] = 0;
    }

    if (eta_xi_given)
    {
        tdouble unitFactorARCSEC=toRad(1.0,ANGLE_UNIT::ARCSEC);
        dev_eta = _eta*unitFactorARCSEC;
        dev_xi = _xi*unitFactorARCSEC;
        dev_azim = atan2(dev_eta,dev_xi);
        dev_norm = sqrt(dev_xi*dev_xi+dev_eta*dev_eta); //approx for small angles. exact is: acos(cos(dev_xi)*cos(dev_eta))
    }

    coord_read=_coord_read;
    sigmas_init=_sigmas_init;
    //check if sigma given when constraints on point
    if (ok&& (!sigmas_given))
    {

        switch(code)
        {
            case CR_CODE::CR_XYZ:
            case CR_CODE::CR_XY:
            case CR_CODE::CR_Z:
            case CR_CODE::NIV_CR:
            case CR_CODE::PLANI_CR:
            case CR_CODE::PLANI_FAR_CR:
                Project::theInfo()->warning(INFO_COR,getFileDepth(),
                                            QT_TRANSLATE_NOOP("QObject","In %s:%d: Constrained point without sigmas is not allowed."),
                                            getFileName().c_str(),lineNumber);
                ok=false;
                break;
            default:
                break;
        }
    }
    if (!ok)
        Project::theInfo()->warning(INFO_COR,getFileDepth(),
                                    QT_TRANSLATE_NOOP("QObject","In %s:%d: Error reading line."),
                                    getFileName().c_str(),lineNumber);
    return ok;
}

bool Point::initCoordinates()
{
    if (!Project::theone()->projection.georefToSpherical(coord_read,coord_init_spher))
        return false;

    if ((fabs(coord_init_spher.x())>MAX_HZ_SIZE_M)||(fabs(coord_init_spher.y())>MAX_HZ_SIZE_M))
    {
        Project::theInfo()->error(INFO_COR,getFileDepth(),
                                  QT_TRANSLATE_NOOP("QObject","%s is too far from local center."),
                                  name.c_str());
        return false;
    }
    return true;
}

bool Point::set_posteriori_variance(const MatX &Qxx)
{
    for (int i=0;i<3;i++)
    {
        if (params.at(i).rank==LeastSquares::no_param_index)
            continue;
        for (int j=0;j<3;j++)
        {
            if (params.at(j).rank==LeastSquares::no_param_index)
                continue;
            ellipsoid.set(i,j,Qxx(params.at(i).rank,params.at(j).rank)*WEIGHT_FACTOR);
        }
    }
    //std::cout<<"For point "<<name<<":\n";

    ellipsoid.compute_eigenvalues(Project::theone()->lsquares.sigma_0);
    for (int i=0;i<3;i++)
    {
        if (!std::isfinite(ellipsoid.get_ellipsAxe(i)))
            return false;
        if (!std::isfinite(ellipsoid.get_variance(i)))
            return false;
    }
    return true;
}

void Point::create_coordinates_constraits()
{
    obsX = obsY = obsZ = nullptr;
    if ((code==CR_CODE::FREE)||(code==CR_CODE::NIV_FREE)||(code==CR_CODE::PLANI_FREE)||(code==CR_CODE::PLANI_FAR_FREE)) return;

    //remove previous coord constraints
    auto *station = getLastStation<Station_Simple>(file->get_fileDepth());
    std::vector<Obs*> obs_coord_constr;
    for (auto & obs : station->observations)
        if ((obs.code==OBS_CODE::COORD_X)||(obs.code==OBS_CODE::COORD_Y)||(obs.code==OBS_CODE::COORD_Z))
            obs_coord_constr.push_back(&obs);
    station->removeObs(obs_coord_constr);

    if ((code==CR_CODE::CR_XYZ)||(code==CR_CODE::CR_XY)||(code==CR_CODE::PLANI_CR)||(code==CR_CODE::PLANI_FAR_CR))
    {
        if (params.at(0).rank!=LeastSquares::no_param_index)
            obsX = station->create_coordinate_constrait_obs(OBS_CODE::COORD_X,coord_init_spher.x(),sigmas_init.x(),lineNumber,file,comment);
        if (params.at(1).rank!=LeastSquares::no_param_index)
            obsY = station->create_coordinate_constrait_obs(OBS_CODE::COORD_Y,coord_init_spher.y(),sigmas_init.y(),lineNumber,file,comment);
    }
    if ((code==CR_CODE::CR_XYZ)||(code==CR_CODE::CR_Z)||(code==CR_CODE::NIV_CR))
    {
        if (params.at(2).rank!=LeastSquares::no_param_index)
            obsZ = station->create_coordinate_constrait_obs(OBS_CODE::COORD_Z,coord_init_spher.z(),sigmas_init.z(),lineNumber,file,comment);
    }

    if (station->observations.empty())
        Project::theone()->deleteStation(station);
}


std::vector<Point *> Point::pointsMeasured() const
{
    std::vector<Point *> measuredPts;
    for (const auto & station : stations_)
    {
        for (const auto & obs : station->observations)
        {
            bool alreadyIn=false;
            for (const auto & measured : measuredPts)
            {
                if (measured==obs.to)
                {
                    alreadyIn=true;
                    break;
                }
            }
            if (!alreadyIn)
                measuredPts.push_back(obs.to);
        }
    }
    return measuredPts;
}



Json::Value Point::toJson(FileRefJson& fileRef) const
{
    Json::Value pt;
    pt["code"]=static_cast<int>(code);
    Json::Value jsparams;
    for (const auto &param:params)
    {
        if (param.rank==LeastSquares::no_param_index)
            continue;
        jsparams[param.name] = param.toJson();
    }
    pt["code_name"]=code_name;
    pt["params"]=jsparams;
    pt["number"]=pointNumber;
    pt["file_id"]=fileRef.getNumber(file);
    pt["comment"]=comment;
    pt["dimension"]=dimension;
    pt["nbr_active_obs"]=nbActiveObs;
    pt["coord_read"]=coord_read.toJson();
    pt["sigmas_init"]=sigmas_init.toJson();
    if (Project::theone()->compensationDone)
    {
        pt["coord_compensated_georef"]=coord_compensated_georef.toJson();
        if (Project::theone()->projection.type==PROJ_TYPE::PJ_GEO)
        {
            Coord latlong;
            Project::theone()->projection.georefToLatLong(coord_compensated_georef,latlong);
            pt["coord_compensated_latlong"]=latlong.toJson();
        }
        pt["shift_from_read"]=shift.toJson();
    }
    if (Project::theone()->projection.type==PROJ_TYPE::PJ_GEO)
    {
        Coord latlong;
        Project::theone()->projection.georefToLatLong(coord_read,latlong);
        pt["coord_init_latlong"]=latlong.toJson();
    }

    Coord coord_compensated_cartesian;
    Project::theone()->projection.sphericalToGlobalCartesian(coord_comp,coord_compensated_cartesian);
    pt["coord_compensated_cartesian"]=coord_compensated_cartesian.toJson();
    pt["ellips"]=ellipsoid.toJson(Project::theone()->lsquares.sigma_0);

    pt["stations"]=Json::arrayValue;
    for (const auto & station: stations_)
        pt["stations"].append(station->toJson(fileRef));

    if (Project::theone()->config.compute_type==COMPUTE_TYPE::type_monte_carlo)
    {
        pt["MC_shift_max"]=MC_shift_max.toJson();
        pt["MC_shift_sq_average"]=MC_shift_sq_average.toJson();
    }
    pt["dev_eta"]=static_cast<double>(dev_eta);
    pt["dev_xi"]=static_cast<double>(dev_xi);

    return pt;
}

/*
  a new Monte-Carlo iteration is finished, update data comparing
  coordInitLocal and coord_comp
  */
void Point::update_MonteCarlo()
{
    //std::cout<<"update_MonteCarlo\n";
    tdouble dx=fabs(coord_comp.x()-coord_init_spher.x());
    tdouble dy=fabs(coord_comp.y()-coord_init_spher.y());
    tdouble dz=fabs(coord_comp.z()-coord_init_spher.z());

    if (dx>MC_shift_max.x()) MC_shift_max.setx(dx);
    MC_shift_sq_average.setx(MC_shift_sq_average.x()+dx*dx);

    if (dy>MC_shift_max.y()) MC_shift_max.sety(dy);
    MC_shift_sq_average.sety(MC_shift_sq_average.y()+dy*dy);

    if (dz>MC_shift_max.z()) MC_shift_max.setz(dz);
    MC_shift_sq_average.setz(MC_shift_sq_average.z()+dz*dz);
#ifdef COR_INFO
    std::cout<<"update Monte-Carlo: "<<name<<" "<<dx<<" "<<dy<<" "<<dz<<"\n";
#endif
}

//when last iteration of monte carlo simulation is finished
void Point::finish_MonteCarlo(int niter)
{
    MC_shift_sq_average.setx( sqrt(MC_shift_sq_average.x()/niter) );
    MC_shift_sq_average.sety( sqrt(MC_shift_sq_average.y()/niter) );
    MC_shift_sq_average.setz( sqrt(MC_shift_sq_average.z()/niter) );
}
