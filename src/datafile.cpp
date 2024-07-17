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

#include "datafile.h"

#include <regex>

#include "filesystem_compat.h"
#include "point.h"
#include "obs.h"
#include "project.h"
#include "station_simple.h"
#include "station_hz.h"
#include "station_bascule.h"
#include "station_axis.h"
#include "station_eq.h"
#include "compile.h"

FileComment::FileComment(const std::string &_text, int _line_num)
    : text(_text), line_num(_line_num)
{
}

//------------------------------------------------------------------------
DataFile::DataFile()
    :filename("?"),current_absolute_path("."),line_num_in_parent(-1),fileDepth(1),fileExists(false)
{
    //std::cout<<"Create datafile "<<current_absolute_path<<filename<<" "<<this<<std::endl;
}

DataFile::DataFile(const std::string &_filename, const std::string &_current_absolute_path, int _line_num_in_parent, unsigned _fileDepth)
    :filename(_filename),current_absolute_path(_current_absolute_path),line_num_in_parent(_line_num_in_parent),
      fileDepth(_fileDepth),fileExists(false)
{
    //std::cout<<"Create datafile "<<current_absolute_path<<filename<<" "<<this<<std::endl;
}

DataFile::~DataFile()
{
#ifdef INFO_DELETE
    std::cout<<"Delete datafile"<<std::endl;
#endif
}

bool DataFile::read(const std::string &_filename, const std::string &_current_absolute_path)
{
    filename=_filename;
    current_absolute_path=_current_absolute_path;
    return read();
}

bool DataFile::read()
{
    //std::cout<<"Read file \""<<current_absolute_path<<" / "<<filename<<"\""<<std::endl;
    Project::theInfo()->msg(INFO_CONF,fileDepth,
                            QT_TRANSLATE_NOOP("QObject","Read file %s..."),
                            filename.c_str());
    if (fileDepth>MAXFILEDEPTH)
    {
        Project::theInfo()->error(INFO_CONF,fileDepth+1,
                                  QT_TRANSLATE_NOOP("QObject","Can't open file \"%s\": max subfile depth=%d."),
                                  filename.c_str(),MAXFILEDEPTH);
        return false;
    }

    uni_stream::ifstream file;
    bool ok=true;

    comments.clear();
    clear();

    std::string file_path=current_absolute_path+filename.c_str();
    file.open(file_path.c_str());
    if (file.is_open())
    {
        int line_num=0;
        fileExists=true;
        for (std::string line; std::getline(file, line); )
        {
            line_num++;
            if (line.empty())
                continue;

            //remove exotic endline chars
            if (line.back()=='\r')
                line.resize(line.size()-1);

            //remove trailing spaces
            auto strEnd = line.find_last_not_of(" ");
            line=line.substr(0, strEnd+1);

            //std::cout<<"Line "<<line_num<<": \""<<line<<"\""<<std::endl;
            //test if line is empty or just a comment
            std::regex regex_line(R"(^\s*([^*]*)(.*)$)");
            std::smatch what;
            if(std::regex_match(line, what, regex_line))
            {
                // what[0] contains the whole string
                // what[1] contains the data part
                // what[2] contains the comment part
                if ((what[1]).length()<2)
                {
                    //std::cout<<"New comment: \""<<line<<"\""<<std::endl;
                    comments.push_back(FileComment(line,line_num));
                }else{
                    //check if sub-file called
                    std::regex regex_subfile(R"(^\s*@([^*\s]*)(.*)$)");
                    std::smatch what_subfile;
                    if(std::regex_match(line, what_subfile, regex_subfile))
                    {
                        // what_subfile[0] contains the whole string
                        // what_subfile[1] contains the link part
                        // what_subfile[2] contains the comment part
                        if ((what_subfile[1]).length()>0)
                        {
                            std::cout<<"call for subfile\n";
                            fs::path next_path(file_path);
                            std::string next_path_str=next_path.parent_path().string()+"/";
                            ok =read_subfile(what_subfile[1],next_path_str,line_num) && ok;
                        }
                    }
                    //else it's a normal data line
                    else ok = interpret_line(line,line_num) && ok;
                }
            }
        }

    }else{
        if (get_line_num_in_parent()>=0)
            Project::theInfo()->warning(INFO_CONF,fileDepth+1,
                                        QT_TRANSLATE_NOOP("QObject","At line %d: \"@%s\" => Can't open file \"%s\"."),
                                        get_line_num_in_parent(),filename.c_str(),
                                        (current_absolute_path+"/"+filename).c_str()); // TODO: use std::filesystem::path
        else
            Project::theInfo()->warning(INFO_CONF,fileDepth+1,
                                        QT_TRANSLATE_NOOP("QObject","Can't open file \"%s\"."),
                                        (current_absolute_path+"/"+filename).c_str()); // TODO: use std::filesystem::path
        fileExists=false;
        ok=false;
    }

    //Project::theInfo()->msg(INFO_CONF,fileDepth,QT_TRANSLATE_NOOP("QObject","...end reading file %s."),filename.c_str());

    return ok;
}


bool DataFile::read_subfile(const std::string &, const std::string &, int line_num)
{
    Project::theInfo()->error(INFO_CONF,get_fileDepth(),
                              QT_TRANSLATE_NOOP("QObject","In %s:%d: This file cannot have subfile!"),
                              this->filename.c_str(),line_num);
    return false;
}

//------------------------------------------------------------------------
NewFile::NewFile(const std::string &_filename)
    : CORFile(_filename,"",-1,0)
{
}

std::unique_ptr<NewFile> NewFile::create(const std::string &_filename)
{
    return std::unique_ptr<NewFile>(new NewFile(_filename));
}

void NewFile::clear()
{
    points.clear();
}

bool NewFile::interpret_line(const std::string &line, int line_num)
{
    points_owner.push_back(std::make_unique<Point>(this,line_num));
    if (points_owner.back()->read_point(line, true))
    {
        points.push_back(points_owner.back().get());
        return true;
    }
    points_owner.pop_back();
    return false;
}

bool NewFile::read_subfile(const std::string &filename, const std::string &absolute_path, int line_num)
{
    return DataFile::read_subfile(filename, absolute_path, line_num);
}

//------------------------------------------------------------------------

CORFile::CORFile(const std::string &_filename,const std::string &_current_absolute_path,int _line_num_in_parent, unsigned _fileDepth)
    :DataFile(_filename,_current_absolute_path,_line_num_in_parent,_fileDepth)
{
}


std::unique_ptr<CORFile> CORFile::create(const std::string &_filename, const std::string &_current_absolute_path,
                                         int _line_num_in_parent, unsigned int _fileDepth)
{
    return std::unique_ptr<CORFile>(new CORFile(_filename, _current_absolute_path, _line_num_in_parent, _fileDepth));
}


void CORFile::clear()
{
    points.clear();
    sub_files.clear();
    new_points.clear();
}

bool CORFile::interpret_line(const std::string &line, int line_num)
{
    //std::cout<<"Interpret \""<<line<<"\" as a COR line"<<std::endl;
    Project::theone()->points.emplace_back(this,line_num);
    Point *pt = &Project::theone()->points.back();
    if (!pt->read_point(line))
    {
        auto ptIsForbidden = pt->isForbidden(); //save state before removing point
        Project::theone()->points.pop_back();
        return ptIsForbidden; // no error if forbidden point
    }
    if (pt->set_point(true)) //set the final copy of the point
    {
        points.push_back(pt);
        return true;
    }
    Project::theone()->points.pop_back();
    return false;
}

bool CORFile::read_subfile(const std::string &filename, const std::string &absolute_path, int line_num)
{
    sub_files.push_back(CORFile::create(filename,absolute_path,line_num, fileDepth+1));
    return sub_files.back()->read();
}


void CORFile::removePoint(Point * point_to_remove)
{
    // Find point in list
    auto itp = std::find(points.begin(), points.end(), point_to_remove);
    if (itp == points.end())
        return;

    // Remove it
    points.erase(itp);

    // And add it in comment list just before the line number greater than its own (or at end)
    FileComment new_comment=point_to_remove->point2comment();
    auto itc = std::find_if(comments.begin(), comments.end(),
                            [new_comment](const FileComment &comment) { return new_comment.line_num < comment.line_num; }
                            );
    comments.insert(itc, new_comment);
}

void CORFile::addPoint(Point * point)
{
    new_points.push_back(point);
}

//recursive, writes all new file
void CORFile::writeNEW(uni_stream::ofstream *file_new, int recurse_level)
{
    tdouble unitFactorARCSEC=fromRad(1.0,ANGLE_UNIT::ARCSEC);

    if ((recurse_level==0)&&(!Project::theone()->config.cleanOutputs))
    {
        //all forbidden points
        if (!Project::theone()->forbidden_points.empty())
        {
            (*file_new)<<" *** forbidden points ***\n";
            for (auto & point : Project::theone()->forbidden_points)
                (*file_new)<<" -1 "<<point<<"\n";
            (*file_new)<<" *** end forbidden points ***\n";
        }

        //all uninitializable points
        if (!Project::theone()->uninitializable_points.empty())
        {
            (*file_new)<<" *** uninitializable points ***\n";
            for (auto & point : Project::theone()->uninitializable_points)
                (*file_new)<<"  *  "<<point<<"\n";
            (*file_new)<<" *** end uninitializable points ***\n";
        }
    }

    if (!Project::theone()->config.cleanOutputs)
        (*file_new)<<"* Begin file "<<filename<<" --------------------------------------\n";

    //all new points
    if (!new_points.empty())
    {
        if (!Project::theone()->config.cleanOutputs)
            (*file_new)<<" *** Added points ***\n";
        for (auto & point : new_points)
        {
            Coord coord_compensated_cartesian;
            Project::theone()->projection.sphericalToGlobalCartesian(point->coord_comp,coord_compensated_cartesian);
            (*file_new)<<"  "<<point->name<<" ";
            (*file_new)<<point->coord_compensated_georef.toString();
            (*file_new)<<" "<<point->ellipsoid.sigmasToString(Project::theone()->lsquares.sigma_0);
            if ((!std::isnan(point->dev_eta)) && (!std::isnan(point->dev_xi)))
                (*file_new)<<" "<<point->dev_eta*unitFactorARCSEC<<" "<<point->dev_xi*unitFactorARCSEC;
            (*file_new)<<" *"<<point->comment<<" **autoinit\n";
         }
        if (!Project::theone()->config.cleanOutputs)
            (*file_new)<<" *** End added points ***\n";
    }

    unsigned int subfile_num=0;
    unsigned int point_num=0;
    unsigned int comment_num=0;
    int current_line_num=1;
    while ((point_num<points.size())||(subfile_num<sub_files.size())||(comment_num<comments.size()))
    {
        //debug
        /*std::cout<<"current_line_num: "<<current_line_num;
        if (point_num<points.size()) std::cout<<" "<<points.at(point_num)->lineNumber;
        if (comment_num<comments.size()) std::cout<<" "<<comments.at(comment_num).line_num;
        if (subfile_num<sub_files.size()) std::cout<<" "<<sub_files.at(subfile_num)->get_line_num_in_parent();
        if (subfile_num<sub_files.size()) std::cout<<" "<<sub_files.at(subfile_num)->get_line_num_in_parent();
        std::cout<<std::endl;*/

        //std::cout<<"for line "<<current_line_num<<": subfile_num="<<subfile_num
        //           <<"; point_num="<<point_num<<"; point_num="<<subfile_num<<std::endl;
        if ((point_num<points.size())&&(points.at(point_num)->lineNumber==current_line_num))
        {
            Coord coord_compensated_cartesian;
            Project::theone()->projection.sphericalToGlobalCartesian(points.at(point_num)->coord_comp,coord_compensated_cartesian);
            //std::cout<<"Point!"<<std::endl;
            (*file_new)<<"  "<<points.at(point_num)->name<<" ";
            (*file_new)<<points.at(point_num)->coord_compensated_georef.toString();
            (*file_new)<<" "<<points.at(point_num)->ellipsoid.sigmasToString(Project::theone()->lsquares.sigma_0);
            if ((!std::isnan(points.at(point_num)->dev_eta)) && (!std::isnan(points.at(point_num)->dev_xi)))
                (*file_new)<<" "<<points.at(point_num)->dev_eta*unitFactorARCSEC<<" "<<points.at(point_num)->dev_xi*unitFactorARCSEC;
            (*file_new)<<" *"<<points.at(point_num)->comment<<"\n";
            point_num++;
        }
        else if ((comment_num<comments.size())&&(comments.at(comment_num).line_num==current_line_num))
        {
            if (!Project::theone()->config.cleanOutputs)
                (*file_new)<<comments.at(comment_num).text<<"\n";
            comment_num++;
        }
        else if ((subfile_num<sub_files.size())&&(sub_files.at(subfile_num)->get_line_num_in_parent()==current_line_num))
        {
            sub_files.at(subfile_num)->writeNEW(file_new,recurse_level+1);
            subfile_num++;
        }else{
            //std::cout<<"Infinite loop writing NEW file..."<<std::endl;
        }
        current_line_num++;
    }

    if (!Project::theone()->config.cleanOutputs)
        (*file_new)<<"* End file "<<filename<<" --------------------------------------\n";
}

//write a COR file with compensated coordinates
//recursive, writes all points
//for contrained points, read coord are written
void CORFile::writeCORcompensated(uni_stream::ofstream *file_cor, bool is_coord_init, bool with_pts_constr, int recurse_level) const
{
    tdouble unitFactorARCSEC=fromRad(1.0,ANGLE_UNIT::ARCSEC);
    if (recurse_level==0)
    {
        //all forbidden points
        if (!Project::theone()->forbidden_points.empty())
        {
            (*file_cor)<<" *** forbidden points ***\n";
            for (auto & point : Project::theone()->forbidden_points)
            {
                (*file_cor)<<" -1 "<<point<<"\n";
             }
            (*file_cor)<<" *** end forbidden points ***\n";
        }

        //all uninitializable points
        if (!Project::theone()->uninitializable_points.empty())
        {
            (*file_cor)<<" *** uninitializable points ***\n";
            for (auto & point : Project::theone()->uninitializable_points)
            {
                (*file_cor)<<"  *  "<<point<<"\n";
             }
            (*file_cor)<<" *** end uninitializable points ***\n";
        }
    }

    (*file_cor)<<"* Begin file "<<filename<<" --------------------------------------\n";

    //all new points
    if (!new_points.empty())
    {
        (*file_cor)<<" *** Added points ***\n";
        for (auto & point : new_points)
        {
            (*file_cor)<<"  "<<static_cast<int>(point->code)<<" "<<point->name<<" ";
            if (is_coord_init)
                (*file_cor)<<point->coord_read.toString();
            else
            {
                (*file_cor)<<point->coord_compensated_georef.toString();
            }
            (*file_cor)<<" "<<point->sigmas_init.toString();
            if ((!std::isnan(point->dev_eta)) && (!std::isnan(point->dev_xi)))
                (*file_cor)<<" "<<point->dev_eta*unitFactorARCSEC<<" "<<point->dev_xi*unitFactorARCSEC;
            (*file_cor)<<" *"<<point->comment<<" **autoinit\n";
         }
        (*file_cor)<<" *** End added points ***\n";
    }

    unsigned int subfile_num=0;
    unsigned int point_num=0;
    unsigned int comment_num=0;
    int current_line_num=1;
    while ((point_num<points.size())||(subfile_num<sub_files.size())||(comment_num<comments.size()))
    {
        //std::cout<<"for line "<<current_line_num<<": subfile_num="<<subfile_num
        //           <<"; point_num="<<point_num<<"; point_num="<<subfile_num<<std::endl;
        if ((point_num<points.size())&&(points.at(point_num)->lineNumber==current_line_num))
        {
            Point * pt = points.at(point_num);
            if (with_pts_constr || pt->isFree())
            {
                bool constr[3]={pt->obsX?pt->obsX->active:false || pt->isXfixed,
                                pt->obsY?pt->obsY->active:false || pt->isYfixed,
                                pt->obsZ?pt->obsZ->active:false || pt->isZfixed};
                Coord export_coord = pt->coord_read;
                if (!is_coord_init)
                    for (unsigned int d=0;d<3;++d)
                    {
                        if (constr[d])
                            export_coord[d]=pt->coord_read[d];
                        else {
                            export_coord[d]=pt->coord_compensated_georef[d];
                        }
                    }
                (*file_cor)<<"  "<<static_cast<int>(points.at(point_num)->code)<<" "<<points.at(point_num)->name<<" ";
                (*file_cor)<<export_coord.toString();
                (*file_cor)<<" "<<points.at(point_num)->sigmas_init.toString();
                if ((!std::isnan(points.at(point_num)->dev_eta)) && (!std::isnan(points.at(point_num)->dev_xi)))
                    (*file_cor)<<" "<<points.at(point_num)->dev_eta*unitFactorARCSEC<<" "<<points.at(point_num)->dev_xi*unitFactorARCSEC;
                (*file_cor)<<" *"<<points.at(point_num)->comment<<"\n";
            }
            point_num++;
        }
        else if ((comment_num<comments.size())&&(comments.at(comment_num).line_num==current_line_num))
        {
            //std::cout<<"Comment!"<<std::endl;
            (*file_cor)<<comments.at(comment_num).text<<"\n";
            comment_num++;
        }
        else if ((subfile_num<sub_files.size())&&(sub_files.at(subfile_num)->get_line_num_in_parent()==current_line_num))
        {
            //std::cout<<"Subfile!"<<std::endl;
            sub_files.at(subfile_num)->writeCORcompensated(file_cor,is_coord_init,with_pts_constr,recurse_level+1);
            subfile_num++;
        }else{
            //std::cout<<"Infinite loop writing COR file..."<<std::endl;
        }
        current_line_num++;
    }

    (*file_cor)<<"* End file "<<filename<<" --------------------------------------\n";
}


//------------------------------------------------------------------------

OBSFile::OBSFile(const std::string &_filename, const std::string &_current_absolute_path,
                 int _line_num_in_parent, unsigned _fileDepth)
    :DataFile(_filename,_current_absolute_path,_line_num_in_parent,_fileDepth)
{
}


std::unique_ptr<OBSFile> OBSFile::create(const std::string &_filename,
                                         const std::string &_current_absolute_path,
                                         int _line_num_in_parent, unsigned int _fileDepth)
{
    return std::unique_ptr<OBSFile>(new OBSFile(_filename, _current_absolute_path,
                                                _line_num_in_parent, _fileDepth));
}

void OBSFile::clear()
{
    sub_files.clear();
}

bool OBSFile::interpret_line(const std::string &line, int line_num)
{
    //std::cout<<"Interpret \""<<line<<"\" as an OBS line"<<std::endl;
    std::regex regex_line(R"(^\s*([^*\s]+)\*?(.*)$)");
    std::smatch matches;
    if (!std::regex_match(line, matches, regex_line))
    {
        Project::theInfo()->warning(INFO_OBS,fileDepth+1,
                                    QT_TRANSLATE_NOOP("QObject","Error in OBS line %s"),
                                    line.c_str());
        return false;
    }

    std::regex regex_code(R"(^\s*()" INT_REGEX R"()\s+.*)");

    std::smatch matches_code;
    if (!std::regex_match(line, matches_code, regex_code))
    {
        Project::theInfo()->warning(INFO_OBS,fileDepth+1,
                                    QT_TRANSLATE_NOOP("QObject","Error in OBS line %s"),
                                    line.c_str());
        return false;
    }else{
        std::istringstream iss(matches_code[1]);
        int code_int = 0;
        if (!(iss>>code_int))
        {
            Project::theInfo()->warning(INFO_OBS,fileDepth+1,
                                        QT_TRANSLATE_NOOP("QObject","Error in OBS line %s"),
                                        line.c_str());
            return false;
        }
        if (code_int<0) code_int=-code_int;
        STATION_TYPE st_type=STATION_TYPE::ST_NONE;
        switch (static_cast<OBS_CODE>(code_int))
        {
            case OBS_CODE::COORD_X:
            case OBS_CODE::COORD_Y:
            case OBS_CODE::COORD_Z:
            case OBS_CODE::DX_SPHER:
            case OBS_CODE::DY_SPHER:
            case OBS_CODE::DIST1:
            case OBS_CODE::DIST_HZ0:
            case OBS_CODE::DIST:
            case OBS_CODE::DH:
            case OBS_CODE::ZEN:
            case OBS_CODE::AZIMUTH:
            case OBS_CODE::CENTER_META:
                st_type=STATION_TYPE::ST_SIMPLE;
                break;

            case OBS_CODE::HZ_ANG:
            case OBS_CODE::HZ_REF:
                st_type=STATION_TYPE::ST_HZ;
                break;
            default:
                break; //TODO: list all to have a warning if new types are added?
        }
        switch (static_cast<STATION_CODE>(code_int))
        {
            case STATION_CODE::BASC_XYZ_CART:
            case STATION_CODE::BASC_ANG_CART:
            case STATION_CODE::BASELINE_GEO_XYZ:
                st_type=STATION_TYPE::ST_BASCULE;
                break;

            case STATION_CODE::AXIS_OBS:
                st_type=STATION_TYPE::ST_AXIS;
                break;
            case STATION_CODE::OBS_EQ_DH:
            case STATION_CODE::OBS_EQ_DIST:
                st_type=STATION_TYPE::ST_EQ;
                break;
            default:
                break; //TODO: list all to have a warning if new types are added?
        }
        if (st_type==STATION_TYPE::ST_NONE)
        {
            Project::theInfo()->warning(INFO_OBS,fileDepth+1,
                                        QT_TRANSLATE_NOOP("QObject","In %s:%d: %s => "
                                                                    "Impossible to determine observation type!"),
                                        filename.c_str(),line_num,line.c_str());
            return false;
        }
        std::regex regex_obs(R"(^\s*()" INT_REGEX R"()\s+([^@][^*\s]*)\s+[^*\s]+.*)");
        std::smatch matches_obs;
        std::regex regex_obs_no_origin(R"(^\s*()" INT_REGEX R"()\s+(@[^*\s]+).*)");
        std::smatch matches_obs_no_origin;

        //get comment
        std::string comment="";
        std::size_t comment_pos = line.find('*');
        if (comment_pos!=std::string::npos)
            comment=line.substr(line.find('*')+1);

        std::string file_path=current_absolute_path+"/"+filename.c_str();
        fs::path next_path=file_path;
        std::string next_path_str=next_path.parent_path().string()+"/";

        if (std::regex_search(line, matches_obs, regex_obs))
        {
            std::string from_name(matches_obs[2].first, matches_obs[2].second);

            //find or create the station
            Point * from=Project::theone()->getPoint(from_name,true);
            if (!from)
            {
                Project::theInfo()->warning(INFO_OBS,fileDepth+1,
                                            QT_TRANSLATE_NOOP("QObject","In %s:%d: %s => "
                                                                        "Error creating point '%s'."),
                                            filename.c_str(),line_num,line.c_str(),from_name.c_str());
                return false;
            }

            Station *station=nullptr;
            switch (st_type)
            {
                case STATION_TYPE::ST_NONE:
                    station=nullptr;
                    break;
                case STATION_TYPE::ST_SIMPLE:
                    station=from->getLastStation<Station_Simple>(get_fileDepth());
                    break;
                case STATION_TYPE::ST_HZ:
                    if (code_int==static_cast<int>(OBS_CODE::HZ_REF)) //if new reference, create a new station
                        station=Station::create<Station_Hz>(from);
                    else
                        station=from->getLastStation<Station_Hz>(get_fileDepth(),STATION_CREATION_MODE::ST_CREAT_WARNING);//warning if has to create a new station without opening
                    break;
                case STATION_TYPE::ST_BASCULE:
                    station=Station::create<Station_Bascule>(from);
                    break;
                case STATION_TYPE::ST_AXIS:
                    station=Station::create<Station_Axis>(from);
                    break;
                default:
                    break;
            }

            if (!station)
            {
                Project::theInfo()->warning(INFO_OBS,fileDepth+1,
                                            QT_TRANSLATE_NOOP("QObject","In %s:%d: %s => Impossible to create station!"),
                                            filename.c_str(),line_num,line.c_str());
                return false;
            }

            bool ok=station->read_obs(line,line_num,this,next_path_str, comment);
            if (station->observations.size()==0) Project::theone()->deleteStation(station);

            return ok;
        }
        else if (std::regex_search(line, matches_obs_no_origin, regex_obs_no_origin))
        {
            Station *station=nullptr;
            switch (st_type)
            {
                case STATION_TYPE::ST_EQ:
                    station=Station::create<Station_Eq>(nullptr);
                    break;
                default:
                    break;
            }
            if (!station)
            {
                Project::theInfo()->warning(INFO_OBS,fileDepth+1,
                                            QT_TRANSLATE_NOOP("QObject","In %s:%d: %s => Impossible to create station!"),
                                            filename.c_str(),line_num,line.c_str());
                return false;
            }
            bool ok=station->read_obs(line,line_num,this,next_path_str, comment);
            if (station->observations.size()==0) Project::theone()->deleteStation(station);
            return ok;
        } else {
            Project::theInfo()->warning(INFO_OBS,fileDepth+1,
                                        QT_TRANSLATE_NOOP("QObject","In %s:%d: %s => Can't interpret line!"),
                                        filename.c_str(),line_num,line.c_str());
            return false;
        }
    }
    Project::theInfo()->warning(INFO_OBS,fileDepth+1,
                                QT_TRANSLATE_NOOP("QObject","Error in OBS file line %d: %s"),
                                line_num,line.c_str());
    return false;
}

bool OBSFile::read_subfile(const std::string &filename, const std::string &absolute_path, int line_num)
{
    sub_files.push_back(OBSFile::create(filename,absolute_path,line_num,fileDepth+1));
    return sub_files.back()->read();
}


//------------------------------------------------------------------------

XYZFile::XYZFile(const std::string &_filename, Station_Bascule *_station,
                 const std::string &_current_absolute_path,
                 int _line_num_in_parent, unsigned _fileDepth)
    :DataFile(_filename,_current_absolute_path,_line_num_in_parent,_fileDepth),station(_station)
{
}

std::unique_ptr<XYZFile> XYZFile::create(const std::string &_filename, Station_Bascule *_station,
                                         const std::string &_current_absolute_path,
                                         int _line_num_in_parent, unsigned int _fileDepth)
{
    return std::unique_ptr<XYZFile>(new XYZFile(_filename, _station, _current_absolute_path,
                                                _line_num_in_parent, _fileDepth));
}

void XYZFile::clear() {}

bool XYZFile::interpret_line(const std::string &line, int line_num)
{
    std::regex regex_line(R"(^\s*([^*]+)\*?(.*)$)");
    std::smatch what;
    if(std::regex_match(line, what, regex_line))
    {
        // what[0] contains the whole string
        // what[1] contains the data part
        // what[2] contains the comment part
        if ((what[1]).length()>1)//there is something on the line
        {
            std::string line_data=what[1];
            station->observations3D.emplace_back(station);
            if (!station->observations3D.back().read_obs(line_data,line_num,this,what[2]))
                station->observations3D.pop_back();
            else
                return true;
        }
    }
    Project::theInfo()->warning(INFO_OBS,fileDepth+1,
                                QT_TRANSLATE_NOOP("QObject","Error in XYZ file line %d: %s"),
                                line_num,line.c_str());
    return false;
}

//write matrix in XYZ file
bool XYZFile::finalizeFile() const
{
    uni_stream::ifstream inFile;
    uni_stream::ofstream outFile;
    std::string line;
    std::string tmpFileName=current_absolute_path+"/tmp.tmp";
    outFile.open(tmpFileName.c_str());
    if (!outFile.is_open())
    {
        Project::theInfo()->warning(INFO_OBS,fileDepth+1,
                                    QT_TRANSLATE_NOOP("QObject","Impossible to create tmp file \"%s\"!"),
                                    tmpFileName.c_str());
        return false;
    }
    inFile.open((current_absolute_path+"/"+filename.c_str()).c_str());
    if (inFile.is_open())
    {
        std::regex regex_line(R"(^\*\*\!  Station.*$)");
        while (std::getline(inFile, line))
        {
            if (line.empty())
                continue;

            //remove exotic endline chars
            if (line.back()=='\r')
                line.resize(line.size()-1);

            if(std::regex_match(line,regex_line))
                break;
            else
                outFile<<line<<"\n";
        }
        Mat3 R_global2vert=global2vertRot(station->origin()->coord_comp, station->origin()->dev_eta, station->origin()->dev_xi);
        Mat3 R_global2instr=station->R_vert2instr*R_global2vert;
        Coord coord_compensated_cartesian;
        Project::theone()->projection.sphericalToGlobalCartesian(station->origin()->coord_comp,coord_compensated_cartesian);

        outFile<<std::fixed<<"**!  Station    : "<<station->origin()->name<<"\n";
        outFile<<"*  S =      "<<coord_compensated_cartesian.toString()<<"\n";
        outFile<<"*       ";
        outFile.precision(WRITEPREC);
        outFile.width(WRITEWIDTH);
        outFile<<R_global2instr(0,0);
        outFile<<" ";
        outFile.width(WRITEWIDTH);
        outFile<<R_global2instr(0,1);
        outFile<<" ";
        outFile.width(WRITEWIDTH);
        outFile<<R_global2instr(0,2)<<std::endl;
        outFile<<"*  R =  ";
        outFile.width(WRITEWIDTH);
        outFile<<R_global2instr(1,0);
        outFile<<" ";
        outFile.width(WRITEWIDTH);
        outFile<<R_global2instr(1,1);
        outFile<<" ";
        outFile.width(WRITEWIDTH);
        outFile<<R_global2instr(1,2)<<std::endl;
        outFile<<"*       ";
        outFile.width(WRITEWIDTH);
        outFile<<R_global2instr(2,0);
        outFile<<" ";
        outFile.width(WRITEWIDTH);
        outFile<<R_global2instr(2,1);
        outFile<<" ";
        outFile.width(WRITEWIDTH);
        outFile<<R_global2instr(2,2)<<std::endl;
        outFile<<"****  instr = R.(global-S) <=> global = R'.instr +S  ****\n";
        outFile<<"****  here global frame is in cartesian  ****\n";

        /*outFile<<"\n*   Measured points (=raw observations) in global cartesian frame:\n";
        for (auto & obs3d:station->observations3D)
        {
            outFile<<"*    "<<obs3d.obs1->to->name<<" ";
            Coord instrCoord=obs3d.obsToInstrumentFrameCoords(false);
            outFile<<station->mes2GlobalCart(instrCoord,false).toString();
            outFile<<"\n";
        }*/

        /*outFile<<"\n*   Measured points in projection frame (as in .new):\n";
        for (auto & obs3d:station->observations3D)
        {
            outFile<<"*    "<<obs3d.obs1->to->name<<" ";
            Coord instrCoord=obs3d.obsToInstrumentFrameCoords(false);
            Coord tmp1,tmp2;
            Project::theone()->projection.globalCartesianToSpherical(station->mes2GlobalCart(instrCoord,false),tmp1);
            Project::theone()->projection.sphericalToGeoref(tmp1,tmp2);
            outFile<<tmp2.toString();
            outFile<<"\n";
        }*/

        Coord origin_global_cart;
        Project::theone()->projection.sphericalToGlobalCartesian(station->origin()->coord_comp, origin_global_cart);
        tdouble radius = Project::theone()->projection.radius;
        tdouble scale_h = radius /(station->origin()->coord_comp.z() + radius); //scale due to ellips height
        outFile<<"\n****  approximate transfo to .cor frame  ****\n";
        outFile<<"****  R_approx' = R_gamma.S_proj.S_h.R_cart2spher'.R' ****\n";
        outFile<<"****  cor = R_approx'.instr + T ****\n";
        outFile<<"**** where:\n";
        Mat3 S_proj;
        tdouble scale_proj = Project::theone()->projection.getInputProjFactors(station->origin()->coord_comp).parallel_scale;
        S_proj<<scale_proj,0.,0., 0.,scale_proj,0., 0.,0.,1.;
        outFile<<"****    S_proj = [["<< S_proj(0,0) <<" "<< S_proj(0,1) <<" "<< S_proj(0,2) <<"] ["
                                    << S_proj(1,0) <<" "<< S_proj(1,1) <<" "<< S_proj(1,2) <<"] ["
                                    << S_proj(2,0) <<" "<< S_proj(2,1) <<" "<< S_proj(2,2)  <<"]]\n";
        Mat3 S_h;
        S_h<<scale_h,0.,0., 0.,scale_h,0., 0.,0.,1.;
        outFile<<"****    S_h = [["<< S_h(0,0) <<" "<< S_h(0,1) <<" "<< S_h(0,2) <<"] ["
                                 << S_h(1,0) <<" "<< S_h(1,1) <<" "<< S_h(1,2) <<"] ["
                                 << S_h(2,0) <<" "<< S_h(2,1) <<" "<< S_h(2,2)  <<"]]\n";
        Mat3 R_gamma;
        tdouble gamma = Project::theone()->projection.getInputProjFactors({0.,0.,0.}).meridian_convergence;
        R_gamma<<cos(gamma),-sin(gamma),0., sin(gamma), cos(gamma),0., 0.,0.,1.;
        outFile<<"****    R_gamma = [["<< R_gamma(0,0) <<" "<< R_gamma(0,1) <<" "<< R_gamma(0,2) <<"] ["
                                 << R_gamma(1,0) <<" "<< R_gamma(1,1) <<" "<< R_gamma(1,2) <<"] ["
                                 << R_gamma(2,0) <<" "<< R_gamma(2,1) <<" "<< R_gamma(2,2)  <<"]]\n";
        //auto R = station->R_vert2instr;
        tdouble angX = -atan( (origin_global_cart.z() + radius) / (scale_proj*radius)
                             * sin(station->origin()->coord_comp.x()/(scale_proj*radius))
                             / sqr(cos(station->origin()->coord_comp.x()/(scale_proj*radius))) );
        tdouble angY = -atan( (origin_global_cart.z() + radius) / (scale_proj*radius)
                             * sin(station->origin()->coord_comp.y()/(scale_proj*radius))
                             / sqr(cos(station->origin()->coord_comp.y()/(scale_proj*radius))) );
        auto R_cart2spher = angEN2Rot_small_angles( angX, angY );
        std::cout<<"center: "<<station->origin()->coord_comp.toString(10)<<"\n";
        std::cout<<"angles: "<<angX<<" "<<angY<<"\n";
        std::cout<<"R_cart2spher:\n"<<R_cart2spher<<"\n";
        outFile<<"****    R_cart2spher = [["<< R_cart2spher(0,0) <<" "<< R_cart2spher(0,1) <<" "<< R_cart2spher(0,2) <<"] ["
                                          << R_cart2spher(1,0) <<" "<< R_cart2spher(1,1) <<" "<< R_cart2spher(1,2) <<"] ["
                                          << R_cart2spher(2,0) <<" "<< R_cart2spher(2,1) <<" "<< R_cart2spher(2,2)  <<"]]\n";
        outFile<<"****    T = "<<station->origin()->coord_compensated_georef.toString()<<"\n";
        Mat3 R_approx = (R_gamma * S_proj * S_h * R_cart2spher.transpose() * R_global2instr.transpose()).transpose();

        // to have the same format as cartesian transfo:
        outFile<<"\n*  approximate transfo to .cor frame:\n";
        outFile<<"*  T =      "<<station->origin()->coord_compensated_georef.toString()<<"\n";
        outFile<<"*              ";
        outFile.width(WRITEWIDTH);
        outFile<<R_approx(0,0);
        outFile<<" ";
        outFile.width(WRITEWIDTH);
        outFile<<R_approx(0,1);
        outFile<<" ";
        outFile.width(WRITEWIDTH);
        outFile<<R_global2instr(0,2)<<std::endl;
        outFile<<"*  R_approx =  ";
        outFile.width(WRITEWIDTH);
        outFile<<R_approx(1,0);
        outFile<<" ";
        outFile.width(WRITEWIDTH);
        outFile<<R_approx(1,1);
        outFile<<" ";
        outFile.width(WRITEWIDTH);
        outFile<<R_approx(1,2)<<std::endl;
        outFile<<"*              ";
        outFile.width(WRITEWIDTH);
        outFile<<R_approx(2,0);
        outFile<<" ";
        outFile.width(WRITEWIDTH);
        outFile<<R_approx(2,1);
        outFile<<" ";
        outFile.width(WRITEWIDTH);
        outFile<<R_approx(2,2)<<std::endl;

        /*outFile<<"\n****  residuals approximation (.new-approx) (.new-cart):  ****\n";
        for (auto & obs3d:station->observations3D)
        {
            outFile<<"*    "<<obs3d.obs1->to->name<<" ";
            Coord instrCoord=obs3d.obsToInstrumentFrameCoords(false);
            Coord approx(R_gamma * S_proj * S_h * R_cart2spher.transpose() * R_global2instr.transpose() * instrCoord.toVect()
                         + station->origin()->coord_compensated_georef.toVect());
            outFile<<(obs3d.obs1->to->coord_compensated_georef - approx).toString()<<"  ";
            outFile<<(obs3d.obs1->to->coord_comp - station->mes2GlobalCart(instrCoord,false)).toString()<<"\n";
            std::cout<<"approx: "<<approx.toString()<<"\n";
            std::cout<<"coord_compensated_georef: "<<obs3d.obs1->to->coord_compensated_georef.toString()<<"\n";
            std::cout<<"coord_comp: "<<obs3d.obs1->to->coord_comp.toString()<<"\n";
            std::cout<<"cartglob: "<<station->mes2GlobalCart(instrCoord,false).toString()<<"\n";
        }*/

        inFile.close();
        outFile.close();

        std::string dst_path = current_absolute_path+"/"+filename;
        int err = uni_stream::rename_file(tmpFileName,dst_path);
        if (err != 0)
        {
            Project::theInfo()->warning(INFO_OBS,fileDepth+1,
                                        QT_TRANSLATE_NOOP("QObject","Error writing to \"%s\" : %s."),
                                        (current_absolute_path+"/"+filename).c_str(), // TODO: use std::filesystem::path
                                        ::strerror(err));
        }

    }else{
        Project::theInfo()->warning(INFO_OBS,fileDepth+1,
                                    QT_TRANSLATE_NOOP("QObject","Can't open file \"%s\"."),
                                    (current_absolute_path+"/"+filename).c_str()); // TODO: use std::filesystem::path
        return false;
    }
    return true;
}

//------------------------------------------------------------------------

AxisFile::AxisFile(const std::string &_filename, Station_Axis *_station,
                   const std::string &_current_absolute_path,
                   int _line_num_in_parent, unsigned _fileDepth)
    :DataFile(_filename,_current_absolute_path,_line_num_in_parent,_fileDepth),station(_station)
{
}

std::unique_ptr<AxisFile> AxisFile::create(const std::string &_filename, Station_Axis *_station,
                                           const std::string &_current_absolute_path,
                                           int _line_num_in_parent, unsigned int _fileDepth)
{
    return std::unique_ptr<AxisFile>(new AxisFile(_filename, _station, _current_absolute_path,
                                                  _line_num_in_parent, _fileDepth));
}

void AxisFile::clear() {}

bool AxisFile::interpret_line(const std::string &line, int line_num)
{
    //File format:
    //target_num  pos_num  ground_pt   sigma_rayon  sigma_perp
    std::regex regex_line(R"(^\s*([^*]+)\*?(.*)$)");
    std::smatch what;
    if(std::regex_match(line, what, regex_line))
    {
        // what[0] contains the whole string
        // what[1] contains the data part
        // what[2] contains the comment part
        if ((what[1]).length()>1)//there is something on the line
        {
            std::string line_data=what[1];
            if (line_data[0]=='L')
            {
                return station->read_constr(line_data,line_num,this,what[2]);
            }else{
                AxisObs obsAx(station);
                if (obsAx.read_obs(line_data,line_num,this,what[2]))
                {
                    station->updateTarget(obsAx.getTargetNum(), obsAx);
                    return true;
                }
            }
        }
    }
    Project::theInfo()->warning(INFO_OBS,fileDepth+1,
                                QT_TRANSLATE_NOOP("QObject","Error in AXE file line %d: %s"),
                                line_num,line.c_str());
    return false;
}

//------------------------------------------------------------------------
EqFile::EqFile(const std::string &_filename, Station_Eq *_station,
               const std::string &_current_absolute_path,
               int _line_num_in_parent, unsigned _fileDepth)
    :DataFile(_filename,_current_absolute_path,_line_num_in_parent,
              _fileDepth),station(_station)
{
}

std::unique_ptr<EqFile> EqFile::create(const std::string &_filename, Station_Eq *_station,
                                       const std::string &_current_absolute_path,
                                       int _line_num_in_parent, unsigned int _fileDepth)
{
    return std::unique_ptr<EqFile>(new EqFile(_filename, _station, _current_absolute_path,
                                              _line_num_in_parent, _fileDepth));
}

void EqFile::clear() {}

bool EqFile::interpret_line(const std::string &line, int line_num)
{
    //File format:
    //from to sigma *comm
    std::regex regex_line(R"(^\s*(\S+\s+\S+\s+)" FLOAT_REGEX R"()\s*([*].*|)$)");
    std::smatch what;
    if(std::regex_match(line, what, regex_line))
    {
        // what[0] contains the whole string
        // what[1] contains the data part
        // what[2] contains the comment part

        bool ok=true;
        bool active=true;
        std::string from_name,to_name;
        Point *from=nullptr;
        Point *to=nullptr;
        tdouble sigma;
        std::istringstream iss(line);

        if (!(iss >> from_name))
        {
            Project::theInfo()->warning(INFO_OBS,get_fileDepth()+1,
                                        QT_TRANSLATE_NOOP("QObject","At line %d: %s => "
                                                                    "Can't convert from point name."),
                                        line_num,line.c_str());
            ok=false;
        }
        //find a point with that name in project
        from=Project::theone()->getPoint(from_name,true);

        if (!(iss >> to_name))
        {
            Project::theInfo()->warning(INFO_OBS,get_fileDepth()+1,
                                        QT_TRANSLATE_NOOP("QObject","At line %d: %s => "
                                                                    "Can't convert to point name."),
                                        line_num,line.c_str());
            ok=false;
        }
        //find a point with that name in project
        to=Project::theone()->getPoint(to_name,true);

        if (from==to)
        {
            Project::theInfo()->error(INFO_OBS,get_fileDepth()+1,
                                      QT_TRANSLATE_NOOP("QObject","At line %d: %s => "
                                                                  "Observation between %s and %s."),
                                      line_num,line.c_str(),from->name.c_str(),to->name.c_str());
            ok=false;
        }

        if (!(iss >> sigma))
        {
            Project::theInfo()->warning(INFO_OBS,get_fileDepth()+1,
                                        QT_TRANSLATE_NOOP("QObject","At line %d: %s => "
                                                                    "Can't convert observation sigma."),
                                        line_num,line.c_str());
            ok=false;
        }

        if (fabs(sigma)<MINIMAL_SIGMA)
        {
            Project::theInfo()->warning(INFO_OBS,get_fileDepth()+1,
                                        QT_TRANSLATE_NOOP("QObject","At line %d: %s => "
                                                                    "Observation sigma is too small."),
                                        line_num,line.c_str());
            ok=false;
        }
        active = sigma>0;

        if (ok)
            station->observations.emplace_back(from,to,station,
                                               static_cast<OBS_CODE>(station->eq_type),active,0,
                                               sigma,0,0,0,line_num,1.0,"m",this,what[2].str());
        return ok;
    }

    Project::theInfo()->warning(INFO_OBS,fileDepth+1,
                                QT_TRANSLATE_NOOP("QObject","Error in EQ file line %d: %s"),
                                line_num,line.c_str());
    return false;
}

