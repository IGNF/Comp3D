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

#ifndef DATAFILE_H
#define DATAFILE_H

#include "uni_stream.h"
#include <memory>
#include <sstream>
#include <vector>

class Point;
class Obs;
class DataFile;
class Station_Bascule;
class Station_Axis;
class Station_Eq;

class FileComment
{
public:
    FileComment(const std::string &_text, int _line_num);
    std::string text;
    int line_num;
};

//------------------------------------------------------------------------
class DataFile
{
public:
    virtual ~DataFile();

    virtual void clear()=0;
    bool read();
    bool read(const std::string &_filename, const std::string &_current_absolute_path);
    virtual bool interpret_line(std::string line,int line_num)=0;
    virtual bool read_subfile(const std::string &filename, const std::string &absolute_path,int line_num);

    std::string get_name() const {return filename;}
    std::string get_path() const {return current_absolute_path;}
    unsigned get_fileDepth() const {return fileDepth;}
    int get_line_num_in_parent() const {return line_num_in_parent;}//only if filed called from an other
    bool exists() const {return fileExists;}

protected:
    DataFile();
    DataFile(const std::string &_filename, const std::string &_current_absolute_path=".", int _line_num_in_parent=-1, unsigned _fileDepth=1);
    DataFile(const DataFile&) = delete;
    DataFile(DataFile&&) = delete;
    DataFile& operator=(const DataFile&) = delete;
    DataFile& operator=(DataFile&&) = delete;

    std::string filename;
    std::string current_absolute_path;
    std::vector<FileComment> comments;
    int line_num_in_parent;//only if file called from an other

    unsigned fileDepth;
    bool fileExists;
};

//------------------------------------------------------------------------
class CORFile:public DataFile
{
public:
    static std::unique_ptr<CORFile> create(const std::string &_filename, const std::string &_current_absolute_path=".",
                                           int _line_num_in_parent=-1, unsigned _fileDepth=0);

    virtual void clear() override;
    virtual bool interpret_line(std::string line,int line_num) override;
    virtual bool read_subfile(const std::string &filename, const std::string &absolute_path,int line_num) override;

    //virtual bool write(std::string new_filename);
    void writeNEW(uni_stream::ofstream *file_new, int recurse_level=0);
    void writeCORcompensated(uni_stream::ofstream *file_cor, bool is_coord_init, bool with_pts_constr=false, int recurse_level=0) const;//recursive, used new2cor tool
    void removePoint(Point * point_to_remove);
    void addPoint(Point * point);
    std::vector <Point*> get_points(){return points;}
protected:
    explicit CORFile(const std::string &_filename, const std::string &_current_absolute_path, int _line_num_in_parent, unsigned _fileDepth);

    std::vector <Point*> points;
    std::vector <Point*> new_points;//points added by computation
    std::vector<std::unique_ptr<CORFile>> sub_files;
};
//------------------------------------------------------------------------
//new is like a simple COR, without points codes and subfiles
class NewFile:public CORFile
{
public:
    static std::unique_ptr<NewFile> create(const std::string &_filename);

    virtual void clear() override;
    virtual bool interpret_line(std::string line,int line_num) override;
    virtual bool read_subfile(const std::string &filename, const std::string &absolute_path,int line_num) override;
    std::vector <Point*> get_points(){return points;}
protected:
    explicit NewFile(const std::string &_filename);
    std::vector <std::unique_ptr<Point>> points_owner;

};
//------------------------------------------------------------------------
class OBSFile:public DataFile
{
public:
    static std::unique_ptr<OBSFile> create(const std::string &_filename,
                                           const std::string &_current_absolute_path=".",
                                           int _line_num_in_parent=-1, unsigned _fileDepth=0);

    virtual void clear() override;
    virtual bool interpret_line(std::string line, int line_num) override;
    virtual bool read_subfile(const std::string &filename, const std::string &absolute_path,int line_num) override;
protected:
    explicit OBSFile(const std::string &_filename, const std::string &_current_absolute_path,
                     int _line_num_in_parent, unsigned _fileDepth);

    std::vector<std::unique_ptr<OBSFile>> sub_files;
};
//------------------------------------------------------------------------
class XYZFile:public DataFile
{
public:
    static std::unique_ptr<XYZFile> create(const std::string &_filename, Station_Bascule *_station,
                                           const std::string &_current_absolute_path=".",
                                           int _line_num_in_parent=-1, unsigned _fileDepth=0);

    virtual void clear() override;
    virtual bool interpret_line(std::string line, int line_num) override;

    bool finalizeFile() const;//write matrix in XYZ file
protected:
    XYZFile(const std::string &_filename, Station_Bascule *_station,
            const std::string &_current_absolute_path,
            int _line_num_in_parent, unsigned _fileDepth);

    Station_Bascule * station;
};
//------------------------------------------------------------------------
class AxisFile:public DataFile
{
public:
    static std::unique_ptr<AxisFile> create(const std::string &_filename, Station_Axis *_station,
                                            const std::string &_current_absolute_path=".",
                                            int _line_num_in_parent=-1, unsigned _fileDepth=0);

    virtual void clear() override;
    virtual bool interpret_line(std::string line, int line_num) override;
protected:
    AxisFile(const std::string &_filename, Station_Axis *_station,
             const std::string &_current_absolute_path,
             int _line_num_in_parent, unsigned _fileDepth);

    Station_Axis * station;
};
//------------------------------------------------------------------------
class EqFile:public DataFile
{
public:
    static std::unique_ptr<EqFile> create(const std::string &_filename, Station_Eq *_station,
                                          const std::string &_current_absolute_path=".",
                                          int _line_num_in_parent=-1, unsigned _fileDepth=0);

    virtual void clear() override;
    virtual bool interpret_line(std::string line, int line_num) override;
protected:
    EqFile(const std::string &_filename, Station_Eq *_station,
           const std::string &_current_absolute_path,
           int _line_num_in_parent, unsigned _fileDepth);

    Station_Eq * station;
};

#endif // DATAFILE_H
