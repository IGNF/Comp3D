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

#include "station_eq.h"
#include "point.h"
#include "project.h"
#include <cmath>

Station_Eq::Station_Eq(Point *origin) : Station(nullptr, STATION_TYPE::ST_EQ), eq_type(STATION_CODE::ST_UNDEF), val(NAN), file(nullptr)
{
    if (origin != nullptr)
        std::cout<<"WARNING! StationEq called with a non null origin point!"<<std::endl;
}


std::string Station_Eq::typeStr() const
{
    return "eq";
}


bool Station_Eq::read_obs(std::string line,
                            int line_number, DataFile *_file, const std::string &current_absolute_path, const std::string &comment)
{
    (void)comment;
    bool ok=true;
    std::string from_name,filename;
    int code_int=-1;
    STATION_CODE code = STATION_CODE::ST_UNDEF;

  #ifdef INFO_EQ
    std::cout<<"Try to read eq obs: "<<line<<std::endl;
  #endif

    std::istringstream iss(line);
    if (!(iss >> code_int))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert observation code."),line_number,line.c_str());
        ok=false;
    }
    code=static_cast<STATION_CODE>(code_int);
    if ((code<STATION_CODE::OBS_EQ_DH)||(code>STATION_CODE::OBS_EQ_DIST))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Line code is incorrect."),line_number,line.c_str());
        ok=false;
    }
    eq_type = code;

    if (!(iss >> filename))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert filename."),line_number,line.c_str());
        ok=false;
    }
    if (filename.at(0)!='@')
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => \'@\' needed before sub file name."),line_number,line.c_str());
        ok=false;
    }
    filename.erase(0,1); //remove first character (@)
    std::replace(filename.begin(),filename.end(),'\\','/');

    if (ok)
    {
        file = EqFile::create(filename,this,current_absolute_path,line_number,_file->get_fileDepth()+1);
        ok=file->read();
        if (observations.empty())
        {
            Project::theInfo()->warning(INFO_OBS,getFile().get_fileDepth(),QT_TRANSLATE_NOOP("QObject","Warning: 0 observation found in %s."),filename.c_str());
            ok=false;
        }

        //create param
        std::ostringstream oss;
        oss<<filename<<"_";
        switch (eq_type) {
            case STATION_CODE::OBS_EQ_DH:
                oss<<"dh"<<"_"<<num;
                break;
            case STATION_CODE::OBS_EQ_DIST:
                oss<<"dist"<<"_"<<num;
                break;
            default:
                oss<<"??"<<"_"<<num;
                break;
        }
        params.clear();
        params.emplace_back(oss.str(),&val,1.0,"m",origin());
    } else {
        file = nullptr;
    }

    return ok;
}


bool Station_Eq::set_obs(LeastSquares *lsquares,
                           bool initialResidual, bool internalConstraints)
{
    bool ok=true;
    for (auto &obs:observations)
        ok = obs.setEquation(lsquares,initialResidual,internalConstraints) && ok;

    return ok;
}

bool Station_Eq::initialize(bool verbose)
{
    (void)verbose;
    mInitOk=false;

    for (auto & obs: observations)
    {
        if (obs.from->isInit() && obs.to->isInit())
        {
            tdouble radius=Project::theone()->projection.radius;
            tdouble Xa=obs.from->coord_comp.x();
            tdouble Ya=obs.from->coord_comp.y();
            tdouble Za=obs.from->coord_comp.z() + radius + obs.instrumentHeight;
            tdouble Xb=obs.to->coord_comp.x();
            tdouble Yb=obs.to->coord_comp.y();
            tdouble Zb=obs.to->coord_comp.z() + radius + obs.targetHeight;
            tdouble C = NAN, S = NAN;
            arc(Xa,Ya,Xb,Yb,C,S,radius);
            tdouble D=sqrt(sqr(Za-Zb)+4*Za*Zb*sqr(sin(asin(S)/2)));
            switch (eq_type) {
                case STATION_CODE::OBS_EQ_DH:
                    val = Zb-Za;
                    mInitOk=true;
                    if (verbose)
                        std::cout<<"Init Station_Eq from dh: "<<val<<"\n";
                    break;
                case STATION_CODE::OBS_EQ_DIST:
                    val = D;
                    mInitOk=true;
                    if (verbose)
                        std::cout<<"Init Station_Eq from dist: "<<val<<"\n";
                    break;
                default:
                    Project::theInfo()->warning(INFO_OBS,file?file->get_fileDepth():1,
                                                QT_TRANSLATE_NOOP("QObject","Obs type %d not usable in Station_Eq."),
                                                static_cast<int>(eq_type));
                    break;
            }
        }
        if (mInitOk) return true;
    }
    return false;
}


Json::Value Station_Eq::toJson(FileRefJson& fileRef) const
{
    Json::Value val = Station::toJson(fileRef);

    val["file_id"]=fileRef.getNumber(file.get());
    val["eq_type"]=static_cast<int>(eq_type);

    return val;
}


bool Station_Eq::isInternal(Obs* /*obs*/)//if the obs is compatible with internal constraints
{
    return true;
}

bool Station_Eq::isOnlyLeveling(Obs* obs)//for internal constraints
{
    return (obs->code == OBS_CODE::EQ_DH);
}

bool Station_Eq::isHz(Obs* /*obs*/)//for internal constraints
{
    return false;
}

bool Station_Eq::isDistance(Obs* obs)//for internal constraints
{
    return (obs->code == OBS_CODE::EQ_DH) || (obs->code == OBS_CODE::EQ_DIST);
}

bool Station_Eq::isBubbuled(Obs* obs)//for internal constraints
{
    return (obs->code == OBS_CODE::EQ_DH);
}

bool Station_Eq::useVertDeflection(Obs* /*obs*/)//to check vertical deflection consistancy
{
    return false;
}

int Station_Eq::numberOfBasicObs(Obs* /*obs*/)//number of lines in the matrix
{
    return 1;
}
