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

#include "tests_coord.h"
#include "tests_basc.h"
#include "tests_nonreg.h"
#include "tests_stab.h"

// Note: This is equivalent to QTEST_APPLESS_MAIN for multiple test classes.
int main(int argc, char** argv)
{
    int status = 0;
    {
        Tests_Coord tc;
        status += QTest::qExec(&tc, argc, argv);
    }
    {
        Tests_Basc tb;
        status += QTest::qExec(&tb, argc, argv);
    }
    {
        Tests_NonReg tn;
        status += QTest::qExec(&tn, argc, argv);
    }
    {
        Tests_Stab ts;
        status += QTest::qExec(&ts, argc, argv);
    }
    if (status)
    {
        std::cout<<"Tests failed.\nNumber of errors: "<<status<<std::endl;
    }else {
        std::cout<<"Tests succeded!\nNo errors."<<std::endl;
    }
    return status;
}
