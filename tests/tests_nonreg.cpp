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

#include "tests_nonreg.h"

#include "../src/filesystem_compat.h"
#include "compcompare.h"


Tests_NonReg::Tests_NonReg()
{
}

void Tests_NonReg::test_template(std::string _refFilename, std::string refMD5, bool verbose, std::vector<std::string> comparePaths,
                                 std::vector<std::string> addNoCompareNodePaths, double tolerance)
{
    std::string _compFilename=_refFilename+"_test.comp";

    //copy .comp file
    try{
        fs::remove(_compFilename);
        fs::copy_file(_refFilename,_compFilename,FS_COPY_OVERWRITE_EXISTING);
    } catch (const fs::filesystem_error& e)
    {
        std::cerr<<e.what()<<std::endl;
    }

    //copy files from origin/ (for .xyz, that are modified by tests)
    fs::path working_path(_refFilename);
    working_path.remove_filename();
    try{
        for(    fs::directory_iterator file(working_path / "origin");
                file != fs::directory_iterator();
                ++file )
        {
            fs::path current(file->path());
            fs::copy_file(current, working_path / current.filename(),FS_COPY_OVERWRITE_EXISTING);
            std::cout<<"Copy "<<current<<" to "<<working_path / current.filename()<<std::endl;
        }
    } catch (const fs::filesystem_error& e)
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
    CompCompare ccp(_compFilename,_refFilename,refMD5,verbose,tolerance);
    if (!addNoCompareNodePaths.empty())
        ccp.addNoCompareNodePaths(addNoCompareNodePaths);
    QCOMPARE(!ccp.isError(),true);
    if (comparePaths.empty())
        QCOMPARE(ccp.checkContent(),true);
    else
        QCOMPARE(ccp.checkOnly(comparePaths),true);
}

void Tests_NonReg::test_axe()
{
    std::cout<<"\nPrepare test_axe"<<std::endl;
    std::string refFile="./data/axe/ex_ref.comp";
    std::string refMD5="ffb0e847bab48af6d009ff39f4424dd4";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_baselines()
{
    std::cout<<"\nPrepare test_baselines"<<std::endl;
    std::string refFile="./data/baselines/ex_ref.comp";
    std::string refMD5="399a84b6b3722a45c4fa74004c380046";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_centering()
{
    std::cout<<"\nPrepare test_centering"<<std::endl;
    std::string refFile="./data/centering/ex_ref.comp";
    std::string refMD5="965193746bb4eb1053c15b7044208656";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_coincident_with_VD()
{
    std::cout<<"\nPrepare test_coincident_with_VD"<<std::endl;
    std::string refFile="./data/coincident_with_VD/ex_ref.comp";
    std::string refMD5="c436e9d6d4701ffdb30cb6056ca0befa";
    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_coord_sigma_neg()
{
    std::cout<<"\nPrepare test_coord_sigma_neg"<<std::endl;
    std::string refFile="./data/coord_sigma_neg/ex_ref.comp";
    std::string refMD5="e858c5e71f7c52b031fbcb6e02503416";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_corcov()
{
    std::cout<<"\nPrepare test_corcov"<<std::endl;
    std::string refFile="./data/corcov/ex_ref.comp";
    std::string refMD5="03345eceaa6addf4bc3e95ae534f42be";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_desact()
{
    std::cout<<"\nPrepare test_desact"<<std::endl;
    std::string refFile="./data/desact/ex_ref.comp";
    std::string refMD5="632e98ea3196b31092fd8b9dd771ea91";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_dev_vert()
{
    std::cout<<"\nPrepare test_dev_vert"<<std::endl;
    std::string refFile="./data/dev_vert/ex_ref.comp";
    std::string refMD5="92ea680cba37dc585f463bbc59bc485e";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_eq_constr()
{
    std::cout<<"\nPrepare test_eq_constr"<<std::endl;
    std::string refFile="./data/eq_constr/ex_ref.comp";
    std::string refMD5="419bf3919d29f16eaca6062863bde073";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_errors()
{
    std::cout<<"\nPrepare test_errors"<<std::endl;
    std::string refFile="./data/errors/ex_ref.comp";
    std::string refMD5="9712a25536b768be3d9efb5005a18850";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_fixed_points()
{
    std::cout<<"\nPrepare test_fixed_points"<<std::endl;
    std::string refFile="./data/fixed_points/ex_ref.comp";
    std::string refMD5="628515ed62a484498f6eaca873b9f7ae";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_fixed_points2()
{
    std::cout<<"\nPrepare test_fixed_points2"<<std::endl;
    std::string refFile="./data/fixed_points2/ex_ref.comp";
    std::string refMD5="daa8095c39439c6f15e2eea1b689f72d";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_fixed_points_IC()
{
    std::cout<<"\nPrepare test_fixed_points_IC"<<std::endl;
    std::string refFile="./data/fixed_points_IC/ex_ref.comp";
    std::string refMD5="7f1d86c38ac2cf48617a12de49f8356b";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_forbidden_pt()
{
    std::cout<<"\nPrepare test_forbidden_pt"<<std::endl;
    std::string refFile="./data/forbidden_pt/ex_ref.comp";
    std::string refMD5="6a0ae37254e48c05614f4c6f5d356a3d";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_geo_mini_L93()
{
    std::cout<<"\nPrepare test_geo_mini_L93"<<std::endl;
    std::string refFile="./data/geo_mini_L93/ex_ref.comp";
    std::string refMD5="74124935616cb2453916e9d392dc6d41";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_geo_mini_NTF()
{
    std::cout<<"\nPrepare test_geo_mini_NTF"<<std::endl;
    std::string refFile="./data/geo_mini_NTF/ex_ref.comp";
    std::string refMD5="c13dafcfc98167de6197e912a647c69d";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_init_cap()
{
    std::cout<<"\nPrepare test_init_cap"<<std::endl;
    std::string refFile="./data/init_cap/ex_ref.comp";
    std::string refMD5="c860f173b885c8a48186723bc4290cb8";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_kernel()
{
    std::cout<<"\nPrepare test_kernel"<<std::endl;
    std::string refFile="./data/kernel/ex_ref.comp";
    std::string refMD5="5d328b2ea406231978c8ba8098f81e09";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_minimal()
{
    std::cout<<"\nPrepare test_minimal"<<std::endl;
    std::string refFile="./data/minimal/ex_ref.comp";
    std::string refMD5="6e02cd8bcab80b83fb593d2d10bea524";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_no_point_accepted()
{
    std::cout<<"\nPrepare test_no_point_accepted"<<std::endl;
    std::string refFile="./data/no_point_accepted/ex_ref.comp";
    std::string refMD5="df355ce22941c3571dae21889b62a44b";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_no_redundancy()
{
    std::cout<<"\nPrepare test_no_redundancy"<<std::endl;
    std::string refFile="./data/no_redundancy/ex_ref.comp";
    std::string refMD5="513e3b84c7c98fe17ac66527a584664e";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_photogra()
{
    std::cout<<"\nPrepare test_photogra"<<std::endl;
    std::string refFile="./data/photogra/ex_ref.comp";
    std::string refMD5="29d91a13299b690108866726058543fb";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_same_init_coords()
{
    std::cout<<"\nPrepare test_same_init_coords"<<std::endl;
    std::string refFile="./data/same_init_coords/ex_ref.comp";
    std::string refMD5="ce2742c485aab64e9b5499472a5365db";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_sigma_too_low()
{
    std::cout<<"\nPrepare test_sigma_too_low"<<std::endl;
    std::string refFile="./data/sigma_too_low/ex_ref.comp";
    std::string refMD5="e4f57da992169331259474fa9512dc53";

    test_template(refFile,refMD5,false);
}

void Tests_NonReg::test_simple2D()
{
    std::cout<<"\nPrepare test_simple2D"<<std::endl;
    std::string refFile="./data/simple2D/ex_ref.comp";
    std::string refMD5="45712f2b433dbdefad4e116fc00f5202";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_simul_IC_mini()
{
    std::cout<<"\nPrepare test_simul_IC_mini"<<std::endl;
    std::string refFile="./data/simul_IC_mini/ex_ref.comp";
    std::string refMD5="1e78005b77dabfac98c16166d2e1e455";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_simul_MonteCarlo_mini()
{
    std::cout<<"\nPrepare test_simul_MonteCarlo_mini"<<std::endl;
    std::string refFile="./data/simul_MonteCarlo_mini/ex_ref.comp";
    std::string refMD5="1de35427f5015f76f3f50e6187c5f666";

    //do not check many random things!
    test_template(refFile, refMD5,false,{},
                  {"/computation/all_sigma0/","*/MC_shift_max/", "*/MC_shift_sq_average/", "*/residual"}
    );
}

void Tests_NonReg::test_subfiles()
{
    std::cout<<"\nPrepare test_subfiles"<<std::endl;
    std::string refFile="./data/subfiles/ex_ref.comp";
    std::string refMD5="a7e8febee2dfe7381bbc411fe5ac30bb";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_syntax_error1()
{
    std::cout<<"\nPrepare test_syntax_error1"<<std::endl;
    std::string refFile="./data/syntax_error1/ex_ref.comp";
    std::string refMD5="79f263e25122993b85527c1fd9b028bf";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_syntax_error2()
{
    std::cout<<"\nPrepare test_syntax_error2"<<std::endl;
    std::string refFile="./data/syntax_error2/ex_ref.comp";
    std::string refMD5="3e2228623b1cb08d51b4e492de7dc063";

    test_template(refFile, refMD5,false);
}

void Tests_NonReg::test_topo()
{
    std::cout<<"\nPrepare test_topo"<<std::endl;
    std::string refFile="./data/topo/ex_ref.comp";
    std::string refMD5="ff909ea05c75b62b99bd0ac10cb8c1d1";

    test_template(refFile, refMD5,false);
}
