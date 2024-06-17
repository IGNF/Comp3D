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

#ifndef CALCTHREAD_H
#define CALCTHREAD_H

#include <QThread>
#include "project.h"
class CalcThread : public QThread
{
    Q_OBJECT
public:
    explicit CalcThread(Project *project, QObject *parent = 0);
    ~CalcThread();
    void run();
signals:
    
public slots:
protected:
    Project *m_project;
};

#endif // CALCTHREAD_H
