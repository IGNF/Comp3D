/**
 * Copyright (c) Institut national de l'information géographique et forestière https://www.ign.fr/
 *
 * File main authors:
 *  - JM Muller
 *  - C Bellon
 *
 * This file is part of Comp3D: https://github.com/IGNF/Comp3D
 *
 * Comp3D is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Comp3D is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Comp3D. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef VARCOVAR_MATRIX_H
#define VARCOVAR_MATRIX_H

#include <vector>
#include <eigen3/Eigen/Dense>
#include "obs.h"
#include "mathtools.h"

class VarCovarMatrix
{
public:
    VarCovarMatrix(const std::string &csv_path, const std::string &current_absolute_path);

    std::vector<Obs*>& getObservations() {return observations;}
    MatX& getMatrix() {return matrix;}
    bool getIsOk(){return isOk;}

protected:
    std::vector<int> selectUsefulParams(const std::vector<std::string> &params_col);
    std::vector<Obs*> observations;
    MatX matrix;
    bool isOk;

};

#endif // VARCOVAR_MATRIX_H
