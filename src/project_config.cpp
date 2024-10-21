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

#include "project_config.h"
#include "uni_stream.h"
#include "filesystem_compat.h"
#include <string>
#include <cerrno>
#include <exception>

#include "project.h"


Project_Config::Project_Config(const  std::string &filename):working_directory(""),config_file(filename),
    name(""),description(""),compute_type(COMPUTE_TYPE::type_compensation),centerLatitude(48.8),localCenter(0,0,0),
    refraction(0.12),filesUnit(ANGLE_UNIT::GRAD),nbDigits(4),invert(true),internalConstr(false),
    maxIterations(100),convergenceCriterion(0.001),forceIterations(0),//localScale(1.0),
    lang(Project::defaultLogLang),useProj(false),projDef(""),useEllipsHeight(true),cleanOutputs(false),displayMap(true),miscMetadata(),
    root_COR_file(""),root_OBS_file(""),coord_cov_file("")
{
    fs::path current_path(config_file);
    working_directory=current_path.parent_path().generic_string();

    if (working_directory.empty()) working_directory+=".";

    working_directory+="/";
    name=current_path.stem().generic_string();
    std::cout<<"Working directory: "<<working_directory<<std::endl;
}

bool Project_Config::loadasJSON()
{
    //std::cout<<"Try to open file: "<<config_file<<std::endl;

    Json::Value root;
    Json::CharReaderBuilder rbuilder;
    std::string config_file_contents,jsondata;
    std::string errors;
    int tmp;

    try
    {
        config_file_contents=get_file_contents(config_file.c_str());
        unsigned pos = config_file_contents.find("=");
        jsondata= config_file_contents.substr(pos+1);
    }
    catch (std::exception& e)
    {
        Project::theInfo()->error(INFO_CONF,1,
                                  QT_TRANSLATE_NOOP("QObject","Exception: %s while reading configuration file \"%s\"."),
                                  e.what(),config_file.c_str());
        return false;
    }
    catch (int e)
    {
        Project::theInfo()->error(INFO_CONF,1,
                                  QT_TRANSLATE_NOOP("QObject","Error num %d: Can't open configuration file \"%s\""
                                                              " (file moved or special characters in the path?)."),
                                  e,config_file.c_str());
        return false;
    }

    std::istringstream jsondataStream(jsondata);
    bool parsingSuccessful = Json::parseFromStream(rbuilder,jsondataStream , &root, &errors);
    if ( !parsingSuccessful )
    {
        Project::theInfo()->error(INFO_CONF,1,
                                  QT_TRANSLATE_NOOP("QObject","Failed to parse configuration:"));
        Project::theInfo()->error(INFO_CONF,1,"%s",errors.c_str());
        return false;
    }

    bool conversionOK=true;
    Json::Value js_config;
    Json::Value json_center;
    Json::Value json_NotFound;//to test if there is an error
    try
    {
        js_config=root["config"];
    }catch (std::exception& e){
        Project::theInfo()->error(INFO_CONF,1,
                                  QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                  e.what(),"config");
        conversionOK=false;
    }
    try
    {
        lang=js_config.get("lang", "en").asString();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"lang");
        //conversionOK=false;
    }
    try
    {
        nbDigits=js_config.get("nb_digits", 4).asInt();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"nb_digits");
        //conversionOK=false;
    }

    try
    {
        maxIterations=js_config.get("max_iterations", 100).asInt();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"max_iterations");
        //conversionOK=false;
    }
    try
    {
        convergenceCriterion=js_config.get("convergence_criterion", 0.0001).asDouble();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"convergence_criterion");
        //conversionOK=false;
    }
    try
    {
        useProj=js_config.get("use_proj", false).asBool();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"use_proj");
        //conversionOK=false;
    }
    try
    {
        projDef=js_config.get("proj_def", "?").asString();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"proj_def");
        //conversionOK=false;
    }

    try
    {
        refraction=js_config.get("refraction", 0.12).asDouble();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"refraction");
        //conversionOK=false;
    }
    try
    {
        forceIterations=js_config.get("force_iterations", 0).asInt();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"force_iterations");
        //conversionOK=false;
    }

    try
    {
        root_COR_file=js_config.get("root_COR_file", "").asString();
    }catch (std::exception& e){
        Project::theInfo()->error(INFO_CONF,1,
                                  QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                  e.what(),"root_COR_file");
        conversionOK=false;
    }
    try
    {
        root_OBS_file=js_config.get("root_OBS_file", "").asString();
    }catch (std::exception& e){
        Project::theInfo()->error(INFO_CONF,1,
                                  QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                  e.what(),"root_OBS_file");
        conversionOK=false;
    }
    try
    {
        coord_cov_file=js_config.get("coord_cov_file", "").asString();
    }catch (std::exception& e){
        Project::theInfo()->error(INFO_CONF,1,
                                  QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                  e.what(),"coord_cov_file");
        conversionOK=false;
    }
    try
    {
        tmp = js_config.get("compute_type", static_cast<int>(COMPUTE_TYPE::type_compensation)).asInt();
        compute_type = static_cast<COMPUTE_TYPE>(tmp);
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"compute_type");
        //conversionOK=false;
    }
    try
    {
        tmp = js_config.get("files_unit", static_cast<int>(ANGLE_UNIT::RAD)).asInt();
        filesUnit = static_cast<ANGLE_UNIT>(tmp);
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"files_unit");
        //conversionOK=false;
    }
    try
    {
        name=js_config.get("name", "?").asString();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"name");
        //conversionOK=false;
    }
    try
    {
        description=js_config.get("description", "").asString();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"description");
        //conversionOK=false;
    }
    try
    {
        centerLatitude=js_config.get("center_latitude", 44.8).asDouble();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"center_latitude");
        //conversionOK=false;
    }
    /*try
    {
        localScale=js_config.get("local_scale", 1.0).asDouble();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),e.what(),"local_scale");
        localScale=1.0;
        //conversionOK=false;
    }*/
    try
    {
        invert=js_config.get("invert_matrix", false).asBool();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"invert_matrix");
        invert=false;
        //conversionOK=false;
    }
    try
    {
        internalConstr=js_config.get("internal_constraints", false).asBool();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"internal_constraints");
        internalConstr=false;
        //conversionOK=false;
    }

    try
    {
        useEllipsHeight=js_config.get("use_ellips_height", true).asBool();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"use_ellips_height");
        //conversionOK=false;
    }

    try
    {
        cleanOutputs=js_config.get("clean_outputs", false).asBool();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"clean_outputs");
        //conversionOK=false;
    }
    try
    {
        displayMap=js_config.get("display_map", true).asBool();
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"display_map");
        //conversionOK=false;
    }
    try
    {
        json_center=js_config.get("local_center", json_NotFound);
    }catch (std::exception& e){
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                    e.what(),"local_center");
        //conversionOK=false;
    }
    if (json_center!=json_NotFound)
    {
        try
        {
            tdouble local_centerX=json_center.get((Json::Value::ArrayIndex)0, 0.0).asDouble();
            localCenter.setx(local_centerX);
        }catch (std::exception& e){
            Project::theInfo()->warning(INFO_CONF,1,
                                        QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                        e.what(),"local_center/X");
            //conversionOK=false;
        }
        try
        {
            tdouble local_centerY=json_center.get((Json::Value::ArrayIndex)1, 0.0).asDouble();
            localCenter.sety(local_centerY);
        }catch (std::exception& e){
            Project::theInfo()->warning(INFO_CONF,1,
                                        QT_TRANSLATE_NOOP("QObject","Exception: %s while reading \"%s\" node."),
                                        e.what(),"local_center/Y");
            //conversionOK=false;
        }
    }else{
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Node \"local_center\" not found!"));
        //conversionOK=false;
    }

    miscMetadata = js_config["misc_metadata"];

    if (!Project::theInfo()->isEmpty())
    {
        Project::theInfo()->warning(INFO_CONF,1,
                                    QT_TRANSLATE_NOOP("QObject","Error while reading configuration file \"%s\"."),
                                    config_file.c_str());
    }

    return conversionOK;
}

std::string Project_Config::get_root_COR_absolute_filename()
{
    fs::path current_COR_path(root_COR_file);
    if (current_COR_path.is_relative())
        return working_directory+current_COR_path.generic_string();
    else return current_COR_path.generic_string();
}

std::string Project_Config::get_root_OBS_absolute_filename()//from comp point of view
{
    fs::path current_OBS_path(root_OBS_file);
    if (current_OBS_path.is_relative())
        return working_directory+current_OBS_path.generic_string();
    else return current_OBS_path.generic_string();
}

std::string Project_Config::get_coord_cov_absolute_filename()//from comp point of view
{
    fs::path current_coord_cov_path(coord_cov_file);
    if (current_coord_cov_path.is_relative())
        return working_directory+current_coord_cov_path.generic_string();
    else return current_coord_cov_path.generic_string();
}

void Project_Config::set_root_COR_filename(const std::string &_root_COR_filename)//from comp point of view
{
    root_COR_file=_root_COR_filename;
}

void Project_Config::set_root_COR_abs_path(const std::string &_root_COR_abs_path)
{
    fs::path COR_path(_root_COR_abs_path);
    root_COR_file=fs::relative(COR_path).generic_string();
}

void Project_Config::set_root_OBS_filename(const std::string &_root_OBS_filename)//from comp point of view
{
    root_OBS_file=_root_OBS_filename;
}

void Project_Config::set_coord_cov_filename(const std::string &_coord_cov_filename)//from comp point of view
{
    coord_cov_file=_coord_cov_filename;
}

bool Project_Config::saveasJSON(Json::Value &js_config)
{
    js_config["lang"]=lang;
    js_config["nb_digits"]=nbDigits;
    js_config["max_iterations"]=(int)maxIterations;
    js_config["convergence_criterion"]=(double)convergenceCriterion;
    js_config["refraction"]=(double)refraction;
    js_config["force_iterations"]=(int)forceIterations;
    js_config["root_COR_file"]=root_COR_file;
    js_config["root_OBS_file"]=root_OBS_file;
    js_config["coord_cov_file"]=coord_cov_file;
    js_config["compute_type"]=static_cast<int>(compute_type);
    js_config["files_unit"]=static_cast<int>(filesUnit);
    js_config["name"]=name;
    js_config["description"]=description;
    js_config["center_latitude"]=(double)centerLatitude;
    js_config["local_center"]=localCenter.toJson();
    //js_config["local_scale"]=(double)localScale;
    js_config["invert_matrix"]=invert;
    js_config["internal_constraints"]=internalConstr;
    js_config["use_proj"]=useProj;
    js_config["proj_def"]=projDef;
    js_config["use_ellips_height"]=useEllipsHeight;
    js_config["clean_outputs"]=cleanOutputs;
    js_config["display_map"]=displayMap;

    js_config["misc_metadata"]=miscMetadata;

    return true;
}

//from http://insanecoding.blogspot.fr/2011/11/how-to-read-in-file-in-c.html
std::string Project_Config::get_file_contents(const char *filename)
{
  uni_stream::ifstream in(filename, std::ios::in | std::ios::binary);
  if (in)
  {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return(contents);
  }
  throw(errno);
}
