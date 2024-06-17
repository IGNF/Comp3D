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

#ifndef EXPORTPOINTSDIALOG_H
#define EXPORTPOINTSDIALOG_H

#include <QDialog>
#include <QGridLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <vector>

#include "project.h"
#include "point.h"

namespace Ui {
class ExportPointsDialog;
}

enum class ExportPointsDialogType{
    simple=0,
    sinex,
    varcov
};

class ExportPointsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportPointsDialog(Project_Config *config, std::list<Point> &points, ExportPointsDialogType type, QWidget *parent = 0);
    ~ExportPointsDialog();
    std::vector <Point*> getSelectedPoints();
    std::vector <bool> getOptions();
    void saveMetaData();
    static const std::map<ExportPointsDialogType,std::string> typesNames;

private:
    void fillSimple();
    void fillVarCov();
    void fillSinex();
    Ui::ExportPointsDialog *ui;
    std::vector<Point*> m_pointsFiltered;
    Project_Config *m_config;

    std::vector <QCheckBox*> exportChecks;
    std::vector <QCheckBox*> optionsChecks;
    //for sinex
    std::vector <QLineEdit*> lineCODE;
    std::vector <QLineEdit*> linePT;
    std::vector <QLineEdit*> lineDOMES;
    std::vector <QComboBox*> comboTECH;
    std::vector <QLineEdit*> lineDESCR;

    ExportPointsDialogType m_type;

    static QStringList techList;
    static int tech2Num(QString str);

public slots:
    void select_all();
    void select_none();
    void update_enabled();

};

#endif // EXPORTPOINTSDIALOG_H
