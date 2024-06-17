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

#include "station_axis.h"
#include "project.h"
#include <cmath>
#include <set>

AxisTarget::AxisTarget(Station_Axis *station, const std::string & targetNum):
     mTargetNum(targetNum), active(true),mStation(station),
    l(NAN), r(NAN), mWingspan(NAN)
{
#ifdef INFO_AXE
    std::cout<<"Create new AxisTarget: "<<targetNum<<std::endl;
#endif
    //create parameters
    std::ostringstream oss_l;
    oss_l<<mStation->origin()->name<<"_st"<<station->getNum()<<"_T"<<targetNum<<"_l";
    mStation->params.emplace_back(oss_l.str(),&l,1.0,"m",station->origin());
    param_l_index=mStation->params.size()-1;    // NOLINT: Can't be in member initializer list
    std::ostringstream oss_r;
    oss_r<<mStation->origin()->name<<"_st"<<station->getNum()<<"_T"<<targetNum<<"_r";
    mStation->params.emplace_back(oss_r.str(),&r,1.0,"m",station->origin());
    param_r_index=mStation->params.size()-1;    // NOLINT: Can't be in member initializer list
}


bool AxisTarget::initialize(bool verbose)
{
    //choose any position
    const Point* pt=nullptr;
    for (const auto& axisObs:allAxisObs)
    {
        if (axisObs.getPt()->isInit())
        {
            pt=axisObs.getPt();
            break;
        }
    }
    if (!pt)
    {
        if (verbose)
            Project::theInfo()->msg(INFO_OBS,1,
                                    QT_TRANSLATE_NOOP("QObject","Impossible to initialize axis target"
                                                                " %s on %s: no initialized point."),
                                    mTargetNum.c_str(),mStation->origin()->name.c_str());
        return false;
    }
    Coord P=pt->coord_init_spher;
    Coord O=mStation->origin()->coord_init_spher;
    Coord n=mStation->getN();
    Coord OP=P-O;
    l=n.scalaire(OP)/sqrt(n.norm2());
    Coord C=O+n*l;
    r=sqrt((C-P).norm2());
#ifdef INFO_AXE
    std::cout<<"Axis target init params: l="<<l<<", r="<<r<<std::endl;
#endif
    return true;
}

void AxisTarget::removeObstoBeDeleted()
{
    auto isToDelete = [](auto const& o) { return o.toBeDeleted; };
    allAxisObs.remove_if(isToDelete);
}

Json::Value AxisTarget::toJson() const
{
    Json::Value val;
    val["num"]=mTargetNum;

    val["axisObs"]=Json::arrayValue;
    for (const auto &axisObs : allAxisObs)
        val["axisObs"].append(axisObs.toJson());
    return val;
}

std::vector<const Point*> AxisTarget::get3BestPos()
{
    mWingspan = -1.0;
    const Point* ptMin=nullptr;
    const Point* ptMax=nullptr;
    const Point* ptMid=nullptr;

    const Point* ptMinX=nullptr;
    const Point* ptMaxX=nullptr;
    const Point* ptMinY=nullptr;
    const Point* ptMaxY=nullptr;
    const Point* ptMinZ=nullptr;
    const Point* ptMaxZ=nullptr;

    //get the max and min points on each dimension
    for (auto &axisObs:allAxisObs)
    {
        if (axisObs.getPt()->isInit())
        {
            if (ptMinX == nullptr) {
                ptMinX=ptMinY=ptMinZ=ptMaxX=ptMaxY=ptMaxZ=axisObs.getPt();
                continue;
            }
            if (ptMinX->coord_init_spher.x()>axisObs.getPt()->coord_init_spher.x())
                ptMinX=axisObs.getPt();
            else if (ptMaxX->coord_init_spher.x()<axisObs.getPt()->coord_init_spher.x())
                ptMaxX=axisObs.getPt();
            if (ptMinY->coord_init_spher.y()>axisObs.getPt()->coord_init_spher.y())
                ptMinY=axisObs.getPt();
            else if (ptMaxY->coord_init_spher.y()<axisObs.getPt()->coord_init_spher.y())
                ptMaxY=axisObs.getPt();
            if (ptMinZ->coord_init_spher.z()>axisObs.getPt()->coord_init_spher.z())
                ptMinZ=axisObs.getPt();
            else if (ptMaxZ->coord_init_spher.z()<axisObs.getPt()->coord_init_spher.z())
                ptMaxZ=axisObs.getPt();
        }
    }

    if (!ptMinX || !ptMaxX || !ptMinY || !ptMaxY || !ptMinZ || !ptMaxZ)
        return {};

    tdouble wX=ptMaxX->coord_init_spher.x()-ptMinX->coord_init_spher.x();
    tdouble wY=ptMaxY->coord_init_spher.y()-ptMinY->coord_init_spher.y();
    tdouble wZ=ptMaxZ->coord_init_spher.z()-ptMinZ->coord_init_spher.z();
    mWingspan=wX;//sqrt(wX*wX+wY*wY+wZ*wZ);
    if (wY>mWingspan) mWingspan=wY;
    if (wZ>mWingspan) mWingspan=wZ;

    //get the two extreme points
    if ((wX>=wY)&&(wX>=wZ))
    {
        ptMin=ptMinX;
        ptMax=ptMaxX;
    }
    else if ((wY>=wX)&&(wY>=wZ))
    {
        ptMin=ptMinY;
        ptMax=ptMaxY;
    }
    else
    {
        ptMin=ptMinZ;
        ptMax=ptMaxZ;
    }

    //find the point the more far from ptMin and ptMax
    tdouble distToMin = NAN,distToMax = NAN, minDist = NAN;
    tdouble bestDist=0;
    for (auto &axisObs:allAxisObs)
    {
        if ((axisObs.getPt()->isInit())&&(axisObs.getPt()!=ptMin)&&(axisObs.getPt()!=ptMax))
        {
            distToMin=(axisObs.getPt()->coord_init_spher-ptMin->coord_init_spher).norm2();
            distToMax=(axisObs.getPt()->coord_init_spher-ptMax->coord_init_spher).norm2();
            minDist=distToMin;
            if (minDist>distToMax)
                minDist=distToMax;
            if (minDist>bestDist)
            {
                bestDist=minDist;
                ptMid=axisObs.getPt();
            }
        }
    }

    return {ptMin,ptMax,ptMid};
}

//----------------------------------------------------------------

Station_Axis::Station_Axis(Point *origin):Station(origin, STATION_TYPE::ST_AXIS),
    file(nullptr),mMainDir(MainDir_unknown),axisOrientFix(nullptr),axisCombi(nullptr)
{
}


std::string Station_Axis::typeStr() const
{
    return "axis";
}

AxisTarget* Station_Axis::updateTarget(std::string target_num, AxisObs &axisObs)
{
    AxisTarget* target_found=nullptr;
    for (auto &target:targets)
    {
        if (target.getTargetNum()==target_num)
        {
            target_found=&target;
            break;
        }
    }
    if (!target_found)
    {
        targets.emplace_back(this, target_num);
        target_found=&targets.back();
    }
    target_found->allAxisObs.push_back(axisObs);
    target_found->allAxisObs.back().setTarget(target_found);

    return target_found;
}

void Station_Axis::removeObsConnectedToPoint(const Point &point)
{
    if (origin()==&point)
    {
        //the station is to remove: juste empty direct obs vector, each obs will be destroyed via AxisObs
        observations.clear();
    }
    if (getAxisCombi()&&(&point==getAxisCombi()->to))
    {
        removeObs({axisCombi});
        axisCombi = nullptr;
    }
    for (auto & tar : getTargets())
    {
        for (auto& obs:tar.allAxisObs)
            if (obs.getPt()==&point)
            {
                obs.toBeDeleted = true;
            }
        tar.removeObstoBeDeleted();
    }

    Station::removeObsConnectedToPoint(point);
}

bool Station_Axis::read_obs(std::string line,
                            int line_number, DataFile *_file, const std::string &current_absolute_path, const std::string &comment)
{
    (void)comment;
    bool ok=true;
    std::string from_name,filename;
    int code=-1;

  #ifdef INFO_AXE
    std::cout<<"Try to read axe obs: "<<line<<std::endl;
  #endif
    std::istringstream iss(line);
    if (!(iss >> code))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert observation code."),line_number,line.c_str());
        ok=false;
    }
    if (code!=static_cast<int>(STATION_CODE::AXIS_OBS))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Line code is incorrect."),line_number,line.c_str());
        ok=false;
    }

    if (!(iss >> from_name))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert from point name."),line_number,line.c_str());
        ok=false;
    }

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


    if (ok)
    {
        //create parameters
        params.clear();
        std::ostringstream oss_da;
        oss_da<<origin()->name<<"_Aa"<<"_"<<num;
        params.emplace_back(oss_da.str(),&n[0],1.0,"",origin());
        std::ostringstream oss_db;
        oss_db<<origin()->name<<"_Ab"<<"_"<<num;
        params.emplace_back(oss_db.str(),&n[1],1.0,"",origin());
        std::ostringstream oss_dc;
        oss_dc<<origin()->name<<"_Ac"<<"_"<<num;
        params.emplace_back(oss_dc.str(),&n[2],1.0,"",origin());
        //all the targets parameter will be created while reading file

        file = AxisFile::create(filename,this,current_absolute_path,line_number,_file->get_fileDepth()+1);

        ok=file->read();

        if (!targets.empty())
        {
            for (auto &target:targets)
            {
                int nb_active_obs=0;
                for (auto &obs:target.allAxisObs)
                {
                    if (obs.obsR->active)
                        nb_active_obs++;
                    if (obs.obsT->active)
                        nb_active_obs++;
                }
                target.active=nb_active_obs<2;
                if (nb_active_obs<2)
                {
                    Project::theInfo()->warning(INFO_OBS,getFile().get_fileDepth(),
                                                QT_TRANSLATE_NOOP("QObject","Warning: removing target id %s: only %d active obs."),
                                                target.getTargetNum().c_str(),nb_active_obs);
                }
            }
        }
        if (observations.empty())
        {
            Project::theInfo()->warning(INFO_OBS,getFile().get_fileDepth(),QT_TRANSLATE_NOOP("QObject","Warning: 0 observation found in %s."),filename.c_str());
            ok=false;
        }

        std::set<std::string> pointsSet;//set of all points names
        std::set<std::string> posTargetSet;//set of all "targetNum_positionNum"
        std::ostringstream oss;
        std::pair<std::set<std::string>::iterator,bool> ret;
        //check obs coherence
        for (auto &target: targets)
        {
            for (auto &aobs: target.allAxisObs)
            {
            #ifdef INFO_AXE
                std::cout<<"Checking "<<aobs->getPosNum()<<"-"<<target->getTargetNum()<<" "<<aobs->getPt()->name<<"\n";
            #endif
                ret = pointsSet.insert(aobs.getPt()->name);
                if (!ret.second)
                {
                    Project::theInfo()->error(INFO_OBS,getFile().get_fileDepth(),
                        QT_TRANSLATE_NOOP("QObject","Error: point %s appears several times in axis station %s."),
                        aobs.getPt()->name.c_str(),filename.c_str());
                    ok=false;
                    aobs.toBeDeleted = true;
                }
                oss.str("");
                oss<<target.getTargetNum()<<"_"<<aobs.getPosNum();
                ret = posTargetSet.insert(oss.str());
                if (!ret.second)
                {
                    Project::theInfo()->error(INFO_OBS,getFile().get_fileDepth(),
                        QT_TRANSLATE_NOOP("QObject","Error: target %s - position %s appears several times in axis station %s."),
                        target.getTargetNum().c_str(),aobs.getPosNum().c_str(),filename.c_str());
                    ok=false;
                    aobs.toBeDeleted = true;
                }
            }
            target.removeObstoBeDeleted();
        }
    }

    return ok;
}


bool Station_Axis::read_constr(const std::string& line, int line_number, DataFile *_file, const std::string& comment)
{
    bool ok=true;
    std::string tmp,to_name;
    Point *to=nullptr;
    std::istringstream iss(line);
    iss >> tmp;//to pass the "L"
    if (!(iss >> to_name))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert to point name."),line_number,line.c_str());
        ok=false;
        return ok;
    }
    //find a point with that name in project
    to=Project::theone()->getPoint(to_name,true);
    if (to == nullptr)
    {
        //Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't find a point named \"%s\"."),line_number,line.c_str(),from_name.c_str());
        ok=false;
    }
    if (to==origin())
    {
        Project::theInfo()->error(INFO_OBS,_file->get_fileDepth()+1,
                                  QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't constrain the axis point \"%s\"."),
                                  line_number,line.c_str(),origin()->name.c_str());
        ok=false;
        return ok;
    }
    observations.emplace_back(origin(),to,this,OBS_CODE::AXIS_COMBI,true,0,0.001,0,0,0,0,1,"x",file.get(),comment);
    axisCombi=&observations.back();
#ifdef INFO_AXE
    std::cout<<"Add axisCombi, station has "<<observations.size()<<" obs.\n";
#endif
    return ok;
}

bool Station_Axis::set_obs(LeastSquares *lsquares,
                           bool initialResidual, bool internalConstraints)
{
    if (!initOk())
    {
        Project::theInfo()->warning(INFO_OBS,getFile().get_fileDepth(),
                                    QT_TRANSLATE_NOOP("QObject","Axis %s not init, it won't be used."),
                                    origin()->name.c_str());
        return false;
    }
    bool ok=true;
    //set obs for targets
    for (auto & target : targets)
        for (auto & axisObs : target.allAxisObs)
            if (!axisObs.set_obs(lsquares,initialResidual,internalConstraints))
                ok=false;

    //set the equation for oritenation: dn.x/y/z = 0
    axisOrientFix->value = 0;
    axisOrientFix->computedValue = 0;
    axisOrientFix->residual = 0;
    axisOrientFix->sigmaTotal = axisOrientFix->sigmaAbs;
    if (initialResidual)
    {
        axisOrientFix->initialNormalizedResidual=axisOrientFix->normalizedResidual;
        axisOrientFix->initialResidual=axisOrientFix->residual;
    }
    static std::vector<tdouble> relations;
    static std::vector<int> positions;
    relations.resize(2);
    positions.resize(2);

    relations[0]=0;
    relations[1]=1;
    positions[0] = LeastSquares::B_index;//index of constant part
    switch (axisOrientFix->code) {
    case OBS_CODE::AXIS_FIX_X:
        positions[1] = params.at(0).rank;
        break;
    case OBS_CODE::AXIS_FIX_Y:
        positions[1] = params.at(1).rank;
        break;
    default:
        positions[1] = params.at(2).rank;
        break;
    }

    ok = lsquares->add_constraint(axisOrientFix,relations,positions,axisOrientFix->sigmaAbsOriginal) && ok;

    //set axisCombi obs
    if (axisCombi)
    {
        //vector(from-other point) must be perpendicular to axis
        Coord A;
        Project::theone()->projection.sphericalToGlobalCartesian(origin()->coord_comp, A);
        Coord B;
        Project::theone()->projection.sphericalToGlobalCartesian(axisCombi->to->coord_comp, B);
        tdouble a = getN().x();
        tdouble b = getN().y();
        tdouble c = getN().z();
        tdouble div=sqrt(getN().norm2()*(B-A).norm2());//not differenciated
        if (div<1e-4) div=1e-4;
        tdouble res = (B-A).scalaire(getN())/div;
        axisCombi->value = 0;
        axisCombi->computedValue = res;
        axisCombi->residual = res;
        axisCombi->sigmaTotal = axisCombi->sigmaAbs;
        axisCombi->D = sqrt((B-A).norm2());
        axisCombi->normalizedResidual = axisCombi->residual/axisCombi->sigmaTotal;
        if (initialResidual)
        {
            axisCombi->initialNormalizedResidual=axisCombi->normalizedResidual;
            axisCombi->initialResidual=axisCombi->residual;
        }

        relations.resize(10);
        positions.resize(10);

        positions[0] = LeastSquares::B_index;//index of constant part
        positions[1] = params.at(0).rank; //a
        positions[2] = params.at(1).rank; //b
        positions[3] = params.at(2).rank; //c
        positions[4] = origin()->params.at(0).rank; //from
        positions[5] = origin()->params.at(1).rank;
        positions[6] = origin()->params.at(2).rank;
        positions[7] = axisCombi->to->params.at(0).rank; //to
        positions[8] = axisCombi->to->params.at(1).rank;
        positions[9] = axisCombi->to->params.at(2).rank;

        relations[0]=axisCombi->residual;
        relations[1]=(B.x()-A.x())/div;
        relations[2]=(B.y()-A.y())/div;
        relations[3]=(B.z()-A.z())/div;
        relations[4]=-a/div;
        relations[5]=-b/div;
        relations[6]=-c/div;
        relations[7]=a/div;
        relations[8]=b/div;
        relations[9]=c/div;
        ok = lsquares->add_constraint(axisCombi,relations,positions,axisCombi->sigmaTotal) && ok;
    }

    return ok;
}

bool Station_Axis::initialize(bool verbose)
{
    //std::cout<<"Initialize Station_Axis on point "<<from->name<<std::endl;
    //determine axis direction
    //choose best target
    mInitOk=false;
    AxisTarget * best_target=nullptr;
    tdouble best_target_wingspan=0;
    for (auto &target:targets)
    {
        target.get3BestPos();
        if (target.mWingspan<0.0) continue;
        if (best_target_wingspan<target.mWingspan)
        {
            best_target_wingspan=target.mWingspan;
            best_target=&target;
        }
    }
    if (!best_target)
    {
        if (verbose)
            Project::theInfo()->msg(INFO_OBS,1,
                                    QT_TRANSLATE_NOOP("QObject","Impossible to initialize axis station on %s: no usable target."),
                                    origin()->name.c_str());
        return false;
    }

    //get 3 good positions of that target
    auto bestPos=best_target->get3BestPos();
    if (bestPos.size()<3)
        return false;
    if ((bestPos[0]==nullptr)||(bestPos[1]==nullptr)||(bestPos[2]==nullptr))
        return false;

    //compute normal
    Coord A=bestPos.at(0)->coord_init_spher;
    Coord B=bestPos.at(1)->coord_init_spher;
    Coord C=bestPos.at(2)->coord_init_spher;
    Coord AB=B-A;
    Coord AC=C-A;
    n=Coord(AB.y()*AC.z()-AB.z()*AC.y(), AB.z()*AC.x()-AB.x()*AC.z(), AB.x()*AC.y()-AB.y()*AC.x());

    //main dir
    if ((fabs(n.x())>fabs(n.y()))&&(fabs(n.x())>fabs(n.z())))
        mMainDir=MainDir_X;

    if (fabs(n.x())>fabs(n.y()))
    {
        if (fabs(n.x())>fabs(n.z()))
            mMainDir=MainDir_X;
        else
            mMainDir=MainDir_Z;
    }else{
        if (fabs(n.y())>fabs(n.z()))
            mMainDir=MainDir_Y;
        else
            mMainDir=MainDir_Z;
    }

    tdouble divider=0;
    //init axis parameters
    switch (mMainDir) {
    case MainDir_X:
        divider=n.x();
        break;
    case MainDir_Y:
        divider=n.y();
        break;
    case MainDir_Z:
        divider=n.z();
        break;
    default:
        if (verbose)
            Project::theInfo()->msg(INFO_OBS,1,
                                    QT_TRANSLATE_NOOP("QObject","Impossible to initialize axis station on %s: no usable target."),
                                    origin()->name.c_str());
        return false;
        break;
    }
    n/=divider;
    //std::cout<<"Axis init params: "<<n.toString()<<std::endl;
    mInitOk=true;
    //handle axis origin
    Coord Or_tmp=(A+B)/2.0;
    if (!origin()->isInit())//don't do anything if point is already init (station init should not change coord_read)
    {
        //from coord_spher to input frame
        Coord coord_read;
        Project::theone()->projection.sphericalToGeoref(Or_tmp,coord_read);
        origin()->set_point(coord_read,Coord(0,0,0),CR_CODE::FREE,true);
    #ifdef INFO_AXE
        std::cout<<"From is free, set point: "<<coord_read.toString()<<"\n";
    #endif
      #ifdef SHOW_CAP
        Project::theInfo()->msg(INFO_CAP,1,
                                QT_TRANSLATE_NOOP("QObject","Initialize point %s as a station."),
                                origin()->name.c_str());
      #endif
        Project::theone()->cor_root->addPoint(origin());//add it to root cor file for .new
    }

    for (auto &target:targets)
        target.initialize(verbose);

    if (!axisOrientFix)
    {
        //add a constraint on one param of axis orientation
        OBS_CODE obs_code = OBS_CODE::UNKNOWN;
        switch (mMainDir) {
        case MainDir_X:
            obs_code=OBS_CODE::AXIS_FIX_X;
            break;
        case MainDir_Y:
            obs_code=OBS_CODE::AXIS_FIX_Y;
            break;
        default:
            obs_code=OBS_CODE::AXIS_FIX_Z;
            break;
        }
        observations.emplace_back(origin(),origin(),this,obs_code,true,0,0.001,0,0,0,0,1,"x",file.get(),"");
        axisOrientFix=&observations.back();
    #ifdef INFO_AXE
        std::cout<<"Add axisOrientFix, station has "<<observations.size()<<" obs.\n";
    #endif
    }

    return true;
}

void Station_Axis::update()
{
    //force on value of n to be 1 (should not be necessary...)
    tdouble divider=0;
    //init axis parameters
    switch (mMainDir) {
    case MainDir_X:
        divider=n.x();
        break;
    case MainDir_Y:
        divider=n.y();
        break;
    case MainDir_Z:
        divider=n.z();
        break;
    default:
        return;
        break;
    }
    n/=divider;
}

Json::Value Station_Axis::toJson(FileRefJson &fileRef) const
{
    Json::Value val = Station::toJson(fileRef);

    val["file_id"]=fileRef.getNumber(file.get());
    val["mainDir"]=mMainDir;

    val["targets"]=Json::arrayValue;
    for (const auto &target : targets)
        val["targets"].append(target.toJson());

    return val;
}


bool Station_Axis::isInternal(Obs* /*obs*/)//if the obs is compatible with internal constraints
{
    return true;
}

bool Station_Axis::isOnlyLeveling(Obs* /*obs*/)//for internal constraints
{
    return false;
}

bool Station_Axis::isHz(Obs* /*obs*/)//for internal constraints
{
    return false;
}

bool Station_Axis::isDistance(Obs* /*obs*/)//for internal constraints
{
    return false;
}

bool Station_Axis::isBubbuled(Obs* /*obs*/)//for internal constraints
{
    return false;
}

bool Station_Axis::useVertDeflection(Obs* /*obs*/)//to check vertical deflection consistancy
{
    return false;
}

int Station_Axis::numberOfBasicObs(Obs* /*obs*/)//number of lines in the matrix
{
    return 1;
}
