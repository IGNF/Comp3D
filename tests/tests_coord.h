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

#ifndef TESTS_COORD_H
#define TESTS_COORD_H

#include <QtTest/QtTest>
#include "../src/project.h"
#include "../src/station_bascule.h"

/*************************
 *  Coordinates conversion
 *************************/
class Tests_Coord: public QObject
{
    Q_OBJECT
public:
    Tests_Coord();
private:
    Coord center;
    Point p1;
    Project project;
    Station_Bascule *st1;

private slots:
    void test_sphericalToCartesian();
    void test_basc_mes2GroundCart();
    void test_basc_toLocal();
    void test_proj();
    void test_NTF();
    void test_RGF();
    void test_angles();
};

//#include "moc_tests_coord.cpp"

#endif // TESTS_COORD_H
