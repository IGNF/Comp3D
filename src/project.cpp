/**
 * Copyright (c) Institut national de l'information géographique et forestière https://www.ign.fr/
 *
 * File main authors:
 *  - JM Muller
 *  - C Meynard
 *
 * This file is part of Comp3D: https://github.com/IGNF/Comp3D
 *
 * Comp3D is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Comp3D is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Comp3D. If not, see <https://www.gnu.org/licenses/>.
 */

#define _POSIX_C_SOURCE 1           // NOLINT: Set to have gmtime_r in <ctime>

#include "project.h"
#include "fformat.h"
#include "filerefjson.h"
#include "misc_tools.h"
#include "point.h"
#include "station_bascule.h"
#include "station_eq.h"
#include "station_hz.h"
#include "uni_stream.h"
#include "filesystem_compat.h"
#include <algorithm>
#include <ctime>
#include <eigen3/Eigen/Dense>
#include <exception>
#include <iostream>
#include <json/json.h>
#include <sstream>


#ifdef USE_QT
#include <QDir>
#include <QFile>
#include <QSettings>
#endif


//TODO(jmm): replace it with "std::filesystem::relative"
//taken from https://stackoverflow.com/questions/10167382/boostfilesystem-get-relative-path
static fs::path relativeTo(const fs::path& from, const fs::path& to)
{
   // Start at the root path and while they are the same then do nothing then when they first
   // diverge take the remainder of the two path and replace the entire from path with ".."
   // segments.
   fs::path::const_iterator fromIter = from.begin();
   fs::path::const_iterator toIter = to.begin();

   // Loop through both
   while (fromIter != from.end() && toIter != to.end() && (*toIter) == (*fromIter))
   {
      ++toIter;
      ++fromIter;
   }

   fs::path finalPath;
   while (fromIter != from.end())
   {
      finalPath /= "..";
      ++fromIter;
   }

   while (toIter != to.end())
   {
      finalPath /= *toIter;
      ++toIter;
   }

   return finalPath;
}

std::string Project::defaultLogLang="en";
Project* Project::m_theone=nullptr;
Info* Project::m_theInfo=nullptr;

Project* Project::theone()
{
    //if (!m_theone)
    //    std::cerr<<"ERROR: There is no project created! (theone)"<<std::endl;
    return m_theone;
}

Info* Project::theInfo()
{
    //if (!m_theInfo)
    //    std::cerr<<"ERROR: There is no project created! (theInfo)"<<std::endl;
    return m_theInfo;
}

Project::Project(const std::string& projectFilename):
    filename(projectFilename),config(projectFilename),lsquares(this),
    use_vertical_deflection(false),
    dataRead(false),hasWarning(false),readyToCompute(false),compensationDone(false),invertedMatrix(false),
    MonteCarloDone(false),
    cor_root(nullptr),obs_root(nullptr),coord_cov(nullptr),previous(Project::theone()),
    unitName(Angles::names[ANGLE_UNIT::GRAD]),biggestEllips(0.0)
{
    if (filename=="?")
    {
        std::cerr<<"Error: project path not provided to create the project!"<<std::endl;
        throw 0;                    // NOLINT : It'a an abort
    }

    m_theone=this;
    m_theInfo=&(m_theone->mInfo);
}

std::string Project::createTemplate(const std::string &filename)
{
    Project_Config config(filename);
    Json::Value js_root;
    Json::Value js_config;
    config.saveasJSON(js_config);
    js_root["config"]=js_config;
    std::ostringstream json_data;
    json_data<<js_root;

    return json_data.str();
}

bool Project::read_config()
{
    bool configOK=config.loadasJSON();
    if (configOK)
    {
        if (config.useProj)
            hasWarning=!projection.initGeo(config.localCenter,config.projDef);
        else
            hasWarning=!projection.initLocal(config.centerLatitude,config.localCenter,1.0);
    }
    else
        Project::theInfo()->error(INFO_CONF,1,
                                  QT_TRANSLATE_NOOP("QObject","Error reading JSON configuration file."));

    unitName=Angles::names[config.filesUnit];
    dataRead=false;
    readyToCompute=false;
    compensationDone=false;
    invertedMatrix=false;
    MonteCarloDone=false;
    messages_read_config=mInfo.toStrFormat();
    return configOK;
}

Project::~Project()
{
#ifdef INFO_DELETE
    std::cout<<"Delete project"<<std::endl;
#endif
    clear();
    mInfo.clear();
    m_theone=previous;
    if (m_theone)
        m_theInfo=&(m_theone->mInfo);
    else
        m_theInfo=nullptr;
}

//remove points, obs, matrices...
void Project::clear()
{
#ifdef INFO_DELETE
    std::cout<<"Clear project"<<std::endl;
#endif
    Station::clearNumStationsPerPointPerType();
    lsquares.clear();
    stations.clear();
    points.clear();
    cor_root = nullptr;
    obs_root = nullptr;
    coord_cov=nullptr;
    forbidden_points.clear();
    uninitializable_points.clear();
    Obs::obsCounter=0;
    dataRead=false;
    hasWarning=false;
    readyToCompute=false;
    compensationDone=false;
    invertedMatrix=false;
    MonteCarloDone=false;
    messages_read_data="";
    messages_set_least_squares="";
    messages_computation="";
}

Point* Project::getPoint(const std::string &name, bool create)
{
    for (auto & point : points)
        if (point.name==name)
            return &point;
    if (create)
    {
        if (name[0]=='@')
        {
            Project::theInfo()->warning(INFO_COR,1,
                                        QT_TRANSLATE_NOOP("QObject","%s: Point name starting with '@' is forbidden."),
                                        name.c_str());
            return nullptr;
        }
        points.emplace_back();
        points.back().name=name;
        return &points.back();
    }
    return nullptr;
}

void Project::resetPointsNum()
{
    //clean points numbers
    int pointCounter=0;
    for (auto & point : points)
    {
        point.pointNumber=pointCounter;
        pointCounter++;
    }
}

void Project::cleanPointBeforeDelete(Point &pt)
{
#ifdef INFO_DELETE
    std::cout<<"Project::cleanPointBeforeDelete "<<&pt<<"\n";
#endif
    pt.toBeDeleted = true;

    //remove stations on this point
    auto stationToDelete = [&pt](auto const& st) { return st->origin() == &pt; };
    stations.remove_if(stationToDelete);
}

void Project::deleteStation(Station* station)
{
#ifdef INFO_DELETE
    std::cout<<"Project::deleteStation "<<station<<"\n";
#endif
#ifdef INFO_DELETE
    std::cout<<"Project::deleteStation origin "<<station->origin()<<"\n";
#endif
    stations.remove_if([station](auto const& st) { return st.get() == station; });
}

void Project::removePointstoDelete()
{
#ifdef INFO_DELETE
    std::cout<<"Points to be deleted: ";
    for (auto& pt: points)
        if (pt.toBeDeleted) std::cout<<pt.name<<" ";
    std::cout<<std::endl;
#endif
    for (auto& pt: points) {
        if (pt.toBeDeleted) {
            for (auto & station : stations)
                station->removeObsConnectedToPoint(pt);
        }
    }

    auto pointToDelete = [](auto const& pt) { return pt.toBeDeleted; };
    points.remove_if(pointToDelete);
    resetPointsNum();
}

bool Project::readData()
{
    clear();
    Project::theInfo()->msg(INFO_CONF,0,QT_TRANSLATE_NOOP("QObject","Reading COR..."));
    cor_root=CORFile::create(config.get_root_COR_relative_filename(),config.working_directory);
    bool ok=cor_root->read();

    dataRead=cor_root->exists();//data is read, even if there are alerts
    if (!dataRead)
    {
        std::cout<<mInfo.toStrRaw()<<std::endl;
        return false;
    }

    Project::theInfo()->msg(INFO_CONF,0,QT_TRANSLATE_NOOP("QObject","Reading OBS..."));
    obs_root = OBSFile::create(config.get_root_OBS_relative_filename(),config.working_directory);
    ok = obs_root->read() && ok;

    if (!config.get_coord_cov_relative_filename().empty())
    {
        Project::theInfo()->msg(INFO_CONF,0,
                                QT_TRANSLATE_NOOP("QObject","Reading coordinates covariance CSV file..."));
        coord_cov=std::make_unique<VarCovarMatrix>(VarCovarMatrix(config.get_coord_cov_relative_filename(),
                                                                  config.working_directory));
        ok = coord_cov->getIsOk() && ok;
        if (!coord_cov->getIsOk())
            coord_cov = nullptr;
    }

    Project::theInfo()->msg(INFO_CONF,0,QT_TRANSLATE_NOOP("QObject","Points setting..."));
    //check for forbidden points
    for (auto &pt: points)
    {
        if (!pt.isInit())
        {
            for (auto & forbidden_point : forbidden_points)
            {
                if (pt.name==forbidden_point)
                {
                    Project::theInfo()->warning(INFO_COR,1,
                                                QT_TRANSLATE_NOOP("QObject","Forbidden point %s."),
                                                pt.name.c_str());
                    cleanPointBeforeDelete(pt);
                    break;
                }
            }
        }
    }
    removePointstoDelete();

    //check if uninit points
    int nb_uninit = 0;
    for (auto & point : points)
        if (!point.isInit())
        {
          #ifdef SHOW_CAP
            Project::theInfo()->warning(INFO_CAP,1,
                                        QT_TRANSLATE_NOOP("QObject","Uninitialized point %s."),
                                        point.name.c_str());
          #endif
            hasWarning=true;
            ++nb_uninit;
        }
    if (hasWarning)
    {
        if (nb_uninit==1)
            Project::theInfo()->warning(INFO_CAP,1,
                                        QT_TRANSLATE_NOOP("QObject","1 uninitialized point."));
        else
            Project::theInfo()->warning(INFO_CAP,1,
                                        QT_TRANSLATE_NOOP("QObject","%d uninitialized points."),
                                        nb_uninit);
    }
    if ((nb_uninit>0) && (config.compute_type!=COMPUTE_TYPE::type_compensation))
        Project::theInfo()->error(INFO_CAP,1,
                                  QT_TRANSLATE_NOOP("QObject","Uninitialized points in simulation mode!"
                                                              " They will be discarded!"));

    updateNumberOfActiveObs();//check if point are unconnected

    //remove unconnected points
    for (auto & point : points)
    {
        if ((point.nbActiveObs==0)&&(point.isFree()||(!point.isInit())))
        {
            Project::theInfo()->warning(INFO_COR,1,
                                        QT_TRANSLATE_NOOP("QObject","Point %s is unconnected to the others, it is removed."),
                                        point.name.c_str());
            cleanPointBeforeDelete(point);
            hasWarning=true;
        }
    }
    removePointstoDelete();

    #ifdef OBS_INFO
      //show every obs
      std::cout<<"readData:\n";
      for (auto & station : stations)
          for (auto & obs: station->observations)
              std::cout<<obs.toString()<<std::endl; //crash?
    #endif

    hasWarning=!ok;

    Project::theInfo()->msg(INFO_CONF,0,QT_TRANSLATE_NOOP("QObject","Read data finished."));


    // forbid internalConstr if some coord parameters are fixed
    if (config.internalConstr)
    {
        bool coordFixedAndCI = false;
        for (auto &point : points)
        {
            switch (point.dimension)
            {
                case 1:
                    if (point.isZfixed)
                        coordFixedAndCI = true;
                    break;
                case 2:
                    if (point.isXfixed || point.isYfixed)
                        coordFixedAndCI = true;
                    break;
                case 3:
                    if (point.isXfixed || point.isYfixed || point.isZfixed)
                        coordFixedAndCI = true;
                    break;
            }
            if (coordFixedAndCI) {
                Project::theInfo()->error(INFO_CONF,1,
                                          QT_TRANSLATE_NOOP("QObject","Error: fixed points used "
                                                                      "with internal constraints!"));
                hasWarning = true;
                dataRead = false;
                ok = false;
                break;
            }
        }
    }

    initialization_CAP();

    updateNumberOfActiveObs();

    //remove stations without obs
    for (auto & station : stations)
    {
        if (station->observations.empty())
        {
            std::cout<<"Remove an empty station "<<station.get()<<" on point "<<station->origin()<<std::endl;
            Project::theInfo()->msg(INFO_OBS,1,
                                    QT_TRANSLATE_NOOP("QObject","Remove an empty station from %s."),
                                    (station->origin())?station->origin()->name.c_str():"_");
        }
    }
    auto stationIsEmpty = [](auto const& st) { return st->observations.empty(); };
    stations.remove_if(stationIsEmpty);

    //check for vertical deflection consistency
    for (auto & point : points)
        if ((!(std::isnan(point.dev_eta))) || (!(std::isnan(point.dev_xi))))
        {
            use_vertical_deflection = true;
            break;
        }
    bool vert_dev_inconsistent=false;
    std::string first_inconsistent_point_name;
    if (use_vertical_deflection)
    {
        for (auto & station : stations)
        {
            for (auto & obs: station->observations)
            {
                if (obs.useVertDeflection())
                {
                    if ((std::isnan(obs.from->dev_eta)) || (std::isnan(obs.from->dev_xi)))
                        first_inconsistent_point_name=obs.from->name;
                }
                if ((std::isnan(obs.to->dev_eta)) || (std::isnan(obs.to->dev_xi)))
                {
                    if ((obs.code==OBS_CODE::DH) //obs types where deflection on target is used
                       || (fabs(obs.targetHeight)>CAP_CLOSE))
                    {
                        first_inconsistent_point_name=obs.to->name;
                    }
                }
                if (!first_inconsistent_point_name.empty())
                {
                    vert_dev_inconsistent=true;
                    goto end_vertical_deflection;
                }
            }
        }
    }
    end_vertical_deflection:
    if (vert_dev_inconsistent)
    {
        Project::theInfo()->error(INFO_CONF,1,
                                  QT_TRANSLATE_NOOP("QObject","Error: inconsistent vertical deflection info."));
        Project::theInfo()->error(INFO_CONF,1,
                                  QT_TRANSLATE_NOOP("QObject","First inconsistent point: %s (there may be others)"),
                                  first_inconsistent_point_name.c_str());
        hasWarning=true;
        use_vertical_deflection=false;
    }

    if (use_vertical_deflection)
        Project::theInfo()->msg(INFO_CONF,0,
                                QT_TRANSLATE_NOOP("QObject","Vertical deflection will be used (value "
                                                            "found for all verticalized points)."));
    else
        Project::theInfo()->msg(INFO_CONF,0,
                                QT_TRANSLATE_NOOP("QObject","Vertical deflection not used."));
    
    //check that z type is coherent with obs
    if (!config.useEllipsHeight) //using altitudes is allowed only if only levelling
        for (auto & point : points)
            if ((point.dimension>1)||(!std::isnan(point.dev_eta))||(!std::isnan(point.dev_xi)))
            {
                Project::theInfo()->error(INFO_COR,1,
                                          QT_TRANSLATE_NOOP("QObject","Error: non-1D points or with vertical "
                                                                      "deflection are forbidden in altitude mode!"));
                ok=false;
                points.clear();
                break;
            }

    messages_read_data=mInfo.toStrFormat();

    return ok;
}

void Project::print_points()
{
    std::cout<<"Project points:\n";
    for (auto & point : points)
        std::cout<<point.toString()<<std::endl;
}


bool Project::initialization_CAP()
{
    if (!dataRead)
    {
        //Project::theInfo()->error(INFO_CAP,1,QT_TRANSLATE_NOOP("QObject","Project not ready, need to read data first."));
        std::cerr<<"Project not ready, need to read data first."<<std::endl;
        return false;
    }

    //re-initialize points coordinates
    for (auto & point : points)
        point.coord_comp=point.coord_init_spher;

    int nbuninitpts=0;
    //initialize stations unknowns
    for (auto & point : points)
    {
        if (!point.isInit())
            nbuninitpts++;
    }
    for (auto & station : stations)
        station->initialize(false);

    bool ok=true;

    int previous_nbuninitpts=nbuninitpts+1;

    if (config.compute_type==COMPUTE_TYPE::type_compensation)
    {
        //try to initialize uninit points
        while ((nbuninitpts>0)&&(previous_nbuninitpts>nbuninitpts))
        {
          #ifdef SHOW_CAP
            Project::theInfo()->msg(INFO_CAP,0,
                                    QT_TRANSLATE_NOOP("QObject","Init pass: there is still %d point(s) to initialize."),
                                    nbuninitpts);
          #endif
            previous_nbuninitpts=nbuninitpts;
            nbuninitpts=0;
            for (auto &pt: points)
            {
                if (!pt.isInit())
                {
                    if(!initPoint(pt))
                    {
                        //std::cout<<tr("Uninitialized point.\n").toCstr();
                        nbuninitpts++;
                    }
                }
            }
            //the new points may make some stations initializable
            for (auto& st:stations)
                st->initialize(false);
        }
        if (nbuninitpts>0)
            Project::theInfo()->msg(INFO_CAP,0,
                                    QT_TRANSLATE_NOOP("QObject","Initialization finished with %d uninitialized points."),
                                    nbuninitpts);
        else
            Project::theInfo()->msg(INFO_CAP,0,
                                    QT_TRANSLATE_NOOP("QObject","All points are initialized."));
    }

    uninitializable_points.clear();
    //remove uninitializable points
    for (auto &pt: points)
    {
        if (!pt.isInit())
        {
            uninitializable_points.push_back(pt.name);
            Project::theInfo()->warning(INFO_CAP,1,
                                        QT_TRANSLATE_NOOP("QObject","Removing point %s."),
                                        pt.name.c_str());
            cleanPointBeforeDelete(pt);
            ok=false;
        }
    }
    removePointstoDelete();

    //remove not init stations
    for (auto& st:stations)
        if (!st->initOk())
            Project::theInfo()->warning(INFO_CAP,1,
                                        QT_TRANSLATE_NOOP("QObject","Will remove uninitializable %s."),
                                        st->toString().c_str());
    auto stationIsNotInit = [](auto const& st) { return !st->initOk(); };
    stations.remove_if(stationIsNotInit);
    resetPointsNum();

    //TODO(jmm): clear all stations (remove AxisTarget with no points)

    return ok;
}

bool Project::set_least_squares(bool internalConstr)
{
    mInfo.clear();
    readyToCompute=false;
    if (!dataRead)
    {
        //Project::theInfo()->error(INFO_LS,1,QT_TRANSLATE_NOOP("QObject","Project not ready, need to read data first."));
        std::cerr<<"Project not ready, need to read data first."<<std::endl;
        return false;
    }

    //re-initialize points coordinates
    for (auto & point : points)
        point.coord_comp=point.coord_init_spher;

    //initialize stations unknowns
    for (auto & station : stations)
        station->initialize(true);

    resetPointsNum();
    bool ok = lsquares.initialize(config.compute_type,internalConstr);

    readyToCompute=ok;
    messages_set_least_squares=mInfo.toStrFormat();
    return ok;
}

//TODO(jmm): factorize localToCartesian part?
bool Project::initPoint(Point& pt_to_init)
{
    //DO NOT return false before the end of the function, to check all kinds of initializations
  #ifdef SHOW_CAP
    Project::theInfo()->msg(INFO_CAP,1,
                            QT_TRANSLATE_NOOP("QObject","Try to initialize point %s."),
                            pt_to_init.name.c_str());
  #endif

    //--------------------------------------------------------------------
    //try initializing as a non-topo station
    for (auto & station : pt_to_init.stations_)
        if (station->initialize(false))
            if (pt_to_init.isInit())
            {
                #ifdef SHOW_CAP
                  Project::theInfo()->msg(INFO_CAP,1,
                                          QT_TRANSLATE_NOOP("QObject","Initialize point %s as a station."),
                                          pt_to_init.name.c_str());
                #endif
                return true; //to correctly count init points
            }

    //--------------------------------------------------------------------
    //try initializing with 8+6+3 obs
    for (auto & point : points)
    {
        Obs* obs_azim=nullptr;
        Obs *obs_zen=nullptr;
        Obs *obs_dist=nullptr;
        if (!point.isInit()) continue;
        for (auto & station : point.stations_)
            for (auto & obs : station->observations)
            {
                if (obs.to!=&pt_to_init) continue;
                if (!obs.active) continue;
                switch (obs.code)
                {
                case OBS_CODE::AZIMUTH:
                    obs_azim=&obs;
                    break;
                case OBS_CODE::DIST1:
                case OBS_CODE::DIST:
                    obs_dist=&obs;
                    break;
                case OBS_CODE::ZEN:
                    obs_zen=&obs;
                    break;
                default:
                    break;
                }
            }
        if (obs_azim&&obs_zen&&obs_dist)//this station can initialize this point
        {
            Coord init;
            tdouble d0=obs_dist->value*sin(obs_zen->value);
            init.setx(obs_dist->from->coord_init_spher.x()+d0*sin(obs_azim->value));
            init.sety(obs_dist->from->coord_init_spher.y()+d0*cos(obs_azim->value));
            init.setz(obs_dist->from->coord_init_spher.z()+obs_dist->value*cos(obs_zen->value)+obs_zen->instrumentHeight-obs_zen->targetHeight);
            Coord coord_read;
            Project::theone()->projection.sphericalToGeoref(init,coord_read);
            pt_to_init.set_point(coord_read,Coord(0,0,0),CR_CODE::FREE,true);
            pt_to_init.coord_comp=pt_to_init.coord_init_spher;
          #ifdef SHOW_CAP
            Project::theInfo()->msg(INFO_CAP,1,
                                    QT_TRANSLATE_NOOP("QObject","Initialize point %s from station %s."),
                                    pt_to_init.name.c_str(),obs_azim->from->name.c_str());
          #endif
            cor_root->addPoint(&pt_to_init);//add it to root cor file for .new
            return true;
        }
    }

    //--------------------------------------------------------------------
    //try initializing with bascule obs
    for (auto & point : points)
    {
        if (!point.isInit()) continue;
        for (auto & station : point.stations_)
        {
            if (!station->initOk())
                continue;
            auto *station_basc = dynamic_cast<Station_Bascule*>(station);
            if (station_basc)
            {
                for (auto & obs3d : station_basc->observations3D)
                {
                    if ((obs3d.obs1->to==&pt_to_init) && obs3d.obs1->active && obs3d.obs2->active && obs3d.obs3->active )
                    {
                        Coord init=station_basc->mes2GlobalCart(obs3d.obsToInstrumentFrameCoords(false),false);
                        Coord coord_spher;
                        Project::theone()->projection.globalCartesianToSpherical(init,coord_spher);
                        Coord coord_read;
                        Project::theone()->projection.sphericalToGeoref(coord_spher,coord_read);
                        pt_to_init.set_point(coord_read,Coord(0,0,0),CR_CODE::FREE,true);
                        pt_to_init.coord_comp=pt_to_init.coord_init_spher;
                      #ifdef SHOW_CAP
                        Project::theInfo()->msg(INFO_CAP,1,
                                                QT_TRANSLATE_NOOP("QObject","Initialize point %s by bascule from %s."),
                                                pt_to_init.name.c_str(),point.name.c_str());
                      #endif
                        cor_root->addPoint(&pt_to_init);//add it to root cor file for .new
                        return true;
                    }
                }
            }
        }
    }

    //--------------------------------------------------------------------
    //try initializing with two Station_Bascule angular without distances (photogrammetric pseudo-intersection)
    Obs3D *obsPhoto1 = nullptr, *obsPhoto2 = nullptr;
    for (auto & point : points)
    {
        if (!point.isInit()) continue;
        if (obsPhoto2)
            break;
        for (auto & station : point.stations_)
        {
            if (!station->initOk())
                continue;
            if (obsPhoto2)
                break;
            if (obsPhoto1 &&
                    ( ((&point) == obsPhoto1->obs1->from) ||
                      (point.coord_comp-obsPhoto1->obs1->from->coord_comp).norm2()<CAP_CLOSE))
                break; //need another point
            auto *station_basc = dynamic_cast<Station_Bascule*>(station);
            if (station_basc && (station_basc->triplet_type==STATION_CODE::BASC_ANG_CART))
            {
                for (auto & obs3d : station_basc->observations3D)
                {
                    if ((obs3d.obs1->to==&pt_to_init) && obs3d.obs1->active && obs3d.obs2->active )
                    {
                        if (obsPhoto1)
                        {
                            obsPhoto2 = &obs3d;
                            break;
                        } else {
                            obsPhoto1 = &obs3d;
                            break;
                        }
                    }
                }
            }
        }
    }
    if (obsPhoto1 && obsPhoto2)
    {
        Coord A;
        projection.sphericalToGlobalCartesian(obsPhoto1->obs1->from->coord_comp, A);
        Coord B;
        projection.sphericalToGlobalCartesian(obsPhoto2->obs1->from->coord_comp, B);
        Coord u = Coord(obsPhoto1->obsToVectGlobalCart(false));
        Coord v = Coord(obsPhoto2->obsToVectGlobalCart(false));
        if (fabs(u.scalaire(v)) > MINIMAL_DIVIDER)
        {
            tdouble C1 = u.x()*v.x() + u.y()*v.y() + u.z()*v.z();
            tdouble C2 = u.x()*u.x() + u.y()*u.y() + u.z()*u.z();
            tdouble C3 = v.x()*v.x() + v.y()*v.y() + v.z()*v.z();
            tdouble C4 = u.x()*(B.x()-A.x()) + u.y()*(B.y()-A.y()) + u.z()*(B.z()-A.z());
            tdouble C5 = v.x()*(B.x()-A.x()) + v.y()*(B.y()-A.y()) + v.z()*(B.z()-A.z());
            tdouble mu = (-C1*C4+C2*C5)/(C1*C1-C2*C3);
            tdouble lambda = (mu*C1+C4)/C2;
            Coord a = A + u * lambda;
            Coord b = B + v * mu;
            Coord c = (a + b) / 2.;
            Coord coord_spher;
            projection.globalCartesianToSpherical(c,coord_spher);
            Coord coord_read;
            projection.sphericalToGeoref(coord_spher,coord_read);
            pt_to_init.set_point(coord_read,Coord(0,0,0),CR_CODE::FREE,true);
            pt_to_init.coord_comp=pt_to_init.coord_init_spher;
          #ifdef SHOW_CAP
            tdouble error = sqrt((a-b).norm2())/2.;
            Project::theInfo()->msg(INFO_CAP,1,
                                    QT_TRANSLATE_NOOP("QObject","Initialize point %s by pseudo-intersection"
                                                                " from %s and %s (error: %fm)."),
                                    pt_to_init.name.c_str(),obsPhoto1->obs1->from->name.c_str(),
                                    obsPhoto2->obs1->from->name.c_str(), error);
          #endif
            cor_root->addPoint(&pt_to_init);//add it to root cor file for .new
            return true;
        }
    }

    //--------------------------------------------------------------------
    //try with centering, Dx Dy or dist small and height diff observations
    for (auto & point : points)
    {
        Obs *obs_Dx=nullptr;
        Obs *obs_Dy=nullptr;
        Obs *obs_DH=nullptr;
        if (!(point.isInit())&&(&point!=&pt_to_init)) continue;
        for (auto & station : point.stations_)
            for (auto & obs : station->observations)
            {
                if ((obs.from==&pt_to_init)&&(!obs.to->isInit())) continue;
                if (obs.to!=&pt_to_init && obs.from!=&pt_to_init) continue;
                if (!obs.active) continue;
                switch (obs.code)
                {
                case OBS_CODE::DX_SPHER:
                    obs_Dx=&obs;
                    break;
                case OBS_CODE::DY_SPHER:
                    obs_Dy=&obs;
                    break;
                case OBS_CODE::DIST1:
                case OBS_CODE::DIST:
                case OBS_CODE::DIST_HZ0:
                    if (fabs(obs.value)<CAP_CLOSE)
                    {
                        obs_Dx=&obs;
                        obs_Dy=&obs;
                    }
                    break;
                case OBS_CODE::DH:
                    obs_DH=&obs;
                    break;
                default:
                    break;
                }
            }
        if (obs_Dx&&obs_Dy&&obs_DH)//this station can initialize this point
        {
            Coord init;

            if (obs_Dx->to==&pt_to_init)
                init.setx(obs_Dx->from->coord_read.x()+obs_Dx->value); //wrong if dist, but value is small
            else
                init.setx(obs_Dx->to->coord_read.x()-obs_Dx->value);
          #ifdef SHOW_CAP
            Point* other_pt=(obs_Dx->to==&pt_to_init)?obs_Dx->from:obs_Dx->to;
          #endif
            if (obs_Dy->to==&pt_to_init)
                init.sety(obs_Dy->from->coord_read.y()+obs_Dy->value);
            else
                init.sety(obs_Dy->to->coord_read.y()-obs_Dy->value);
            if (obs_DH->to==&pt_to_init)
                init.setz(obs_DH->from->coord_read.z()+obs_DH->value-obs_DH->instrumentHeight+obs_DH->targetHeight);
            else
                init.setz(obs_DH->to->coord_read.z()-obs_DH->value+obs_DH->instrumentHeight-obs_DH->targetHeight);
            pt_to_init.set_point(init,Coord(0,0,0),CR_CODE::FREE,true);
            //pt->coord_comp=pt->coordInitLocal;
            //projection.localToCartesian(pt->coord_comp,pt->coord_compensated_cartesian);
          #ifdef SHOW_CAP
            Project::theInfo()->msg(INFO_CAP,1,
                                    QT_TRANSLATE_NOOP("QObject","Initialize point %s by DE DN DH on %s."),
                                    pt_to_init.name.c_str(),other_pt->name.c_str());
          #endif
            cor_root->addPoint(&pt_to_init);//add it to root cor file for .new
            return true;
        }
    }

    //--------------------------------------------------------------------
    //try with 3d obs from a init station (rayonnement)
    //look for a station looking at pt with hz, zen, dist (relations in matrixOrdering is not enough)
    for (auto & point : points)
    {
        Obs *obs_hz=nullptr;
        Obs *obs_zen=nullptr;
        Obs *obs_dist=nullptr;
        if (!point.isInit()) continue;
        for (auto & station : point.stations_)
            for (auto & obs : station->observations)
            {
                if (obs.to!=&pt_to_init) continue;
                if (!obs.active) continue;
                switch (obs.code)
                {
                case OBS_CODE::HZ_REF:
                case OBS_CODE::HZ_ANG:
                    if (station->initOk()) obs_hz=&obs;
                    break;
                case OBS_CODE::DIST1:
                case OBS_CODE::DIST:
                    obs_dist=&obs;
                    break;
                case OBS_CODE::ZEN:
                    obs_zen=&obs;
                    break;
                default:
                    break;
                }
            }
        if (obs_hz&&obs_zen&&obs_dist)//this station can initialize this point
        {
            auto* obs_hz_station = dynamic_cast<Station_Hz*>(obs_hz->station);
            Coord init;
            tdouble d0=obs_dist->value*sin(obs_zen->value);
            init.setx(obs_dist->from->coord_init_spher.x()+d0*sin(obs_hz_station->g0+obs_hz->value));
            init.sety(obs_dist->from->coord_init_spher.y()+d0*cos(obs_hz_station->g0+obs_hz->value));
            init.setz(obs_dist->from->coord_init_spher.z()+obs_dist->value*cos(obs_zen->value)+obs_zen->instrumentHeight-obs_zen->targetHeight);
            Coord coord_read;
            Project::theone()->projection.sphericalToGeoref(init,coord_read);
            pt_to_init.set_point(coord_read,Coord(0,0,0),CR_CODE::FREE,true);
            pt_to_init.coord_comp=pt_to_init.coord_init_spher;
          #ifdef SHOW_CAP
            Project::theInfo()->msg(INFO_CAP,1,
                                    QT_TRANSLATE_NOOP("QObject","Initialize point %s from station %s."),
                                    pt_to_init.name.c_str(),obs_hz->from->name.c_str());
          #endif
            cor_root->addPoint(&pt_to_init);//add it to root cor file for .new
            return true;
        }
    }

    //--------------------------------------------------------------------
    //try with point 1D height diff observations=> just if there is no planimetric obs on this point
    Obs *obs_DH=nullptr;
    //first, check if only DH
    bool only_DH=true;
    for (auto & station : stations)
    {
        for (auto & obs : station->observations)
        {
            if ((obs.from==&pt_to_init)||(obs.to==&pt_to_init))
            {
                if (obs.code!=OBS_CODE::DH && obs.code!=OBS_CODE::EQ_DH) //it is not a 1D point
                {
                    only_DH=false;
                    break;
                }
                if ((obs.active)&&((obs.from->isInit())||(obs.to->isInit()))) {
                    if (((obs.code==OBS_CODE::DH))||((obs.code==OBS_CODE::EQ_DH) && obs.station->initOk()))
                        obs_DH=&obs; //continue searching for non-1d obs
                }
            }
        }
        if (!only_DH) break;
    }

    if (only_DH && obs_DH) //point is 1d
    {
        Coord init;
        init.setx(config.localCenter.x());
        init.sety(config.localCenter.y());
        if (obs_DH->code == OBS_CODE::DH)
        {
            if (obs_DH->to==&pt_to_init)
                init.setz(obs_DH->from->coord_read.z()+obs_DH->value-obs_DH->instrumentHeight+obs_DH->targetHeight);
            else
                init.setz(obs_DH->to->coord_read.z()-obs_DH->value+obs_DH->instrumentHeight-obs_DH->targetHeight);
        } else if (obs_DH->code == OBS_CODE::EQ_DH)
        {
            if (obs_DH->to==&pt_to_init)
                init.setz(obs_DH->from->coord_read.z()+dynamic_cast<Station_Eq*>(obs_DH->station)->val);
            else
                init.setz(obs_DH->to->coord_read.z()-dynamic_cast<Station_Eq*>(obs_DH->station)->val);
        }
      #ifdef SHOW_CAP
        Point* other_pt=(obs_DH->to==&pt_to_init)?obs_DH->from:obs_DH->to;
      #endif
        pt_to_init.set_point(init,Coord(0,0,0),CR_CODE::NIV_FREE,true);
      #ifdef SHOW_CAP
        Project::theInfo()->msg(INFO_CAP,1,
                                QT_TRANSLATE_NOOP("QObject","Initialize point 1D %s by leveling on %s."),
                                pt_to_init.name.c_str(),other_pt->name.c_str());
      #endif
        cor_root->addPoint(&pt_to_init);//add it to root cor file for .new
        return true;
    }

    //--------------------------------------------------------------------
    //try with angular obs from 2 stations (intersection)
    std::vector<Obs *> obsHzLookingAtPt;
    //std::vector<Obs *> obsZenLookingAtPt;
    bool anNonPlaniObsFound=false;
    for (auto & station : pt_to_init.stations_)
    {
        for (auto & obs : station->observations)
        {
            if (!obs.active) continue;
            switch (obs.code)
            {
            case OBS_CODE::HZ_REF:
            case OBS_CODE::HZ_ANG:
            case OBS_CODE::DX_SPHER:
            case OBS_CODE::DY_SPHER:
            case OBS_CODE::DIST_HZ0:
                break;
            default:
                anNonPlaniObsFound=true;
                break;
            }
            if (anNonPlaniObsFound) break;
        }
        if (anNonPlaniObsFound) break;
    }
    for (auto & point : points)
    {
        if (!point.isInit()) continue;
        bool anObsFound=false;//only one obs per point
        for (auto & station : point.stations_)
        {
            for (auto & obs : station->observations)
            {
                if (obs.to!=&pt_to_init) continue;
                if (!obs.active) continue;
                switch (obs.code)
                {
                case OBS_CODE::HZ_REF:
                case OBS_CODE::HZ_ANG:
                    if (station->initOk()) obsHzLookingAtPt.push_back(&obs);
                    anObsFound=true;
                    break;
                //case ZEN:
                //    obsZenLookingAtPt.push_back(obs);
                //    break;
                case OBS_CODE::DX_SPHER:
                case OBS_CODE::DY_SPHER:
                case OBS_CODE::DIST_HZ0:
                    break;
                default:
                    anNonPlaniObsFound=true;
                    break;
                }
                if (anObsFound && anNonPlaniObsFound) break;
            }
            if (anObsFound && anNonPlaniObsFound) break;
        }
    }
    //have to find 2 hz obs with ang diff around pi/2
    {
        Coord init;
        Obs *obs_hzA = nullptr;
        Obs *obs_hzB = nullptr;
        //here AzDiff is between 0 and PI/2, closest to 0 is the best
        tdouble bestAzDiff=PI/2;
        const tdouble sufficientAzDiff=PI/16;
        const tdouble insufficientAzDiff=7*PI/16;
        for (unsigned int i=0;i<obsHzLookingAtPt.size();i++)
        {
            Obs* obs1=obsHzLookingAtPt[i];
            auto *obs1_station = dynamic_cast<Station_Hz*>(obs1->station);
            tdouble az1=obs1_station->g0+obs1->value;
            for (unsigned int j=i+1;j<obsHzLookingAtPt.size();j++)
            {
                Obs* obs2=obsHzLookingAtPt[j];
                auto *obs2_station = dynamic_cast<Station_Hz*>(obs2->station);
                tdouble az2=obs2_station->g0+obs2->value;
                tdouble azDiff=fmod(az1-az2,PI);
                if (azDiff<0) azDiff+=PI;
                azDiff=fabs(azDiff-PI/2);
                //std::cout<<"test: "<<(az1-az2)*180/PI<<" "<<fmod(az1-az2,PI)*180/PI<<" "<<azDiff*180/PI<<" "<<bestAzDiff*180/PI<<"\n";
                if (azDiff<bestAzDiff)
                {
                    bestAzDiff=azDiff;
                    obs_hzA=obs1;
                    obs_hzB=obs2;
                }
                if (bestAzDiff<sufficientAzDiff)
                    break;
            }
            if (bestAzDiff<sufficientAzDiff)
                break;
        }
        //std::cout<<obsHzLookingAtPt.size()<<": "<<sufficientAzDiff*180/PI<<"<"<<bestAzDiff*180/PI<<"<"<<insufficientAzDiff*180/PI<<"\n";
        if (bestAzDiff<insufficientAzDiff && obs_hzA && obs_hzB)
        {
            //possible to compute intersection
            tdouble azA=dynamic_cast<Station_Hz*>(obs_hzA->station)->g0+obs_hzA->value;
            tdouble azB=dynamic_cast<Station_Hz*>(obs_hzB->station)->g0+obs_hzB->value;
            Point *ptA=obs_hzA->from;
            Point *ptB=obs_hzB->from;
            tdouble mu1=(-cos(azB)*(ptB->coord_init_spher.x()-ptA->coord_init_spher.x())
                        +sin(azB)*(ptB->coord_init_spher.y()-ptA->coord_init_spher.y()))
                    /(-sin(azA)*cos(azB)+sin(azB)*cos(azA));
            tdouble mu2=(-cos(azA)*(ptB->coord_init_spher.x()-ptA->coord_init_spher.x())
                        +sin(azA)*(ptB->coord_init_spher.y()-ptA->coord_init_spher.y()))
                    /(-sin(azA)*cos(azB)+sin(azB)*cos(azA));
            //std::cout<<"mu1 for "<<pt->name<<": "<<mu1<<std::endl;
            //std::cout<<"A: "<<ptA->coord_init_spher.toString()<<" "<<azA<<"\n";
            //std::cout<<"B: "<<ptB->coord_init_spher.toString()<<" "<<azB<<"\n";
            if ((mu1>0)&&(mu2>0))
            {
                init.setx(ptA->coord_init_spher.x()+mu1*sin(azA));
                init.sety(ptA->coord_init_spher.y()+mu1*cos(azA));
                init.setz(ptA->coord_init_spher.z());//try to improve it later
                //search for zen obs
                for (auto & point : points)
                {
                    if (!point.isInit()) continue;
                    for (auto & station : point.stations_)
                    {
                        for (auto & obs : station->observations)
                        {
                            if (obs.to!=&pt_to_init) continue;
                            if (!obs.active) continue;
                            if (obs.code==OBS_CODE::ZEN)
                            {
                                //compute z diff
                                tdouble distHz=sqrt(sqr(point.coord_init_spher.x()-init.x())
                                                   +sqr(point.coord_init_spher.y()-init.y()));
                                init.setz(point.coord_init_spher.z()-distHz*tan(obs.value+PI/2));
                                Coord coord_read;
                                Project::theone()->projection.sphericalToGeoref(init,coord_read);
                                pt_to_init.set_point(coord_read,Coord(0,0,0),CR_CODE::FREE,true);
                                pt_to_init.coord_comp=pt_to_init.coord_init_spher;
                              #ifdef SHOW_CAP
                                Project::theInfo()->msg(INFO_CAP,1,
                                                        QT_TRANSLATE_NOOP("QObject","Initialize point %s by"
                                                                                    " intersection from stations"
                                                                                    " %s and %s, z from station %s."),
                                                        pt_to_init.name.c_str(),obs_hzA->from->name.c_str(),
                                                        obs_hzB->from->name.c_str(),obs.from->name.c_str());
                              #endif
                                cor_root->addPoint(&pt_to_init);//add it to root cor file for .new
                                return true;
                            }
                        }
                    }
                }
                if (!anNonPlaniObsFound)// if it is not a plani point, do not know what to do...
                {
                    //there is no 1d or 3d obs on this point, say this is a 2d point
                    Coord coord_read;
                    Project::theone()->projection.sphericalToGeoref(init,coord_read);
                    pt_to_init.set_point(coord_read,Coord(0,0,0),CR_CODE::PLANI_FREE,true);
                    pt_to_init.coord_comp=pt_to_init.coord_init_spher;
                  #ifdef SHOW_CAP
                    Project::theInfo()->msg(INFO_CAP,1,
                                            QT_TRANSLATE_NOOP("QObject","Initialize 2d point %s by intersection"
                                                                        " from stations %s and %s."),
                                            pt_to_init.name.c_str(),obs_hzA->from->name.c_str(),
                                            obs_hzB->from->name.c_str());
                  #endif
                    cor_root->addPoint(&pt_to_init);//add it to root cor file for .new
                    return true;
                }
            }
        }
    }


    //--------------------------------------------------------------------
    //try with 3d obs to 2 init points (relevement)
    {
        Obs *obs_hzA   = nullptr;
        Obs *obs_zenA  = nullptr;
        Obs *obs_distA = nullptr;
        Obs *obs_hzB   = nullptr;
        Obs *obs_zenB  = nullptr;
        Obs *obs_distB = nullptr;
        std::vector<Point *> ptsFromThis=pt_to_init.pointsMeasured();
        Point *ptA = nullptr;
        Point *ptB = nullptr;
        for (auto & ptTmp : ptsFromThis)
        {
            if (!ptTmp->isInit()) continue;
            if (ptA==nullptr) ptA=ptTmp;
            if (obs_hzA && obs_zenA && obs_distA) ptB=ptTmp;
            for (auto & station : pt_to_init.stations_)
            {
                //obs_hzA=0;obs_hzB=0;//??? to make sure to have the same g0 ??
                for (auto & obs : station->observations)
                {
                    if (obs_hzA && obs_zenA && obs_distA && obs_hzB && obs_zenB && obs_distB) break;
                    if (!ptB && obs_hzA && obs_zenA && obs_distA) break;
                    if (!obs.active) continue;
                    if (obs.to==ptTmp)
                    {
                        switch (obs.code)
                        {
                        case OBS_CODE::HZ_REF:
                        case OBS_CODE::HZ_ANG:
                            if (ptB==nullptr)
                                obs_hzA=&obs;
                            else
                                obs_hzB=&obs;
                            break;
                        case OBS_CODE::DIST1:
                        case OBS_CODE::DIST:
                            if (ptB==nullptr)
                                obs_distA=&obs;
                            else
                                obs_distB=&obs;
                            break;
                        case OBS_CODE::ZEN:
                            if (ptB==nullptr)
                                obs_zenA=&obs;
                            else
                                obs_zenB=&obs;
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }

        if (obs_hzA && obs_zenA && obs_distA && obs_hzB && obs_zenB && obs_distB)//this point can be initialized
        {
            if (obs_hzA->station==obs_hzB->station)
            {

              #ifdef SHOW_CAP
                Project::theInfo()->msg(INFO_CAP,1,
                                        QT_TRANSLATE_NOOP("QObject","Initialize point %s from %s and"
                                                                    " %s with angles and distances."),
                                        pt_to_init.name.c_str(),obs_hzA->to->name.c_str(),
                                        obs_hzB->to->name.c_str());
              #endif
                Coord init;
                tdouble dist_c=sqrt(sqr(obs_hzA->to->coord_init_spher.x()-obs_hzB->to->coord_init_spher.x())
                                    +sqr(obs_hzA->to->coord_init_spher.y()-obs_hzB->to->coord_init_spher.y()));
                tdouble dist_a=obs_distB->value*sin(obs_zenB->value);
                tdouble dist_b=obs_distA->value*sin(obs_zenA->value);
                if (dist_a+dist_b<dist_c)
                {
                    //bad case, just put the new point in the middle...
                    init.setx((obs_distA->to->coord_init_spher.x()+obs_distB->to->coord_init_spher.x())/2);
                    init.sety((obs_distA->to->coord_init_spher.y()+obs_distB->to->coord_init_spher.y())/2);
                    init.setz((obs_distA->to->coord_init_spher.z()+obs_distB->to->coord_init_spher.z())/2);
                    Coord coord_read;
                    Project::theone()->projection.sphericalToGeoref(init,coord_read);
                    pt_to_init.set_point(coord_read,Coord(0,0,0),CR_CODE::FREE,true);
                    pt_to_init.coord_comp=pt_to_init.coord_init_spher;
                    cor_root->addPoint(&pt_to_init);//add it to root cor file for .new

                    //error_msg<<tr("Too bad init.").toCstr()<<std::endl;
                    //std::cout<<tr("Too bad init.").toCstr()<<std::endl;
                    return true;
                }
                if (dist_a>dist_b+dist_c)
                {
                    //bad case, just put the new point at B + AB
                    init.setx(2*obs_distB->to->coord_init_spher.x()-obs_distA->to->coord_init_spher.x());
                    init.sety(2*obs_distB->to->coord_init_spher.y()-obs_distA->to->coord_init_spher.y());
                    init.setz(2*obs_distB->to->coord_init_spher.z()-obs_distA->to->coord_init_spher.z());
                    Coord coord_read;
                    Project::theone()->projection.sphericalToGeoref(init,coord_read);
                    pt_to_init.set_point(coord_read,Coord(0,0,0),CR_CODE::FREE,true);
                    pt_to_init.coord_comp=pt_to_init.coord_init_spher;
                    cor_root->addPoint(&pt_to_init);//add it to root cor file for .new

                    //error_msg<<tr("Too bad init.").toCstr()<<std::endl;
                    //std::cout<<tr("Too bad init.").toCstr()<<std::endl;
                    return true;
                }
                if (dist_b>dist_a+dist_c)
                {
                    //bad case, just put the new point at A + BA
                    init.setx(2*obs_distA->to->coord_init_spher.x()-obs_distB->to->coord_init_spher.x());
                    init.sety(2*obs_distA->to->coord_init_spher.y()-obs_distB->to->coord_init_spher.y());
                    init.setz(2*obs_distA->to->coord_init_spher.z()-obs_distB->to->coord_init_spher.z());
                    Coord coord_read;
                    Project::theone()->projection.sphericalToGeoref(init,coord_read);
                    pt_to_init.set_point(coord_read,Coord(0,0,0),CR_CODE::FREE,true);
                    pt_to_init.coord_comp=pt_to_init.coord_init_spher;
                    cor_root->addPoint(&pt_to_init);//add it to root cor file for .new

                    //error_msg<<tr("Too bad init.").toCstr()<<std::endl;
                    //std::cout<<tr("Too bad init.").toCstr()<<std::endl;
                    return true;
                }
                tdouble alpha=acos((dist_b*dist_b+dist_c*dist_c-dist_a*dist_a)/(2*dist_b*dist_c));
                tdouble az_AB=atan2(obs_hzB->to->coord_init_spher.x()-obs_hzA->to->coord_init_spher.x(),
                                     obs_hzB->to->coord_init_spher.y()-obs_hzA->to->coord_init_spher.y());
                tdouble angle_ACB=obs_hzB->value-obs_hzA->value;
                if (angle_ACB<0) angle_ACB+=2*PI;
                tdouble az_AC=az_AB;
                if (angle_ACB>PI) az_AC-=alpha;
                else az_AC+=alpha;

                init.setx(obs_distA->to->coord_init_spher.x()+dist_b*sin(az_AC));
                init.sety(obs_distA->to->coord_init_spher.y()+dist_b*cos(az_AC));
                init.setz(obs_distA->to->coord_init_spher.z()-obs_distA->value*cos(obs_zenA->value)-obs_zenA->instrumentHeight+obs_zenA->targetHeight);

                Coord coord_read;
                Project::theone()->projection.sphericalToGeoref(init,coord_read);
                pt_to_init.set_point(coord_read,Coord(0,0,0),CR_CODE::FREE,true);
                pt_to_init.coord_comp=pt_to_init.coord_init_spher;

                cor_root->addPoint(&pt_to_init);//add it to root cor file for .new
                return true;
            }
        }
    }

    //--------------------------------------------------------------------
    //try with ang obs to 3 init points (angular resection)
    {
        Obs *obs_hzA  = nullptr;
        Obs *obs_zenA = nullptr;
        Obs *obs_hzB  = nullptr;
        Obs *obs_zenB = nullptr;
        Obs *obs_hzC  = nullptr;
        Obs *obs_zenC = nullptr;
        std::vector<Point *> ptsFromThis=pt_to_init.pointsMeasured();
        Point *ptA = nullptr;
        Point *ptB = nullptr;
        Point *ptC = nullptr;
        for (auto & ptTmp : ptsFromThis)
        {
            if (!ptTmp->isInit()) continue;
            if (ptA==nullptr) ptA=ptTmp;
            if (obs_hzA && obs_zenA && (obs_hzB==nullptr)) ptB=ptTmp;
            if (obs_hzB && obs_zenB && (obs_hzC==nullptr)) ptC=ptTmp;
            for (auto & station : pt_to_init.stations_)
            {
                //obs_hzA=0;obs_hzB=0;obs_hzC=0;//??? to make sure to have the same g0 ??
                for (auto & obs : station->observations)
                {
                    if (obs_hzA && obs_zenA && obs_hzB && obs_zenB && obs_hzC && obs_zenC ) break;
                    if (!ptB && obs_hzA && obs_zenA) break;
                    if (!ptC && obs_hzB && obs_zenB) break;
                    if (!obs.active) continue;
                    if (obs.to==ptTmp)
                    {
                        switch (obs.code)
                        {
                        case OBS_CODE::HZ_REF:
                        case OBS_CODE::HZ_ANG:
                            if (ptB==nullptr)
                                obs_hzA=&obs;
                            else{
                                if (ptC==nullptr)
                                {
                                    if (fabs(obs.value-obs_hzA->value)>PI/3)
                                        obs_hzB=&obs;
                                }else{
                                    if ((fabs(obs.value-obs_hzA->value)>PI/4)&&(fabs(obs.value-obs_hzB->value)>PI/4))
                                        obs_hzC=&obs;
                                }
                            }
                            break;
                        case OBS_CODE::ZEN:
                            if (ptB==nullptr)
                                obs_zenA=&obs;
                            else{
                                if (ptC==nullptr)
                                    obs_zenB=&obs;
                                else
                                    obs_zenC=&obs;
                            }
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }

        if (obs_hzA && obs_zenA && obs_hzB && obs_zenB && obs_hzC && obs_zenC)//this point can be initialized
        {
            if ((obs_hzA->station==obs_hzB->station)&&(obs_hzA->station==obs_hzC->station))
            {
              #ifdef SHOW_CAP
                Project::theInfo()->msg(INFO_CAP,1,
                                        QT_TRANSLATE_NOOP("QObject","Initialize point %s by resection"
                                                                    " from %s, %s and %s."),
                                        pt_to_init.name.c_str(),ptA->name.c_str(),ptB->name.c_str(),
                                        ptC->name.c_str());
              #endif
                Coord init;
                //see http://www.uphf.fr/coursenligne/topographie/partie4/papier.pdf p17
                tdouble alpha=obs_hzC->value-obs_hzB->value;
                tdouble beta=obs_hzA->value-obs_hzC->value;
                tdouble gamma=obs_hzB->value-obs_hzA->value;

                tdouble ang_a=atan2(ptC->coord_init_spher.x()-ptA->coord_init_spher.x(),
                                    ptC->coord_init_spher.y()-ptA->coord_init_spher.y())
                             -atan2(ptB->coord_init_spher.x()-ptA->coord_init_spher.x(),
                                    ptB->coord_init_spher.y()-ptA->coord_init_spher.y());
                tdouble ang_b=atan2(ptA->coord_init_spher.x()-ptB->coord_init_spher.x(),
                                    ptA->coord_init_spher.y()-ptB->coord_init_spher.y())
                             -atan2(ptC->coord_init_spher.x()-ptB->coord_init_spher.x(),
                                    ptC->coord_init_spher.y()-ptB->coord_init_spher.y());
                tdouble ang_c=atan2(ptB->coord_init_spher.x()-ptC->coord_init_spher.x(),
                                    ptB->coord_init_spher.y()-ptC->coord_init_spher.y())
                             -atan2(ptA->coord_init_spher.x()-ptC->coord_init_spher.x(),
                                    ptA->coord_init_spher.y()-ptC->coord_init_spher.y());

                tdouble p=1/(1/tan(ang_a)-1/tan(alpha));
                tdouble m=1/(1/tan(ang_b)-1/tan(beta));
                tdouble n=1/(1/tan(ang_c)-1/tan(gamma));

                init.setx((p*ptA->coord_init_spher.x()+m*ptB->coord_init_spher.x()+n*ptC->coord_init_spher.x())/(m+n+p));
                init.sety((p*ptA->coord_init_spher.y()+m*ptB->coord_init_spher.y()+n*ptC->coord_init_spher.y())/(m+n+p));

                tdouble hzdist=sqrt(sqr(init.x()-ptA->coord_init_spher.x())+sqr(init.y()-ptA->coord_init_spher.y()));
                init.setz(ptA->coord_init_spher.z()-hzdist*cos(obs_zenA->value)-obs_zenA->instrumentHeight+obs_zenA->targetHeight);

                Coord coord_read;
                Project::theone()->projection.sphericalToGeoref(init,coord_read);
                pt_to_init.set_point(coord_read,Coord(0,0,0),CR_CODE::FREE,true);
                pt_to_init.coord_comp=pt_to_init.coord_init_spher;

                cor_root->addPoint(&pt_to_init);//add it to root cor file for .new
                return true;
            }
        }
    }

    //-------------------------------------------------------
  #ifdef SHOW_CAP
    Project::theInfo()->msg(INFO_CAP,1,
                            QT_TRANSLATE_NOOP("QObject","Can't autoinitialize point %s!"),
                            pt_to_init.name.c_str());
  #endif
    return false;
}


void Project::outputConversion()
{
    for (auto & point : points)
    {
        projection.sphericalToGeoref(point.coord_comp, point.coord_compensated_georef);
        point.shift = point.coord_compensated_georef - point.coord_read;
    }
}

bool Project::updateEllipsoids()
{
    biggestEllips = 0;
    if (!invertedMatrix) return true;

    for (auto & point : points)
    {
        bool ptok = point.set_posteriori_variance(lsquares.Qxx);
        if (!ptok && std::isfinite(lsquares.sigma_0))
        {
            Project::theInfo()->warning(INFO_LS,1,
                                        QT_TRANSLATE_NOOP("QObject","Error on output sigmas of point %s: %s"),
                                        point.name.c_str(), point.ellipsoid.sigmasToString(lsquares.sigma_0).c_str());
            lsquares.computeKernel();
            return false;
        }
    }

    int nbrEllipsTooBig = 0;
    Point *ptbig = nullptr;
    for (auto & point : points)
    {
        if (point.ellipsoid.get_ellipsAxe(0) > biggestEllips)
        {
            biggestEllips = point.ellipsoid.get_ellipsAxe(0);
            ptbig = &point;
        }
        if (point.ellipsoid.get_ellipsAxe(0) > ELLIPS_TOO_BIG)
            ++nbrEllipsTooBig;
    }
    if (ptbig && (biggestEllips > ELLIPS_TOO_BIG))
    {
        if (nbrEllipsTooBig>1)
            Project::theInfo()->warning(INFO_LS,1,
                                        QT_TRANSLATE_NOOP("QObject","Big ellipsoid semi-axes found on"
                                                                    "%s (%.1fm) and %d other points"),
                                        ptbig->name.c_str(), biggestEllips, nbrEllipsTooBig);
        else
            Project::theInfo()->warning(INFO_LS,1,
                                        QT_TRANSLATE_NOOP("QObject","Big ellipsoid semi-axis found on %s (%.1fm)"),
                                        ptbig->name.c_str(), biggestEllips);
        Project::theInfo()->warning(INFO_LS,1,
                                    QT_TRANSLATE_NOOP("QObject","and sigma0 = %.3f"),
                                    lsquares.sigma_0);
        lsquares.computeKernel();
    }
    return true;
}

bool Project::computation(bool invert, bool saveNew)
{
    std::cout<<"------------------------------"<<std::endl;
    bool ok=true;
    if (readyToCompute)
    {
        if (lsquares.iterate(invert,config.compute_type))
        {
            if (saveNew)
                saveNEW();
        }else{
            Project::theInfo()->error(INFO_LS,1,
                                      QT_TRANSLATE_NOOP("QObject","Error during computation."));
            ok=false;
        }
    }else{
        Project::theInfo()->error(INFO_LS,1,
                                  QT_TRANSLATE_NOOP("QObject","No computation, least squares are not set!"));
        ok=false;
    }
    messages_computation=mInfo.toStrFormat();
    saveasJSON();
    return ok;
}

bool Project::saveasJSON()
{
    FileRefJson filesRef;
    
    Json::Value js_root;
    //Json::Value js_input;
    Json::Value js_points;

    Json::Value js_config;
    config.saveasJSON(js_config);

    Json::Value js_all_point_names;
    for (auto & point : points)
    {
        js_all_point_names.append(point.name);
        js_points[point.name]=point.toJson(filesRef);
    }

    Json::Value js_all_forbidden_point_names;
    for (auto & pointname : forbidden_points)
        js_all_forbidden_point_names.append(pointname);

    Json::Value js_all_uninitializable_point_names;
    for (auto & pointname : uninitializable_points)
        js_all_uninitializable_point_names.append(pointname);


    js_root["COMP3D_VERSION"]=COMP3D_VERSION;
    js_root["COMP3D_COPYRIGHT"]=COMP3D_COPYRIGHT;
    js_root["COMP3D_LICENSE"]=COMP3D_LICENSE;
    js_root["COMP3D_REPO"]=COMP3D_REPO;
    js_root["COMP3D_COMMIT"]=GIT_VERSION;
    js_root["COMP3D_OPTIONS"]=COMP3D_OPTIONS;
    if (fs::exists(config.config_file))
    {
        fs::path current_config_path(config.config_file);
        js_root["config_file"]=fs::canonical(current_config_path).string();
    }else{
        js_root["config_file"]=config.config_file;
    }
    js_root["computation"]=lsquares.toJson(filesRef);
    js_root["computation"]["projection"]=projection.toJson();
    js_root["computation"]["compensation_done"]=compensationDone;
    js_root["computation"]["inverted_matrix"]=invertedMatrix;
    js_root["computation"]["Monte_Carlo_done"]=MonteCarloDone;
    js_root["computation"]["use_vertical_deflection"]=use_vertical_deflection;
    js_root["computation"]["unit_name"]=unitName;

    if (invertedMatrix)
        js_root["computation"]["biggest_ellips"]=biggestEllips;

    js_root["computation"]["messages_read_config"]=messages_read_config;
    js_root["computation"]["messages_read_data"]=messages_read_data;
    js_root["computation"]["messages_set_least_squares"]=messages_set_least_squares;
    js_root["computation"]["messages_computation"]=messages_computation;

    js_root["config"]=js_config;
    js_root["all_point_names"]=js_all_point_names;
    js_root["all_forbidden_point_names"]=js_all_forbidden_point_names;
    js_root["all_uninitializable_point_names"]=js_all_uninitializable_point_names;
    updateNumberOfActiveObs(); //to make sure nbActiveObs is computed for each station
    js_root["points"]=js_points;
    
    Json::Value js_stations;
    for (auto & station : stations)
    {
        if (!station->origin())
            js_stations.append(station->toJson(filesRef));
    }
    js_root["other_stations"]=js_stations;
    
    
    Json::Value js_all_data_files;
    for (const auto& it : filesRef.all())
    {
        std::stringstream oss;
        oss << it.second;
        std::string file_path=it.first->get_path()+it.first->get_name();
        fs::path file_path_path(file_path);
        fs::path comp_path(config.config_file);
        js_all_data_files[oss.str()]=relativeTo( comp_path.parent_path(),file_path_path ).string();
    }
    js_root["all_data_files"]=js_all_data_files;
    
    std::ostringstream json_data;
    json_data<<js_root;
    std::string json_data_str=json_data.str();

    uni_stream::ofstream fileout((config.config_file).c_str());
    //std::string outputConfig = writer.write( root );
    fileout << "data=\n";
    fileout << json_data_str<<std::endl;
    fileout.close();

    return false;
}

//record NEW, CSV
void Project::saveNEW()
{
    fs::path current_path(config.get_root_COR_absolute_filename());

    if (Project::theone()->compensationDone)
    {
        uni_stream::ofstream file_new;
        current_path.replace_extension(".new");
        file_new.open(current_path.string().c_str());
        std::cout<<QObject::tr("Write file ").toCstr()<<current_path.string()<<std::endl;
        cor_root->writeNEW(&file_new);
        file_new.close();
    }
}


#undef FFMT_STRICT_SINEX_ANGLE
#undef FFMT_COMPACT_SOLUTION_ESTIMATE

// Les angles sont affiches en degres, minutes, secondes.
// Pour afficher un arrondi correct a la premiere decimale de la seconde,
//  - On ajoute 0.05 seconde a l'angle (0.05/3600)
//  - On tronque la seconde a 1 chiffre apres la virgule
namespace FortranFormat {
  void FFArg(FContext& ctx, const DMSAngle& a) {
      FFArg(ctx, a.degrees);
      FFArg(ctx, a.minutes);
#ifdef FFMT_STRICT_SINEX_ANGLE
      FFArg(ctx, trunc(10.0 * a.secondes) / 10.0);
#else
      FFArg(ctx, static_cast<int>(a.secondes));
      FFArg(ctx, trunc(10.0 * (a.secondes - static_cast<int>(a.secondes))) / 10.0);
#endif
  }

  void FFArg(FContext& ctx, const tm& tm) {
      FFArg(ctx, tm.tm_year % 100);
      FFArg(ctx, tm.tm_yday+1);        // Day of year should start at 1
      FFArg(ctx, tm.tm_hour*3600+tm.tm_min*60+tm.tm_sec);
  }
} // namespace FortranFormat



bool Project::exportVarCov(const std::string &filename, std::vector<Point *> &selectedPoints,
                           bool addStationsParams, bool addOtherParams)
{
    try
    {
        Json::Value jsonCODE = config.miscMetadata["CODE"];
        Json::Value jsonPT = config.miscMetadata["PT"];
        Json::Value jsonDOMES = config.miscMetadata["DOMES"];
        Json::Value jsonTECH = config.miscMetadata["TECH"];
        Json::Value jsonDESCR = config.miscMetadata["DESCR"];
        Json::Value jsonSINEX = config.miscMetadata["SINEX"];

        //export var/covar matrix in csv
        uni_stream::ofstream matrixFile(filename);
        matrixFile.precision(12);
        //matrixFile<<std::fixed;
        std::vector<Parameter*> selected_params; //neg for params that does not exist
        for (auto &point:selectedPoints)
        {
            for (auto &param: point->params)
                if (param.rank!=LeastSquares::no_param_index)
                    selected_params.push_back(&param);
            if (addStationsParams) {
                for (auto &station: point->stations_)
                    for (auto &param: station->params)
                        if (param.rank!=LeastSquares::no_param_index)
                            selected_params.push_back(&param);
            }
        }
        if (addOtherParams)
            for (auto &station: stations)
                if (!station->origin())
                    for (auto &param: station->params)
                        if (param.rank!=LeastSquares::no_param_index)
                            selected_params.push_back(&param);


        matrixFile<<"param"<<"\t";
        for (const auto& param : selected_params)
            matrixFile<<param->name<<"\t";
        matrixFile<<"\n";
        for (unsigned i=0;i<selected_params.size();i++)
        {
            matrixFile<<selected_params[i]->name<<"\t";
            for (unsigned j=0;j<selected_params.size();j++)
                matrixFile<<lsquares.Qxx(selected_params[i]->rank,
                                        selected_params[j]->rank)*WEIGHT_FACTOR<<"\t";
            matrixFile<<"\n";
        }
        matrixFile.close();
        return true;
    }
    catch (std::exception& e)
    {
        std::cout<<"exportVarCov exception: "<<e.what()<<std::endl;
        return false;
    }
}

bool Project::exportSINEX(const std::string &filename,std::vector<Point*> &selectedPoints)
{
    try
    {
        Json::Value jsonCODE = config.miscMetadata["CODE"];
        Json::Value jsonPT = config.miscMetadata["PT"];
        Json::Value jsonDOMES = config.miscMetadata["DOMES"];
        Json::Value jsonTECH = config.miscMetadata["TECH"];
        Json::Value jsonDESCR = config.miscMetadata["DESCR"];
        Json::Value jsonSINEX = config.miscMetadata["SINEX"];
        std::vector<Point*> selectedPointsFiltered;
        for (auto &point:selectedPoints)
            if (point->dimension==3)
                selectedPointsFiltered.push_back(point);
        size_t numberOfParams=selectedPointsFiltered.size()*3;

        //create local var/covar matrix
        std::vector<int> all_params_rank; //neg for params that does not exist
        for (auto &point:selectedPointsFiltered)
        {
            all_params_rank.push_back(point->params[0].rank);
            all_params_rank.push_back(point->params[1].rank);
            all_params_rank.push_back(point->params[2].rank);
        }

        MatX varCovar_local(selectedPointsFiltered.size()*3,selectedPointsFiltered.size()*3);
        for (unsigned int i=0;i<all_params_rank.size();i++)
            for (unsigned int j=0;j<all_params_rank.size();j++)
            {
                if ((all_params_rank[i]<0)||(all_params_rank[j]<0))
                    varCovar_local(i,j)=0;
                else
                    varCovar_local(i,j)=lsquares.Qxx(all_params_rank[i],all_params_rank[j])*WEIGHT_FACTOR*sqr(lsquares.sigma_0);
            }
        //std::cout<<"Var/Cov local=\n"<<varCovar_local<<std::endl;

        //create full rotation matrix :
        SpMat R_total(static_cast<Eigen::Index>(selectedPointsFiltered.size()*3),static_cast<Eigen::Index>(selectedPointsFiltered.size()*3));
        for (int n=0;n<static_cast<int>(selectedPointsFiltered.size());n++)
            for (int i=0;i<3;i++)
                for (int j=0;j<3;j++)
                    R_total.insert(n*3+i,n*3+j)=projection.RotGlobal2Geocentric(i,j);
        //std::cout<<"R_total=\n"<<MatX(R_total)<<std::endl;

        //compute var/cov geocentric
        MatX varCovar_geocentric(selectedPointsFiltered.size()*3,selectedPointsFiltered.size()*3);
        if (projection.type==PROJ_TYPE::PJ_GEO)
            varCovar_geocentric=R_total*varCovar_local*R_total.transpose();
        else
            varCovar_geocentric=varCovar_local;
        //std::cout<<"varCovar_geocentric=\n"<<MatX(varCovar_geocentric)<<std::endl;
        
        uni_stream::ofstream sinexFile(filename);
        const char* separator="'*-------------------------------------------------------------------------------'";
        //header
        struct tm genTime{},obsTime{};

        time_t timestamp = time( nullptr );
#ifndef _MSC_VER
        gmtime_r(&timestamp, &genTime);
#else
        gmtime_s(&genTime, &timestamp); // Yes, args are in reverse order in Microsoft CRT ...
#endif
        timestamp = jsonSINEX.get("obsTime",0).asInt64();
#ifndef _MSC_VER
        gmtime_r(&timestamp, &obsTime);
#else
        gmtime_s(&obsTime, &timestamp);
#endif
        FortranFormat::FFormat fmt;
        
        sinexFile << fmt.toStringf(
                    "'%=SNX 2.10',1X,A3, 1X,I2.2,1H:,I3.3,1H:,I5.5, 1X,A3, 1X,I2.2,1H:,I3.3,1H:,I5.5, 1X,I2.2,1H:,I3.3,1H:,I5.5, 1X,'C', 1X,I5.5",
                    jsonSINEX.get("fileAgency", "").asString(),
                    genTime,
                    jsonSINEX.get("dataAgency", "").asString(),
                    obsTime,obsTime,
                    numberOfParams);
        sinexFile << fmt.toStringf(separator);
        sinexFile << fmt.toStringf("'+FILE/COMMENT'");
        sinexFile << fmt.toStringf("'* File created by ', A60.60", COMP3D_VERSION);

        fs::path config_path(config.config_file);
        sinexFile << fmt.toStringf("'* Original computation file: ', A49.49", config_path.filename().string());
        sinexFile << fmt.toStringf("'* Matrix Scalling Factor used: ', F15.4", sqr(lsquares.sigma_0));
        sinexFile << fmt.toStringf("'-FILE/COMMENT'");
        sinexFile << fmt.toStringf(separator);

        sinexFile << fmt.toStringf("'+SITE/ID'");
        sinexFile << fmt.toStringf("'*CODE PT __DOMES__ T _STATION DESCRIPTION__ APPROX_LON_ APPROX_LAT_ _APP_H_'");
        for (auto &point:selectedPointsFiltered)
        {
            Coord latlong;
            Project::theone()->projection.georefToLatLong(point->coord_compensated_georef,latlong);
#ifdef FFMT_STRICT_SINEX_ANGLE
            sinexFile << fmt.toStringf("1X,A4.4, 1X,A2, 1X,A9.9, 1X,A1, 1X,A22.22, 1X,I3,1X,I2,1X,F4.1, 1X,I3,1X,I2,1X,F4.1, 1X,F7.1",
#else
            sinexFile << fmt.toStringf("1X,A4.4, 1X,A2, 1X,A9.9, 1X,A1, 1X,A22.22, 1X,I3,1X,I2,1X,I2.2,F0.1, 1X,I3,1X,I2,1X,I2.2,F0.1, 1X,F7.1",
#endif
                    jsonCODE[point->name].asString(),
                    jsonPT[point->name].asString(),
                    jsonDOMES[point->name].asString(),
                    jsonTECH[point->name].asString(),
                    jsonDESCR[point->name].asString(),
                    DMSAngle(latlong.x() + 0.05 / 3600.0, ANGLE_UNIT::DEG, true),
                    DMSAngle(latlong.y() + 0.05 / 3600.0, ANGLE_UNIT::DEG, false),
                    point->coord_compensated_georef.z());
        }

        sinexFile << fmt.toStringf("'-SITE/ID'");
        sinexFile << fmt.toStringf(separator);
        sinexFile << fmt.toStringf("'+SOLUTION/EPOCHS'");
        sinexFile << fmt.toStringf("'*Code PT SOLN T Data_start__ Data_end____ Mean_epoch__'");
        sinexFile << fmt.toStringf("'-SOLUTION/EPOCHS'");
        sinexFile << fmt.toStringf(separator);

        sinexFile << fmt.toStringf("'+SOLUTION/ESTIMATE'");
        sinexFile << fmt.toStringf("'*INDEX TYPE__ CODE PT SOLN _REF_EPOCH__ UNIT S __ESTIMATED VALUE____ _STD_DEV___'");
        unsigned int n=0;
        for (auto &point:selectedPointsFiltered)
        {
            Coord cartGeocentric;
            projection.sphericalToCartGeocentric(point->coord_comp,cartGeocentric);
#ifdef FFMT_COMPACT_SOLUTION_ESTIMATE
            for (int i=0; i<3; i++) {
                const char *type;
                tdouble value;
                switch (i) {
                case 0: type="STAX"; value = cartGeocentric.x() ; break;
                case 1: type="STAY"; value = cartGeocentric.y() ; break;
                case 2: type="STAZ"; value = cartGeocentric.z() ; break;
                }
                sinexFile << fmt.toStringf("1X,I5, 1X,A6.6, 1X,A4, 1X,A2, 1X,A4, 1X,I2.2,1H:,I3.3,1H:,I5.5, 1X,A4.4, 1X,A1, 1X,E21.15, 1X,E11.5",    // SINEX Doc says E11.6 which is invalid
                    (n+1)%10000,
                    type,
                    jsonCODE[point->name].asString(),
                    jsonPT[point->name].asString(),
                    "1",
                    obsTime,
                    "m",
                    "2",
                    value,
                    sqrt(varCovar_geocentric(n,n)));
                n++;
            }

#else
            sinexFile << fmt.toStringf("1X,I5, 1X,A6.6, 1X,A4, 1X,A2, 1X,A4, 1X,I2.2,1H:,I3.3,1H:,I5.5, 1X,A4.4, 1X,A1, 1X,E21.15, 1X,E11.5",    // SINEX Doc says E11.6 which is invalid
                    (n+1)%10000,
                    "STAX",
                    jsonCODE[point->name].asString(),
                    jsonPT[point->name].asString(),
                    "1",
                    obsTime,
                    "m",
                    "2",
                    cartGeocentric.x(),
                    sqrt(varCovar_geocentric(n,n)));
            n++;
            sinexFile << fmt.toStringf("1X,I5, 1X,A6.6, 1X,A4, 1X,A2, 1X,A4, 1X,I2.2,1H:,I3.3,1H:,I5.5, 1X,A4.4, 1X,A1, 1X,E21.15, 1X,E11.5",    // SINEX Doc says E11.6 which is invalid
                    (n+1)%10000,
                    "STAY",
                    jsonCODE[point->name].asString(),
                    jsonPT[point->name].asString().c_str(),
                    "1",
                    obsTime,
                    "m",
                    "2",
                    cartGeocentric.y(),
                    sqrt(varCovar_geocentric(n,n)));
            n++;
            sinexFile << fmt.toStringf("1X,I5, 1X,A6.6, 1X,A4, 1X,A2, 1X,A4, 1X,I2.2,1H:,I3.3,1H:,I5.5, 1X,A4.4, 1X,A1, 1X,E21.15, 1X,E11.5",    // SINEX Doc says E11.6 which is invalid
                    (n+1)%10000,
                    "STAZ",
                    jsonCODE[point->name].asString(),
                    jsonPT[point->name].asString().c_str(),
                    "1",
                    obsTime,
                    "m",
                    "2",
                    cartGeocentric.z(),
                    sqrt(varCovar_geocentric(n,n)));
            n++;
#endif
        }
        sinexFile << fmt.toStringf("'-SOLUTION/ESTIMATE'");
        sinexFile << fmt.toStringf(separator);

        sinexFile << fmt.toStringf("'+SOLUTION/MATRIX_ESTIMATE L COVA'");
        sinexFile << fmt.toStringf("'*PARA1 PARA2 ____PARA2+0__________ ____PARA2+1__________ ____PARA2+2__________'");

        std::array<tdouble,3> val{};
        int num_val=0;
        for (unsigned int lig=1;lig<all_params_rank.size()+1;lig++)
        {
            unsigned int start_col=1;
            for (unsigned int j=1;j<=lig;j++)
            {
                val.at(num_val) = varCovar_geocentric(lig-1,j-1);
                num_val++;
                if (((j>1)&&(j%3==0))||(j==lig))
                {
                    switch (j%3) {
                    case 0:
                        sinexFile << fmt.toStringf("1X,I5, 1X,I5, 3(1X,E21.15)",                     // SInex Doc says: E21.14
                                 lig, start_col, val[0], val[1], val[2]);
                        break;
                    case 1:
                        sinexFile << fmt.toStringf("1X,I5, 1X,I5, 1X,E21.15",                        // SInex Doc says: E21.14
                                 lig, start_col, val[0]);
                        break;
                    case 2:
                        sinexFile << fmt.toStringf("1X,I5, 1X,I5, 2(1X,E21.15)",                     // SInex Doc says: E21.14
                                 lig, start_col, val[0],val[1]);
                        break;
                    }
                    num_val=0;
                    start_col=j+1;
                }
            }
        }

        sinexFile << fmt.toStringf("'-SOLUTION/MATRIX_ESTIMATE L COVA'");

        //footer
        sinexFile << fmt.toStringf("'%ENDSNX'");

        sinexFile.flush();
        if (! sinexFile)
        {
            std::cout<<"exportSinex error writing file \""<<filename<<"\""<<std::endl;
            return false;
        }
        std::cout<<"exportSinex done."<<std::endl;
        return true;
    }
    catch (std::exception& e)
    {
        std::cout<<"exportSinex exception: "<<e.what()<<std::endl;
        return false;
    }
}

bool Project::exportRelPrec(const std::string &filename,std::vector<Point*> &selectedPoints)
{
    try
    {
        uni_stream::ofstream relFile(filename);
        relFile.precision(12);
        for (unsigned i=0;i<selectedPoints.size()-1;i++)
        {
            Point *ptA=selectedPoints.at(i);
            Coord cartGlobalA;
            projection.sphericalToGlobalCartesian(ptA->coord_comp,cartGlobalA);
            for (unsigned j=i+1;j<selectedPoints.size();j++)
            {
                Point *ptB=selectedPoints.at(j);
                Coord cartGlobalB;
                projection.sphericalToGlobalCartesian(ptB->coord_comp,cartGlobalB);

                std::vector<Point*> currPoints({ptA,ptB});
                std::vector<int> all_params_rank; //neg for params that does not exist
                for (auto &point:currPoints)
                {
                    all_params_rank.push_back(point->params[0].rank);
                    all_params_rank.push_back(point->params[1].rank);
                    all_params_rank.push_back(point->params[2].rank);
                }

                MatX varCovar(6,6);
                for (unsigned int i2=0;i2<all_params_rank.size();i2++)
                    for (unsigned int j2=0;j2<all_params_rank.size();j2++)
                    {
                        if ((all_params_rank[i2]<0)||(all_params_rank[j2]<0))
                            varCovar(i2,j2)=0;
                        else
                            varCovar(i2,j2)=lsquares.Qxx(all_params_rank[i2],all_params_rank[j2])*WEIGHT_FACTOR;
                    }

                std::cout<<"Var/Cov local=\n"<<varCovar<<std::endl;

                relFile<<ptA->name<<"-"<<ptB->name<<": ";
                if ((ptA->dimension==3)&&(ptB->dimension==3))
                {
                    //distance 3d
                    tdouble distAB=sqrt((cartGlobalA-cartGlobalB).norm2());
                    MatX Fdist(6,1);
                    Fdist(0,0)=(cartGlobalA.x()-cartGlobalB.x())/distAB;
                    Fdist(1,0)=(cartGlobalA.y()-cartGlobalB.y())/distAB;
                    Fdist(2,0)=(cartGlobalA.z()-cartGlobalB.z())/distAB;
                    Fdist(3,0)=-(cartGlobalA.x()-cartGlobalB.x())/distAB;
                    Fdist(4,0)=-(cartGlobalA.y()-cartGlobalB.y())/distAB;
                    Fdist(5,0)=-(cartGlobalA.z()-cartGlobalB.z())/distAB;

                    tdouble sigma_dist=sqrt((Fdist.transpose()*varCovar*Fdist)(0,0))*lsquares.sigma_0;
                    //std::cout<<"Fdist=\n"<<Fdist<<std::endl;
                    //std::cout<<"sigma_dist=\n"<<sigma_dist<<std::endl;

                    relFile<<"3d dist="<<distAB<<"+/-"<<sigma_dist<<"m; ";
                }else{
                    relFile<<"no 3d dist; ";
                }
                if ((ptA->dimension>=2)&&(ptB->dimension>=2))
                {
                    //distance 2d
                    tdouble distAB=sqrt(sqr(cartGlobalA.x()-cartGlobalB.x())+sqr(cartGlobalA.y()-cartGlobalB.y()));
                    MatX Fdist(6,1);
                    Fdist(0,0)=(cartGlobalA.x()-cartGlobalB.x())/distAB;
                    Fdist(1,0)=(cartGlobalA.y()-cartGlobalB.y())/distAB;
                    Fdist(2,0)=0;
                    Fdist(3,0)=-(cartGlobalA.x()-cartGlobalB.x())/distAB;
                    Fdist(4,0)=-(cartGlobalA.y()-cartGlobalB.y())/distAB;
                    Fdist(5,0)=0;

                    tdouble sigma_dist=sqrt((Fdist.transpose()*varCovar*Fdist)(0,0))*lsquares.sigma_0;
                    //std::cout<<"Fdist=\n"<<Fdist<<std::endl;
                    //std::cout<<"sigma_dist=\n"<<sigma_dist<<std::endl;

                    relFile<<"2d dist="<<distAB<<"+/-"<<sigma_dist<<"m; ";
                }else{
                    relFile<<"no 2d dist; ";
                }
                //precisions 1d
                static const std::array<std::string,3> dim_name{"x","y","z"};
                for (unsigned i2=0;i2<dim_name.size();i2++)
                    relFile<<"d"<<dim_name.at(i2)<<"="<<cartGlobalB.toVect()[i2]-cartGlobalA.toVect()[i2]<<"+/-"<<sqrt(varCovar(i2,i2) + varCovar(i2+3,i2+3) - 2*varCovar(i2,i2+3))*lsquares.sigma_0<<"m; ";
                relFile<<"\n";
            }
        }
        relFile.close();
        return true;
    }
    catch (std::exception& e)
    {
        std::cout<<"exportRelPrec exception: "<<e.what()<<std::endl;
        return false;
    }
}


bool Project::exportSightMatrix(const std::string &filename)
{
    try
    {
        uni_stream::ofstream matrixFile(filename);
        std::vector<std::string> pointsNames;
        std::vector<std::string> stationsNames;
        for (auto &point: points)
        {
            pointsNames.push_back(point.name);
            if (!point.stations_.empty())
                stationsNames.push_back(point.name);
        }
        Eigen::MatrixXi sightMatrix(points.size(),stationsNames.size());
        sightMatrix.setZero(static_cast<Eigen::Index>(points.size()),static_cast<Eigen::Index>(stationsNames.size()));
        int stationNum=0;
        for (auto &point: points)
        {
            for (auto &station: point.stations_)
                for (auto &obs: station->observations)
                    if (obs.from->getPointNumber()!=obs.to->getPointNumber())
                        sightMatrix(obs.to->getPointNumber(),stationNum)=1;
            if (!point.stations_.empty())
                stationNum++;
        }

        matrixFile<<"Station\t";
        for (const auto& name :stationsNames)
                matrixFile<<name<<"\t";
        matrixFile<<"\n";

        for (unsigned int i=0;i<pointsNames.size();i++)
        {
            matrixFile<<pointsNames.at(i)<<"\t";
            for (unsigned int j=0;j<stationsNames.size();j++)
                matrixFile<<sightMatrix(i,j)<<"\t";
            matrixFile<<"\n";
        }

        matrixFile.close();
        return true;
    }
    catch (std::exception& e)
    {
        std::cout<<"ExportSightMatrix exception: "<<e.what()<<std::endl;
        return false;
    }
}


int Project::updateNumberOfActiveObs(bool internalOnly)
{
    // update points nbActiveObs
    for (auto & point : points) point.nbActiveObs=0;
    for (auto & station : stations)
    {
        for (auto & obs: station->observations)
        {
            if (obs.active && (!internalOnly || obs.isInternal()))
            {
                if (station->origin()) station->origin()->nbActiveObs+=obs.numberOfBasicObs();
                if ((obs.from) && (station->origin()!=obs.from)) obs.from->nbActiveObs+=obs.numberOfBasicObs();
                if ((obs.to) && (obs.from!=obs.to)) obs.to->nbActiveObs+=obs.numberOfBasicObs();
            }
        }
    }

    // update stations and get total
    int total=0;
    for (auto & station : stations)
    {
        total+=station->updateNumberOfActiveObs(internalOnly);
    }

    std::cout<<QObject::tr("Number of active obs: ").toCstr()<<total<<std::endl;
    return total;
}


unsigned int Project::totalNumberOfObs()
{
    unsigned total=0;
    for (auto & station : stations)
        total+=station->observations.size();

    //std::cout<<QObject::tr("Total number of obs: ").toCstr()<<total<<std::endl;
    return total;
}


#ifdef USE_QT
void Project::prepareJson(const std::string &_filePath, const std::string & /*unused*/)
{
    std::cout<<"Updating files... "<<std::endl;
    const std::string &filePath=_filePath;
    //copy files for visualization
    QDir projectDirectory(filePath.c_str());
    projectDirectory.cdUp();
    projectDirectory.mkdir("res");
    copyAllRes(":/gui/html/",projectDirectory.absolutePath()+"/res/");

    QDir currentFile(filePath.c_str());
    std::cout<<"Write html file... "<<std::flush;
    std::string htmlFileName=filePath+".html"; //seems slow on windows

    uni_stream::ofstream visuFile;
    visuFile.open(htmlFileName,uni_stream::ofstream::trunc);

    visuFile<<
R"(<!DOCTYPE html>
<html lang="fr">
<head>
<link rel="shortcut icon" type="image/x-icon" href="res/logo_comp.png" />
  <link rel="stylesheet" href="res/leaflet.css"/>
  <script src="res/leaflet.js"></script>
  <script src="res/l.ellipse.min.js"></script>
  <script src="res/visu_comp.js"></script>
  <script src="res/Chart.js"></script>
  <script> var data_file=")" +currentFile.dirName().toStdString()+ R"(";</script>
  <link rel="stylesheet" href="res/comp3d.css"/>
  <meta charset="utf-8"/>
  <title>JavaScript error!</title>
</head>
<body>
<div id="container">
  <div id="sideBar">
    <button class="menu_button" onclick="document.getElementById('sideBar').classList.toggle('collapsed');document.getElementById('main').classList.toggle('side_collapsed');setTimeout(function(){ mymap.invalidateSize()}, 500);"><img src="res/burger.png" width="20" height="20"></button>
    <div id="logos">
      <img src="res/logo_IGN.jpg" alt="IGN">
      <img src="res/logo_comp.png" alt="Comp3D">
    </div>
  </div>
  <div id="main">
    <h1 id="top">Comp3D</h1>
    <h1 id="title">JavaScript error!</h1>
  </div >
</div >
</body>
</html>
)";
    visuFile.close();
    std::cout<<"done!"<<std::endl;
}
#endif
