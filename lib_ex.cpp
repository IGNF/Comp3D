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

#include <iostream>
#include <string>
#include "src/project.h"
#include "src/mathtools.h"
#include "src/datafile.h"
#include "src/station_simple.h"


int main_lib(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    std::cout<<"\n----------------------------------------------------------------"<<std::endl;
    std::cout<<" "<<COMP3D_VERSION<<" -- git commit "<<GIT_VERSION<<std::endl;
    std::cout<<"options: "<<COMP3D_OPTIONS<<" -- built "<<__DATE__<<" "<<__TIME__<<std::endl;
    std::cout<<COMP3D_COPYRIGHT<<" -- "<<COMP3D_LICENSE<<std::endl;
    std::cout<<COMP3D_REPO<<std::endl;
    std::cout<<"----------------------------------------------------------------\n"<<std::endl;

    Project project("toto.comp");
    project.mInfo.setDirect(false);

    project.config.compute_type=COMPUTE_TYPE::type_compensation;
    project.config.centerLatitude=45.0;//< in DMS!
    project.config.localCenter=Coord(0,0,0);
    project.config.refraction=0.12;
    project.config.filesUnit=ANGLE_UNIT::GRAD;
    project.config.maxIterations=10;
    project.config.convergenceCriterion=1e-3;
    project.config.forceIterations=0;
    project.projection.initLocal(project.config.centerLatitude,project.config.localCenter,1.0);

    project.points.emplace_back(nullptr,1);
    project.points.back().read_point("1 PT1 10 10 10 0.01 0.01 0.01");
    project.points.back().set_point(true);
    project.points.emplace_back(nullptr,1);
    project.points.back().read_point("0 PT2 20 10 15 0.01 0.01 0.01");
    project.points.back().set_point(true);

    auto file=OBSFile::create("?");
    Station_Simple *station = project.points.front().getLastStation<Station_Simple>(1);
    station->read_obs("-4 PT1 PT2 10.1 0.01 0 0 0",1,file.get(),".","");
    station->read_obs("-5 PT1 PT2  0.2 0.01 0 0 0",1,file.get(),".","");
    station->read_obs("-6 PT1 PT2  5.3 0.01 0 0 0",1,file.get(),".","");

    project.dataRead=true;
    project.set_least_squares(false);
    project.computation(false,true);
    project.print_points();

    std::cout<<"\n------- Errors:\n"<<project.mInfo.toStrRaw()<<"------- end of errors.\n"<<std::endl;

    std::cout<<"----------------------------------------------------------------"<<std::endl;
    std::cout<<"------------------- end of "<<COMP3D_VERSION<<" ------------------"<<std::endl;
    std::cout<<"----------------------------------------------------------------"<<std::endl;
    return 0;
}

