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

#ifndef STATION_EQ_H
#define STATION_EQ_H

#include "station.h"
#include "compile.h"
#include <string>
#include <vector>


class Station_Eq : public Station
{
#ifdef USE_QT
    Q_OBJECT
#endif
public:
    std::string typeStr() const override;

    bool read_obs(std::string line, int line_number, DataFile * _file, const std::string &current_absolute_path, const std::string &comment) override;
    bool set_obs(LeastSquares *lsquares, bool initialResidual, bool internalConstraint) override;
    bool initialize(bool verbose) override;
    void update() override {}//nothing to do

    Json::Value toJson(FileRefJson& fileRef) const override;

    bool isInternal(Obs* obs) override;//if the obs is compatible with internal constraints
    bool isOnlyLeveling(Obs* obs) override;//for internal constraints
    bool isHz(Obs* obs) override;//for internal constraints
    bool isDistance(Obs* obs) override;//for internal constraints
    bool isBubbuled(Obs* obs) override;//for internal constraints
    bool useVertDeflection(Obs* obs) override;//to check vertical deflection consistancy
    int numberOfBasicObs(Obs* obs) override;//number of lines in the matrix
    STATION_CODE eq_type;
    const EqFile& getFile() const {return *file;}
    tdouble val;//value of unknown, equivalent to *(params[0].value)
protected:
    template <typename T>
    friend T* Station::create(Point *origin);
    explicit Station_Eq(Point* origin);

    std::unique_ptr<EqFile> file;
};

#endif // STATION_EQ_H
