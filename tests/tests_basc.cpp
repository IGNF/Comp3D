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

#include "tests_basc.h"

#include <boost/filesystem.hpp>
#include "station_bascule.h"

Tests_Basc::Tests_Basc() :
    filename("./data/bascule/ex_ref.comp"),
    project(filename+"_test.comp")
{
    std::cout<<"Prepare Tests_Basc"<<std::endl;
    try{
        boost::filesystem::remove(filename+"_test.comp");
        boost::filesystem::copy(filename,filename+"_test.comp");
    } catch (const boost::filesystem::filesystem_error& e)
    {
        std::cerr<<e.what()<<std::endl;
    }

    //copy files from origin/ (for .xyz, that are modified by tests)
    boost::filesystem::path working_path(filename);
    working_path.remove_filename();
    try{
        for(    boost::filesystem::directory_iterator file(working_path / "origin");
                file != boost::filesystem::directory_iterator();
                ++file )
        {
            boost::filesystem::path current(file->path());
            boost::filesystem::remove(working_path / current.filename());
            boost::filesystem::copy(current, working_path / current.filename());
            std::cout<<"Copy "<<current<<" to "<<working_path / current.filename()<<std::endl;
        }
    } catch (const boost::filesystem::filesystem_error& e)
    {
        std::cout<<e.what()<<std::endl;
    }

    project.mInfo.setDirect(false);
}

void Tests_Basc::test_set()
{
    std::cout<<"Tests_Basc::test_set start"<<std::endl;

    QCOMPARE(project.read_config(),true);
    QCOMPARE(project.readData(),true);

    Point *p2=project.getPoint("2");
    QVERIFY(p2);
    QVERIFY(p2->stations_.size()>0);
    Station_Bascule *basc2=dynamic_cast<Station_Bascule*>(p2->stations_.front());
    QVERIFY(basc2);

    QCOMPARE(qAbs(basc2->R_vert2instr(0,0)-1)<0.00001,true);
    QCOMPARE(qAbs(basc2->R_vert2instr(1,0)-0)<0.00001,true);
    QCOMPARE(qAbs(basc2->R_vert2instr(2,0)+7.57175e-06)<0.00001,true);
    QCOMPARE(qAbs(basc2->R_vert2instr(0,1)-0)<0.00001,true);
    QCOMPARE(qAbs(basc2->R_vert2instr(1,1)-1)<0.00001,true);
    QCOMPARE(qAbs(basc2->R_vert2instr(2,1)-0)<0.00001,true);
    QCOMPARE(qAbs(basc2->R_vert2instr(0,2)-7.57175e-06)<0.00001,true);
    QCOMPARE(qAbs(basc2->R_vert2instr(1,2)-0)<0.00001,true);
    QCOMPARE(qAbs(basc2->R_vert2instr(2,2)-1)<0.00001,true);

    QCOMPARE(project.set_least_squares(project.config.internalConstr),true);
    QCOMPARE(project.computation(project.config.invert,true),true);

    std::cout<<"Tests_Basc::test_set end"<<std::endl;
}

void Tests_Basc::test_result()
{
    //centered, vertical station, z=100
    Point *p1=project.getPoint("1");
    QVERIFY(p1);
    QCOMPARE(qAbs(p1->coord_compensated_georef.x()-0)<0.00001,true);
    QCOMPARE(qAbs(p1->coord_compensated_georef.y()-0)<0.00001,true);
    QCOMPARE(qAbs(p1->coord_compensated_georef.z()-99.999216)<0.00001,true);
    Station_Bascule *basc1=dynamic_cast<Station_Bascule*>(p1->stations_.front());
    QCOMPARE(qAbs(basc1->angleInstr2Vert())<0.00001,true);
    QCOMPARE(qAbs(basc1->observations3D.front().obs1->residual-0.0015647)<0.000001,true);
    QCOMPARE(qAbs(basc1->observations3D.front().obs2->residual-0)<0.000001,true);
    QCOMPARE(qAbs(basc1->observations3D.front().obs3->residual-1.144468e-06)<0.000001,true);

    //(200,0) non-vertical station, z=100, asymmetric points
    Point *p2=project.getPoint("2");
    QVERIFY(p2);
    QCOMPARE(qAbs(p2->coord_compensated_georef.x()-200.000392)<0.00001,true);
    QCOMPARE(qAbs(p2->coord_compensated_georef.y()-0)<0.00001,true);
    QCOMPARE(qAbs(p2->coord_compensated_georef.z()-99.998845)<0.00001,true);
    Station_Bascule *basc2=dynamic_cast<Station_Bascule*>(p2->stations_.front());
    QCOMPARE(qAbs(basc2->angleInstr2Vert()*200/PI-0.00048204)<0.000001,true);
    QCOMPARE(qAbs(basc2->observations3D.front().obs1->residual-0.0027381574)<0.000001,true);
    QCOMPARE(qAbs(basc2->observations3D.front().obs2->residual-0)<0.000001,true);
    QCOMPARE(qAbs(basc2->observations3D.front().obs3->residual+0.000246342)<0.000001,true);

    //(1000,0) vertical station, z=1000 (scale error between spherical and cartesian)
    Point *p4=project.getPoint("4");
    QVERIFY(p4);
    QCOMPARE(qAbs(p4->coord_compensated_georef.x()-1000)<0.00001,true);
    QCOMPARE(qAbs(p4->coord_compensated_georef.y()-0)<0.00001,true);
    QCOMPARE(qAbs(p4->coord_compensated_georef.z()-9999.999215)<0.00001,true);
    Station_Bascule *basc4=dynamic_cast<Station_Bascule*>(p4->stations_.front());
    QCOMPARE(qAbs(basc4->angleInstr2Vert())<0.00001,true);
    QCOMPARE(qAbs(basc4->observations3D.front().obs1->residual-0.156471529)<0.000001,true);
    QCOMPARE(qAbs(basc4->observations3D.front().obs2->residual-0)<0.000001,true);
    QCOMPARE(qAbs(basc4->observations3D.front().obs3->residual-0.0081217)<0.000001,true);

    //test read XYZ
    Mat3 R_vert2instr_before=basc2->R_vert2instr;
    basc1->readXYZrotation("./data/bascule/scan1.xyz");
    QCOMPARE(qAbs(R_vert2instr_before(0,0)-basc2->R_vert2instr(0,0))<0.00001,true);
    QCOMPARE(qAbs(R_vert2instr_before(0,1)-basc2->R_vert2instr(0,1))<0.00001,true);
    QCOMPARE(qAbs(R_vert2instr_before(0,2)-basc2->R_vert2instr(0,2))<0.00001,true);
    QCOMPARE(qAbs(R_vert2instr_before(1,0)-basc2->R_vert2instr(1,0))<0.00001,true);
    QCOMPARE(qAbs(R_vert2instr_before(1,1)-basc2->R_vert2instr(1,1))<0.00001,true);
    QCOMPARE(qAbs(R_vert2instr_before(1,2)-basc2->R_vert2instr(1,2))<0.00001,true);
    QCOMPARE(qAbs(R_vert2instr_before(2,0)-basc2->R_vert2instr(2,0))<0.00001,true);
    QCOMPARE(qAbs(R_vert2instr_before(2,1)-basc2->R_vert2instr(2,1))<0.00001,true);
    QCOMPARE(qAbs(R_vert2instr_before(2,2)-basc2->R_vert2instr(2,2))<0.00001,true);

}
