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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QApplication>
#include <sstream>
#include "project.h"
#include "calcthread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QApplication *a, QWidget *parent = nullptr);
    ~MainWindow();
private:
    Ui::MainWindow *ui;
protected:
    Project *project;

    bool readyToCompute,afterCompensation;//gui states: project null, project non-null, readyToCompute, calculating, afterCompensation
    std::map<COMPUTE_TYPE, QString> startButtonText;

    QApplication * m_app;
    CalcThread * m_calcThread;

    void setAfterCompensation(bool after);
    void setProject(Project *aProject);
    void closeProject();
    bool checkProject();
    bool checkInverse();


    void resizeEvent(QResizeEvent * event);

public slots:
    bool openProject(std::string filename="");
    bool newProject();
    bool readData(bool firstRead=false);
    void showAbout();

    bool startComputation();
    void askToStopComputation();
    void changePreferences();
    bool reload();
    bool openConfig();
    void showJson();
    void showHelp();

    void enterKernel();
    void enterInvert();
    void updateSigma0(int number, double convergence);
    void endOfComputation();

    //---- tools -----
    bool sightMatrixToObs();
    bool infinityAscToBas();
    bool exportCoordinates();
    bool exportSightMatrix();
    bool exportVarCov();
    bool exportSinex();
    bool exportRelPrec();
    bool exportParamVari();
    bool applyTransfo();//apply the transfo found in a xyz file to a cor file
    bool export2cor();
    bool convertCoordCartProj();
    void projectTemplate();
};

#endif // MAINWINDOW_H
