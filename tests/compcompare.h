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

#ifndef COMPCOMPARE_H
#define COMPCOMPARE_H

#include <json/json.h>

/**********************
 * Compare two .comp files
 * show differences
 *********************/
class CompCompare
{
public:
    CompCompare(const std::string &_compFilename, const std::string &_refFilename, const std::string &_refMD5sum, bool _verbose, double _tolerancy);
    void addMoreTolerancyLeaves(std::vector<std::string> &moreTolerancyLeaves2);
    void addNoComparePaths(std::vector<std::string> &noComparePaths2);
    void addNoCompareLeaves(std::vector<std::string> &noCompareLeaves2);
    bool openFile(std::string filename, Json::Value &root, std::string md5sum="");
    bool checkContent();//<check all the data
    bool checkOnly(std::vector<std::string> comparePaths);//<check only selected nodes
    bool checkOnly(std::vector< std::pair<std::string, std::string> > comparePaths);//<check only selected nodes, with new names (if not "")
    bool isError() const{return error;}
protected:
    bool checkNode(Json::Value nodeComp, Json::Value nodeRef, std::string path);//recursive
    bool checkFiles(Json::Value nodeComp, Json::Value nodeRef, const std::string &path);
    std::string compFilename;
    std::string refFilename;
    std::string refMD5um;
    Json::Value rootComp;
    Json::Value rootRef;
    bool error;
    bool verbose;
    double tolerancy;
    std::vector<std::string> moreTolerancyLeaves;//leaves where tolerancy is *1000
    std::vector<std::string> noComparePaths, noCompareLeaves;
    bool checkStrings(Json::Value nodeComp, Json::Value nodeRef, const std::string &path);
};

#endif // COMPCOMPARE_H
