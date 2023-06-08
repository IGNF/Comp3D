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

#include "mathtools.h"
#include "project.h"

#include <boost/math/distributions/chi_squared.hpp>
#include <boost/random.hpp>
#include <boost/random/normal_distribution.hpp>
#include <exception>
#include <iostream>
#include <sstream>


std::map<ANGLE_UNIT, std::string> Angles::names
{
    {ANGLE_UNIT::RAD,"r"},{ANGLE_UNIT::GRAD,"g"},
    {ANGLE_UNIT::DEG,"°"},{ANGLE_UNIT::ARCSEC,"\""}
};

//TODO(jmm): really need to use a struct?
struct Randomness
{
    static boost::mt19937 rng;
    static boost::normal_distribution<> nd;
    static boost::variate_generator<boost::mt19937&,boost::normal_distribution<> > var_nor;
};


#ifdef RANDOM_SEED
boost::mt19937 Randomness::rng(std::time(nullptr));//seeding
#else
boost::mt19937 Randomness::rng;//no seeding
#endif

boost::normal_distribution<> Randomness::nd(0.0, 1.0);//normal distribution (centered, normed)
boost::variate_generator<boost::mt19937&,boost::normal_distribution<> > Randomness::var_nor(rng, nd);

const tdouble PI=boost::math::constants::pi<tdouble>();

//see http://www.boost.org/doc/libs/1_35_0/libs/math/doc/sf_and_dist/html/math_toolkit/dist/stat_tut/weg/cs_eg/chi_sq_test.html
tdouble chi2_min(tdouble confidence,int num_freedom)
{
    boost::math::chi_squared dist(num_freedom);
    tdouble Sd=1.0;
    tdouble lower_limit = sqrt((num_freedom) * Sd * Sd / boost::math::quantile(complement(dist, (1-confidence)/2.0)));
    return lower_limit;
}

tdouble chi2_max(tdouble confidence,int num_freedom)
{
    boost::math::chi_squared dist(num_freedom);
    tdouble Sd=1.0;
    tdouble upper_limit = sqrt((num_freedom) * Sd * Sd / boost::math::quantile(complement(dist, 1-(1-confidence)/2.0)));
    return upper_limit;
}


tdouble sqr(tdouble x)
{
    return x*x;
}

tdouble sign(tdouble x)
{
    if (x>0.0)
        return 1.0;
    if (x<0.0)
        return -1.0;
    return 0.0;
}

//Cosinus et sinus de l'arc joignant A et B (en radians)
//with spherical coordinates divided by radius
bool arc(tdouble Xa, tdouble Ya, tdouble Xb, tdouble Yb, tdouble &C, tdouble &S, tdouble radius)
{
    tdouble tmp=sqrt(sqr(Xb/radius-Xa/radius)+sqr(Yb/radius-Ya/radius));
    C=cos(tmp);
    S=sin(tmp);
    /*Coord latlongA, latlongB; //version libproj: does not work if not applied everywhere
    Project::theone()->projection.sphericalToLatLong(Coord(Xa, Ya, 0), latlongA);
    Project::theone()->projection.sphericalToLatLong(Coord(Xb, Yb, 0), latlongB);
    tdouble angX = (latlongB.x()-latlongA.x())*PI/180*cos(toRad(Project::theone()->projection.latitude, DEG));
    tdouble angY = (latlongB.y()-latlongA.y())*PI/180;
    tdouble ang=sqrt(sqr(angX)+sqr(angY));
    *C=cos(ang);
    *S=sin(ang);
    std::cout<<"arc: "<<tmp<<" ("<<Xb/radius-Xa/radius<<" "<<Yb/radius-Ya/radius<<")   "
             <<ang<<" ("<<angX<<" "<<angY<<")   "<<cos(toRad(Project::theone()->projection.latitude, DEG))<<std::endl;*/
    return true;
}

//Cosinus et sinus de l'arc joignant A et B (en radians)
//Nouvelle formule Janvier 1994 : produit scalaire et norme du produit vectoriel
//arc between two points in cartesian
bool arc_cartesian(tdouble Xa,tdouble Ya,tdouble Xb,tdouble Yb,tdouble &C, tdouble &S)
{
    try
    {
        tdouble Za = sqrt(1-sqr(Xa)-sqr(Ya));
        tdouble Zb = sqrt(1-sqr(Xb)-sqr(Yb));
        C  = Xa*Xb+Ya*Yb+Za*Zb;
        S  = sqrt(sqr(Ya*Zb-Yb*Za)+sqr(Xa*Zb-Xb*Za)+sqr(Xa*Yb-Xb*Ya));
    }
    catch (std::exception& e)
    {
        std::cout<<"Error: \"" << e.what() <<"\""<<" when computing arc."<<std::endl;
        return false;
    }
    return true;
}

//Azimut entre 2 points A et B, coordonnees en metres, C de l'angle AOB
//Nouvelle formule Janvier 1994
tdouble azimuth(tdouble Xa,tdouble Ya,tdouble Xb,tdouble Yb,tdouble C,tdouble radius)
{
    tdouble Za = sqrt(1-sqr(Xa/radius)-sqr(Ya/radius));
    tdouble Zb = sqrt(1-sqr(Xb/radius)-sqr(Yb/radius));
    return atan2(Xb-C*Xa,Yb*Za-Ya*Zb) + Project::theone()->projection.getStereoMeridianConvergence( {Xa, Ya, 0.} );
}

tdouble gauss_random(tdouble m, tdouble s)
{
    return Randomness::var_nor()*s+m;
}

tdouble toRad(tdouble ang,ANGLE_UNIT unit)
{
    tdouble result=ang;
    switch(unit)
    {
    case ANGLE_UNIT::RAD:
        break;
    case ANGLE_UNIT::GRAD:
        result= ang*PI/200.0;
        break;
    case ANGLE_UNIT::DEG:
        result= ang*PI/180.0;
        break;
    case ANGLE_UNIT::ARCSEC:
        result= ang*PI/180.0/3600.0;
        break;
    }
    return result;
}

tdouble fromRad(tdouble ang, ANGLE_UNIT unit)
{
    tdouble result=ang;
    switch(unit)
    {
    case ANGLE_UNIT::RAD:
        break;
    case ANGLE_UNIT::GRAD:
        result= ang/PI*200.0;
        break;
    case ANGLE_UNIT::DEG:
        result= ang/PI*180.0;
        break;
    case ANGLE_UNIT::ARCSEC:
        result= ang/PI*180.0*3600.0;
        break;
    }
    return result;
}

tdouble convertAngle(tdouble ang, ANGLE_UNIT unit_from, ANGLE_UNIT unit_to)
{
    return fromRad(toRad(ang, unit_from), unit_to);
}

/*
 * in orginal comp3d, matrix are Type_Rotation = array[0..8] of extended;
 * M[2]=m(0,2) etc...
 **/
void abc2R(tdouble a,tdouble b,tdouble c,tdouble theta,Mat3 &R)
{
    tdouble cost1 = 1-cos(theta);
    tdouble sint  =   sin(theta);
    tdouble d     = sqrt(sqr(a)+sqr(b)+sqr(c));
    tdouble u     = a/d;
    tdouble v     = b/d;
    tdouble w     = c/d;
    R(0,0)= 1-(1-sqr(u))*cost1;
    R(0,1)=  -w*sint+u*v*cost1;
    R(0,2)=   v*sint+u*w*cost1;
    R(1,0)=   w*sint+u*v*cost1;
    R(1,1)= 1-(1-sqr(v))*cost1;
    R(1,2)=  -u*sint+v*w*cost1;
    R(2,0)=  -v*sint+u*w*cost1;
    R(2,1)=   u*sint+v*w*cost1;
    R(2,2)= 1-(1-sqr(w))*cost1;
}

void R2abc(const Mat3 &R, tdouble &theta, tdouble &a, tdouble &b, tdouble &c)
{
    tdouble u = R(0,1)-R(1,0);  // = R-tr(R)
    tdouble v = R(0,2)-R(2,0);
    tdouble w = R(1,2)-R(2,1);
    tdouble sint  = sqrt(sqr(u)+sqr(v)+sqr(w))/2;
    tdouble cost  = (R(0,0)+R(1,1)+R(2,2)-1)/2;   // = (trace(R)-1)/2

    theta = atan2(sint,cost);
    if (sint != 0.0)
    {
       a = -w/(2*sint);
       b =  v/(2*sint);
       c = -u/(2*sint);
    }else{
       a = cost;
       b = 0;
       c = 0;
    }
}

bool Thomson_Shut(const std::vector<Coord> &Xm, const std::vector<Coord> &Xt, Mat3 &R, Vect3 &T, tdouble &scale)
{
    /*--------------------------------------------------------------------------
        Similitude de passage d'un systeme de coordonnees M (model)
                a un systeme de coordonnees T (terrain).
        Il faut au moins 3 points.
            NP		nombre de points
            XM,YM,ZM	coordonnees de depart
            XT,YT,ZT	coordonnees d'arrivee
            R		rotation de la similitude
            T		translation de la similitude
            E		homothetie

        On a : (xt,yt,zt) = T + E * R (xm,ym,zm)
     --------------------------------------------------------------------------*/
    if ((Xm.size()!=Xt.size())||(Xm.size()<3))
    {
        std::cout<<"Unable to perform Thomson_Shut with only "<<Xm.size()<<" and "<<Xt.size()<<" points."<<std::endl;
        return false;
    }

    Coord Gt(0,0,0);//gravity center of "terrain" frame
    Coord Gm(0,0,0);//gravity center of "model" frame
    for (const auto &c :Xm)
        Gm+=c;
    Gm/=Xm.size();          // NOLINT: narrowing conversion from long to double
    for (const auto &c :Xt)
        Gt+=c;
    Gt/=Xt.size();          // NOLINT: narrowing conversion from long to double

    tdouble rm=0;//model radius
    tdouble rt=0;//terrain radius
    for (const auto &c :Xm)
        rm+=sqr(c.x()-Gm.x())+sqr(c.y()-Gm.y())+sqr(c.z()-Gm.z());
    for (const auto &c :Xt)
        rt+=sqr(c.x()-Gt.x())+sqr(c.y()-Gt.y())+sqr(c.z()-Gt.z());

    if ((fabs(rm)<0.000001)||(fabs(rt)<0.000001))
    {
        std::cout<<"Unable to perform Thomson_Shut, points are mixed up!"<<std::endl;
        return false;
    }

    scale=sqrt(rt/rm);

    Coord X1;
    Coord X2;
    //least squares for rotation only
    MatX A(Xm.size()*3,3);
    VectX B(Xm.size()*3);
    for (Eigen::Index i=0;i<static_cast<Eigen::Index>(Xt.size());i++)
    {
        X1=Xm[i]-Gm;
        X1*=scale;
        X2=Xt[i]-Gt;
        /*A(i*3+0,0)=0;
        A(i*3+0,1)=-(X1.z()+X2.z());
        A(i*3+0,2)=X1.y()+X2.y();
        B(i*3+0)=  X1.x()-X2.x();

        A(i*3+1,0)=X1.z()+X2.z();
        A(i*3+1,1)= -(X1.x()+X2.x());
        A(i*3+1,2)=0;
        B(i*3+1)=  X1.y()-X2.y();

        A(i*3+2,0)=-(X1.y()+X2.y());
        A(i*3+2,1)=X1.x()+X2.x();
        A(i*3+2,2)=0;
        B(i*3+2)=  X1.z()-X2.z();*/

        A(i*3+0,0)=0;
        A(i*3+0,1)=-(X1.z()+X2.z());
        A(i*3+0,2)=X1.y()+X2.y();
        B(i*3+0)=  X1.x()-X2.x();

        A(i*3+1,0)=X1.z()+X2.z();
        A(i*3+1,1)=0;
        A(i*3+1,2)= -(X1.x()+X2.x());
        B(i*3+1)=  X1.y()-X2.y();

        A(i*3+2,0)=-(X1.y()+X2.y());
        A(i*3+2,1)=X1.x()+X2.x();
        A(i*3+2,2)=0;
        B(i*3+2)=  X1.z()-X2.z();
    }
    MatX AtA=(A.transpose())*A;
    MatX AtB=(A.transpose())*B;
    VectX solution = AtA.colPivHouseholderQr().solve(AtB);

    tdouble N=solution.norm();

    if (N<1e-30)
    {
        solution(0)=0;
        solution(1)=0;
        solution(2)=1;
        N=0;
    }else
        solution=solution/N;

    N=2*atan(N);
    abc2R(solution[0],solution[1],solution[2],N,R);

    Vect3 vect_Gm,vect_Gt;
    vect_Gm<<Gm.x(),Gm.y(),Gm.z();
    vect_Gt<<Gt.x(),Gt.y(),Gt.z();
    T=R*vect_Gm;//translation
    T=T*scale;
    T=vect_Gt-T;

    return true;
}

Mat3 axiator(const Vect3 &V)
{
    Mat3 Ax;
    Ax<<    0,-V(2), V(1),
         V(2),    0,-V(0),
        -V(1), V(0),    0;
    return Ax;
}

Json::Value R2json(Mat3 R)
{
    Json::Value val;
    for (int j=0;j<3;j++)
    {
        Json::Value val_col;
        for (int i=0;i<3;i++)
            val_col.append(static_cast<double>(R(j,i)));
        val.append(val_col);
    }
    return val;
}

Mat3 angEN2Rot_small_angles(tdouble ang_e, tdouble ang_n)
{
    Mat3 Reta;
    Reta <<  cos(ang_e), 0, -sin(ang_e),
                0    , 1,    0,
             sin(ang_e), 0,  cos(ang_e);

    Mat3 Rxi;
    Rxi <<  1,    0,      0,
            0, cos(ang_n), -sin(ang_n),
            0, sin(ang_n),  cos(ang_n);
    return Reta*Rxi;//small rotations, order is not important
}

Mat3 vertDeflection2Rot(tdouble eta, tdouble xi)
{
    if (Project::theone()->use_vertical_deflection)
        return angEN2Rot_small_angles(eta, xi);
    return Mat3::Identity();
}

Mat3 global2vertRot(const Coord &center_spherical, tdouble eta, tdouble xi)
{
    Mat3 R_global2vert;
    Mat3 Rx, Ry;
    Coord latlong;
    Project::theone()->projection.sphericalToLatLong(center_spherical, latlong);
    latlong = latlong - Project::theone()->projection.centerGeographic;
    tdouble angX = latlong.x()*PI/180;
    tdouble angY = latlong.y()*PI/180;
    Rx << cos(angX), 0, -sin(angX),
                  0, 1,          0,
          sin(angX), 0,  cos(angX);

    Ry << 1,          0,          0,
          0,  cos(angY), -sin(angY),
          0,  sin(angY),  cos(angY);

    R_global2vert = Rx*Ry;

    R_global2vert = vertDeflection2Rot(eta, xi) * R_global2vert;
    return R_global2vert;
}

DMSAngle::DMSAngle(tdouble ang, ANGLE_UNIT unit, bool forcePositive)
{
    tdouble angle_rad=toRad(ang, unit);
    //force to be in -2pi:+2pi
    while (angle_rad<-2*PI)
        angle_rad+=2*PI;
    while (angle_rad>2*PI)
        angle_rad-=2*PI;
    if ((forcePositive)&&(angle_rad<0))
        angle_rad+=2*PI;
    isNegative = angle_rad<0;
    angle_rad = fabs(angle_rad);
    tdouble angle_deg=angle_rad/PI*180.0;
    degrees = static_cast<int>(angle_deg);
    minutes = static_cast<int>((angle_deg-degrees)*60);
    secondes = ((angle_deg-degrees-minutes/60.0)*3600);
}

std::string angToDMSString(tdouble angle, ANGLE_UNIT unit, bool forcePositive, int nb_dec)
{
    std::ostringstream oss;
    DMSAngle dms(angle, unit, forcePositive);
    if (dms.isNegative)
        oss << "-";
    oss<<dms.degrees<<"°"<<dms.minutes<<"\'"<<std::setprecision(nb_dec)<<std::fixed<<dms.secondes<<"\"";
    return oss.str();
}

void normalizeAngle(tdouble &a) // -PI to +2*PI
{
    if (std::isfinite(a))
    {
        while (a < -PI)
           a += 2*PI;
        while (a > 2*PI)
           a -= 2*PI;
    }
}

bool pointsAreAligned(const std::vector<Coord> &pts)
{
    if (pts.size()<3)
        return true;
    Coord farthest = pts[0]; // farthest point from pts[0]
    for (const auto & p: pts)
        if ((p-pts[0]).norm2()>(farthest-pts[0]).norm2()) farthest = p;

    for (unsigned int i=1;i<pts.size();++i)
        if ((farthest-pts[0]).dotProduct(pts[i]-pts[0]).norm2() > CAP_CLOSE*CAP_CLOSE )
            return false;

    return true;
}
