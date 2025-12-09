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

#include "obs.h"

#include <sstream>
#include <iostream>

#include "leastsquares.h"
#include "point.h"
#include "compile.h"
#include "station_simple.h"
#include "station_hz.h"
#include "station_bascule.h"
#include "station_eq.h"
#include "project.h"

int Obs::obsCounter=0;


std::map<OBS_CODE,std::string> Obs::obsTypeName{
    {OBS_CODE::UNKNOWN,"?"},{OBS_CODE::COORD_X, "crd_x"}, {OBS_CODE::COORD_Y, "crd_y"}, {OBS_CODE::COORD_Z, "crd_z"},
    {OBS_CODE::DIST1,"dist"},{OBS_CODE::DIST_HZ0,"dstHz0"},{OBS_CODE::DIST,"dist"},{OBS_CODE::DH,"d_h"},{OBS_CODE::HZ_ANG,"hz"},
    {OBS_CODE::ZEN,"zen"},{OBS_CODE::HZ_REF,"tour"},{OBS_CODE::AZIMUTH,"azim"},
    {OBS_CODE::DX_SPHER,"d_x"},{OBS_CODE::DY_SPHER,"d_y"},
    {OBS_CODE::EQ_DH, "eq_dh"}, {OBS_CODE::EQ_DIST, "eq_dist"},
    {OBS_CODE::BASCULE_X,"bsc_x"},{OBS_CODE::BASCULE_Y,"bsc_y"},{OBS_CODE::BASCULE_Z,"bsc_z"},
    {OBS_CODE::BASCULE_HZ,"bsc_h"},{OBS_CODE::BASCULE_ZEN,"bsc_v"},{OBS_CODE::BASCULE_DIST,"bsc_d"},
    {OBS_CODE::BASELINE_X,"bl_x"},{OBS_CODE::BASELINE_Y,"bl_y"},{OBS_CODE::BASELINE_Z,"bl_z"},
    {OBS_CODE::AXIS_R,"ax_r"},
    {OBS_CODE::AXIS_T,"ax_t"},
    {OBS_CODE::AXIS_COMBI,"ax_c"},
    {OBS_CODE::AXIS_FIX_X,"ax_x"},
    {OBS_CODE::AXIS_FIX_Y,"ax_y"},
    {OBS_CODE::AXIS_FIX_Z,"ax_z"},
    {OBS_CODE::INTCONST_DX,"CI_tx"},{OBS_CODE::INTCONST_DY,"CI_ty"},{OBS_CODE::INTCONST_DZ,"CI_tz"},
    {OBS_CODE::INTCONST_RX,"CI_rx"},{OBS_CODE::INTCONST_RY,"CI_ry"},{OBS_CODE::INTCONST_RZ,"CI_rz"},
    {OBS_CODE::INTCONST_SC,"CI_sc"}
};


Obs::Obs(Point *_from, Point *_to, Station *_station, OBS_CODE _code, bool _active, tdouble _value_original,
         tdouble _sigma_abs_original, tdouble _sigma_rel, tdouble _instrument_height, tdouble _target_height,
         int _line, tdouble _unit_factor, const std::string &_unit_str, DataFile *_file, const std::string &_comment) :
    lineNumber(_line),from(_from),to(_to),station(_station),code(_code),unitFactor(_unit_factor),
    value(_value_original*_unit_factor),originalValue(_value_original),
    sigmaAbs(_sigma_abs_original*_unit_factor),sigmaAbsOriginal(_sigma_abs_original),
    sigmaRel(_sigma_rel),sigmaTotal(0.0),sigmaAposteriori(-1.0),
    instrumentHeight(_instrument_height),targetHeight(_target_height),
    computedValue(0.0),deflectionCorrection(0.0),residual(0.0),normalizedResidual(0.0),residualStd(0.0),
    initialResidual(0.0),initialNormalizedResidual(0.0),sigmaResidual(-1.0),obsRedondancy(-1.0),
    standardizedResidual(NAN),appliedNormalisation(1.0),
    comment(_comment),active(_active),activeRead(_active),varianceFromMatrix(false),unitStr(_unit_str),file(_file),
    obsNumber(obsCounter++),obsRank(-1),C(0),S(0),D(0),D0(0),Dhz(0),azim(0),zen(0)
{
    //if (sigmaAbsOriginal<0), the obs sould be inactive
    sigmaAbsOriginal=fabs(sigmaAbsOriginal);
    sigmaAbs=fabs(sigmaAbs);
    if (fabs(sigmaRel)<0.0000001)
        sigmaTotal=sigmaAbs;
  #ifdef OBS_INFO
    std::cout<<"Create obs: "<<this->toString()<<std::endl;
  #endif

}

Obs::Obs(const Obs &other):lineNumber(other.lineNumber),from(other.from),to(other.to),station(other.station),code(other.code),
    value(other.value),sigmaAbs(other.sigmaAbs),sigmaRel(other.sigmaRel),instrumentHeight(other.instrumentHeight),
    targetHeight(other.targetHeight),computedValue(other.computedValue),deflectionCorrection(other.deflectionCorrection),
    residual(other.residual),normalizedResidual(other.normalizedResidual),initialResidual(other.initialResidual),
    initialNormalizedResidual(other.initialNormalizedResidual),sigmaResidual(other.sigmaResidual),obsRedondancy(other.obsRedondancy),
    standardizedResidual(other.standardizedResidual),appliedNormalisation(other.appliedNormalisation),comment(other.comment),
    active(other.active),activeRead(other.activeRead),varianceFromMatrix(other.varianceFromMatrix)
{
    if (sigmaAbsOriginal<0)
    {
        std::cout<<"WARNING! Copy obs with inconsistant sigmaAbsOriginal ?"<<std::endl;
        sigmaAbsOriginal=-sigmaAbsOriginal;
        sigmaAbs=-sigmaAbs;
        active=false;
        activeRead=false;
    }

    std::cout<<"Copy obs: "<<this->toString()<<std::endl;
}


void Obs::reset()
{
    obsRank=-1;
    computedValue=0.0;
    deflectionCorrection=0.0;
    residual=0.0;
    normalizedResidual=0.0;
    residualStd=0.0;
    initialResidual=0.0;
    initialNormalizedResidual=0.0;
    sigmaResidual=-1.0;
    obsRedondancy=-1.0;
    standardizedResidual=NAN;
    sigmaAposteriori=-1.0;
    D=0.0;
    D0=0.0;
    Dhz=0.0;
    azim=0.0;
    zen=0.0;
}

std::string Obs::toString() const
{
    std::ostringstream oss;
    oss<<obsNumber<<", type "<<obsTypeName[code]<<": \""<<static_cast<int>(code)<<" "<<(from?from->name:"?")
       <<" "<<(to?to->name:"?")<<" "<<originalValue<<" "<<sigmaAbsOriginal<<" "<<sigmaRel<<" "<<instrumentHeight
       <<" "<<targetHeight<<"\" in "<<(file?file->get_name():"?")<<":"<<lineNumber;
    return oss.str();
}

std::string Obs::toObsFile(bool withComputedValue) const
{
    std::ostringstream oss;
    oss<<static_cast<int>(code)<<" "<<(from?from->name:"?")<<" "<<(to?to->name:"?")<<" ";
    oss.setf( std::ios::fixed, std:: ios::floatfield ); // precision is number of digits after decimal point
    if (withComputedValue)
    {
        oss.precision(Project::theone()->config.nbDigits);
        oss<<computedValue/unitFactor;
    }else{
        oss.precision(Project::theone()->config.nbDigits);
        oss<<originalValue;
    }
    oss<<" "<<sigmaAbsOriginal<<" "<<sigmaRel<<" "<<instrumentHeight<<" "<<targetHeight<<std::endl;
    return oss.str();
}

Json::Value Obs::toJson(FileRefJson &filesRef) const
{
    Json::Value val;
    val["line"]=lineNumber;
    val["code"]=static_cast<int>(code);
    val["number"]=(int)obsNumber;
    val["rank"]=(int)obsRank;
    val["from"]=from?from->name:"?";
    val["to"]=to?to->name:"?";
    val["sigma_abs"]=(double)sigmaAbsOriginal;
    val["sigma_rel"]=(double)sigmaRel;
    val["sigma_total"]=(double)(sigmaTotal/unitFactor);

    val["instrument_height"]=(double)instrumentHeight;
    val["target_height"]=(double)targetHeight;
    val["original_value"]=(double)originalValue;
    val["computed_value"]=(double)(computedValue/unitFactor);
    val["deflection_correction"]=(double)(deflectionCorrection/unitFactor);
    val["residual"]=(double)(residual/unitFactor);
    val["normalized_residual"]=(double)normalizedResidual;
    val["obs_length"]=(double)D;
    val["obs_azim"]=(double)azim;
    val["obs_zen"]=(double)zen;
    val["unit_factor"]=(double)unitFactor;
    val["active"]=active;
    val["active_read"]=activeRead;
    val["variance_from_matrix"]=varianceFromMatrix;
    val["unit_str"]=unitStr;

    val["comment"]=comment;
    
    val["file_id"]=filesRef.getNumber(file);

    if (obsRedondancy>-1) //the matrix has been inverted and obs is active
    {
        val["sigma_a_posteriori"]=(double)(sigmaAposteriori/unitFactor);
        val["residual_std"]=(double)residualStd;
        val["obs_redondancy"]=(double)obsRedondancy;
        if (!std::isnan(standardizedResidual)) //some redondancy is mandatory
        {
            val["sigma_residual"]=(double)sigmaResidual/unitFactor;
            val["standardized_residual"]=(double)standardizedResidual;
            tdouble wmax=2.5;//type I error alpha=1%
            tdouble delta=4.1;//type II error beta=5%
            tdouble nabla=delta*(sigmaTotal/unitFactor)/(sqrt(obsRedondancy/100));
            val["nabla"]=(double)nabla;
            if (fabs(standardizedResidual)>wmax)
            {
                tdouble probableError=-(residual/unitFactor)/(obsRedondancy/100);
                val["probable_error"]=(double)probableError;
            }
        }
    }

    return val;
}



std::string Obs::getTypeName(OBS_CODE _code)
{
    if (Obs::obsTypeName.find(_code)!=Obs::obsTypeName.end())
        return Obs::obsTypeName.find(_code)->second;
    else
        return "Observation code undefined";

}


bool Obs::accept1D()
{
    switch (code)
    {
        case OBS_CODE::DH:
        case OBS_CODE::COORD_Z:
        case OBS_CODE::EQ_DH:
            return true;
        default://TODO: list all to have a warning if new types are added?
            Project::theInfo()->warning(INFO_OBS,file?file->get_fileDepth():1,
                                        QT_TRANSLATE_NOOP("QObject","Observation %s can't be used with 1D point %s or %s."),
                                        toString().c_str(),from->name.c_str(),to->name.c_str());
            return false;
    }
}

bool Obs::accept2D()
{
    switch (code)
    {
        case OBS_CODE::COORD_X:
        case OBS_CODE::COORD_Y:
        case OBS_CODE::DIST_HZ0:
        case OBS_CODE::HZ_ANG:
        case OBS_CODE::HZ_REF:
        case OBS_CODE::AZIMUTH:
        case OBS_CODE::DX_SPHER:
        case OBS_CODE::DY_SPHER:
            return true;
        default://TODO: list all to have a warning if new types are added?
            Project::theInfo()->warning(INFO_OBS,file?file->get_fileDepth():1,
                                        QT_TRANSLATE_NOOP("QObject","Observation %s can't be used with 2D point %s or %s."),
                                        toString().c_str(),from->name.c_str(),to->name.c_str());
            return false;
    }
}

bool Obs::isInternal()
{
    return station->isInternal(this);
}

bool Obs::isOnlyLeveling()
{
    return station->isOnlyLeveling(this);
}

bool Obs::isHz()
{
    return station->isHz(this);
}

bool Obs::isDistance()
{
    return station->isDistance(this);
}

bool Obs::isBubbuled()
{
    return station->isBubbuled(this);
}

bool Obs::useVertDeflection()
{
    return station->useVertDeflection(this);
}

int Obs::numberOfBasicObs()
{
    return station->numberOfBasicObs(this);
}

bool Obs::checkPointsDimension()
{
    bool ok=true;
    if ((from->dimension==1)||(to->dimension==1))
        ok = ok & accept1D();
    if ((from->dimension==2)||(to->dimension==2))
        ok = ok & accept2D();
    return ok;
}

/**
 * @brief Computes infos for observations:
 *   C, S, D, D0, azim, zen, sigmaTotal, computedValue, deflectionCorrection residual
 *   normalizedResidual, initialNormalizedResidual, initialResidual
 * @return true if ok
 */
bool Obs::computeInfos(bool isInitialResidual)
{
    bool ok=true;
    tdouble radius=Project::theone()->projection.radius;
    tdouble Xa=from->coord_comp.x();
    tdouble Ya=from->coord_comp.y();
    tdouble Za=from->coord_comp.z() + radius + instrumentHeight;
    tdouble Xb=to->coord_comp.x();
    tdouble Yb=to->coord_comp.y();
    tdouble Zb=to->coord_comp.z() + radius + targetHeight;

    if (Project::theone()->use_vertical_deflection)
    {
        if (fabs(instrumentHeight)>CAP_CLOSE)
        {
            Xa+=instrumentHeight*from->dev_eta;
            Ya+=instrumentHeight*from->dev_xi;
        }
        if (fabs(targetHeight)>CAP_CLOSE)
        {
            Xb+=targetHeight*to->dev_eta;
            Yb+=targetHeight*to->dev_xi;
        }
    }

    arc(Xa,Ya,Xb,Yb,C,S,radius);
    D0 = radius*asin(S); // hz dist on ellipsoid
    Dhz = Za * asin(S); // hz dist on origin level
    //distance spatiale. Formule + compliquee, mais stable
    D=sqrt(sqr(Za-Zb)+4*Za*Zb*sqr(sin(asin(S)/2)));
    //azim and zen are used for vertical deflection correction
    azim = azimuth(Xa,Ya,Xb,Yb,C,radius);
    zen = atan2(S,C-Za/Zb)-Project::theone()->config.refraction*Dhz/2/radius;

    tdouble vertical_deflection = 0;
    deflectionCorrection = 0.0;

    //update sigmatotal
    switch (code)
    {
        case OBS_CODE::COORD_X:
        case OBS_CODE::COORD_Y:
        case OBS_CODE::COORD_Z:
            sigmaTotal=sigmaAbs;
            break;
        case OBS_CODE::DIST_HZ0:
        case OBS_CODE::DIST1:
        case OBS_CODE::DIST:
        case OBS_CODE::DH:
        case OBS_CODE::DX_SPHER:
        case OBS_CODE::DY_SPHER:
        case OBS_CODE::EQ_DH:
        case OBS_CODE::EQ_DIST:
            sigmaTotal=sigmaAbs+sigmaRel*D;
            break;
        case OBS_CODE::HZ_ANG:
        case OBS_CODE::HZ_REF:
        case OBS_CODE::AZIMUTH:
        case OBS_CODE::ZEN:
            sigmaTotal=sigmaAbs+sigmaRel/D;
            break;
        default:
            Project::theInfo()->warning(INFO_OBS,file?file->get_fileDepth():1,
                                        QT_TRANSLATE_NOOP("QObject","Observation type  %d: %s can't be used."),
                                        static_cast<int>(code),toString().c_str());
            ok=false;
    }

    bool dim_ok = checkPointsDimension();
    if (!dim_ok)
    {
        if (active)
            return false;
        else{
            computedValue = NAN;
            deflectionCorrection = NAN;
            residual = NAN;
            normalizedResidual = NAN;
            initialNormalizedResidual = NAN;
            initialResidual = NAN;
            Project::theone()->hasWarning = true;
            return true;//do not stop computation just for desactivated obs
        }
    }

    if (fabs(sigmaTotal)<MINIMAL_SIGMA)
    {
        Project::theInfo()->warning(INFO_OBS,file?file->get_fileDepth():1,
                                    QT_TRANSLATE_NOOP("QObject","Observation %s has a sigma too low!"),
                                    toString().c_str());
        return false;
    }

    if (Project::theone()->config.compute_type==COMPUTE_TYPE::type_compensation)
    {
        switch (code)
        {
        case OBS_CODE::DIST1://slope distance
        case OBS_CODE::DIST:
            computedValue=D;
            break;
        case OBS_CODE::DIST_HZ0://hz dist on ellipsoid
            computedValue=radius*asin(S);
            break;
        case OBS_CODE::DH://dz
            computedValue=Zb-Za;
            if (Project::theone()->use_vertical_deflection)
            {
                vertical_deflection = (from->dev_norm*cos(azim-from->dev_azim)+(to->dev_norm*cos(azim-to->dev_azim)))/2;
                deflectionCorrection = -D0*vertical_deflection;
            }
            break;

        case OBS_CODE::DX_SPHER://diff x spherical coords
            computedValue=Xb-Xa;
            if (Project::theone()->use_vertical_deflection)
                deflectionCorrection=(Zb-Za)*from->dev_eta;
            break;

        case OBS_CODE::DY_SPHER://diff y spherical coords
            computedValue=Yb-Ya;
            if (Project::theone()->use_vertical_deflection)
                deflectionCorrection=(Zb-Za)*from->dev_xi;
            break;

        case OBS_CODE::ZEN://zenital
            computedValue=atan2(S,C-Za/Zb)-Project::theone()->config.refraction*Dhz/2/radius;
            if (Project::theone()->use_vertical_deflection)
                deflectionCorrection=from->dev_norm*cos(azim-from->dev_azim);
            break;

        case OBS_CODE::AZIMUTH:
            //{auto factors = Project::theone()->projection.getInputProjFactors({Xa, Ya,0});
            //std::cout<<"fix az input conv "<<Xa<<": "<<factors.meridian_convergence<<std::endl;
            //std::cout<<"fix az stereo conv "<<Xa<<": "<<Project::theone()->projection.getStereoMeridianConvergence( {Xa, Ya, 0.} )<<std::endl;}
            computedValue = azimuth(Xa,Ya,Xb,Yb,C,radius);
            if (Project::theone()->use_vertical_deflection)
            {
                vertical_deflection = from->dev_norm*cos(azim-from->dev_azim+PI/2);
                deflectionCorrection=vertical_deflection/tan(zen);
            }
            break;

        case OBS_CODE::CENTER_META:
            std::cout<<"ERROR: CENTER_META!!!!"<<std::endl;
            throw 0;

        case OBS_CODE::COORD_X://X constraint
            computedValue=Xa;
            break;

        case OBS_CODE::COORD_Y://Y constraint
            computedValue=Ya;
            break;

        case OBS_CODE::COORD_Z://Z constraint
            computedValue=Za-radius;
            break;

        case OBS_CODE::HZ_REF://no difference with HZ_ANG, just used to create a new hz station (g0 unknown)
        case OBS_CODE::HZ_ANG://hz angle
            computedValue = azimuth(Xa,Ya,Xb,Yb,C,radius) - dynamic_cast<Station_Hz*>(station)->g0;
            if (Project::theone()->use_vertical_deflection)
            {
                vertical_deflection = from->dev_norm*cos(azim-from->dev_azim+PI/2);
                deflectionCorrection=vertical_deflection/tan(zen);
            }
            break;
        case OBS_CODE::EQ_DH:
            computedValue=Zb-Za-dynamic_cast<Station_Eq*>(station)->val;
            if (Project::theone()->use_vertical_deflection)
            {
                vertical_deflection = (from->dev_norm*cos(azim-from->dev_azim)+(to->dev_norm*cos(azim-to->dev_azim)))/2;
                deflectionCorrection = -D0*vertical_deflection;
            }
            break;
        case OBS_CODE::EQ_DIST:
            computedValue=D-dynamic_cast<Station_Eq*>(station)->val;
            break;

        default:
            Project::theInfo()->warning(INFO_OBS,file?file->get_fileDepth():1,
                                        QT_TRANSLATE_NOOP("QObject","Observation type %d can't be used."),
                                        static_cast<int>(code));
            return false;
        }

        computedValue-=deflectionCorrection;

        //TODO: check if sigmaTotal and computedValue are finite?

        switch (code)
        {
        case OBS_CODE::AZIMUTH:
        case OBS_CODE::HZ_REF:
        case OBS_CODE::HZ_ANG:
            residual=computedValue-value-2*PI;
            normalizeAngle(residual);
            normalizeAngle(computedValue);
            break;
        default:
            residual=computedValue-value;
        }

        normalizedResidual = residual/sigmaTotal;
        if (isInitialResidual)
        {
            initialNormalizedResidual=normalizedResidual;
            initialResidual=residual;
            //continue;
        }
    }else{
        computedValue=0;
        deflectionCorrection = 0.0;
        residual=0;
        normalizedResidual=0;
        initialNormalizedResidual=0;
        initialResidual=0;
    }

    return ok;
}

bool Obs::setEquation(LeastSquares *lsquares, bool isInitialResidual, bool internalConstraints)
{
    static std::vector<tdouble> relations;
    static std::vector<int> positions;
    bool ok=true;

    if ((internalConstraints)&&((!isInternal()))) return true;

    if (!computeInfos(isInitialResidual))
    {
        Project::theInfo()->warning(INFO_OBS,file?file->get_fileDepth():1,
                                    QT_TRANSLATE_NOOP("QObject","Impossible to set observation %s"),
                                    toString().c_str());
        return false;
    }
    //std::cout<<"Setting: "<<this<<std::endl;
    //std::cout<<toString()<<std::endl;

    tdouble radius=Project::theone()->projection.radius;
    tdouble Xa=from->coord_comp.x();
    tdouble Ya=from->coord_comp.y();
    tdouble Za=from->coord_comp.z() + radius + instrumentHeight;
    tdouble Xb=to->coord_comp.x();
    tdouble Yb=to->coord_comp.y();
    tdouble Zb=to->coord_comp.z() + radius + targetHeight;

    if (Project::theone()->use_vertical_deflection)
    {
        if (fabs(instrumentHeight)>CAP_CLOSE)
        {
            Xa+=instrumentHeight*from->dev_eta;
            Ya+=instrumentHeight*from->dev_xi;
        }
        if (fabs(targetHeight)>CAP_CLOSE)
        {
            Xb+=targetHeight*to->dev_eta;
            Yb+=targetHeight*to->dev_xi;
        }
    }

    //TODO: use case
    if (((D<MINIMAL_DIVIDER)&&((code==OBS_CODE::ZEN)||(code==OBS_CODE::DIST)))
            ||((D0<MINIMAL_DIVIDER)&&((code==OBS_CODE::DIST_HZ0)||(code==OBS_CODE::AZIMUTH)||(code==OBS_CODE::HZ_ANG)||(code==OBS_CODE::HZ_REF))))
    {
        Project::theInfo()->error(INFO_OBS,file?file->get_fileDepth():1,
                                  QT_TRANSLATE_NOOP("QObject","Observation %s can't be used since the points are too close!"),
                                  toString().c_str());
        return false;
    }

    //std::cout<<"Ranks: "<<from->params.at(0).rank<<" "<<from->params.at(1).rank<<" "<<from->params.at(2).rank<<"\n";
    //std::cout<<"------ "<<to->params.at(0).rank<<" "<<to->params.at(1).rank<<" "<<to->params.at(2).rank<<std::endl;

    if (fabs(sigmaTotal)<MINIMAL_SIGMA) //useful? already checked in computeInfos()
    {
        Project::theInfo()->warning(INFO_OBS,file?file->get_fileDepth():1,
                                    QT_TRANSLATE_NOOP("QObject","Observation %s has a sigma too low!"),
                                    toString().c_str());
        return false;
    }

    if (Project::theone()->config.compute_type==COMPUTE_TYPE::type_monte_carlo)
    {
        residual = gauss_random(0,sigmaTotal);
        // if ((code==COORD_X)||(code==COORD_Y)||(code==COORD_Z))
            // residual =0; // just to check, act as delphi comp
    }
    if (Project::theone()->config.compute_type==COMPUTE_TYPE::type_propagation)
    {
        residual = 0;
    }
    tdouble f;//temporary result

    switch (code)
    {
    case OBS_CODE::DIST1://slope distance
    case OBS_CODE::DIST:
        //TODO: use an other formula for small distances
        relations.resize(7);
        positions.resize(7);

        relations[0]=residual;
        f=Za*Zb/sqr(radius)/D;
        relations[1]=(Xa-Xb)*f;
        relations[2]=(Ya-Yb)*f;
        relations[3]=(Za-Zb*C)/D;
        relations[4]=-relations[1];
        relations[5]=-relations[2];
        relations[6]=(Zb-Za*C)/D;
        positions[0] = lsquares->B_index;//index of constant part
        positions[1] = from->params.at(0).rank;
        positions[2] = from->params.at(1).rank;
        positions[3] = from->params.at(2).rank;
        positions[4] = to->params.at(0).rank;
        positions[5] = to->params.at(1).rank;
        positions[6] = to->params.at(2).rank;
        break;

    case OBS_CODE::DIST_HZ0://hz dist
        //TODO: use an other formula for small distances
        relations.resize(5);
        positions.resize(5);
        relations[0]=residual;
        relations[1]=(Xa-Xb)/D0;
        relations[2]=(Ya-Yb)/D0;
        relations[3]=-relations[1];
        relations[4]=-relations[2];

        positions[0] = lsquares->B_index;//index of constant part
        positions[1] = from->params.at(0).rank;
        positions[2] = from->params.at(1).rank;
        positions[3] = to->params.at(0).rank;
        positions[4] = to->params.at(1).rank;
        break;

    case OBS_CODE::DH://dz
        relations.resize(3);
        positions.resize(3);

        relations[0]=residual;
        relations[1]=-1;
        relations[2]=+1;
        positions[0] = lsquares->B_index;//index of constant part
        positions[1] = from->params[2].rank;
        positions[2] = to->params[2].rank;
        break;

    case OBS_CODE::DX_SPHER://diff x spherical coords
        relations.resize(3);
        positions.resize(3);

        relations[0]=residual;
        relations[1]=-1;
        relations[2]=+1;
        positions[0] = lsquares->B_index;//index of constant part
        positions[1] = from->params.at(0).rank;
        positions[2] = to->params.at(0).rank;
        break;

    case OBS_CODE::DY_SPHER://diff y spherical coords
        relations.resize(3);
        positions.resize(3);

        relations[0]=residual;
        relations[1]=-1;
        relations[2]=+1;
        positions[0] = lsquares->B_index;//index of constant part
        positions[1] = from->params.at(1).rank;
        positions[2] = to->params.at(1).rank;
        break;

    case OBS_CODE::ZEN://zenital
        relations.resize(7);
        positions.resize(7);

        relations[0]=residual;
        f=Zb*(Zb-Za*C)/(radius*D0*sqr(D));
        relations[1]=(Xa-Xb)*f;
        relations[2]=(Ya-Yb)*f;
        f=S/sqr(D);
        relations[3]=Zb*f;
        relations[4]=-relations[1];
        relations[5]=-relations[2];
        relations[6]=-Za*f;
        positions[0] = lsquares->B_index;//index of constant part
        positions[1] = from->params.at(0).rank;
        positions[2] = from->params.at(1).rank;
        positions[3] = from->params.at(2).rank;
        positions[4] = to->params.at(0).rank;
        positions[5] = to->params.at(1).rank;
        positions[6] = to->params.at(2).rank;
        break;

    case OBS_CODE::AZIMUTH:
        relations.resize(5);
        positions.resize(5);

        relations[0]=residual;
        relations[1]=(Ya-Yb)/sqr(D0);
        relations[2]=(Xb-Xa)/sqr(D0);
        relations[3]=-relations[1];
        relations[4]=-relations[2];

        positions[0] = lsquares->B_index;//index of constant part
        positions[1] = from->params.at(0).rank;
        positions[2] = from->params.at(1).rank;
        positions[3] = to->params.at(0).rank;
        positions[4] = to->params.at(1).rank;
        break;

    case OBS_CODE::CENTER_META:
        std::cout<<"ERROR: CENTER_META!!!!"<<std::endl;
        throw 0;

    case OBS_CODE::COORD_X://X constraint
        relations.resize(2);
        positions.resize(2);

        relations[0]=residual;
        relations[1]=+1;
        positions[0] = lsquares->B_index;//index of constant part
        positions[1] = from->params.at(0).rank;
        break;

    case OBS_CODE::COORD_Y://Y constraint
        relations.resize(2);
        positions.resize(2);

        relations[0]=residual;
        relations[1]=+1;
        positions[0] = lsquares->B_index;//index of constant part
        positions[1] = from->params.at(1).rank;
        break;

    case OBS_CODE::COORD_Z://Z constraint
        relations.resize(2);
        positions.resize(2);

        relations[0]=residual;
        relations[1]=+1;
        positions[0] = lsquares->B_index;//index of constant part
        positions[1] = from->params[2].rank;
        break;

    case OBS_CODE::HZ_REF://no difference with HZ_ANG, just used to create a new hz station (g0 unknown)
    case OBS_CODE::HZ_ANG://hz angle
        relations.resize(6);
        positions.resize(6);

        relations[0]=residual;
        relations[1]=(Ya-Yb)/sqr(D0);
        relations[2]=(Xb-Xa)/sqr(D0);
        relations[3]=-relations[1];
        relations[4]=-relations[2];

        positions[0] = lsquares->B_index;//index of constant part
        positions[1] = from->params.at(0).rank;
        positions[2] = from->params.at(1).rank;
        positions[3] = to->params.at(0).rank;
        positions[4] = to->params.at(1).rank;
        relations[5] = -1;
        positions[5] = station->params.at(0).rank; // g0
        break;
    case OBS_CODE::EQ_DH:
        relations.resize(4);
        positions.resize(4);

        relations[0]=residual;
        relations[1]=-1;
        relations[2]=+1;
        relations[3]=-1;
        positions[0] = lsquares->B_index;//index of constant part
        positions[1] = from->params[2].rank;
        positions[2] = to->params[2].rank;
        positions[3] = station->params[0].rank;
        break;
    case OBS_CODE::EQ_DIST:
        relations.resize(8);
        positions.resize(8);
        relations[0]=residual;
        f=Za*Zb/sqr(radius)/D;
        relations[1]=(Xa-Xb)*f;
        relations[2]=(Ya-Yb)*f;
        relations[3]=(Za-Zb*C)/D;
        relations[4]=-relations[1];
        relations[5]=-relations[2];
        relations[6]=(Zb-Za*C)/D;
        relations[7]=-1;
        positions[0] = lsquares->B_index;//index of constant part
        positions[1] = from->params.at(0).rank;
        positions[2] = from->params.at(1).rank;
        positions[3] = from->params.at(2).rank;
        positions[4] = to->params.at(0).rank;
        positions[5] = to->params.at(1).rank;
        positions[6] = to->params.at(2).rank;
        positions[7] = station->params[0].rank;
        break;
    default:
        Project::theInfo()->warning(INFO_OBS,file?file->get_fileDepth():1,
                                    QT_TRANSLATE_NOOP("QObject","Observation type %d can't be used."),
                                    static_cast<int>(code));
        return false;
    }

    if (active)
        ok = lsquares->add_constraint(this,relations,positions,sigmaTotal) && ok;

    return ok;
}
