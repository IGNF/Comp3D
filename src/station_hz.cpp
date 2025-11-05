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

#include "station_hz.h"
#include "compile.h"
#include "mathtools.h"
#include "project.h"
#include <iostream>
#include <regex>
#include <sstream>

Station_Hz::Station_Hz(Point *origin):Station(origin, STATION_TYPE::ST_HZ),g0(NAN)
{
    std::ostringstream oss;
    oss<<mOrigin->name<<"_G0"<<"_"<<num;
    params.clear();
    params.emplace_back(oss.str(),&g0,toRad(1.0,Project::theone()->config.filesUnit),Project::theone()->unitName,mOrigin);
}

std::string Station_Hz::typeStr() const
{
    return "hz_unknown";
}

bool Station_Hz::isInternal(Obs* /*obs*/)//if the obs is compatible with internal constraints
{
    return true;
}

bool Station_Hz::isOnlyLeveling(Obs* /*obs*/)//for internal constraints
{
    return false;
}

bool Station_Hz::isHz(Obs* /*obs*/)//for internal constraints
{
    return false;
}

bool Station_Hz::isDistance(Obs* /*obs*/)//for internal constraints
{
    return false;
}

bool Station_Hz::isBubbuled(Obs* /*obs*/)//for internal constraints
{
    return true;
}

bool Station_Hz::useVertDeflection(Obs* /*obs*/)//to check vertical deflection consistancy
{
    return true;
}


int Station_Hz::numberOfBasicObs(Obs* /*obs*/)//number of lines in the matrix
{
    return 1;
}

// TODO(jmm): use return to explain there is a missing "7" code?
//works even if there is no "7" code.
//only the first obs is used to compute g0 in order to avoid modulo troubles
bool Station_Hz::initialize(bool verbose)
{
    if (!mOrigin->isInit())
    {
        mInitOk=false;
        return false;
    }

    //look for first obs on an init point
    Obs * first_obs=nullptr;
    for (auto & obs : observations)
        if (obs.to->dimension>=2)
        {
            first_obs=&obs;
            break;
        }

    if (!first_obs)
    {
        //try to find g0 with an azimuth
        for (auto & station : mOrigin->stations_)
            for (auto & subobs : station->observations)
            {
                if (subobs.code==OBS_CODE::AZIMUTH)
                {
                    Point* ptAz=subobs.to;
                    for (auto & obs : observations)
                    {
                        if (obs.to==ptAz)
                        {
                            g0= subobs.value - obs.value;
                            mInitOk=true;
                            return true;
                        }
                    }
                }

            }
        if (verbose)
            Project::theInfo()->warning(INFO_OBS,1,
                                        QT_TRANSLATE_NOOP("QObject","Error: impossible to initialize hz station on point %s."),
                                        origin()->name.c_str());
        mInitOk=false;
        return false;
    }
    //Project::theInfo()->msg(INFO_OBS,1,QT_TRANSLATE_NOOP("QObject","Initialize hz station on point %s opening on %s."),
    //                        from->name.c_str(),first_obs->to->name.c_str());

    //automatically set first obs to HZ_REF
    //clean other HZ_REF (may have been set during CAP initialization)
    for (auto & obs : observations)
        obs.code=OBS_CODE::HZ_ANG;
    first_obs->code=OBS_CODE::HZ_REF;

    tdouble radius=Project::theone()->projection.radius;
    tdouble Xa=mOrigin->coord_comp.x();
    tdouble Ya=mOrigin->coord_comp.y();
    tdouble Xb=first_obs->to->coord_comp.x();
    tdouble Yb=first_obs->to->coord_comp.y();

    tdouble C=NAN,S=NAN;
    arc(Xa,Ya,Xb,Yb,C,S,radius);
    g0= azimuth(Xa,Ya,Xb,Yb,C,radius) - first_obs->value;

    //vertical deflection correction
    if (Project::theone()->use_vertical_deflection)
    {
        first_obs->computeInfos(true);//to update azim and zen
        tdouble vertical_deflection = mOrigin->dev_norm*cos(first_obs->getAzim()-mOrigin->dev_azim+PI/2);
        g0-=vertical_deflection/tan(first_obs->getZen());
    }
    normalizeAngle(g0);
#ifdef OBS_INFO
    std::cout<<first_obs->toString()<<" initial g0: "<<g0<<std::endl;
#endif
    mInitOk=true;
    return true;
}

bool Station_Hz::read_obs(std::string line, int line_number, DataFile *_file,
                          const std::string & /*current_absolute_path*/,const std::string &comment)
{
    bool ok=true;
    bool active=true;

    std::string from_name,to_name;
    Point *from=nullptr;
    Point *to=nullptr;
    OBS_CODE code = OBS_CODE::UNKNOWN;
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
                                    _file->get_name().c_str(),line_number,
                                    (line+(comment.empty()?"":"*"+comment)).c_str());
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
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert point name."),
                                    line_number,line.c_str());
        ok=false;
    }
    //find a point with that name in project
    from=Project::theone()->getPoint(from_name,true);

    if (!(iss >> to_name))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert point name."),
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
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert absolute sigma."),
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
        if (iss >> instrument_height)
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

    active=active&&(sigma_abs_original>0);

    //angle conversions
    tdouble unitFactor=1.0;
    std::string unitName="m";
    if ((code==OBS_CODE::HZ_ANG)||(code==OBS_CODE::HZ_REF))
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

    if (ok)
    {
        observations.emplace_back(from,to,this,code,active,value_original,sigma_abs_original,
                                  sigma_rel_original,instrument_height,target_height,
                                  line_number,unitFactor,unitName,_file,comment);
        if (!observations.back().checkPointsDimension())
            observations.pop_back();
    }

    return ok;
}

//returns false if problem (e.g. impossible to compute value)
bool Station_Hz::set_obs(LeastSquares *lsquares, bool initialResidual, bool internalConstraints)
{
    if (!mInitOk)
    {
        Project::theInfo()->warning(INFO_OBS,1,
                                    QT_TRANSLATE_NOOP("QObject","G0 from %s not init, it won't be used."),
                                    mOrigin->name.c_str());
        return false;
    }

    bool ok=true;
    for (auto &obs:observations)
        ok = ok && obs.setEquation(lsquares,initialResidual,internalConstraints);

    return ok;
}
