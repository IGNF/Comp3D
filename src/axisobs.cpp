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

#include "axisobs.h"
#include "mathtools.h"
#include "project.h"
#include "station_axis.h"

AxisObs::AxisObs(Station_Axis *from):
    obsR(nullptr),obsT(nullptr),mStation(from),
    mTargetNum("?"),mPosNum("?"),mSigmaR(0),mSigmaT(0),mTarget(nullptr),mPt(nullptr)
{
#ifdef OBS_INFO
    std::cout<<"Create new AxisObs from "<<mStation->origin()->name<<std::endl;
#endif
}

std::string AxisObs::toString() const
{
    std::ostringstream oss;
    //oss<<this<<" ";
    oss<<"Obs axis target "<<mTarget->getTargetNum()<<" pos "<<getPosNum()<<":\n  "<<obsR->toString()<<"\n  "<<obsT->toString()<<"\n ";
    return oss.str();
}

Json::Value AxisObs::toJson() const
{
    Json::Value val;
    val["num_obs_r"]=obsR->getObsNumber();
    val["num_obs_t"]=obsT->getObsNumber();
    return val;
}

bool AxisObs::read_obs(const std::string& line, int line_number, DataFile *_file, const std::string& comment)
{
    bool ok=true;
    std::string to_name;
    bool active_r=true;
    bool active_t=true;

  #ifdef OBS_INFO
    std::cout<<"Try to read obs: "<<line<<std::endl;
  #endif
    std::istringstream iss(line);

    if ((!(iss >> mTargetNum))||(!(iss >> mPosNum)))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => "
                                                                "Can't convert observation value."),
                                    line_number,line.c_str());
        ok=false;
    }

    if (!(iss >> to_name))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => "
                                                                "Can't convert to point name."),
                                    line_number,line.c_str());
        ok=false;
    }
    //find a point with that name in project
    if (!(mPt=Project::theone()->getPoint(to_name,true)))
    {
        //Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
        //                            QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't find a point named \"%s\"."),
        //                            line_number,line.c_str(),from_name.c_str());
        ok=false;
    }
    if (mStation->origin()==mPt)
    {
        Project::theInfo()->error(INFO_OBS,_file->get_fileDepth()+1,
                                  QT_TRANSLATE_NOOP("QObject","At line %d: %s => "
                                                              "Observation between %s and %s."),
                                  line_number,line.c_str(),mStation->origin()->name.c_str(),
                                  mPt->name.c_str());
        ok=false;
    }

    if (!(iss >> mSigmaR))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => "
                                                                "Can't convert observation sigma_r."),
                                    line_number,line.c_str());
        ok=false;
    }

    if (!(iss >> mSigmaT))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => "
                                                                "Can't convert observation sigma_t."),
                                    line_number,line.c_str());
        ok=false;
    }

    active_r=(mSigmaR>0);
    active_t=(mSigmaT>0);

    if ((fabs(mSigmaR)<MINIMAL_SIGMA)||(fabs(mSigmaT)<MINIMAL_SIGMA))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => "
                                                                "Observation sigma_abs is too small."),
                                    line_number,line.c_str());
        ok=false;
    }

    if (ok)
    {
        mStation->observations.emplace_back(mStation->origin(),mPt,mStation,OBS_CODE::AXIS_R,active_r,0,mSigmaR,
                                            0,0,0,
                                            line_number,1,"m",_file,comment);
        this->obsR=&mStation->observations.back();
        mStation->observations.emplace_back(mStation->origin(),mPt,mStation,OBS_CODE::AXIS_T,active_t,0,mSigmaT,
                                            0,0,0,
                                            line_number,1,"m",_file,comment);
        this->obsT=&mStation->observations.back();
    }
    return ok;
}

bool AxisObs::set_obs(LeastSquares *lsquares, bool initialResidual, bool /*unused*/)
{
    std::vector<tdouble> relations;
    std::vector<int> positions;
    relations.resize(12);
    positions.resize(12);

    //forbid 1D and 2D points
    if ((obsR->from->dimension<3)||(obsR->to->dimension<3))
    {
        Project::theInfo()->warning(INFO_OBS,mStation->getFile().get_fileDepth(),
                                    QT_TRANSLATE_NOOP("QObject","Observation %s can't be used with "
                                                                "non-3D point %s or %s."),
                                    obsR->toString().c_str(),obsR->from->name.c_str(),
                                    obsR->to->name.c_str());
        return false;
    }

    bool ok=true;

    Coord P_coord_compensated_cartesian;
    Project::theone()->projection.sphericalToGlobalCartesian(mPt->coord_comp,
                                                       P_coord_compensated_cartesian);
    Coord S_coord_compensated_cartesian;
    Project::theone()->projection.sphericalToGlobalCartesian(mStation->origin()->coord_comp,
                                                       S_coord_compensated_cartesian);


    tdouble a = mStation->getN().x();
    tdouble b = mStation->getN().y();
    tdouble c = mStation->getN().z();
    tdouble l = mTarget->getL();
    tdouble r = mTarget->getR();
    Coord Or  = S_coord_compensated_cartesian;
    Coord P   = P_coord_compensated_cartesian;
    Coord C   = Or+mStation->getN()*l;

    tdouble dx=P.x()-C.x();
    tdouble dy=P.y()-C.y();
    tdouble dz=P.z()-C.z();
    tdouble r_calc=sqrt((P-C).norm2());

    tdouble res_r= r_calc - r;
    tdouble res_t= mStation->getN().scalaire(C-P);

    positions[0]=  LeastSquares::B_index;//index of constant part
    positions[1]=  mStation->params.at(0).rank; //axis a
    positions[2]=  mStation->params.at(1).rank; //axis b
    positions[3]=  mStation->params.at(2).rank; //axis c
    positions[4]=  obsR->from->params.at(0).rank; //Station
    positions[5]=  obsR->from->params.at(1).rank;
    positions[6]=  obsR->from->params.at(2).rank;
    positions[7]=  mStation->params.at(mTarget->getParamLindex()).rank; //target l
    positions[8]=  mStation->params.at(mTarget->getParamRindex()).rank; //target r
    positions[9]=  obsR->to->params.at(0).rank;   //Target
    positions[10]= obsR->to->params.at(1).rank;
    positions[11]= obsR->to->params.at(2).rank;

    obsR->value = 0; //here all obs are f(X)=0
    obsT->value = 0;

    obsR->D = r_calc;
    obsT->D = r_calc;

    obsR->deflectionCorrection = 0;// TODO(jmm): ?
    obsT->deflectionCorrection = 0;

    obsR->computedValue = 0;
    obsT->computedValue = 0;

    //sigma total is sigmaAbs transformed into equation unit
    obsR->sigmaTotal = obsR->sigmaAbs;                //m
    obsT->sigmaTotal = obsT->sigmaAbs;                //m
    if (Project::theone()->config.compute_type==COMPUTE_TYPE::type_monte_carlo)
    {
        obsR->residual = gauss_random(0,obsR->sigmaTotal);
        obsT->residual = gauss_random(0,obsT->sigmaTotal);
    }
    else if (Project::theone()->config.compute_type==COMPUTE_TYPE::type_propagation)
    {
        obsR->residual = 0;
        obsT->residual = 0;
    }
    else
    {
        obsR->computedValue = res_r;
        obsR->residual = res_r;
        obsT->computedValue = res_t;
        obsT->residual = res_t;
    }
    obsR->normalizedResidual = obsR->residual/obsR->sigmaTotal;
    obsT->normalizedResidual = obsT->residual/obsT->sigmaTotal;
    if (initialResidual)
    {
        obsR->initialNormalizedResidual=obsR->normalizedResidual;
        obsR->initialResidual=obsR->residual;
        obsT->initialNormalizedResidual=obsT->normalizedResidual;
        obsT->initialResidual=obsT->residual;
    }

    //obsR
    relations[0]  = obsR->residual;
    relations[1]  = -l*dx/r_calc;             //axis a
    relations[2]  = -l*dy/r_calc;
    relations[3]  = -l*dz/r_calc;
    relations[4]  = -2*dx/r_calc;              //Station
    relations[5]  = -2*dy/r_calc;
    relations[6]  = -2*dz/r_calc;
    relations[7]  = (-a*dx-b*dy-c*dz)/r_calc; //target l
    relations[8]  = -1;                       //target r
    relations[9]  = -relations[4];            //Target
    relations[10] = -relations[5];
    relations[11] = -relations[6];
    switch (mStation->getMainDir()) {
    case MainDir_X:
        relations[1]=0;
        break;
    case MainDir_Y:
        relations[2]=0;
        break;
    case MainDir_Z:
        relations[3]=0;
        break;
    default:
        break;
    }
    if (obsR->active)
        ok = lsquares->add_constraint(obsR,relations,positions,obsR->sigmaTotal) && ok;

    //obsT
    relations[0]  = obsT->residual;
    relations[1]  = -P.x()+Or.x()+2*a*l;
    relations[2]  = -P.y()+Or.y()+2*b*l;
    relations[3]  = -P.z()+Or.z()+2*c*l;
    relations[4]  = a;
    relations[5]  = b;
    relations[6]  = c;
    relations[7]  = a*a+b*b+c*c;
    relations[8]  = 0;
    relations[9]  = -a;
    relations[10] = -b;
    relations[11] = -c;
    switch (mStation->getMainDir()) {
    case MainDir_X:
        relations[1]=0;
        break;
    case MainDir_Y:
        relations[2]=0;
        break;
    case MainDir_Z:
        relations[3]=0;
        break;
    default:
        break;
    }
    if (obsT->active)
        ok = lsquares->add_constraint(obsT,relations,positions,obsT->sigmaTotal) && ok;

    return ok;
}
