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

#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#include <sstream>
#include <json/json.h>

#include "coord.h"
#include "mathtools.h"

enum class COMPUTE_TYPE
{
    type_compensation=0,
    type_propagation,
    type_monte_carlo
};

class Project_Config
{
public:
    explicit Project_Config(const std::string &filename);
    bool loadasJSON();
    bool saveasJSON(Json::Value &js_config);
    static std::string get_file_contents(const char *filename);

    std::string get_root_COR_absolute_filename();//from comp point of view
    std::string get_root_COR_relative_filename(){return root_COR_file;}//from config file point of view
    std::string get_root_OBS_absolute_filename();//from comp point of view
    std::string get_root_OBS_relative_filename(){return root_OBS_file;}//from config file point of view
    std::string get_coord_cov_absolute_filename();//from comp point of view
    std::string get_coord_cov_relative_filename(){return coord_cov_file;}//from config file point of view

    void set_root_COR_filename(const std::string &_root_COR_filename);//from comp point of view
    void set_root_COR_abs_path(const std::string &_root_COR_filename);
    void set_root_OBS_filename(const std::string &_root_OBS_filename);//from comp point of view
    void set_coord_cov_filename(const std::string &_coord_cov_filename);//from comp point of view

    std::string working_directory;
    std::string config_file;
    std::string name,description;
    COMPUTE_TYPE compute_type;
    tdouble centerLatitude;//< in Degrees!
    Coord localCenter;

    tdouble refraction;
    ANGLE_UNIT filesUnit;
    int nbDigits;
    bool invert;
    bool internalConstr;
    int maxIterations;
    tdouble convergenceCriterion;
    int forceIterations;
    //tdouble localScale;
    std::string lang;
    bool useProj;
    std::string projDef;//<proj4 definition (ex: "+proj=utm +zone=11 +ellps=WGS84")
    bool useEllipsHeight;
    bool cleanOutputs;
    bool displayMap;

    Json::Value miscMetadata;

protected:
    std::string root_COR_file,root_OBS_file,coord_cov_file;
};

#endif // PROJECT_CONFIG_H
