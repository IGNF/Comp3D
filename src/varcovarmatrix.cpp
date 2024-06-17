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

#include "varcovarmatrix.h"

#include <fstream>
#include <chrono>
#include "project.h"
#include "compile.h"
#include "point.h"

std::vector<int> VarCovarMatrix::selectUsefulParams(const std::vector<std::string> &params_col)
{
    std::vector<int> selectedIndices;
    observations.clear();
    for (unsigned long i=0; i<params_col.size(); i++)
    {
        std::string param = params_col[i];
        std::size_t pos = param.find_last_of("_");
        std::string pt_name = param.substr(0,pos);
        std::string pt_dim = param.substr(pos+1);
        if (pt_name.empty() || pt_dim.empty())
        {
            Project::theInfo()->error(INFO_OBS,1,
                                      QT_TRANSLATE_NOOP("QObject","Error in parameter name: %s"),
                                      param.c_str());
            return {};
        }
        Point* thePoint = Project::theone()->getPoint(pt_name, false);
        if (thePoint)
        {
            if (pt_dim=="x")
            {
                if (thePoint->obsX)
                {
                    observations.push_back(thePoint->obsX);
                    selectedIndices.push_back(i);
                #ifdef CORCOV_INFO
                    std::cout<<"Cov "<<i<<" on obs: "<<thePoint->obsX->toString()<<"\n";
                #endif
                }
            }
            else if (pt_dim=="y")
            {
                if (thePoint->obsY)
                {
                    observations.push_back(thePoint->obsY);
                    selectedIndices.push_back(i);
                #ifdef CORCOV_INFO
                    std::cout<<"Cov "<<i<<" on obs: "<<thePoint->obsY->toString()<<"\n";
                #endif
                }
            }
            else if (pt_dim=="z")
            {
                if (thePoint->obsZ)
                {
                    observations.push_back(thePoint->obsZ);
                    selectedIndices.push_back(i);
                #ifdef CORCOV_INFO
                    std::cout<<"Cov "<<i<<" on obs: "<<thePoint->obsZ->toString()<<"\n";
                #endif
                }
            }
            //other parameters are stations unknowns, not used
        }
    }
    return selectedIndices;
}

VarCovarMatrix::VarCovarMatrix(const std::string &csv_path, const std::string &current_absolute_path) :
    observations(), matrix(), isOk(false)
{
    auto matrix_reading_start = std::chrono::steady_clock::now();

    uni_stream::ifstream file_stream(current_absolute_path+csv_path); //uni stream
    if(!file_stream.good())
    {
        Project::theInfo()->error(INFO_OBS,1,
                                  QT_TRANSLATE_NOOP("QObject","Error while reading %s!"),
                                  csv_path.c_str());
        return;
    }
    std::string line;
    std::vector<std::string> params_col;
    std::vector<std::string> params_row;
    std::vector<tdouble> vals;

    std::getline(file_stream, line);//read first line: params_col
    std::replace(line.begin(), line.end(), ',', ' ');
    std::stringstream line_stream(line);
    std::string token;
    line_stream>>token;//useless (0,0)
    while (line_stream>>token)
        params_col.push_back(token);
    auto selectedIndices = selectUsefulParams(params_col);

    matrix.resize(selectedIndices.size(),selectedIndices.size());
    int num_row = 0;
    int num_row_index = 0;
    while (std::getline(file_stream, line))
    {
        if (num_row != selectedIndices.at(num_row_index))
        {
            ++num_row;
            if (num_row>=(int)params_col.size())
                break;
            continue;
        }
        int num_col = -1; //1st col is param name
        int num_col_index=0;
        std::replace(line.begin(), line.end(), ',', ' ');
        line_stream.clear();
        line_stream.str(line);
        while (line_stream>>token)
        {
            if (num_col == -1)
            {
                params_row.push_back(token);
            #ifdef CORCOV_INFO
                std::cout<<"reading row "<<num_row<<": "<<token<<"\n";
            #endif
            } else {
                if (num_col == selectedIndices.at(num_col_index))
                {
                    matrix(num_row_index, num_col_index) = std::stod(token);
                    ++num_col_index;
                    if (num_col_index>=(int)selectedIndices.size())
                        break;
                }
            }
            ++num_col;
        }
        ++num_row_index;
        if (num_row_index>=(int)selectedIndices.size())
            break;
        ++num_row;
        if (num_row>=(int)params_col.size())
            break;
    }

    //check params names
    for (unsigned int i = 0; i < params_row.size(); i++)
        if (params_row[i]!=params_col[selectedIndices[i]])
        {
            Project::theInfo()->error(INFO_OBS,1,
                                      QT_TRANSLATE_NOOP("QObject","Error param %u: %s != %s"),
                                      i, params_col[selectedIndices[i]].c_str(), params_row[i].c_str());
            return;
        }

    std::cout<<"Params checked.\n";

    // set sigmas
    for (unsigned long i=0; i<observations.size(); i++)
    {
        observations[i]->sigmaAbs = sqrt(matrix(i,i));
        observations[i]->varianceFromMatrix = true;
        observations[i]->sigmaRel = 0;
        if (observations[i]->code==OBS_CODE::COORD_X)
            observations[i]->from->sigmas_init.setx(observations[i]->sigmaAbs);
        if (observations[i]->code==OBS_CODE::COORD_Y)
            observations[i]->from->sigmas_init.sety(observations[i]->sigmaAbs);
        if (observations[i]->code==OBS_CODE::COORD_Z)
            observations[i]->from->sigmas_init.setz(observations[i]->sigmaAbs);
    }

    auto matrix_reading_end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = matrix_reading_end-matrix_reading_start;
    std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";

#ifdef CORCOV_INFO
    std::cout<<"CorCov matrix:\n"<<matrix<<"\n";
#endif

    isOk = true;
}
