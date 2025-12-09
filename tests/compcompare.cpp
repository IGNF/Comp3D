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

#include "compcompare.h"

#include <iostream>
#include <QCryptographicHash>
#include "project_config.h"

CompCompare::CompCompare(const std::string &_compFilename, const std::string &_refFilename, const std::string &_refMD5sum, bool _verbose, double _tolerance):
    compFilename(_compFilename),refFilename(_refFilename),refMD5um(_refMD5sum),error(false),verbose(_verbose),tolerance(_tolerance),
    moreToleranceNodes({"residual_std", "standardized_residual", "normalized_residual", "all_sigma0", "sigma_0_init", "obs_azim", "obs_zen"}),
    noCompareNodePaths({"/COMP3D_*", "/computation/computation_duration","/computation/computation_start",
                        "/computation/solver_name","/config_file", "/all_data_files*", "/computation/all_parameters_names/", "*/params/*/rank"})

{
    if (!openFile(compFilename,rootComp))
    {
        std::cout<<"Error reading "<<compFilename<<std::endl;
        error=true;
    }
    if (!openFile(refFilename,rootRef,refMD5um))
    {
        std::cout<<"Error reading "<<refFilename<<std::endl;
        error=true;
    }
    for (const auto& path: noCompareNodePaths)
        noCompareRE.push_back(QRegExp(path.c_str(),Qt::CaseSensitive,QRegExp::PatternSyntax::WildcardUnix));
}

void CompCompare::addMoreToleranceNodes(std::vector<std::string> &moreToleranceNodes2)
{
    for (auto & str:moreToleranceNodes2)
        moreToleranceNodes.push_back(str);
    std::cout<<"moreToleranceNodes: ";
    for (auto & str:moreToleranceNodes)
        std::cout<<str<<" ";
    std::cout<<std::endl;
}

void CompCompare::addNoCompareNodePaths(const std::vector<std::string> &moreNoComparePaths)
{
    for (auto & str:moreNoComparePaths)
            noCompareRE.push_back(QRegExp(str.c_str(),Qt::CaseSensitive,QRegExp::PatternSyntax::WildcardUnix));
    std::cout<<"noCompareNodePaths: ";
    for (auto & str:noCompareRE)
        std::cout << str.pattern().toStdString() <<" ";
    std::cout<<std::endl;
}

bool CompCompare::openFile(std::string filename,Json::Value &root,std::string md5sum)
{
    Json::CharReaderBuilder rbuilder;
    std::string config_file_contents,jsondata;
    std::string errors;
    std::cout<<"Opening file "<<filename<<std::endl;

    try
    {
        config_file_contents=Project_Config::get_file_contents(filename.c_str());
        // Remove all '\r', may happen if we are running on Windows or the file is coming from a Windows system
        config_file_contents.erase( std::remove(config_file_contents.begin(), config_file_contents.end(), '\r'), config_file_contents.end() );
        unsigned long pos = config_file_contents.find("=");
        jsondata= config_file_contents.substr(pos+1);
    }
    catch (std::exception& e)
    {
        printf("Exception: %s while reading configuration file \"%s\".",e.what(),filename.c_str());
        return false;
    }
    catch (int e)
    {
        printf("Error num %d: Can't open configuration file \"%s\".",e,filename.c_str());
        return false;
    }

    if (!md5sum.empty())
    {
        std::string hash=QCryptographicHash::hash(QByteArray(config_file_contents.c_str(),config_file_contents.size()),QCryptographicHash::Md5).toHex().toStdString();
        std::cout<<"MD5 given     : "<<md5sum<<"\n";
        std::cout<<"MD5 calculated: "<<hash;
        if (md5sum!=hash)
        {
            std::cout<<" Not good!"<<std::endl;
            return false;
        }else
             std::cout<<" Ok!"<<std::endl;
    }

    std::istringstream jsondataStream(jsondata);
    bool parsingSuccessful = Json::parseFromStream(rbuilder,jsondataStream , &root, &errors);
    if ( !parsingSuccessful )
    {
        printf("Failed to parse configuration: %s.",errors.c_str());
        return false;
    }

    return true;
}

bool CompCompare::checkContent()
{
    if (error) return false;
    //check each node of reference exept date and some other special ones
    return checkNode(rootComp,rootRef,"", false);
}


bool CompCompare::checkStrings(Json::Value nodeComp, Json::Value nodeRef, const std::string& path)
{
    QString val1=nodeRef.asString().c_str();
    QString val2=nodeComp.asString().c_str();
    val1.replace("\\","/");
    val2.replace("\\","/");
    if (!(val1==val2)) {
        std::cout<<"Error! Difference found for node "<<path<<": ";
        std::cout<<val1.toStdString()<<" /VS/ "<<val2.toStdString()<<"\n";
        std::cout<<std::endl;
        return false;
    } else {
        return true;
    }
}

bool CompCompare::checkFiles(Json::Value nodeComp, Json::Value nodeRef, const std::string& path)
{
    Json::Value filesRef = rootRef["all_data_files"];
    if (! filesRef) {
        std::cout << "Error! Missing node \"all_data_files\" in " << refFilename << "\n" << std::endl;
        return false;
    }
    Json::Value filesComp = rootComp["all_data_files"];
    if (! filesComp) {
        std::cout << "Error! Missing node \"all_data_files\" in " << compFilename << "\n" << std::endl;
        return false;
    }
    if (nodeComp == -1 && nodeRef == -1)
        return true;
    Json::Value fileComp = filesComp[nodeComp.asString()];
    Json::Value fileRef = filesRef[nodeRef.asString()];
    if (nodeComp == -1 || nodeRef == -1) {              // 'xor' because 'and' already tested above
        std::cout<<"Error! Difference found for node "<<path<<": ";
        std::cout << fileRef  << "(" << nodeRef << ") /VS/ " << fileComp << "(" << nodeComp << ")\n" << std::endl;
        return false;
    }
    return checkStrings(fileComp,fileRef,path);
}

bool CompCompare::checkNode(Json::Value nodeComp, Json::Value nodeRef, std::string path, bool moreTolerance)
{
    bool ok=true;
    if (verbose)
        std::cout<<"Call checkNode on "<<path<<std::endl;

    if (nodeRef.type()!=nodeComp.type())
    {
        std::cout<<"Error! Difference found for node "<<path<<": ";
        std::cout<<nodeRef.type()<<" /VS/ "<<nodeComp.type()<<"\n";
        std::cout<<std::endl;
        return false;
    }
    std::size_t found = path.find_last_of("/\\");
    std::string name = path.substr(found+1);
    if (std::find(moreToleranceNodes.begin(), moreToleranceNodes.end(),name)!=moreToleranceNodes.end())
        moreTolerance = true;
    switch (nodeRef.type()) {
        case Json::stringValue:
            return checkStrings(nodeComp,nodeRef,path);
        case Json::intValue:
        case Json::uintValue:
        case Json::booleanValue:
            if (!(nodeRef==nodeComp))
            {
                std::cout<<"Error! Difference found for node "<<path<<": ";
                std::cout<<nodeRef<<" /VS/ "<<nodeComp<<"\n";
                std::cout<<std::endl;
                return false;
            }else
                return true;
        case Json::realValue:
            if (path.find("axes")!=std::string::npos)
            {
                if ((fabs(nodeRef.asDouble()-nodeComp.asDouble())>tolerance)&&(fabs(nodeRef.asDouble()-nodeComp.asDouble())-200>tolerance))
                {
                    std::cout<<"Error! Difference found for axes node "<<path<<": ";
                    std::cout<<nodeRef<<" /VS/ "<<nodeComp<<"\n";
                    std::cout<<std::endl;
                    return false;
                }
            }else if (fabs(nodeRef.asDouble()-nodeComp.asDouble())>tolerance)
            {
                if (moreTolerance && (fabs(nodeRef.asDouble()-nodeComp.asDouble())<tolerance*1000))
                    return true;
                std::cout<<"Error! Difference found for node "<<path<<": ";
                std::cout<<nodeRef<<" /VS/ "<<nodeComp<<"\n";
                std::cout<<std::endl;
                return false;
            }else
                return true;
        case Json::arrayValue:
        case Json::objectValue:
        default:
            break;
    }

    if (nodeRef.type()==Json::objectValue)
    {
        //if not terminal, check sub nodes
        std::vector <std::string> nodeRefMembers=nodeRef.getMemberNames();
        //check all nodeRef nodes
        for (auto &name:nodeRefMembers)
        {
            auto subNodePath = path + "/" + name;
            auto testPath = subNodePath;
            if (nodeRef[name].isArray() || nodeRef[name].isObject())
                testPath += "/";

            if (std::find_if(noCompareRE.begin(), noCompareRE.end(),[testPath](const QRegExp& re){return re.exactMatch(testPath.c_str());})==noCompareRE.end())
            {
                if (verbose)
                    std::cout<<"Checking "<<subNodePath<<"\n";
                //test if node exists in nodeComp
                if (!nodeComp.isMember(name))
                {
                    std::cout<<subNodePath<<": Error! Node not existing in comp file!\n";
                    std::cout<<std::endl;
                    return false;
                }
                Json::Value subNodeComp=nodeComp[name];
                Json::Value subNodeRef=nodeRef[name];
                if (name == "file_id" && subNodeRef.type() == Json::intValue && subNodeComp.type() == Json::intValue)
                    ok = ok && checkFiles(subNodeComp, subNodeRef, subNodePath);
                else
                    ok = ok && checkNode(subNodeComp, subNodeRef, subNodePath, moreTolerance);
                if (!ok)
                    return false;//skip the rest
            }else{
                if (verbose)
                    std::cout<<"Skip "<<subNodePath<<"\n";
            }
        }
    }else if (nodeRef.type()==Json::arrayValue){
        if (nodeRef.size()!=nodeComp.size())
        {
            std::cout<<path<<": Error! Array with different sizes: "<<nodeRef.size()<<"/"<<nodeComp.size()<<".\n";
            std::cout<<std::endl;
            return false;
        }
        for (unsigned int i=0;i<nodeRef.size();i++)
        {
            std::ostringstream oss;
            oss<<path<<"/["<<i<<"]";
            if (verbose)
                std::cout<<"Checking "<<oss.str()<<"\n";
            Json::Value subNodeComp=nodeComp[i];
            Json::Value subNodeRef=nodeRef[i];
            ok = ok && checkNode(subNodeComp, subNodeRef, oss.str(), moreTolerance);
            if (!ok)
                return false;//skip the rest
        }
    }
    return ok;
}

bool CompCompare::checkOnly(std::vector<std::string> comparePaths)
{
    bool pathFoundInRef = true;
    for (auto& path:comparePaths)
    {
        pathFoundInRef = true;
        QRegExp rx("/");
        QStringList path_tokens = QString(path.c_str()).split(rx);
        Json::Value subNodeComp=rootComp;
        Json::Value subNodeRef=rootRef;
        //std::cout<<"checkOnly: "<<path<<std::endl;
        for (auto& token:path_tokens)
        {
            if (token.isEmpty()) continue;
            //std::cout<<"Token: "<<token.toStdString()<<std::endl;
            subNodeComp=subNodeComp[token.toStdString()];
            subNodeRef=subNodeRef[token.toStdString()];
            if (!subNodeComp.type())
            {
                std::cout<<"Error! Path \""<<path<<"\" does not exist in "<<compFilename<<"."<<std::endl;
                return false;
            }
            if (!subNodeRef.type())
            {
                pathFoundInRef = false;
                break;
            }
        }
        if (pathFoundInRef)
            if (!checkNode(subNodeComp,subNodeRef,path,false))
                return false;
    }
    return true;
}

bool CompCompare::checkOnly(std::vector< std::pair<std::string, std::string> > comparePaths)
{
    bool pathFoundInRef = true;
    for (auto& oldnewpath:comparePaths)
    {
        pathFoundInRef = true;
        auto& oldpath = oldnewpath.first;
        auto& newpath = oldnewpath.second;
        QRegExp rx("/");
        QStringList oldpath_tokens = QString(oldpath.c_str()).split(rx);
        if (newpath.empty()) newpath = oldpath; //support <"path",""> if path is the same before and after
        QStringList newpath_tokens = QString(newpath.c_str()).split(rx);
        if (oldpath_tokens.size()!=newpath_tokens.size())
        {
            std::cout<<"Error! With test setup : paths \""<<oldpath<<"\" and \""<<newpath<<"\" do not have the same depth."<<std::endl;
            return false;
        }
        Json::Value subNodeComp=rootComp;
        Json::Value subNodeRef=rootRef;
        //std::cout<<"checkOnly: "<<path<<std::endl;
        for (int i=0;i<oldpath_tokens.size();++i)
        {
            auto oldtoken = oldpath_tokens[i];
            auto newtoken = newpath_tokens[i];
            if (oldtoken.isEmpty()) continue;
            if (newtoken.isEmpty()) continue;
            subNodeComp=subNodeComp[newtoken.toStdString()];
            subNodeRef=subNodeRef[oldtoken.toStdString()];
            if (!subNodeComp.type())
            {
                std::cout<<"Error! Path \""<<newpath<<"\" does not exist in "<<compFilename<<"."<<std::endl;
                return false;
            }
            if (!subNodeRef.type())
            {
                pathFoundInRef = false;
                break;
            }
        }
        if (pathFoundInRef)
            if (!checkNode(subNodeComp,subNodeRef,newpath, false))
                return false;
    }
    return true;
}
