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

#include <string>
#include <sstream>

#include "tests_coord.h"




Tests_Coord::Tests_Coord():
    center(0.0,0.0,0.0),p1(),project("./data/simple/toto.comp"),st1(Station::create<Station_Bascule>(&p1))
{
    project.mInfo.setDirect(false);
    project.config.centerLatitude=45.0;//< in DMS!
    project.config.localCenter=Coord(0,0,0);
    project.config.refraction=0.12;
    project.config.filesUnit=ANGLE_UNIT::GRAD;
    project.config.maxIterations=10;
    project.config.convergenceCriterion=1e-3;
    project.config.forceIterations=0;
    project.config.nbDigits=8;
    project.projection.initLocal(project.config.centerLatitude,project.config.localCenter,1.0);
}

void Tests_Coord::test_sphericalToCartesian()
{
    Coord in,out1,out2,out3,in2,vert_ref_spher;
    in.set(0,0,0);
    vert_ref_spher.set(0,0,0);
    tdouble dev_eta=0.0001;
    tdouble dev_xi=-0.0002;
    Project::theone()->projection.sphericalToGlobalCartesian(in,out1);
    Project::theone()->projection.globalCartesianToVertCartesian(out1, out2,vert_ref_spher, dev_eta, dev_xi);
    Project::theone()->projection.vertCartesianToGlobalCartesian(out2, out3,vert_ref_spher, dev_eta, dev_xi);
    Project::theone()->projection.globalCartesianToSpherical(out3, in2);
    QCOMPARE((qAbs(in.x()-in2.x()))<0.000001, true);
    QCOMPARE((qAbs(in.y()-in2.y()))<0.000001, true);
    QCOMPARE((qAbs(in.z()-in2.z()))<0.000001, true);

    in.set(1000,0,0);
    vert_ref_spher.set(0,0,0);
    Project::theone()->projection.sphericalToGlobalCartesian(in,out1);
    Project::theone()->projection.globalCartesianToVertCartesian(out1, out2,vert_ref_spher, dev_eta, dev_xi);
    Project::theone()->projection.vertCartesianToGlobalCartesian(out2, out3,vert_ref_spher, dev_eta, dev_xi);
    Project::theone()->projection.globalCartesianToSpherical(out3, in2);
    QCOMPARE((qAbs(in.x()-in2.x()))<0.000001, true);
    QCOMPARE((qAbs(in.y()-in2.y()))<0.000001, true);
    QCOMPARE((qAbs(in.z()-in2.z()))<0.000001, true);

    in.set(1000,5,24);
    vert_ref_spher.set(1000,5,24);
    Project::theone()->projection.sphericalToGlobalCartesian(in,out1);
    Project::theone()->projection.globalCartesianToVertCartesian(out1, out2,vert_ref_spher, dev_eta, dev_xi);
    Project::theone()->projection.vertCartesianToGlobalCartesian(out2, out3,vert_ref_spher, dev_eta, dev_xi);
    Project::theone()->projection.globalCartesianToSpherical(out3, in2);
    QCOMPARE((qAbs(in.x()-in2.x()))<0.000001, true);
    QCOMPARE((qAbs(in.y()-in2.y()))<0.000001, true);
    QCOMPARE((qAbs(in.z()-in2.z()))<0.000001, true);

    in.set(1010,0,0);
    vert_ref_spher.set(1000,0,2);
    Project::theone()->projection.sphericalToGlobalCartesian(in,out1);
    Project::theone()->projection.globalCartesianToVertCartesian(out1, out2,vert_ref_spher, dev_eta, dev_xi);
    Project::theone()->projection.vertCartesianToGlobalCartesian(out2, out3,vert_ref_spher, dev_eta, dev_xi);
    Project::theone()->projection.globalCartesianToSpherical(out3, in2);
    QCOMPARE((qAbs(in.x()-in2.x()))<0.000001, true);
    QCOMPARE((qAbs(in.y()-in2.y()))<0.000001, true);
    QCOMPARE((qAbs(in.z()-in2.z()))<0.000001, true);

    in.set(1000,-10,0);
    vert_ref_spher.set(1000,0,0);
    Project::theone()->projection.sphericalToGlobalCartesian(in,out1);
    Project::theone()->projection.globalCartesianToVertCartesian(out1, out2,vert_ref_spher, dev_eta, dev_xi);
    Project::theone()->projection.vertCartesianToGlobalCartesian(out2, out3,vert_ref_spher, dev_eta, dev_xi);
    Project::theone()->projection.globalCartesianToSpherical(out3, in2);
    QCOMPARE((qAbs(in.x()-in2.x()))<0.000001, true);
    QCOMPARE((qAbs(in.y()-in2.y()))<0.000001, true);
    QCOMPARE((qAbs(in.z()-in2.z()))<0.000001, true);
}


void Tests_Coord::test_basc_mes2GroundCart()
{
    Coord mesure,mesure_ground;

    p1.coord_comp.set(0,0,0);
    st1->R_vert2instr=Mat3::Identity();
    st1->mInitOk=true;//initialize() is better
    mesure.set(10,0,0);
    mesure_ground=st1->mes2GlobalCart(mesure,false);
    std::cout<<mesure_ground.toString()<<std::endl;

    QCOMPARE((qAbs(mesure_ground.x()-10))<0.000001, true);
    QCOMPARE((qAbs(mesure_ground.y()-0))<0.000001, true);
    QCOMPARE((qAbs(mesure_ground.z()+0))<0.000001, true);

    p1.coord_comp.set(1000,0,0);
    st1->R_vert2instr=Mat3::Identity();
    mesure.set(10,0,0);
    mesure_ground=st1->mes2GlobalCart(mesure,false);
    std::cout<<mesure_ground.toString()<<std::endl;

    QCOMPARE((qAbs(mesure_ground.x()-1009.99999977))<0.000001, true);
    QCOMPARE((qAbs(mesure_ground.y()-0))<0.000001, true);
    QCOMPARE((qAbs(mesure_ground.z()+0.08047506))<0.000001, true);
}

void Tests_Coord::test_basc_toLocal()
{
    //QCOMPARE((qAbs(0.01))<0.000001, true);
}

void Tests_Coord::test_proj()
{
    Projection proj;

}

void Tests_Coord::test_NTF()
{
#ifdef DEBUG_NTF
    {
        Projection::init_allEPSG();
        std::vector<Coord> allPtsNTF={{606400, 127100,0},{606500, 127100,0},{606500, 127200,0},{606400, 127200,0}};//Lambert1
        std::vector<Coord> allPtsCart;
        std::vector<Coord> allPtsSter;
        std::vector<Coord> allPtsLocalCart;
        std::vector<Coord> allPtsSpher;
        PJ_COORD p1,p2,p3;
        PJ *Pg = proj_create_crs_to_crs(nullptr, "IGNF:LAMB1", "+proj=latlong EPSG:6011", nullptr);
        std::ostringstream oss;
        if (!Pg)
        {
            std::cout<<"Error "<<proj_errno(Pg)<<": "<<proj_errno_string(proj_errno(Pg))<<std::endl;
        }
        QCOMPARE(Pg!=nullptr, true);

        Projection proj;
        proj.initGeo(allPtsNTF[0],"IGNF:LAMB1");

        p1.uvw.u = allPtsNTF[0].x();
        p1.uvw.v = allPtsNTF[0].y();
        p1.uvw.w = 0;
        p2 = proj_trans(Pg, PJ_FWD, p1);
        Coord center_geo(p2.uvw.u,p2.uvw.v,0);
        printf("geog: %12.8f %12.8f\n",p2.uvw.u,p2.uvw.v);
        oss<<std::fixed;
        oss.width(15);
        oss<<"+proj=sterea +lat_0="<<p2.uvw.v<<" +lon_0="<<p2.uvw.u<<" +k_0=1 +x_0=0 +y_0=0";

        std::string stereo_def=oss.str();
        std::cout<<"stereo_def: "<<stereo_def<<std::endl;
        PJ *P = proj_create_crs_to_crs(nullptr, "IGNF:LAMB1", "+proj=geocent +ellps=WGS84  ", nullptr);
        //PJ *P  = proj_create_crs_to_crs(nullptr, "IGNF:LAMB1", "+proj=latlong +ellps=WGS84 ", nullptr);
        //with +ellps=WGS84 the scale is perfect, but impossible to keep it in stereo
        //PJ *P = proj_create_crs_to_crs(nullptr, "IGNF:LAMB1", stereo_def.c_str(), nullptr);
        PJ *P2 = proj_create_crs_to_crs(nullptr, "+proj=latlong",stereo_def.c_str(), nullptr);
        if (!P)
            std::cout<<"Error "<<proj_errno(P)<<": "<<proj_errno_string(proj_errno(P))<<std::endl;
        QCOMPARE(P!=nullptr, true);
        if (!P2)
            std::cout<<"Error "<<proj_errno(P2)<<": "<<proj_errno_string(proj_errno(P2))<<std::endl;
        QCOMPARE(P2!=nullptr, true);

        for (auto &pt:allPtsNTF)
        {
            p1.uvw.u = pt.x();
            p1.uvw.v = pt.y();
            p1.uvw.w = 0;
            p2 = proj_trans(P, PJ_FWD, p1);
            allPtsCart.push_back(Coord(p2.xyz.x,p2.xyz.y,p2.xyz.z));
            printf("%12.4f %12.4f %12.4f => %12.4f %12.4f %12.4f\n",p1.xyz.x,p1.xyz.y,p1.xyz.z,p2.xyz.x,p2.xyz.y,p2.xyz.z);
            p3 = proj_trans(P2, PJ_FWD, p2);
            allPtsSter.push_back(Coord(p3.xyz.x,p3.xyz.y,p3.xyz.z));
            printf("                                       => %12.4f %12.4f %12.4f\n",p3.xyz.x,p3.xyz.y,p3.xyz.z);
        }
        proj_destroy(P);
        proj_destroy(P2);
        for (auto &pt:allPtsCart)
        {
            tdouble longitude=center_geo.x()*PI/180.0;
            tdouble latitude=center_geo.y()*PI/180.0;
            Eigen::Matrix3d Rz;
            Rz <<  cos(longitude), sin(longitude), 0,
                 -sin(longitude), cos(longitude), 0,
                    0,                          0,                      1;

            Eigen::Matrix3d Ry;
            Ry<< cos(PI/2-latitude), 0,  -sin(PI/2-latitude),
                    0 ,         1,       0,
                 sin(PI/2-latitude), 0,   cos(PI/2-latitude);

            Eigen::Matrix3d Rz2;
            Rz2<< 0,  1,  0,
                 -1,  0,  0,
                  0,  0,  1;

            Eigen::Matrix3d RotLocal2Geocentric=(Rz2*Ry*Rz).transpose();

            allPtsLocalCart.push_back(Coord(RotLocal2Geocentric.transpose()*(pt-allPtsCart[0]).toVect()));
            std::cout<<"Local cart: "<<allPtsLocalCart.back().toString()<<"\n";

            Coord spher;
            proj.cartesianToSpherical(allPtsLocalCart.back(),spher);
            allPtsSpher.push_back(spher);
            std::cout<<"Local spher: "<<allPtsSpher.back().toString()<<"\n";
        }
        double mu=(sqrt((allPtsCart[0]-allPtsCart[1]).norm2())-100)/100.0*1e6;
        std::cout<<"dNorm(AB)="<<mu<<"\n";
        //QCOMPARE(qAbs(mu-57)<1, true);
        mu=(sqrt((allPtsCart[1]-allPtsCart[2]).norm2())-100)/100.0*1e6;
        std::cout<<"dNorm(BC)="<<mu<<"\n";
        //QCOMPARE(qAbs(mu-57)<1, true);
        mu=(sqrt((allPtsCart[2]-allPtsCart[3]).norm2())-100)/100.0*1e6;
        std::cout<<"dNorm(CD)="<<mu<<"\n";
        mu=(sqrt((allPtsCart[3]-allPtsCart[0]).norm2())-100)/100.0*1e6;
        std::cout<<"dNorm(DA)="<<mu<<"\n";

        mu=(sqrt((allPtsSter[0]-allPtsSter[1]).norm2())-100)/100.0*1e6;
        std::cout<<"dNorm(AB)="<<mu<<"\n";
        //QCOMPARE(qAbs(mu-57)<1, true);
        mu=(sqrt((allPtsSter[1]-allPtsSter[2]).norm2())-100)/100.0*1e6;
        std::cout<<"dNorm(BC)="<<mu<<"\n";
        //QCOMPARE(qAbs(mu-57)<1, true);
        mu=(sqrt((allPtsSter[2]-allPtsSter[3]).norm2())-100)/100.0*1e6;
        std::cout<<"dNorm(CD)="<<mu<<"\n";
        mu=(sqrt((allPtsSter[3]-allPtsSter[0]).norm2())-100)/100.0*1e6;
        std::cout<<"dNorm(DA)="<<mu<<"\n";

        mu=(sqrt((allPtsLocalCart[0]-allPtsLocalCart[1]).norm2())-100)/100.0*1e6;
        std::cout<<"dNorm(AB)="<<mu<<"\n";
        //QCOMPARE(qAbs(mu-57)<1, true);
        mu=(sqrt((allPtsLocalCart[1]-allPtsLocalCart[2]).norm2())-100)/100.0*1e6;
        std::cout<<"dNorm(BC)="<<mu<<"\n";
        //QCOMPARE(qAbs(mu-57)<1, true);
        mu=(sqrt((allPtsLocalCart[2]-allPtsLocalCart[3]).norm2())-100)/100.0*1e6;
        std::cout<<"dNorm(CD)="<<mu<<"\n";
        mu=(sqrt((allPtsLocalCart[3]-allPtsLocalCart[0]).norm2())-100)/100.0*1e6;
        std::cout<<"dNorm(DA)="<<mu<<"\n";

        mu=(sqrt((allPtsSpher[0]-allPtsSpher[1]).norm2())-100)/100.0*1e6;
        std::cout<<"dNorm(AB)="<<mu<<"\n";
        //QCOMPARE(qAbs(mu-57)<1, true);
        mu=(sqrt((allPtsSpher[1]-allPtsSpher[2]).norm2())-100)/100.0*1e6;
        std::cout<<"dNorm(BC)="<<mu<<"\n";
        //QCOMPARE(qAbs(mu-57)<1, true);
        mu=(sqrt((allPtsSpher[2]-allPtsSpher[3]).norm2())-100)/100.0*1e6;
        std::cout<<"dNorm(CD)="<<mu<<"\n";
        mu=(sqrt((allPtsSpher[3]-allPtsSpher[0]).norm2())-100)/100.0*1e6;
        std::cout<<"dNorm(DA)="<<mu<<"\n";
    }
#endif

    std::vector<Coord> allPtsNTF={{606400, 127100,0},{606500, 127100,0},{606500, 127200,0},{606400, 127200,0}};//Lambert1
    std::vector<Coord> allPtsSpher;
    Projection proj;
    proj.initGeo(allPtsNTF[0],"IGNF:LAMB1");
    for (auto &pt:allPtsNTF)
    {
        Coord spher;
        proj.georefToSpherical(pt,spher);
        allPtsSpher.push_back(spher);
        std::cout<<"Local spher: "<<allPtsSpher.back().toString()<<"\n";
    }
    double mu=(sqrt((allPtsSpher[0]-allPtsSpher[1]).norm2())-100)/100.0*1e6;
    std::cout<<"dNorm(AB)="<<mu<<"\n";
    QCOMPARE(qAbs(mu-50)<1, true);
    mu=(sqrt((allPtsSpher[1]-allPtsSpher[2]).norm2())-100)/100.0*1e6;
    std::cout<<"dNorm(BC)="<<mu<<"\n";
    QCOMPARE(qAbs(mu-51)<1, true);
    mu=(sqrt((allPtsSpher[2]-allPtsSpher[3]).norm2())-100)/100.0*1e6;
    std::cout<<"dNorm(CD)="<<mu<<"\n";
    mu=(sqrt((allPtsSpher[3]-allPtsSpher[0]).norm2())-100)/100.0*1e6;
    std::cout<<"dNorm(DA)="<<mu<<"\n";
}


void Tests_Coord::test_RGF()
{
    std::vector<Coord> allPtsNTF={{657723, 6860710,0},{657823, 6860710,0},{657823, 6860810,0},{657723, 6860810,0}};//Lambert93
    std::vector<Coord> allPtsSpher;
    Projection proj;
    proj.initGeo(allPtsNTF[0],"IGNF:LAMB93");
    for (auto &pt:allPtsNTF)
    {
        Coord spher;
        proj.georefToSpherical(pt,spher);
        allPtsSpher.push_back(spher);
        std::cout<<"Local spher: "<<allPtsSpher.back().toString()<<"\n";
    }
    double mu=(sqrt((allPtsSpher[0]-allPtsSpher[1]).norm2())-100)/100.0*1e6;
    std::cout<<"dNorm(AB)="<<mu<<"\n";
    QCOMPARE(qAbs(mu-115)<1, true);
    mu=(sqrt((allPtsSpher[1]-allPtsSpher[2]).norm2())-100)/100.0*1e6;
    std::cout<<"dNorm(BC)="<<mu<<"\n";
    QCOMPARE(qAbs(mu-115)<1, true);
    mu=(sqrt((allPtsSpher[2]-allPtsSpher[3]).norm2())-100)/100.0*1e6;
    std::cout<<"dNorm(CD)="<<mu<<"\n";
    mu=(sqrt((allPtsSpher[3]-allPtsSpher[0]).norm2())-100)/100.0*1e6;
    std::cout<<"dNorm(DA)="<<mu<<"\n";
}


void Tests_Coord::test_angles()
{
    std::map<std::pair<tdouble, bool>,std::string> angles_rad_dms = {
        {{-10,false}, "-212°57'28.06247096\""},
        {{-10,true}, "147°2'31.93752904\""},
        {{-8,false}, "-98°21'58.44997677\""},
        {{-5,false}, "-286°28'44.03123548\""},
        {{-2,false}, "-114°35'29.61249419\""},
        {{-1,false}, "-57°17'44.80624710\""},
        {{0,false}, "0°0'0.00000000\""},
        {{1,false}, "57°17'44.80624710\""},
        {{3,false}, "171°53'14.41874129\""},
        {{11,false}, "270°15'12.86871806\""},
    };

    for (auto& v : angles_rad_dms)
    {
        auto ang_rad = v.first.first;
        auto force_pos = v.first.second;
        auto ang_str = v.second;
        std::string ang_dms_str = angToDMSString(ang_rad, ANGLE_UNIT::RAD, force_pos,8);
        //std::cout<<"angle: "<<ang_rad<<":"<<force_pos<<": "<<ang_dms_str<<" "<<ang_str<<"\n";
        QCOMPARE(ang_dms_str, ang_str);
    }
}

//QTEST_MAIN(Tests_Coord)
//#include "moc_tests_coord.cpp"

