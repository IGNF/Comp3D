/**
 * Copyright (c) Institut national de l'information géographique et forestière https://www.ign.fr/
 *
 * File main authors:
 *  - JM Muller
 *  - C Meynard
 *
 * This file is part of Comp3D: https://github.com/IGNF/Comp3D
 *
 * Comp3D is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Comp3D is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Comp3D. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PROJECT_H
#define PROJECT_H

#include "compile.h"
#include "datafile.h"
#include "info.h"
#include "leastsquares.h"
#include "project_config.h"
#include "projection.h"
#include "varcovarmatrix.h"
#include <string>
#include <vector>

class Point;

class Project
#ifdef USE_QT
        : public QObject
#endif
{
#ifdef USE_QT
    Q_OBJECT
#endif
public:
    //Project(std::string _name,std::string _description,tdouble _latitude, Coord _center,ANGLE_UNIT _files_unit,int _nb_digits,bool _simulation);
    explicit Project(const std::string &projectFilename="");

    Project(const Project&) = delete;
    Project& operator=(const Project&) = delete;
    Project(Project&&) = delete;
    Project& operator=(Project&&) = delete;

    ~Project()
#ifdef USE_QT
        override
#endif
    ;

    static std::string createTemplate(const std::string &filename);

    bool read_config();
    unsigned int totalNumberOfObs();//number of lines in interface

    bool readData();
    bool initialization_CAP();

    bool set_least_squares(bool internalConstr);

    void print_points();
    bool exportSightMatrix(const std::string &filename);

    bool computation(bool invert, bool saveNew);
    void updateEllipsoids();

    bool saveasJSON();

    void saveNEW();//record NEW, 3D, CSV, [NEWGEOG]

    bool exportVarCov(const std::string &filename, std::vector<Point*> &selectedPoints,
                      bool addStationsParams, bool addOtherParams);
    bool exportSINEX(const std::string &filename, std::vector<Point*> &selectedPoints);
    bool exportRelPrec(const std::string &filename, std::vector<Point*> &selectedPoints);

    Point* getPoint(const std::string &name, bool create=false);

    std::list<Point> points;//every point
    std::list<std::unique_ptr<Station>> stations;//every station
    std::vector<std::string> forbidden_points;//forbid auto init
    std::vector<std::string> uninitializable_points;//impossible to auto init

    Projection projection;
    std::string filename;
    Project_Config config;
    LeastSquares lsquares;
    Info mInfo;
    std::string messages_read_config,messages_read_data,messages_set_least_squares,messages_computation;
    bool use_vertical_deflection;

    static Project* theone();
    static Info* theInfo();
    bool dataRead,hasWarning,readyToCompute,compensationDone,invertedMatrix,MonteCarloDone;

    void clear();//remove points, obs, matrices...
    int updateNumberOfActiveObs(bool internalOnly=false);//< update stations and points and return number of lines in obs matrix
    void cleanPointBeforeDelete(Point &pt);
    void deleteStation(Station* station); //<not to be used in a loop on stations!

    bool initPoint(Point &pt_to_init);//return false if impossible
    void resetPointsNum();//when some points were removed, point s number has to be fixede to be continuous
    void outputConversion();//<set points comp coord

#ifdef USE_QT
    static void prepareJson(const std::string &_filePath, const std::string &_projectName);
#endif


    const CORFile& get_cor_root() const {return *cor_root;}

    std::unique_ptr<CORFile> cor_root;
    std::unique_ptr<OBSFile> obs_root;
    std::unique_ptr<VarCovarMatrix> coord_cov;
    Project* previous;//for projects stacking
    std::string unitName;
    static std::string defaultLogLang;
    static Project* m_theone;
    static Info* m_theInfo;
protected:
    tdouble biggestEllips;//biggest dimension of a point ellipsoid
    void removePointstoDelete();
};


#endif // PROJECT_H
