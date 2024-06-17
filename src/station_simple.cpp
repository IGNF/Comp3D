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

#include "station_simple.h"

#include "compile.h"
#include "mathtools.h"
#include "project.h"
#include <iostream>
#include <regex>
#include <sstream>

Station_Simple::Station_Simple(Point *origin):Station(origin, STATION_TYPE::ST_SIMPLE)
{
}

Obs* Station_Simple::create_coordinate_constrait_obs(OBS_CODE _code,tdouble _value,tdouble _sigma,int line,
                                                     DataFile *_file,const std::string& _comment)
{
    if (fabs(_sigma)<MINIMAL_SIGMA)
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","At line %d: => Constraint sigma "
                                                                "on point %s is too small."),
                                    line,origin()->name.c_str());
        return nullptr;
    }
    observations.emplace_back(origin(),origin(),this,_code,(_sigma>0),_value,_sigma,0,0,0,line,1.0,"m",_file,_comment);
    return &observations.back();
}

std::string Station_Simple::typeStr() const
{
    return "simple";
}

bool Station_Simple::isInternal(Obs* obs)//if the obs is compatible with internal constraints
{
    if ((obs->code==OBS_CODE::COORD_X || obs->code==OBS_CODE::COORD_Y) && (obs->from->code==CR_CODE::PLANI_FAR_CR))
        return true;//we keep X and Y for far 2d points when internal constraints

    return obs->code!=OBS_CODE::COORD_X && obs->code!=OBS_CODE::COORD_Y && obs->code!=OBS_CODE::COORD_Z && obs->code!=OBS_CODE::AZIMUTH;
}

bool Station_Simple::isOnlyLeveling(Obs* obs)//for internal constraints
{
    return obs->code==OBS_CODE::DH;
}

bool Station_Simple::isHz(Obs* /*obs*/)//for internal constraints
{
    return false; //azimuths are not internal
}

bool Station_Simple::isDistance(Obs* obs)//for internal constraints
{
    if ((obs->code==OBS_CODE::DX_SPHER)||(obs->code==OBS_CODE::DY_SPHER)
            ||(obs->code==OBS_CODE::DIST1)||(obs->code==OBS_CODE::DIST_HZ0)||(obs->code==OBS_CODE::DIST)||(obs->code==OBS_CODE::DH))
        return fabs(obs->value) > CAP_CLOSE;
    return false;
}

bool Station_Simple::isBubbuled(Obs* obs)//for internal constraints
{
    return obs->code!=OBS_CODE::COORD_X && obs->code!=OBS_CODE::COORD_Y && obs->code!=OBS_CODE::COORD_Z && obs->code!=OBS_CODE::DIST1 && obs->code!=OBS_CODE::DIST;
}

bool Station_Simple::useVertDeflection(Obs* obs)//to check vertical deflection consistancy
{
    return isBubbuled(obs) || (fabs(obs->instrumentHeight)>CAP_CLOSE);
}


int Station_Simple::numberOfBasicObs(Obs* /*obs*/)//number of lines in the matrix
{
    return 1;
}

bool Station_Simple::initialize(bool /*verbose*/)
{
    if (!origin()->isInit())
    {
        mInitOk=false;
        return false;
    }
    mInitOk=true;
    return true;
}

bool Station_Simple::read_obs(std::string line, int line_number, DataFile *_file,
                              const std::string& /*current_absolute_path*/, const std::string &comment)
{
    bool ok=true;
    bool active=true;
    std::string from_name,to_name;
    Point *from=nullptr;
    Point *to=nullptr;
    OBS_CODE code=OBS_CODE::UNKNOWN;
    tdouble value_original=0,sigma_abs_original=0,sigma_rel_original=0;//before angle conversion
    tdouble instrument_height=0,target_height=0;

    //test full line structure
    static std::vector<std::regex> allFomsRegex={
        std::regex(R"(^\s*)" INT_REGEX R"(\s+\S+\s+\S+)" SPACE_FLOAT_REGEX "{2,3}" COMMENT_EOL_REGEX),    //sigma_abs or //sigma_abs sigma_rel
        std::regex(R"(^\s*)" INT_REGEX R"(\s+\S+\s+\S+)" SPACE_FLOAT_REGEX "{5}"   COMMENT_EOL_REGEX)     //sigma_abs sigma_rel inst_h targ_h
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
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","In %s:%d: Wrong format in: %s"),
                                    _file->get_name().c_str(),line_number,line.c_str());
        return false;
    }

  #ifdef OBS_INFO
    std::cout<<"Try to read obs: "<<line<<std::endl;
  #endif
    std::istringstream iss(line);
    int code_int = 0;
    if (!(iss >> code_int))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert observation code."),
                                    line_number,line.c_str());
        ok=false;
    }
    if (code_int<0)
    {
        active=false;
        code_int=-code_int;
    }
    code=static_cast<OBS_CODE>(code_int);

    if (!(iss >> from_name))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert from point name."),
                                    line_number,line.c_str());
        ok=false;
    }
    //find a point with that name in project
    from=Project::theone()->getPoint(from_name,true);

    if (!(iss >> to_name))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert to point name."),
                                    line_number,line.c_str());
        ok=false;
    }
    //find a point with that name in project
    to=Project::theone()->getPoint(to_name,true);

    if (from==to)
    {
        Project::theInfo()->error(INFO_OBS,_file->get_fileDepth()+1,
                                  QT_TRANSLATE_NOOP("QObject","At line %d: %s => Observation between %s and %s."),
                                  line_number,line.c_str(),from->name.c_str(),to->name.c_str());
        ok=false;
    }

    if (!(iss >> value_original))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert observation value."),
                                    line_number,line.c_str());
        ok=false;
    }

    if (!(iss >> sigma_abs_original))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert observation sigma_abs."),
                                    line_number,line.c_str());
        ok=false;
    }

    if (fabs(sigma_abs_original)<MINIMAL_SIGMA)
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => Observation sigma_abs is too small."),
                                    line_number,line.c_str());
        ok=false;
    }

    if ((iss >> sigma_rel_original))
    {
        if ((iss >> instrument_height))
        {
            if (!(iss >> target_height))
            {
                Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                            QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert target_height."),
                                            line_number,line.c_str());
                ok=false;
            }
        }
    }

    if (sigma_rel_original<0.0)
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => Relative sigma can't be negative!"),
                                    line_number,line.c_str());
        ok=false;
    }

    if (code==OBS_CODE::CENTER_META)
    {
        //for code CENTER_META, we create 2 basic obs (DE and DN), with val=0, sigmaX=sigma_abs and sigmaY=sigma_rel
        //if sigma_rel==0, let's say that sigmaX=sigmaY=sigma_abs
        value_original=0.0;
        if (fabs(sigma_rel_original)<MINIMAL_SIGMA)
        {
            sigma_rel_original=sigma_abs_original;
        }
    }else active=active&&(sigma_abs_original>0);//don't change active for CENTER_META, we will use it for two obs

    if ((Project::theone()->config.compute_type==COMPUTE_TYPE::type_compensation)
            &&((code==OBS_CODE::DIST1)||(code==OBS_CODE::DIST_HZ0)||(code==OBS_CODE::DIST))&&(active)&&(value_original<0.0))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => Distance can't be negative!"),
                                    line_number,line.c_str());
        ok=false;
    }

    //angle conversions
    tdouble unitFactor=1.0;
    std::string unitName="m";
    if ((code==OBS_CODE::AZIMUTH)||(code==OBS_CODE::ZEN))
    {
        unitFactor=toRad(1.0,Project::theone()->config.filesUnit);
        unitName=Project::theone()->unitName;
        if (fabs(value_original*unitFactor)>2*PI)
        {
            Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                        QT_TRANSLATE_NOOP("QObject","At line %d: %s => Angle value out of range!"),
                                        line_number,line.c_str());
            ok=false;
        }
    }

    if ((code==OBS_CODE::ZEN)&&(from->dimension==2))
    {
        //make no error for zen of 2d-points, it is automatically recorded by the instrument
        std::cout<<"At line "<<line_number<<": "<<line<<" => Obs removed due to incompatible point dimension.\n";
        return ok;//do not add this obs to the station
    }

    if (ok)
    {
        if (code!=OBS_CODE::CENTER_META)
        {
            observations.emplace_back(from,to,this,code,active,value_original,sigma_abs_original,
                                      sigma_rel_original,instrument_height,target_height,
                                      line_number,unitFactor,unitName,_file,comment);
            if (!observations.back().checkPointsDimension())
                observations.pop_back();
        }else{
            observations.emplace_back(from,to,this,OBS_CODE::DX_SPHER,(active&&(sigma_abs_original>0)),
                                      value_original,sigma_abs_original,
                                      0.0,instrument_height,target_height,
                                      line_number,unitFactor,unitName,_file,comment);
            if (!observations.back().checkPointsDimension())
                observations.pop_back();
            observations.emplace_back(from,to,this,OBS_CODE::DY_SPHER,(active&&(sigma_rel_original>0)),
                                      value_original,sigma_rel_original,
                                      0.0,instrument_height,target_height,
                                      line_number,unitFactor,unitName,_file,comment);
            if (!observations.back().checkPointsDimension())
                observations.pop_back();
        }
    }
    return ok;
}


//returns false if problem (e.g. impossible to compute value)
bool Station_Simple::set_obs(LeastSquares *lsquares, bool initialResidual, bool internalConstraints)
{
    bool ok=true;
    for (auto &obs:observations)
        ok = obs.setEquation(lsquares,initialResidual,internalConstraints) && ok;

    return ok;
}
