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

#ifndef TEST_BASC_H
#define TEST_BASC_H

#include <QtTest/QtTest>
#include "../src/project.h"

/*************************
 *  Bascule
 *  Cart / Spheri (z and plani)
 *************************/
class Tests_Basc : public QObject
{
    Q_OBJECT
public:
    explicit Tests_Basc();

private:
    std::string filename;
    Project project;

private slots:
    void test_set();
    void test_result();
};

#endif // TEST_BASC_H
