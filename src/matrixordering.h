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

#ifndef MATRIXORDERING_H
#define MATRIXORDERING_H

#include <eigen3/Eigen/Dense>
#include <iostream>
#include <vector>

class Project;

/**
  Obs matrix ordering using Cuthill McKee algo.
  See:  https://tel.archives-ouvertes.fr/file/index/docid/952252/filename/TH2013PEST1164_complete.pdf
        http://www.boost.org/doc/libs/1_37_0/libs/graph/doc/cuthill_mckee_ordering.html
        http://ciprian-zavoianu.blogspot.fr/2009/01/project-bandwidth-reduction.html

  each point is a group of unknowns.
  compute all relations between groups to create a relations graph
  use Cuthill McKee algo to re-arrange it => get the new ordering
  use this ordering in least squares
  */
class MatrixOrdering
{
public:
    explicit MatrixOrdering(Project * _project);
    bool computeRelations();
    void orderRelations();

    std::vector<unsigned int> newOrder;
protected:
    Project * project;
    Eigen::Array<bool,Eigen::Dynamic,Eigen::Dynamic> relations;
};

#endif // MATRIXORDERING_H
