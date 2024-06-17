/**
 * Copyright (c) Institut national de l'information géographique et forestière https://www.ign.fr/
 *
 * File main authors:
 *  - JM Muller
 *  - C Meynard
 *
 * This file is part of Comp3D: https://github.com/IGNF/Comp3D
 *
 * Comp3D is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Comp3D is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Comp3D. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MAINTABLES_H
#define MAINTABLES_H

#include <QSplitter>

namespace Ui {
class MainTables;
}

class Project;
class Point;
class Obs;

class MainTables : public QSplitter
{
    Q_OBJECT

public:
    explicit MainTables(QWidget *parent = nullptr);
    ~MainTables();

    void updateGraphics();      // when window is resized
    void updateContent(bool initSort=false);    // update diplayed lists

    void setProject(const Project *aProject);
    void setAfterCompensation(bool after);

    void disableInteraction();

private slots:
    void splitterMoved();
    void obsClicked(int row, int column);

private:
    void openObsFile(QString path);
    void setObsSelectionActiveState(int state);
    void obsContextMenu(const QPoint &pos);
    void setObsActiveStyle(int row, bool active, bool modified);
    void setObsActive(Obs* obs, bool active);
    void disableSort();
    void enableSort();
    std::string toFromToolTip(const Point *point);

    const Project *project;
    bool  afterCompensation;
    bool showCoord,showObs;
    bool interactive;

    Ui::MainTables *ui;
};

#endif // MAINTABLES_H
