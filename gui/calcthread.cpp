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

#include "calcthread.h"

CalcThread::CalcThread(Project *project, QObject *parent) :
    QThread(parent),m_project(project)
{
}

CalcThread::~CalcThread()
{

}


void CalcThread::run()
{
    bool ok = m_project->computation(m_project->config.invert,true);
    if (ok)
        std::cout<<"CalcThread::run() successful."<<std::endl;
    else
        std::cout<<"CalcThread::run() failed!"<<std::endl;
}

