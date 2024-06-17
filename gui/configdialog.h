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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>

#include "project.h"

namespace Ui {
class ConfigDialog;
}

class ConfigDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ConfigDialog(Project *project,QWidget *parent = 0);
    void updateProjectConfig();

    ~ConfigDialog();
    void updateDisplay();//update what has to be hidden or shown
    void recordToProject();//when click on OK
private:
    Ui::ConfigDialog *ui;
    Project *m_project;

public slots:
    void selectCOR();
    void selectOBS();
    void selectCoordCov();
    void updateInvertCheckBox();
    void updateIterationsLabel();
    void updateOkButton();
};

#endif // CONFIGDIALOG_H
