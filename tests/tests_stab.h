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

#ifndef TESTS_REF_H
#define TESTS_REF_H

#include <QtTest/QtTest>
#include "../src/project.h"

/************************************************************
 * Stability tests
 * several computations where only some values are checked
 * references computations should not be updated
 * to have a comparison with old versions or other softwares
 ************************************************************/
class Tests_Stab : public QObject
{
    Q_OBJECT
public:
    explicit Tests_Stab();

private:
    void test_template(std::string _refFilename, std::string refMD5, bool verbose,
                       std::vector< std::pair<std::string, std::string> > moreComparePaths={},
                       double tolerance=0.0001);
private slots:
    void test_changement_de_repere();
    void test_convergence();
    void test_egouts();
    void test_modane();
    void test_polygone_K06();
};

#endif // TESTS_REF_H
