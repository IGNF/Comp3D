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

#include "tests_stab.h"

#include <boost/filesystem.hpp>
#include "compcompare.h"

Tests_Stab::Tests_Stab()
{

}


void Tests_Stab::test_template(std::string _refFilename, std::string refMD5, bool verbose,
                               std::vector< std::pair<std::string,std::string> > moreComparePaths,
                               double tolerancy)
{
    std::vector< std::pair<std::string,std::string> > comparePaths={
        {"/computation/compensationDone", "/computation/compensation_done"}, // for old stability tests
        {"/computation/compensation_done", "/computation/compensation_done"}, // for new stability tests
        {"/computation/projection/center_latitude", ""},
        {"/computation/nbr_active_obs", ""},
        {"/computation/nbr_all_obs", ""},
        {"/computation/nbr_iterations", ""},
        {"/computation/nbr_parameters", ""},
        {"/computation/sigma_0_final", ""}
    };
    comparePaths.reserve(comparePaths.size()+moreComparePaths.size());
    comparePaths.insert(comparePaths.end(), moreComparePaths.begin(), moreComparePaths.end());

    std::string _compFilename=_refFilename+"_test.comp";

    //copy .comp file
    try{
        boost::filesystem::remove(_compFilename);
        boost::filesystem::copy_file(_refFilename,_compFilename,boost::filesystem::copy_option::overwrite_if_exists);
    } catch (const boost::filesystem::filesystem_error& e)
    {
        std::cerr<<e.what()<<std::endl;
    }

    //copy files from origin/ (for .xyz, that are modified by tests)
    boost::filesystem::path working_path(_refFilename);
    working_path.remove_filename();
    try{
        for(    boost::filesystem::directory_iterator file(working_path / "origin");
                file != boost::filesystem::directory_iterator();
                ++file )
        {
            boost::filesystem::path current(file->path());
            boost::filesystem::copy_file(current, working_path / current.filename(),boost::filesystem::copy_option::overwrite_if_exists);
            std::cout<<"Copy "<<current<<" to "<<working_path / current.filename()<<std::endl;
        }
    } catch (const boost::filesystem::filesystem_error& e)
    {
        std::cout<<e.what()<<std::endl;
    }

    Project project(_compFilename);
    project.mInfo.setDirect(false);
    project.read_config();
    project.saveasJSON(); //make sure the json file is cleared
    project.readData();
    project.set_least_squares(project.config.internalConstr);
    project.computation(project.config.invert,true);
    CompCompare ccp(_compFilename,_refFilename,refMD5,verbose,tolerancy);
    QCOMPARE(!ccp.isError(),true);
    QCOMPARE(ccp.checkOnly(comparePaths),true);
}

void Tests_Stab::test_changement_de_repere()
{
    std::cout<<"\nPrepare test_changement_de_repere"<<std::endl;
    std::string refFile="./data_stab/changement_de_repere/ex_ref.comp";
    std::string refMD5="a023e10793d5e4f3e3df984cbcae227a";

    test_template(refFile, refMD5, false, {{"/computation/all_sigma0",""}});
}

void Tests_Stab::test_convergence()
{
    std::cout<<"\nPrepare test_convergence"<<std::endl;
    std::string refFile="./data_stab/convergence/ex_ref.comp";
    std::string refMD5="082654c3d9492740e45cbe25676f3dfb";

    test_template(refFile, refMD5, false, {{"/computation/all_sigma0",""}});
}

void Tests_Stab::test_egouts()
{
    std::cout<<"\nPrepare test_egouts"<<std::endl;
    std::string refFile="./data_stab/egouts/ex_ref.comp";
    std::string refMD5="9c9159aab240ebf6fae535cbe649e3a2";

    test_template(refFile, refMD5, false, {}, 0.0003); // more tolerancy because of the new sigma_total
}

void Tests_Stab::test_modane()
{
    std::cout<<"\nPrepare test_modane"<<std::endl;
    std::string refFile="./data_stab/modane/ex_ref.comp";
    std::string refMD5="3de3773373dbfc2e289aa7952108b526";

    test_template(refFile, refMD5, false, {{"/computation/all_sigma0",""}});
}

void Tests_Stab::test_polygone_K06()
{
    std::cout<<"\nPrepare test_polygone_K06"<<std::endl;
    std::string refFile="./data_stab/polygone_K06/ex_ref.comp";
    std::string refMD5="0af6a40dcf66522794a3729bc61c94c7";

    test_template(refFile, refMD5, false, {{"/computation/all_sigma0",""}});
}
