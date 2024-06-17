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

#ifndef TEST_NONREG_H
#define TEST_NONREG_H

#include <QtTest/QtTest>
#include "../src/project.h"

/****************************************************
 * Non-regression tests
 * several computations where every value is checked
 * => updated often, after cautious checking
 ****************************************************/
class Tests_NonReg : public QObject
{
    Q_OBJECT
public:
    explicit Tests_NonReg();

private:
    void test_template(std::string _refFilename, std::string refMD5, bool verbose, std::vector<std::string> comparePaths={},
                       std::vector<std::string> addNoComparePaths={}, std::vector<std::string> addNoCompareLeaves={}, double tolerancy=0.00001);
private slots:
    void test_axe();
    void test_baselines();
    void test_centering();
    void test_coincident_with_VD();
    void test_coord_sigma_neg();
    void test_corcov();
    void test_desact();
    void test_dev_vert();
    void test_eq_constr();
    void test_errors();
    void test_fixed_points();
    void test_fixed_points2();
    void test_fixed_points_IC();
    void test_forbidden_pt();
    void test_geo_mini_L93();
    void test_geo_mini_NTF();
    void test_init_cap();
    void test_kernel();
    void test_minimal();
    void test_no_point_accepted();
    void test_no_redundancy();
    void test_photogra();
    void test_same_init_coords();
    void test_sigma_too_low();
    void test_simple2D();
    void test_simul_IC_mini();
    void test_simul_MonteCarlo_mini();
    void test_subfiles();
    void test_syntax_error1();
    void test_syntax_error2();
    void test_topo();
};

#endif // TEST_NONREG_H
