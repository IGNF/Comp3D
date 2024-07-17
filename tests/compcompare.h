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
#include <QRegExp>

/**********************
 * Compare two .comp files
 * show differences
 *********************/


// NoComparePath use wildcard pattern
//
// a non-leaf node name (objects && arrays) has the form:
// /node_0/.../node_n/          // trailing '/'
//
// a lead node name has the form:
// /node_0/.../leaf             // no trailing '/'
//
// Wildcard patterns use '*', '?' and '[]' with the same meaning as in a Unix shell.
// Wildcard can be backslashed (double backslash in a C-string: "/node/\\[0\\]/rank" to match exactly "/node/[0]/rank"
//
// Warning: '/' has no special meaning ! It will be matched like any other character
//
// To filter out all under /test/computation:
// "/test/computation/"
//
// To filter all "rank" leaves:
// "*/rank"
//
// To filter all "rank" leaves under "computation":
// "/computation/*/rank"      // will not filter out /computation/rank !
//
// To filter all "rank" leaves in an array:
// "*\\]/rank"
//
//

class CompCompare
{
public:
    CompCompare(const std::string &_compFilename, const std::string &_refFilename, const std::string &_refMD5sum, bool _verbose, double _tolerance);
    void addMoreToleranceNodes(std::vector<std::string> &moreToleranceNodes2);
    void addNoCompareNodePaths(const std::vector<std::string>& moreNoComparePaths);
    bool openFile(std::string filename, Json::Value &root, std::string md5sum="");
    bool checkContent();//<check all the data
    bool checkOnly(std::vector<std::string> comparePaths);//<check only selected nodes
    bool checkOnly(std::vector< std::pair<std::string, std::string> > comparePaths);//<check only selected nodes, with new names (if not "")
    bool isError() const{return error;}
protected:
    bool checkNode(Json::Value nodeComp, Json::Value nodeRef, std::string path, bool moreTolerance);//recursive
    bool checkFiles(Json::Value nodeComp, Json::Value nodeRef, const std::string &path);
    std::string compFilename;
    std::string refFilename;
    std::string refMD5um;
    Json::Value rootComp;
    Json::Value rootRef;
    bool error;
    bool verbose;
    double tolerance;
    std::vector<std::string> moreToleranceNodes;//leaves where tolerance is *1000
    std::vector<std::string> noCompareNodePaths;
    std::vector<QRegExp> noCompareRE;
    bool checkStrings(Json::Value nodeComp, Json::Value nodeRef, const std::string &path);
};

#endif // COMPCOMPARE_H
