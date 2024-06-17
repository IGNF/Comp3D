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

#include "station_bascule.h"

#include "mathtools.h"
#include "project.h"
#include <cmath>
#include <regex>

Obs3D::Obs3D(Station_Bascule *_station):obs1(nullptr),obs2(nullptr),obs3(nullptr),station(_station)
{

}

std::string Obs3D::toString() const
{
    std::ostringstream oss;
    //oss<<this<<" ";
    oss<<"Obs Triplet:\n  "<<obs1->toString()<<"\n  "<<obs2->toString()<<"\n  "<<obs3->toString()<<"\n";
    return oss.str();
}

Json::Value Obs3D::toJson() const
{
    Json::Value val;
    val["num_obs1"]=obs1->getObsNumber();
    val["num_obs2"]=obs2->getObsNumber();
    val["num_obs3"]=obs3->getObsNumber();
    return val;
}

bool Obs3D::read_obs(const std::string& line, int line_number, DataFile *_file, const std::string& comment)
{
    bool ok=true;
    std::string to_name;
    Point *to=nullptr;
    tdouble value1_original=0;//before angle conversion
    tdouble sigma_abs1_original=0,sigma_rel_original=0;//before angle conversion
    tdouble value2_original=0;//before angle conversion
    tdouble sigma_abs2_original=0;//before angle conversion
    tdouble value3_original=0;//before angle conversion
    tdouble sigma_abs3_original=0;//before angle conversion
    tdouble sigma_rel_ang_original=0,sigma_rel_dist_original=0;//only for angle-angle-dist type
    tdouble sigma_xy = NAN,sigma_xz = NAN,sigma_yz = NAN;//only for geocentric, combined with sigma_abs[123]_original
    tdouble target_height=0;
    bool active1=true;
    bool active2=true;
    bool active3=true;

  #ifdef OBS_INFO
    std::cout<<"Try to read obs: "<<line<<std::endl;
  #endif
    std::istringstream iss(line);

    if (!(iss >> to_name))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert to point name."),line_number,line.c_str());
        ok=false;
    }
    //find a point with that name in project
    to=Project::theone()->getPoint(to_name,true);
    if (to == nullptr)
    {
        //Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't find a point named \"%s\"."),line_number,line.c_str(),from_name.c_str());
        ok=false;
    }

    if (station->origin()==to)
    {
        Project::theInfo()->error(INFO_OBS,_file->get_fileDepth()+1,
                                  QT_TRANSLATE_NOOP("QObject","At line %d: %s => Observation between %s and %s."),
                                  line_number,line.c_str(),station->origin()->name.c_str(),to->name.c_str());
        ok=false;
    }
    // TODO(jmm): check points dimension (make Point::is3D()? true if 0 or 3 ?)

    if ((!(iss >> value1_original))||(!(iss >> value2_original))||(!(iss >> value3_original)))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert observation value."),
                                    line_number,line.c_str());
        ok=false;
    }

    if (station->triplet_type==STATION_CODE::BASC_XYZ_CART)
    {
        //for sigmas the possibilities are:
        // sigmaXYZ
        // sigmaXYZ sigmaXYZ_PPM
        // sigmaX sigmaY sigmaZ
        // sigmaX sigmaY sigmaZ sigmaXYZ_PPM
        // sigmaX sigmaY sigmaZ sigmaXYZ_PPM targetH

        if (!(iss >> sigma_abs1_original))
        {
            Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert observation sigma_abs."),line_number,line.c_str());
            ok=false;
        }
        if (!(iss >> sigma_abs2_original))
        {
            sigma_abs2_original=sigma_abs1_original;
            sigma_abs3_original=sigma_abs1_original;
            sigma_rel_original=0;
        }else{
            if (!(iss >> sigma_abs3_original))
            {
                sigma_rel_original=sigma_abs2_original;
                sigma_abs2_original=sigma_abs1_original;
                sigma_abs3_original=sigma_abs1_original;
            }else{
                iss >> sigma_rel_original;
                iss >> target_height;
            }
        }
    }
    else if (station->triplet_type==STATION_CODE::BASC_ANG_CART)
    {
        // sigma_ang_abs sigma_dist_abs sigma_dist_rel
        // sigma_ang_abs sigma_ang_ppm sigma_dist_abs sigma_dist_rel
        // sigma_ang_abs sigma_ang_ppm sigma_dist_abs sigma_dist_rel targetH
        if ((!(iss >> sigma_abs1_original))||(!(iss >> sigma_rel_ang_original))||(!(iss >> sigma_abs3_original)))
        {
            Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert observation sigma_abs."),line_number,line.c_str());
            ok=false;
        }
        if (!(iss >> sigma_rel_dist_original))
        {
            sigma_rel_dist_original=sigma_abs3_original;
            sigma_abs3_original=sigma_rel_ang_original;
            sigma_rel_ang_original=0;
        }else
            iss >> target_height;
        sigma_abs2_original=sigma_abs1_original;
    }
    else if (station->triplet_type==STATION_CODE::BASELINE_GEO_XYZ)
    {
        // xx xy xz yy yz zz
        if ((!(iss >> sigma_abs1_original))||(!(iss >> sigma_xy))||(!(iss >> sigma_xz))
          ||(!(iss >> sigma_abs2_original))||(!(iss >> sigma_yz))||(!(iss >> sigma_abs3_original)))
        {
            Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert observation sigmas."),line_number,line.c_str());
            ok=false;
        }
        iss >> target_height;
    }
    int test_endline = 0;
    if (iss >> test_endline)
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Too many values."),line_number,line.c_str());
        ok=false;
    }

    if ((Project::theone()->config.compute_type==COMPUTE_TYPE::type_compensation)
            &&(station->triplet_type==STATION_CODE::BASC_ANG_CART)
            &&(sigma_abs3_original>0)&&(value3_original<=0.0))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Distance can't be negative or zero!"),line_number,line.c_str());
        ok=false;
    }

    //angle conversions
    tdouble angUnitFactor=toRad(1.0,Project::theone()->config.filesUnit);
    std::string angUnitName=Project::theone()->unitName;

    active1=(sigma_abs1_original>0);
    active2=(sigma_abs2_original>0);
    active3=(sigma_abs3_original>0);

    if ((station->triplet_type==STATION_CODE::BASC_XYZ_CART)||(station->triplet_type==STATION_CODE::BASC_ANG_CART))
    {
        if ((fabs(sigma_abs1_original)<MINIMAL_SIGMA)||(fabs(sigma_abs2_original)<MINIMAL_SIGMA)||(fabs(sigma_abs3_original)<MINIMAL_SIGMA))
        {
            Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Observation sigma_abs is too small."),line_number,line.c_str());
            ok=false;
        }
        if (sigma_rel_original<0.0)
        {
            Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Relative sigma can't be negative!"),line_number,line.c_str());
            return false;
        }
    }

    if (ok)
    {
        if (station->triplet_type==STATION_CODE::BASC_XYZ_CART)
        {
            station->observations.emplace_back(station->origin(),to,station,OBS_CODE::BASCULE_X,active1,value1_original,sigma_abs1_original,
                                               sigma_rel_original,0,target_height,
                                               line_number,1,"m",_file,comment);
            this->obs1=&station->observations.back();
            station->observations.emplace_back(station->origin(),to,station,OBS_CODE::BASCULE_Y,active2,value2_original,sigma_abs2_original,
                                               sigma_rel_original,0,target_height,
                                               line_number,1,"m",_file,comment);
            this->obs2=&station->observations.back();
            station->observations.emplace_back(station->origin(),to,station,OBS_CODE::BASCULE_Z,active3,value3_original,sigma_abs3_original,
                                               sigma_rel_original,0,target_height,
                                               line_number,1,"m",_file,comment);
            this->obs3=&station->observations.back();
        }
        else if (station->triplet_type==STATION_CODE::BASC_ANG_CART)
        {
            if ((fabs(value1_original*angUnitFactor)>2*PI)||(fabs(value2_original*angUnitFactor)>2*PI))
            {
                Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth()+1,QT_TRANSLATE_NOOP("QObject","At line %d: %s => Angle value out of range!"),line_number,line.c_str());
                return false;
            }
            station->observations.emplace_back(station->origin(),to,station,OBS_CODE::BASCULE_HZ,active1,value1_original,sigma_abs1_original,
                                               sigma_rel_ang_original,0,target_height,
                                               line_number,angUnitFactor,angUnitName,_file,comment);
            this->obs1=&station->observations.back();
            station->observations.emplace_back(station->origin(),to,station,OBS_CODE::BASCULE_ZEN,active2,value2_original,sigma_abs1_original,
                                               sigma_rel_ang_original,0,target_height,
                                               line_number,angUnitFactor,angUnitName,_file,comment);
            this->obs2=&station->observations.back();
            station->observations.emplace_back(station->origin(),to,station,OBS_CODE::BASCULE_DIST,active3,value3_original,sigma_abs3_original,
                                               sigma_rel_dist_original,0,target_height,
                                               line_number,1,"m",_file,comment);
            this->obs3=&station->observations.back();
        }
        else if (station->triplet_type==STATION_CODE::BASELINE_GEO_XYZ)
        {
            active1 = active2 = active3 = active1 && active2 && active3; //semi-active baselines not allowed

            sigma_abs1_original*=station->varianceFactor; //apply variance factor
            sigma_abs2_original*=station->varianceFactor;
            sigma_abs3_original*=station->varianceFactor;
            sigma_xy*=station->varianceFactor;
            sigma_xz*=station->varianceFactor;
            sigma_yz*=station->varianceFactor;

            station->observations.emplace_back(station->origin(),to,station,OBS_CODE::BASELINE_X,active1,value1_original,sqrt(sigma_abs1_original),
                                               0,station->stationHeight,target_height,
                                               line_number,1,"m",_file,comment);
            this->obs1=&station->observations.back();
            station->observations.emplace_back(station->origin(),to,station,OBS_CODE::BASELINE_Y,active2,value2_original,sqrt(sigma_abs2_original),
                                               0,station->stationHeight,target_height,
                                               line_number,1,"m",_file,comment);
            this->obs2=&station->observations.back();
            station->observations.emplace_back(station->origin(),to,station,OBS_CODE::BASELINE_Z,active3,value3_original,sqrt(sigma_abs3_original),
                                               0,station->stationHeight,target_height,
                                               line_number,1,"m",_file,comment);
            this->obs3=&station->observations.back();

            baselineXYZ << value1_original, value2_original, value3_original;//baseline in cartesian geocentric
            baselineVarCovar << fabs(sigma_abs1_original), sigma_xy, sigma_xz,
                                sigma_xy, fabs(sigma_abs2_original), sigma_yz,
                                sigma_xz, sigma_yz, fabs(sigma_abs3_original);
            //check if matrix is correct (all egenvalues positive)
            Eigen::Matrix<std::complex<tdouble>, Eigen::Dynamic, 1> eigenValues=baselineVarCovar.eigenvalues();
            for (int i=0; i<3; ++i)
            {
                if (eigenValues[i].real()<sqr(MINIMAL_SIGMA))
                {
                    std::cout<<"baselineVarCovar eigenvalues: "<<eigenValues<<std::endl;
                    Project::theInfo()->error(INFO_LS,1,QT_TRANSLATE_NOOP("QObject","Baseline covariance matrix from %s to %s (file %s:%d) is not positive-definite."),
                                              station->origin()->name.c_str(), to->name.c_str(),
                                              _file->get_name().c_str(), line_number);
                    ok = false;
                    return ok;
                }
            }
        }
        else
        {
            //Unknown type of bascule observation
            ok=false;
        }
    }

    return ok;
}

/**
  Set the 3d observation
  Using cartesian coordinates
  **/
bool Obs3D::set_obs(LeastSquares *lsquares, bool initialResidual, bool /*unused*/)
{
    static std::vector<tdouble> relations;
    static std::vector<int> positions;
    if (station->isGeocentric)
    {
        relations.resize(7);
        positions.resize(7);
    }else{
        if (station->isVertical)
        {
            relations.resize(8);
            positions.resize(8);
        }else{
            relations.resize(10);
            positions.resize(10);
        }
    }

    //forbid 1D and 2D points
    if ((obs1->from->dimension<3)||(obs1->to->dimension<3))
    {
        Project::theInfo()->warning(INFO_OBS,station->getFile().get_fileDepth(),QT_TRANSLATE_NOOP("QObject","Observation %s can't be used with non-3D point %s or %s."),
                                    obs1->toString().c_str(),obs1->from->name.c_str(),obs1->to->name.c_str());
        return false;
    }

    bool ok=true;

    Coord to_spherical_with_height, to_coord_compensated_globalCartesian, to_coord_compensated_vertCartesian;
    to_spherical_with_height = obs1->to->coord_comp + Coord(0, 0, obs1->targetHeight);
    if (Project::theone()->use_vertical_deflection && (obs1->targetHeight>CAP_CLOSE))
        to_spherical_with_height += Coord(obs1->targetHeight*obs1->to->dev_eta, obs1->targetHeight*obs1->to->dev_xi, 0);

    Project::theone()->projection.sphericalToGlobalCartesian(to_spherical_with_height,
                                                       to_coord_compensated_globalCartesian);
    Project::theone()->projection.globalCartesianToVertCartesian(to_coord_compensated_globalCartesian,to_coord_compensated_vertCartesian,
                                                             station->origin()->coord_comp, station->origin()->dev_eta, station->origin()->dev_xi);

    //computation is in vertCartesian
    //rotation between cartesian astro on station (where obs is made) and spherical frame on target
    Mat3 station2targetFrame = global2vertRot(obs1->to->coord_comp, 0, 0) *
            global2vertRot(station->origin()->coord_comp, station->origin()->dev_eta, station->origin()->dev_xi).transpose();
    //rotation between cartesian astro on station (where obs is made) and spherical frame on station
    Mat3 station2astro = vertDeflection2Rot(station->origin()->dev_eta, station->origin()->dev_xi);
    Mat3 ptJacobianCart, ptJacobianSpher;

    //M=target
    Vect3 M=to_coord_compensated_vertCartesian.toVect();//target
    //S=Station
    Vect3 S=Coord(0,0,obs1->instrumentHeight).toVect();//Station, no need for dev_eta, dev_xi for instrumentHeight since frame is already aligned

    //MS=M-S, compensated vector in vertical cartesian frame
    Vect3 MS=M-S;
    tdouble dist=MS.norm();

    //std::cout<<"to_coord_compensated_cartesian: "<<M.transpose()<<std::endl;

    if (dist<MINIMAL_DIVIDER)
    {
        ok=false;
        Project::theInfo()->warning(INFO_OBS,station->getFile().get_fileDepth(),QT_TRANSLATE_NOOP("QObject","Observation %s can't be used since the points are too close!"),
                                    obs1->toString().c_str());
        return ok;
    }

    obs1->D=dist;
    obs2->D=dist;
    obs3->D=dist;

    positions[0] = LeastSquares::B_index;//index of constant part
    positions[1]= obs1->from->params.at(0).rank; //Station
    positions[2]= obs1->from->params.at(1).rank;
    positions[3]= obs1->from->params.at(2).rank;
    positions[4]= obs1->to->params.at(0).rank;   //Target
    positions[5]= obs1->to->params.at(1).rank;
    positions[6]= obs1->to->params.at(2).rank;
    if (!station->isGeocentric)
    {
        positions[7]= station->params.at(0).rank;    //Rotation c
        if (!station->isVertical)
        {
            positions[8]= station->params.at(1).rank;//Rotation a
            positions[9]= station->params.at(2).rank;//Rotation b
        }
    }

    if (station->triplet_type==STATION_CODE::BASC_XYZ_CART)//Scanner observations
    {
        //U: compensated vector in laser frame
        Vect3 U=station->R_vert2instr*MS;

        obs1->sigmaTotal=obs1->sigmaAbs+obs1->sigmaRel*dist;
        obs2->sigmaTotal=obs2->sigmaAbs+obs2->sigmaRel*dist;
        obs3->sigmaTotal=obs3->sigmaAbs+obs3->sigmaRel*dist;

        //std::cout<<"basc obs: "<<U<<"\n  => "<<obs1->value<<" "<<obs2->value<<" "<<obs3->value<<"\n";
        if (Project::theone()->config.compute_type==COMPUTE_TYPE::type_monte_carlo)
        {
            obs1->residual = gauss_random(0,obs1->sigmaTotal);
            obs2->residual = gauss_random(0,obs2->sigmaTotal);
            obs3->residual = gauss_random(0,obs3->sigmaTotal);
        }
        else if (Project::theone()->config.compute_type==COMPUTE_TYPE::type_propagation)
        {
            obs1->residual = 0;
            obs2->residual = 0;
            obs3->residual = 0;
        }
        else
        {
            obs1->residual = U(0)-obs1->value;//residuals in instrument frame
            obs2->residual = U(1)-obs2->value;
            obs3->residual = U(2)-obs3->value;
        }
        obs1->normalizedResidual = obs1->residual/obs1->sigmaTotal;
        obs2->normalizedResidual = obs2->residual/obs2->sigmaTotal;
        obs3->normalizedResidual = obs3->residual/obs3->sigmaTotal;
        if (initialResidual)
        {
            obs1->initialNormalizedResidual=obs1->normalizedResidual;
            obs1->initialResidual=obs1->residual;
            obs2->initialNormalizedResidual=obs2->normalizedResidual;
            obs2->initialResidual=obs2->residual;
            obs3->initialNormalizedResidual=obs3->normalizedResidual;
            obs3->initialResidual=obs3->residual;
            //continue;
        }

        //MAxe(MS,Ax);
        Mat3 Ax=axiator(MS);
        //Mprd(R,Ax,V,3,3,3);
        Mat3 V=station->R_vert2instr*Ax;

        relations[0]  = obs1->residual;
        ptJacobianCart(0) = station->R_vert2instr(0,0);
        ptJacobianCart(1) = station->R_vert2instr(0,1);
        ptJacobianCart(2) = station->R_vert2instr(0,2);
        ptJacobianSpher = station2astro * ptJacobianCart;
        relations[1]  = -ptJacobianSpher(0);
        relations[2]  = -ptJacobianSpher(1);
        relations[3]  = -ptJacobianSpher(2);
        ptJacobianSpher = station2targetFrame * ptJacobianCart;
        relations[4]  = ptJacobianSpher(0);
        relations[5]  = ptJacobianSpher(1);
        relations[6]  = ptJacobianSpher(2);
        relations[7] = -V(0,2);
        if (!station->isVertical)
        {
            relations[8]  = -V(0,0);
            relations[9]  = -V(0,1);
        }
        if (obs1->active)
            ok = lsquares->add_constraint(obs1,relations,positions,obs1->sigmaTotal) && ok;

        relations[0]  = obs2->residual;
        ptJacobianCart(0) = station->R_vert2instr(1,0);
        ptJacobianCart(1) = station->R_vert2instr(1,1);
        ptJacobianCart(2) = station->R_vert2instr(1,2);
        ptJacobianSpher = station2astro * ptJacobianCart;
        relations[1]  = -ptJacobianSpher(0);
        relations[2]  = -ptJacobianSpher(1);
        relations[3]  = -ptJacobianSpher(2);
        ptJacobianSpher = station2targetFrame * ptJacobianCart;
        relations[4]  = ptJacobianSpher(0);
        relations[5]  = ptJacobianSpher(1);
        relations[6]  = ptJacobianSpher(2);
        relations[7] = -V(1,2);
        if (!station->isVertical)
        {
            relations[8]  = -V(1,0);
            relations[9]  = -V(1,1);
        }
        if (obs2->active)
            ok = lsquares->add_constraint(obs2,relations,positions,obs2->sigmaTotal) && ok;

        relations[0]  = obs3->residual;
        ptJacobianCart(0) = station->R_vert2instr(2,0);
        ptJacobianCart(1) = station->R_vert2instr(2,1);
        ptJacobianCart(2) = station->R_vert2instr(2,2);
        ptJacobianSpher = station2astro * ptJacobianCart;
        relations[1]  = -ptJacobianSpher(0);
        relations[2]  = -ptJacobianSpher(1);
        relations[3]  = -ptJacobianSpher(2);
        ptJacobianSpher = station2targetFrame * ptJacobianCart;
        relations[4]  = ptJacobianSpher(0);
        relations[5]  = ptJacobianSpher(1);
        relations[6]  = ptJacobianSpher(2);
        relations[7] = -V(2,2);
        if (!station->isVertical)
        {
            relations[8]  = -V(2,0);
            relations[9]  = -V(2,1);
        }
        if (obs3->active)
            ok = lsquares->add_constraint(obs3,relations,positions,obs3->sigmaTotal) && ok;
    }
    else if (station->triplet_type==STATION_CODE::BASC_ANG_CART)//Tracker observations
    {
        //U: compensated vector in laser frame
        Vect3 U=station->R_vert2instr*MS;

        Coord coordFrame=currVectorToInstrumentFrameCoords();
        obs1->sigmaTotal=obs1->sigmaAbs+obs1->sigmaRel/dist;
        obs2->sigmaTotal=obs2->sigmaAbs+obs2->sigmaRel/dist;
        obs3->sigmaTotal=obs3->sigmaAbs+obs3->sigmaRel*dist;

        tdouble dist_pseudo_plani2=coordFrame.x()*coordFrame.x()+coordFrame.y()*coordFrame.y();
        tdouble dist_pseudo_plani=sqrt(dist_pseudo_plani2);
        tdouble dist2=sqr(dist);

        Mat3 cov;
        cov(0,0) = -coordFrame.y()/dist_pseudo_plani2; // Hz % x
        cov(0,1) =  coordFrame.x()/dist_pseudo_plani2; // Hz % y
        cov(0,2) =                      0; // Hz % z

        cov(1,0) = -coordFrame.x()*coordFrame.z()/(dist2*dist_pseudo_plani); // Zen % x
        cov(1,1) = -coordFrame.y()*coordFrame.z()/(dist2*dist_pseudo_plani); // Zen % y
        cov(1,2) =          dist_pseudo_plani / dist2; // Zen % z

        cov(2,0) =  coordFrame.x()/dist; // Dist % x
        cov(2,1) =  coordFrame.y()/dist; // Dist % y
        cov(2,2) =  coordFrame.z()/dist; // Dist % z

        //std::cout<<"basc obs: "<<U<<"\n  => "<<obs1->value<<" "<<obs2->value<<" "<<obs3->value<<"\n";
        if (Project::theone()->config.compute_type==COMPUTE_TYPE::type_monte_carlo)
        {
            obs1->residual = gauss_random(0,obs1->sigmaTotal);
            obs2->residual = gauss_random(0,obs2->sigmaTotal);
            obs3->residual = gauss_random(0,obs3->sigmaTotal);
        }
        else if (Project::theone()->config.compute_type==COMPUTE_TYPE::type_propagation)
        {
            obs1->residual = 0;
            obs2->residual = 0;
            obs3->residual = 0;
        }
        else
        {
            obs1->residual = atan2(U(1),U(0))-obs1->value-2*PI;
            normalizeAngle(obs1->residual);
            obs2->residual = atan2(U(2),sqrt(U(1)*U(1) + U(0)*U(0)))-obs2->value-2*PI;
            normalizeAngle(obs2->residual);
            obs3->residual = sqrt(U(0)*U(0)+U(1)*U(1)+U(2)*U(2))-obs3->value;

        }
        obs1->normalizedResidual = obs1->residual/obs1->sigmaTotal;
        obs2->normalizedResidual = obs2->residual/obs2->sigmaTotal;
        obs3->normalizedResidual = obs3->residual/obs3->sigmaTotal;
        if (initialResidual)
        {
            obs1->initialNormalizedResidual=obs1->normalizedResidual;
            obs1->initialResidual=obs1->residual;
            obs2->initialNormalizedResidual=obs2->normalizedResidual;
            obs2->initialResidual=obs2->residual;
            obs3->initialNormalizedResidual=obs3->normalizedResidual;
            obs3->initialResidual=obs3->residual;
            //continue;
        }

        //MAxe(MS,Ax);
        Mat3 Ax=axiator(MS);
        //Mprd(R,Ax,V,3,3,3);
        Mat3 V=station->R_vert2instr*Ax;
        //Mprd(Cov,R,RR,3,3,3);
        Mat3 RR=cov*station->R_vert2instr;
        //Mprd(Cov,V,V,3,3,3);
        Mat3 W=cov*V;

        relations[0]  = obs1->residual;
        ptJacobianCart(0) = RR(0,0);
        ptJacobianCart(1) = RR(0,1);
        ptJacobianCart(2) = RR(0,2);
        ptJacobianSpher = station2astro * ptJacobianCart;
        relations[1]  = -ptJacobianSpher(0);
        relations[2]  = -ptJacobianSpher(1);
        relations[3]  = -ptJacobianSpher(2);
        ptJacobianSpher = station2targetFrame * ptJacobianCart;
        relations[4]  = ptJacobianSpher(0);
        relations[5]  = ptJacobianSpher(1);
        relations[6]  = ptJacobianSpher(2);
        relations[7] = -W(0,2);
        if (!station->isVertical)
        {
            relations[8]  = -W(0,0);
            relations[9]  = -W(0,1);
        }
        if (obs1->active)
            ok = lsquares->add_constraint(obs1,relations,positions,obs1->sigmaTotal) && ok;

        relations[0]  = obs2->residual;
        ptJacobianCart(0) = RR(1,0);
        ptJacobianCart(1) = RR(1,1);
        ptJacobianCart(2) = RR(1,2);
        ptJacobianSpher = station2astro * ptJacobianCart;
        relations[1]  = -ptJacobianSpher(0);
        relations[2]  = -ptJacobianSpher(1);
        relations[3]  = -ptJacobianSpher(2);
        ptJacobianSpher = station2targetFrame * ptJacobianCart;
        relations[4]  = ptJacobianSpher(0);
        relations[5]  = ptJacobianSpher(1);
        relations[6]  = ptJacobianSpher(2);
        relations[7] = -W(1,2);
        if (!station->isVertical)
        {
            relations[8]  = -W(1,0);
            relations[9]  = -W(1,1);
        }
        if (obs2->active)
            ok = lsquares->add_constraint(obs2,relations,positions,obs2->sigmaTotal) && ok;

        relations[0]  = obs3->residual;
        ptJacobianCart(0) = RR(2,0);
        ptJacobianCart(1) = RR(2,1);
        ptJacobianCart(2) = RR(2,2);
        ptJacobianSpher = station2astro * ptJacobianCart;
        relations[1]  = -ptJacobianSpher(0);
        relations[2]  = -ptJacobianSpher(1);
        relations[3]  = -ptJacobianSpher(2);
        ptJacobianSpher = station2targetFrame * ptJacobianCart;
        relations[4]  = ptJacobianSpher(0);
        relations[5]  = ptJacobianSpher(1);
        relations[6]  = ptJacobianSpher(2);
        relations[7] = -W(2,2);
        if (!station->isVertical)
        {
            relations[8]  = -W(2,0);
            relations[9]  = -W(2,1);
        }
        if (obs3->active)
            ok = lsquares->add_constraint(obs3,relations,positions,obs3->sigmaTotal) && ok;
    }
    else if (station->triplet_type==STATION_CODE::BASELINE_GEO_XYZ)//Baseline observations
    {
        //compute current rotation from geocentric to station vertical
        Mat3 R_global2vert=global2vertRot(obs1->from->coord_comp, obs1->from->dev_eta, obs1->from->dev_xi);
        Mat3 R = R_global2vert * Project::theone()->projection.RotGlobal2Geocentric.transpose();
        //baseline in cartesian vertical to station
        Vect3 V=obsToInstrumentFrameCoords(Project::theone()->config.compute_type != COMPUTE_TYPE::type_compensation).toVect();
        std::cout<<"Baseline "<<obs1->from->name<<"->"<<obs1->to->name<<" in vert frame: "<<V.transpose()<<std::endl;
        std::cout<<"diff coords: in vert frame: "<<MS.transpose()<<std::endl;

        positions[0] = LeastSquares::B_index;//index of constant part
        positions[1]= obs1->from->params.at(0).rank; //Station
        positions[2]= obs1->from->params.at(1).rank;
        positions[3]= obs1->from->params.at(2).rank;
        positions[4]= obs1->to->params.at(0).rank;   //Target
        positions[5]= obs1->to->params.at(1).rank;
        positions[6]= obs1->to->params.at(2).rank;

        //rotate varCovar matrix
        Mat3 varCovar_vert;
        varCovar_vert = R * baselineVarCovar * R.transpose();

        obs1->value = obs1->originalValue = V(0);
        obs2->value = obs2->originalValue = V(1);
        obs3->value = obs3->originalValue = V(2);
        obs1->sigmaTotal=sqrt(varCovar_vert(0,0));
        obs2->sigmaTotal=sqrt(varCovar_vert(1,1));
        obs3->sigmaTotal=sqrt(varCovar_vert(2,2));

        if ((std::isnan(obs1->sigmaTotal))
                ||(std::isnan(obs2->sigmaTotal))
                ||(std::isnan(obs3->sigmaTotal)))
        {
            Project::theInfo()->error(INFO_LS,1,
                                      QT_TRANSLATE_NOOP("QObject","Baseline covariance matrix from  %s to %s "
                                                                  "(file %s:%d) is not positive-definite."),
                                      obs1->from->name.c_str(), obs1->to->name.c_str(),
                                      obs1->file->get_name().c_str(), obs1->lineNumber);
            std::cout<<"R:\n"<<R<<"\n";
            std::cout<<"baselineVarCovar:\n"<<baselineVarCovar<<std::endl;
            ok = false;
            return ok;
        }

        if (Project::theone()->config.compute_type==COMPUTE_TYPE::type_monte_carlo)
        {
            obs1->residual = gauss_random(0,obs1->sigmaTotal);
            obs2->residual = gauss_random(0,obs2->sigmaTotal);
            obs3->residual = gauss_random(0,obs3->sigmaTotal);
        }
        else if (Project::theone()->config.compute_type==COMPUTE_TYPE::type_propagation)
        {
            obs1->residual = 0;
            obs2->residual = 0;
            obs3->residual = 0;
        }
        else
        {
            obs1->residual = MS(0)-V(0);//residuals in instrument frame
            obs2->residual = MS(1)-V(1);
            obs3->residual = MS(2)-V(2);
        }
        obs1->normalizedResidual = obs1->residual/obs1->sigmaTotal;
        obs2->normalizedResidual = obs2->residual/obs2->sigmaTotal;
        obs3->normalizedResidual = obs3->residual/obs3->sigmaTotal;
        if (initialResidual)
        {
            obs1->initialNormalizedResidual=obs1->normalizedResidual;
            obs1->initialResidual=obs1->residual;
            obs2->initialNormalizedResidual=obs2->normalizedResidual;
            obs2->initialResidual=obs2->residual;
            obs3->initialNormalizedResidual=obs3->normalizedResidual;
            obs3->initialResidual=obs3->residual;
            //continue;
        }

        relations[0]  = obs1->residual;
        ptJacobianCart(0) = 1;
        ptJacobianCart(1) = 0;
        ptJacobianCart(2) = 0;
        ptJacobianSpher = station2astro * ptJacobianCart;
        relations[1]  = -ptJacobianSpher(0);
        relations[2]  = -ptJacobianSpher(1);
        relations[3]  = -ptJacobianSpher(2);
        ptJacobianSpher = station2targetFrame * ptJacobianCart;
        relations[4]  = ptJacobianSpher(0);
        relations[5]  = ptJacobianSpher(1);
        relations[6]  = ptJacobianSpher(2);
        if (obs1->active)
            ok = lsquares->add_constraint(obs1,relations,positions,obs1->sigmaTotal) && ok;

        relations[0]  = obs2->residual;
        ptJacobianCart(0) = 0;
        ptJacobianCart(1) = 1;
        ptJacobianCart(2) = 0;
        ptJacobianSpher = station2astro * ptJacobianCart;
        relations[1]  = -ptJacobianSpher(0);
        relations[2]  = -ptJacobianSpher(1);
        relations[3]  = -ptJacobianSpher(2);
        ptJacobianSpher = station2targetFrame * ptJacobianCart;
        relations[4]  = ptJacobianSpher(0);
        relations[5]  = ptJacobianSpher(1);
        relations[6]  = ptJacobianSpher(2);
        if (obs2->active)
            ok = lsquares->add_constraint(obs2,relations,positions,obs2->sigmaTotal) && ok;

        relations[0]  = obs3->residual;
        ptJacobianCart(0) = 0;
        ptJacobianCart(1) = 0;
        ptJacobianCart(2) = 1;
        ptJacobianSpher = station2astro * ptJacobianCart;
        relations[1]  = -ptJacobianSpher(0);
        relations[2]  = -ptJacobianSpher(1);
        relations[3]  = -ptJacobianSpher(2);
        ptJacobianSpher = station2targetFrame * ptJacobianCart;
        relations[4]  = ptJacobianSpher(0);
        relations[5]  = ptJacobianSpher(1);
        relations[6]  = ptJacobianSpher(2);
        if (obs3->active)
            ok = lsquares->add_constraint(obs3,relations,positions,obs3->sigmaTotal) && ok;

        //std::cout<<"varCovar_vert: \n"<<varCovar_vert<<std::endl;
        if ((obs1->active)&&(obs2->active)&&(obs3->active))
            ok = lsquares->add_covariance({obs1, obs2, obs3}, varCovar_vert) && ok;
    }
    else
    {
        //Unknown type of bascule observation
        ok=false;
    }
    return ok;
}

Vect3 Obs3D::obsToVectGlobalCart(bool simul)
{
    Coord vect_instr = obsToInstrumentFrameCoords(simul);
    tdouble norm = sqrt(vect_instr.norm2());
    if (norm==0.0)
    {
        if (station->triplet_type==STATION_CODE::BASC_ANG_CART)
        {
            vect_instr.setx(cos(obs2->value) * cos(obs1->value)); //cosPhi*cosTeta);
            vect_instr.sety(cos(obs2->value) * sin(obs1->value)); //cosPhi*sinTeta
            vect_instr.setz(sin(obs2->value));          //sinPhi;
        } else {
            Project::theInfo()->warning(INFO_OBS,1,
                                        "obsToInstrumentFrameCoords norm should not be 0!");
            //std::cout<<obs3->toString()<<"\n";
            return {0,0,0};
        }
    } else {
        vect_instr /= norm;
    }
    Mat3 Rvert2global = global2vertRot(station->origin()->coord_comp, station->origin()->dev_eta, station->origin()->dev_xi).transpose();
    return Rvert2global * station->R_vert2instr.transpose()*vect_instr.toVect();
}

//< get vector between coord_comp in instrument frame
Coord Obs3D::currVectorToInstrumentFrameCoords()
{
    Coord target_in_instr_coords;

    Coord from_coord_compensated_vertCartesian(0,0,obs1->instrumentHeight);

    Coord to_compensated_spherical_with_height = obs1->to->coord_comp + Coord(0, 0, obs1->targetHeight);
    if (Project::theone()->use_vertical_deflection && (obs1->targetHeight > CAP_CLOSE))
            to_compensated_spherical_with_height += Coord(obs1->targetHeight*obs1->to->dev_eta,
                                                          obs1->targetHeight*obs1->to->dev_xi,0);

    Coord to_coord_compensated_globalCartesian, to_coord_compensated_vertCartesian;
    Project::theone()->projection.sphericalToGlobalCartesian(to_compensated_spherical_with_height, to_coord_compensated_globalCartesian);
    Project::theone()->projection.globalCartesianToVertCartesian(to_coord_compensated_globalCartesian,to_coord_compensated_vertCartesian,
                                                                 station->origin()->coord_comp, station->origin()->dev_eta, station->origin()->dev_xi);

    return Coord(station->R_vert2instr*(to_coord_compensated_vertCartesian-from_coord_compensated_vertCartesian).toVect());
}

//< get coordinates of the target point in instrument frame, do not check for active obs
Coord Obs3D::obsToInstrumentFrameCoords(bool simul)
{
    Coord target_in_instr_coords;
    if (simul)//act as if the station has the same orientation as the ground
    {
        Coord from_coord_compensated_vertCartesian(0,0,obs1->instrumentHeight);

        Coord to_compensated_spherical_with_height = obs1->to->coord_comp + Coord(0, 0, obs1->targetHeight);
        if (Project::theone()->use_vertical_deflection && (obs1->targetHeight > CAP_CLOSE))
                to_compensated_spherical_with_height += Coord(obs1->targetHeight*obs1->to->dev_eta,
                                                              obs1->targetHeight*obs1->to->dev_xi,0);

        Coord to_coord_compensated_globalCartesian, to_coord_compensated_vertCartesian;
        Project::theone()->projection.sphericalToGlobalCartesian(to_compensated_spherical_with_height, to_coord_compensated_globalCartesian);
        Project::theone()->projection.globalCartesianToVertCartesian(to_coord_compensated_globalCartesian,to_coord_compensated_vertCartesian,
                                                                     station->origin()->coord_comp, station->origin()->dev_eta, station->origin()->dev_xi);

        target_in_instr_coords.setx(to_coord_compensated_vertCartesian.x()-from_coord_compensated_vertCartesian.x());
        target_in_instr_coords.sety(to_coord_compensated_vertCartesian.y()-from_coord_compensated_vertCartesian.y());
        target_in_instr_coords.setz(to_coord_compensated_vertCartesian.z()-from_coord_compensated_vertCartesian.z());
    }else{
        if (station->triplet_type==STATION_CODE::BASC_XYZ_CART)//Scanner observations
        {
            target_in_instr_coords.setx(obs1->value);
            target_in_instr_coords.sety(obs2->value);
            target_in_instr_coords.setz(obs3->value);
        }
        else if (station->triplet_type==STATION_CODE::BASC_ANG_CART)//Tracker observations
        {
            target_in_instr_coords.setx(obs3->value * cos(obs2->value) * cos(obs1->value)); //D*cosPhi*cosTeta);
            target_in_instr_coords.sety(obs3->value * cos(obs2->value) * sin(obs1->value)); //D*cosPhi*sinTeta
            target_in_instr_coords.setz(obs3->value * sin(obs2->value));          //D*sinPhi;
        }
        else if (station->triplet_type==STATION_CODE::BASELINE_GEO_XYZ)//baseline observations
        {
            Mat3 R_global2vert;
            if (obs1->from->isInit())
                R_global2vert=global2vertRot(obs1->from->coord_comp, obs1->from->dev_eta, obs1->from->dev_xi);
            else
                R_global2vert=Mat3::Identity(); //approximation for init
            Mat3 R = R_global2vert * Project::theone()->projection.RotGlobal2Geocentric.transpose();
            Vect3 V = R * baselineXYZ; //baseline in cartesian vertical to station
            target_in_instr_coords.set(V);
        }
        else
        {
            //Unknown type of bascule observation
            std::cout<<"obsToStationFrameCoords: Unknown type of bascule observation"<<std::endl;
        }
    }
    return target_in_instr_coords;
}

//--------------------------------------------------------------------------

Station_Bascule::Station_Bascule(Point *origin):Station(origin, STATION_TYPE::ST_BASCULE),isVertical(false),isGeocentric(false),
    da(0.0),db(0.0),dc(0.0),a(0.0),b(0.0),c(0.0),R_vert2instr(Mat3::Zero()),
    stationHeight(0.0),varianceFactor(1.0),triplet_type(STATION_CODE::BASC_XYZ_CART),file(nullptr)
{
}


std::string Station_Bascule::typeStr() const
{
    return "bascule";
}

bool Station_Bascule::initialize(bool verbose)
{
    if (isGeocentric)
    {
        if (Project::theone()->projection.type!=PROJ_TYPE::PJ_GEO)
            return false;

        R_vert2instr=Mat3::Identity();
        mInitOk=false;
        //initialize from point if not already done
        if (!origin()->isInit())
        {
            for (auto & obs3d : observations3D) //find one obs on an init point
            {
                if (obs3d.obs1->to->isInit())
                {
                    Coord to_globalCart;
                    Project::theone()->projection.sphericalToGlobalCartesian(obs3d.obs1->to->coord_comp,to_globalCart);
                    Coord init_globalCart=to_globalCart-obs3d.obsToInstrumentFrameCoords(false);
                    Coord coord_spher;
                    Project::theone()->projection.globalCartesianToSpherical(init_globalCart,coord_spher);
                    Coord coord_read;
                    Project::theone()->projection.sphericalToGeoref(coord_spher,coord_read);
                    origin()->set_point(coord_read,Coord(0,0,0),CR_CODE::FREE,true);
                    origin()->coord_comp=origin()->coord_init_spher;
                    mInitOk=true;
                    #ifdef SHOW_CAP
                      Project::theInfo()->msg(INFO_CAP,1,QT_TRANSLATE_NOOP("QObject","Initialize point %s as a station."),origin()->name.c_str());
                    #endif
                      Project::theone()->cor_root->addPoint(origin());//add it to root cor file for .new
                    break;
                }
            }
        } else {
            mInitOk=true;
        }
        return true;
    }

    if ((triplet_type==STATION_CODE::BASC_ANG_CART) && (origin()->isInit()))
        for (auto & obs3d : observations3D)
        {
            if (obs3d.obs3->to->isInit()) {
                if ((!obs3d.obs3->active) && (obs3d.obs3->originalValue == 0.0)) // automatically update dist if photogrammetry obs
                {
                    Coord from_cart, to_cart;
                    Project::theone()->projection.sphericalToGlobalCartesian(obs3d.obs3->from->coord_comp, from_cart);
                    Project::theone()->projection.sphericalToGlobalCartesian(obs3d.obs3->to->coord_comp, to_cart);
                    obs3d.obs3->value = sqrt((from_cart - to_cart).norm2());
                    //std::cout<<"fix dist "<<obs3d.obs3->toString()<<": "<<obs3d.obs3->value<<"\n";
                }
            }
        }

    //std::cout<<"Thomson_Shut "<<origin()->name<<"\n";
    //if (isVertical) std::cout<<"    vertical\n";
    Mat3 R_Thomson_Shut;
    std::vector<Coord> Xm;//instr obs coords (for constrained points)
    std::vector<Coord> Xt;//global coords
    std::vector<Coord> Xm2;//instr obs coords (for free points, if not enought constrained points)
    std::vector<Coord> Xt2;//global coords
    //std::cout<<"Pts: ";
    for (auto & obs3d : observations3D)
    {
        if (obs3d.obs1->to->isInit())
        {
            Coord to_compensated_spherical_with_height = obs3d.obs1->to->coord_comp + Coord(0, 0, obs3d.obs1->targetHeight);
            if (Project::theone()->use_vertical_deflection &&(obs3d.obs1->targetHeight > CAP_CLOSE))
                    to_compensated_spherical_with_height += Coord(obs3d.obs1->targetHeight*obs3d.obs1->to->dev_eta,
                                                                  obs3d.obs1->targetHeight*obs3d.obs1->to->dev_xi,0);

            Coord to_compensated_globalCartesian, to_compensated_vertCartesian;
            Project::theone()->projection.sphericalToGlobalCartesian(to_compensated_spherical_with_height, to_compensated_globalCartesian);
            Project::theone()->projection.globalCartesianToVertCartesian(to_compensated_globalCartesian, to_compensated_vertCartesian,
                                                                         origin()->coord_comp, origin()->dev_eta, origin()->dev_xi);
            //std::cout<<obs3d.obs1->to->name<<" ";
            if (obs3d.obs1->to->isFree())
            {
                Xm2.push_back(to_compensated_vertCartesian-Coord(0,0,obs3d.obs1->instrumentHeight));
                Xt2.push_back(obs3d.obsToInstrumentFrameCoords(Project::theone()->config.compute_type!=COMPUTE_TYPE::type_compensation));
            } else {
                Xm.push_back(to_compensated_vertCartesian-Coord(0,0,obs3d.obs1->instrumentHeight));
                Xt.push_back(obs3d.obsToInstrumentFrameCoords(Project::theone()->config.compute_type!=COMPUTE_TYPE::type_compensation));
            }
        }
    }

    if ((Xm.size()<3) || pointsAreAligned(Xm))
    {
        Xm.insert(Xm.end(), Xm2.begin(), Xm2.end());
        Xt.insert(Xt.end(), Xt2.begin(), Xt2.end());
    }

    //std::cout<<Xm.size()<<" mes"<<"\n";
    if (isVertical && !Xm.empty())
    {
        Xt.push_back(Xt[0]);
        Xm.push_back(Xm[0]);
        Xt.back().setz(Xt[0].z()+10);Xm.back().setz(Xm[0].z()+10);//one fake obs for verticalization
    }

//    for (unsigned int i=0;i<Xm.size();i++)
//        std::cout<<"   "<<Xt[i].toString(3)<<" "<<Xm[i].toString(3)<<std::endl;

    Vect3 T;
    tdouble scale = NAN;

    if ( (Xm.size()<3) || (!Thomson_Shut(Xt,Xm,R_Thomson_Shut,T,scale)) )
    {
        //std::cout<<"Impossible to initialize station bascule "<<from->name<<"!"<<std::endl;
        if (verbose)
            Project::theInfo()->warning(INFO_CAP,1,
                                        QT_TRANSLATE_NOOP("QObject","Impossible to initialize "
                                                                    "station bascule %s!"),
                                        origin()->name.c_str());
        mInitOk=false;
        return false;
    }

    Coord coord_compensated_cartesian_previous;
    Project::theone()->projection.sphericalToGlobalCartesian(origin()->coord_comp,coord_compensated_cartesian_previous);
    Coord coord_compensated_cartesian(T(0),T(1),T(2));
    coord_compensated_cartesian+=coord_compensated_cartesian_previous;
    Project::theone()->projection.globalCartesianToSpherical(coord_compensated_cartesian,origin()->coord_comp);
    if (!origin()->isInit()) // coord_read is used for CAP
        Project::theone()->projection.sphericalToGeoref(origin()->coord_comp,origin()->coord_read);

    tdouble theta = NAN;
    R_vert2instr=R_Thomson_Shut.transpose() ;
    R2abc(R_vert2instr,theta,a,b,c);
    if (isVertical)
    {
        a=0;b=0;//force matrix to be only a rotation around Z
        if (fabs(theta)<0.000001)
            c=1;
        abc2R(a,b,c,theta,R_vert2instr);
    }
    da=0;db=0;dc=0;
    mInitOk=true;

    //initialize from point if not already done
    if (!origin()->isInit())
    {
        origin()->set_point(origin()->coord_read,Coord(0,0,0),CR_CODE::FREE,true);
      #ifdef SHOW_CAP
        Project::theInfo()->msg(INFO_CAP,1,
                                QT_TRANSLATE_NOOP("QObject","Initialize point %s as a station."),
                                origin()->name.c_str());
      #endif
        Project::theone()->cor_root->addPoint(origin());//add it to root cor file for .new
    }

    //std::cout<<"Init of bascule station. Params= "<<a<<" "<<b<<" "<<c<<" "<<theta<<"\n";
    //std::cout<<"Coord init: "<<from->coord_read.toString()<<"\n";
    return true;
}

void Station_Bascule::update()
{
    tdouble dtheta= sqrt(sqr(da)+sqr(db)+sqr(dc));
    Mat3 dR;
    abc2R(da,db,dc,dtheta,dR);
    R_vert2instr=R_vert2instr*dR;
    tdouble theta=0;
    R2abc(R_vert2instr,theta,a,b,c);
    da=0;db=0;dc=0;
}

Json::Value Station_Bascule::toJson(FileRefJson &fileRef) const
{
    if ((Project::theone()->compensationDone) && (!isGeocentric))
        file->finalizeFile();
    tdouble _a = NAN,_b = NAN,_c = NAN,_theta = NAN;
    R2abc(R_vert2instr,_theta,_a,_b,_c);

    Json::Value val = Station::toJson(fileRef);
    
    val["file_id"]=fileRef.getNumber(file.get());
    Json::Value val_params;
    val_params["a"]=static_cast<double>(_a);
    val_params["b"]=static_cast<double>(_b);
    val_params["c"]=static_cast<double>(_c);
    val_params["theta_rd"]=static_cast<double>(_theta);

    Mat3 R_global2vert=global2vertRot(origin()->coord_comp, origin()->dev_eta, origin()->dev_xi);
    Mat3 R_global2instr=R_vert2instr*R_global2vert;

    //Matrix between vertical on station point and instrument frame (should be I if bubbuled)
    for (unsigned int i=0;i<3;i++)
        for (unsigned int j=0;j<3;j++)
            val_params["R_vert2instr"].append(R_vert2instr(i,j));

    //Matrix between instrument frame and global cartesian frame
    for (unsigned int i=0;i<3;i++)
        for (unsigned int j=0;j<3;j++)
            val_params["R_global2instr"].append(R_global2instr(i,j));

    if (!isGeocentric)
        val["params_rot"]=val_params;

    val["observations3D"]=Json::arrayValue;
    for (const auto &obs3d : observations3D)
        val["observations3D"].append(obs3d.toJson());

    val["vertical"]=isVertical;
    val["geocentric"]=isGeocentric;
    val["triplet_type"]=static_cast<int>(triplet_type);

    if (isGeocentric)
    {
        val["station_height"]=static_cast<double>(stationHeight);
        val["variance_factor"]=static_cast<double>(varianceFactor);
    }

    val["ang_to_vert"]=angleInstr2Vert()/toRad(1.0,Project::theone()->config.filesUnit);
    return val;
}

tdouble Station_Bascule::angleInstr2Vert() const
{
    VectX v(3);
    v << 0, 0, 1.0;
    VectX u=R_vert2instr*v;

    tdouble dot_prod=v[0]*u[0]+v[1]*u[1]+v[2]*u[2];
    tdouble ang_to_vert=0;
    if (fabs(dot_prod)<1) ang_to_vert=acos(dot_prod);
    if (ang_to_vert<0.01) //use sin if angle is small
    {
        tdouble normxy = sqrt(u[0]*u[0]+u[1]*u[1]);
        ang_to_vert=asin(normxy);
    }
    return ang_to_vert;
}

bool Station_Bascule::isInternal(Obs* /*obs*/)//if the obs is compatible with internal constraints
{
    return true;
}

bool Station_Bascule::isOnlyLeveling(Obs* /*obs*/)//for internal constraints
{
    return false;
}

bool Station_Bascule::isHz(Obs* obs)//for internal constraints
{
    //only baselines with noticable planimetric composant fix hz...
    if (obs->code!=OBS_CODE::BASELINE_X && obs->code!=OBS_CODE::BASELINE_Y && obs->code!=OBS_CODE::BASELINE_Z)
        return false;

    for (auto & obs3d : observations3D)
    {
        if ((obs==obs3d.obs1)||(obs==obs3d.obs2)||(obs==obs3d.obs3))
        {
            Coord v = obs3d.obsToInstrumentFrameCoords(Project::theone()->config.compute_type != COMPUTE_TYPE::type_compensation);
            return sqrt(sqr(v.x())+sqr(v.y()))>CAP_CLOSE;
        }
    }
    return false;
}

bool Station_Bascule::isDistance(Obs* obs)//for internal constraints
{
    return ((obs->code==OBS_CODE::BASCULE_DIST)||(obs->code==OBS_CODE::BASCULE_X)||(obs->code==OBS_CODE::BASCULE_Y)||(obs->code==OBS_CODE::BASCULE_Z)
            ||(obs->code==OBS_CODE::BASELINE_X)||(obs->code==OBS_CODE::BASELINE_Y)||(obs->code==OBS_CODE::BASELINE_Z));
}

bool Station_Bascule::isBubbuled(Obs* /*obs*/)//for internal constraints
{
    return isVertical;
}

bool Station_Bascule::useVertDeflection(Obs* obs)//to check vertical deflection consistancy
{
    return (!isGeocentric) || (fabs(obs->instrumentHeight)>CAP_CLOSE); //vert defl is used to compute station deflection
}


int Station_Bascule::numberOfBasicObs(Obs* /*obs*/)//number of lines in the matrix
{
    return 1;
}

void Station_Bascule::changeObsActivation(Obs& obs, bool active)
{
    if (active)
        obs.active = true;
    else {
        if (triplet_type==STATION_CODE::BASELINE_GEO_XYZ) //semi-active baselines not allowed
        {
            for (auto & obs3d : observations3D)
            {
                if ((&obs==obs3d.obs1)||(&obs==obs3d.obs2)||(&obs==obs3d.obs3))
                {   // deactivate all obs3d
                    obs3d.obs1->active = false;
                    obs3d.obs2->active = false;
                    obs3d.obs3->active = false;
                }
            }
        }
    }

}


bool Station_Bascule::read_obs(std::string line,
                              int line_number, DataFile *_file, const std::string &current_absolute_path, const std::string & /*comment*/)
{
    bool ok=true;
    std::string from_name;
    std::string filename;
    int code_int=-1;

  #ifdef OBS_INFO
    std::cout<<"Try to read obs: "<<line<<std::endl;
  #endif
    std::istringstream iss(line);
    if (!(iss >> code_int))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth(),
                                    QT_TRANSLATE_NOOP("QObject","At line %d: %s => "
                                                                " Can't convert observation code."),
                                    line_number,line.c_str());
        ok=false;
    }
    triplet_type=static_cast<STATION_CODE>(code_int);
    if ((triplet_type!=STATION_CODE::BASC_XYZ_CART)&&(triplet_type!=STATION_CODE::BASC_ANG_CART)&&(triplet_type!=STATION_CODE::BASELINE_GEO_XYZ))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth(),QT_TRANSLATE_NOOP("QObject","At line %d: %s => Line code is incorrect."),line_number,line.c_str());
        ok=false;
    }

    isGeocentric=(triplet_type==STATION_CODE::BASELINE_GEO_XYZ);

    if (isGeocentric && (Project::theone()->projection.type!=PROJ_TYPE::PJ_GEO))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth(),QT_TRANSLATE_NOOP("QObject","At line %d: %s => Baselines not available in local compensation."),line_number,line.c_str());
        ok=false;
    }

    if (!(iss >> from_name))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth(),QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert from point name."),line_number,line.c_str());
        ok=false;
    }

    //find a point with that name in project
    mOrigin=Project::theone()->getPoint(from_name,true);
    if (origin()->dimension<0)
    {
        //error_msg<<QObject::tr("At line ").toCstr()<<line_number<<": "<<line<<" => ";
        //error_msg<<QObject::tr("Can't find a point named \"").toCstr()<<from_name<<"\".\n";
    }
    //ok=false;

    if (!(iss >> filename))
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth(),QT_TRANSLATE_NOOP("QObject","At line %d: %s => Can't convert file name."),line_number,line.c_str());
        ok=false;
    }
    if (filename.at(0)!='@')
    {
        Project::theInfo()->warning(INFO_OBS,_file->get_fileDepth(),QT_TRANSLATE_NOOP("QObject","At line %d: %s => \'@\' needed before sub file name."),line_number,line.c_str());
        ok=false;
    }
    filename.erase(0,1); //remove first character (@)

    if (!isGeocentric) //test if bascule is verticalized
    {
        int vertical_int=-1;
        if (iss >> vertical_int)
            isVertical=(vertical_int>=0);
    }else{ //test if variance factor and station height
        if ((iss >> varianceFactor))
            iss >> stationHeight;
        varianceFactor*=varianceFactor;//varianceFactor=K**2
        isVertical=false;
    }

    if (ok)
    {
        //std::cout<<"Will read "<<current_absolute_path<<"/"<<filename<<std::endl;
        file = XYZFile::create(filename,this,current_absolute_path,line_number,_file->get_fileDepth()+1);

        ok=file->read();

        if (!observations3D.empty())
        {
            if (!isGeocentric) //no parameters for geocentric
            {
                //create parameters
                params.clear();
                std::ostringstream oss_dc;
                oss_dc<<origin()->name<<"_Rc"<<"_"<<num;
                params.emplace_back(oss_dc.str(),&dc,1.0,"",origin());
                if (!isVertical)
                {
                    std::ostringstream oss_da;
                    oss_da<<origin()->name<<"_Ra"<<"_"<<num;
                    params.emplace_back(oss_da.str(),&da,1.0,"",origin());
                    std::ostringstream oss_db;
                    oss_db<<origin()->name<<"_Rb"<<"_"<<num;
                    params.emplace_back(oss_db.str(),&db,1.0,"",origin());
                }
            }
        }
        if (observations.empty())
        {
            Project::theInfo()->warning(INFO_OBS,getFile().get_fileDepth(),QT_TRANSLATE_NOOP("QObject","Warning: 0 observation found in %s."),filename.c_str());
            ok=false;
        }
    }

    return ok;
}


bool Station_Bascule::readXYZrotation(const std::string& filename)
{
    //open file
    std::cout<<"Try to read file \""<<filename<<"\"..."<<std::endl;
    file = XYZFile::create(filename,this,"",0,1);
    uni_stream::ifstream fileStream;
    std::string line;
    bool ok=true;

    Project::theInfo()->msg(INFO_OBS,getFile().get_fileDepth(),QT_TRANSLATE_NOOP("QObject","Read file %s..."),filename.c_str());
    //int position_number=-1;

    fileStream.open(filename.c_str());
    if (fileStream.is_open())
    {
        while (std::getline(fileStream, line))
        {
            if (line.empty())
                continue;

            //remove exotic endline chars
            if (line.back()=='\r')
                line.resize(line.size()-1);
            std::regex regex_line(R"(^\*\*\!  Station.*$)");
            if(std::regex_match(line,regex_line))
                break;
        }
        std::istringstream iss(line);
        std::string tmp;
        iss>>tmp; //read "!*"
        iss>>tmp; //read "Station"
        iss>>tmp; //read ":"
        readStationName="";
        if (!(iss >> readStationName))
        {
            Project::theInfo()->warning(INFO_OBS,getFile().get_fileDepth(),
                                        QT_TRANSLATE_NOOP("QObject","At line %s => Can't convert point name."),
                                        line.c_str());
            ok=false;
        }
        mOrigin=Project::theone()->getPoint(readStationName,true);
        if (!mOrigin)
        {
            Project::theInfo()->error(INFO_OBS,getFile().get_fileDepth(),
                                      QT_TRANSLATE_NOOP("QObject","Error: impossible to find bascule center \"%s\""),
                                      readStationName.c_str());
            ok=false;
        }

        //read matrix :
         std::getline(fileStream, line);//comp coord line
        iss.clear();iss.str(line);
        tdouble tmp_d = NAN;
        iss >> tmp >> tmp_d;
        readStationCoordCartGlobal.setx(tmp_d);
        iss >> tmp_d;
        readStationCoordCartGlobal.sety(tmp_d);
        iss >> tmp_d;
        readStationCoordCartGlobal.setz(tmp_d);
        std::getline(fileStream, line);
        iss.clear();iss.str(line);
        iss >> tmp >> R_vert2instr(0,0) >> R_vert2instr(1,0) >> R_vert2instr(2,0);
        std::getline(fileStream, line);
        iss.clear();iss.str(line);
        iss >> tmp >> R_vert2instr(0,1) >> R_vert2instr(1,1) >> R_vert2instr(2,1);
        std::getline(fileStream, line);
        iss.clear();iss.str(line);
        iss >> tmp >> R_vert2instr(0,2) >> R_vert2instr(1,2) >> R_vert2instr(2,2);
    }else{
        Project::theInfo()->warning(INFO_OBS,getFile().get_fileDepth(),
                                    QT_TRANSLATE_NOOP("QObject","Can't open file  %s."),
                                    filename.c_str());
        ok=false;
    }

    return ok;
}

Coord Station_Bascule::ptCartGlobal2Instr(const Coord &pt_cart_global, bool fromFile)
{
    Vect3 M=pt_cart_global.toVect();//target
    Vect3 S;//Station
    if (fromFile)
        S=readStationCoordCartGlobal.toVect();
    else{
        Coord coord_compensated_cartesian;
        Project::theone()->projection.sphericalToGlobalCartesian(origin()->coord_comp,coord_compensated_cartesian);
        S=coord_compensated_cartesian.toVect();
    }
    //MS=M-S, compensated vector in field frame
    Vect3 MS=M-S;
    //U: compensated vector in laser frame
    Mat3 R_global2vert=global2vertRot(origin()->coord_comp, origin()->dev_eta, origin()->dev_xi);
    Mat3 R_global2instr=R_vert2instr*R_global2vert;
    Vect3 U=R_global2instr*MS;
    return Coord(U);
}

Mat3 Station_Bascule::sigmaGlobal2Instr(const Mat3 &varCovarGlobal)
{
    return R_vert2instr*varCovarGlobal*R_vert2instr.transpose();
}

Coord Station_Bascule::mes2GlobalCart(const Coord &target_in_instr_coords, bool fromFile)
{
    if (!initOk())
        return {NAN, NAN, NAN};
    Vect3 M=target_in_instr_coords.toVect();//target
    Vect3 S;//Station
    if (fromFile)
        S=readStationCoordCartGlobal.toVect();
    else{
        Coord coord_compensated_cartesian;
        Project::theone()->projection.sphericalToGlobalCartesian(origin()->coord_comp,coord_compensated_cartesian);
        S=coord_compensated_cartesian.toVect();
    }
        //U: vector oriented like ground frame
    Mat3 R_global2vert=global2vertRot(origin()->coord_comp, origin()->dev_eta, origin()->dev_xi);
    Mat3 R_global2instr=R_vert2instr*R_global2vert;
    Vect3 U=R_global2instr.transpose()*M;
    Vect3 G=U+S;
    return Coord(G);
}

Mat3 Station_Bascule::sigmaInstr2Global(const Mat3 &varCovarInstr)
{
    return R_vert2instr.transpose()*varCovarInstr;
}

//returns false if problem (e.g. impossible to compute value)
bool Station_Bascule::set_obs(LeastSquares *lsquares, bool initialResidual, bool internalConstraints)
{
    if (!initOk())
    {
        Project::theInfo()->warning(INFO_OBS,getFile().get_fileDepth(),
                                    QT_TRANSLATE_NOOP("QObject","Bascule %s not init, it won't be used."),
                                    origin()->name.c_str());
        return false;
    }
    bool ok=true;

    tdouble _a = NAN,_b = NAN,_c = NAN,_theta = NAN;
    R2abc(R_vert2instr,_theta,_a,_b,_c);
    //std::cout<<"Rotation: "<<from->name<<" "<<_a<<" "<<_b<<" "<<_c<<" "<<_theta<<"\n";

    for (auto & obs3d : observations3D)
        if (!obs3d.set_obs(lsquares,initialResidual,internalConstraints))
            ok=false;

    return ok;
}


void Station_Bascule::removeObsConnectedToPoint(const Point &point)
{
    observations3D.remove_if([&point](const auto& obs3d){return obs3d.obs1->to == &point || obs3d.obs1->from == &point;});
    Station::removeObsConnectedToPoint(point);
}
