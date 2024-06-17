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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QScrollBar>
#include <QInputDialog>
#include <QTranslator>
#include <QLocale>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QTextStream>
#include <QSettings>
#include <vector>
#include <thread>

#include "errordialog.h"
#include "station_hz.h"
#include "station_simple.h"
#include "configdialog.h"
#include "exportcoorddialog.h"
#include "exportpointsdialog.h"
#include "applytransfodialog.h"
#include "sightmatrixdialog.h"
#include "infinityasctobasdialog.h"
#include "conversiondialog.h"
#include "dialogpreferences.h"
#include "sinexdialog.h"

MainWindow::MainWindow(QApplication *a, QWidget *parent) :
    QMainWindow(parent),ui(new Ui::MainWindow),project(nullptr),
    readyToCompute(false),afterCompensation(false),m_app(a),m_calcThread(nullptr)
{
    ui->setupUi(this);
    ui->statusbar->showMessage(QString(COMP3D_VERSION)+" "+COMP3D_COPYRIGHT+" - "+COMP3D_LICENSE);
    startButtonText[COMPUTE_TYPE::type_compensation]=tr("&Start Compensation");
    startButtonText[COMPUTE_TYPE::type_propagation]=tr("&Start Propagation");
    startButtonText[COMPUTE_TYPE::type_monte_carlo]=tr("&Start Monte-Carlo");

    setAfterCompensation(false);
    setProject(nullptr);

    connect(this->ui->actionExit,SIGNAL(triggered()),this,SLOT(close()));
    connect(this->ui->actionOpen_project,SIGNAL(triggered()),this,SLOT(openProject()));
    connect(this->ui->actionNew_project,SIGNAL(triggered()),this,SLOT(newProject()));
    connect(this->ui->configPushButton,SIGNAL(pressed()),this,SLOT(openConfig()));
    connect(this->ui->actionProject_settings,SIGNAL(triggered()),this,SLOT(openConfig()));

    connect(this->ui->reloadPushButton,SIGNAL(pressed()),this,SLOT(reload()));
    connect(this->ui->action_Reload,SIGNAL(triggered()),this,SLOT(reload()));

    connect(this->ui->stop_pushButton,SIGNAL(pressed()),this,SLOT(askToStopComputation()));
    connect(this->ui->action_Interrupt,SIGNAL(triggered()),this,SLOT(askToStopComputation()));
    connect(this->ui->startComputPushButton,SIGNAL(pressed()),this,SLOT(startComputation()));
    connect(this->ui->action_Start,SIGNAL(triggered()),this,SLOT(startComputation()));
    connect(this->ui->viewLogPushButton,SIGNAL(pressed()),this,SLOT(showJson()));
    connect(this->ui->action_View_Log,SIGNAL(triggered()),this,SLOT(showJson()));

    connect(this->ui->actionAbout,SIGNAL(triggered()),this,SLOT(showAbout()));
    connect(this->ui->actionContents,SIGNAL(triggered()),this,SLOT(showHelp()));
    connect(this->ui->actionPreferences,SIGNAL(triggered()),this,SLOT(changePreferences()));

    //tools
    connect(this->ui->actionSight_Matrix_to_Obs,SIGNAL(triggered()),this,SLOT(sightMatrixToObs()));
    connect(this->ui->action_Infinity_ASC_to_BAS,SIGNAL(triggered()),this,SLOT(infinityAscToBas()));
    connect(this->ui->actionExport_Coordinates,SIGNAL(triggered()),this,SLOT(exportCoordinates()));
    connect(this->ui->actionExport_Sight_Matrix,SIGNAL(triggered()),this,SLOT(exportSightMatrix()));
    connect(this->ui->actionExport_VarCov,SIGNAL(triggered()),this,SLOT(exportVarCov()));
    connect(this->ui->actionExport_to_sinex,SIGNAL(triggered()),this,SLOT(exportSinex()));
    connect(this->ui->actionExport_relative_precisions,SIGNAL(triggered()),this,SLOT(exportRelPrec()));
    connect(this->ui->actionExport_parameters_variations,SIGNAL(triggered()),this,SLOT(exportParamVari()));
    connect(this->ui->actionApply_Transfo_to_File,SIGNAL(triggered()),this,SLOT(applyTransfo()));
    connect(this->ui->actionExport_to_Cor,SIGNAL(triggered()),this,SLOT(export2cor()));
    connect(this->ui->action_Cartesian_Proj,SIGNAL(triggered()),this,SLOT(convertCoordCartProj()));
    connect(this->ui->action_Project_File_Template,SIGNAL(triggered()),this,SLOT(projectTemplate()));
}

MainWindow::~MainWindow()
{
    delete ui;
    if (m_calcThread) delete m_calcThread;
    if (project) delete project;
}

void MainWindow::setProject(Project *aProject)
{
    project = aProject;
    ui->mainTables->setProject(aProject);
}

void MainWindow::setAfterCompensation(bool after)
{
    afterCompensation = after;
    ui->mainTables->setAfterCompensation(after);
}

void MainWindow::closeProject()
{
    delete project;
    setProject(nullptr);
    setAfterCompensation(false);
    readyToCompute = false;

    ui->mainTables->updateContent(true);

    ui->configPushButton->setEnabled(false);
    ui->reloadPushButton->setEnabled(false);
    ui->startComputPushButton->setEnabled(false);
    ui->viewLogPushButton->setEnabled(false);
    ui->stop_pushButton->setEnabled(false);

    ui->action_Start->setEnabled(false);
    ui->actionProject_settings->setEnabled(false);
    ui->action_Reload->setEnabled(false);
    ui->action_View_Log->setEnabled(false);
    ui->action_Interrupt->setEnabled(false);
}


void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("About Comp3D"), QString(COMP3D_VERSION)+"\nCommit: "+ GIT_VERSION +
                       "\nOptions: "+ COMP3D_OPTIONS.c_str()+"\n"+tr("Built ")+__DATE__+" "+__TIME__+
                       "\n\n"+COMP3D_COPYRIGHT+"\n"+COMP3D_CONTACT+" -- "+COMP3D_REPO+"\n"+COMP3D_LICENSE);
}

bool MainWindow::newProject()
{
    /*if (project)
    {
        int ret = QMessageBox::warning(this, tr("New project"),
                                       tr("Are you sure you want to close current project and loose unsaved data?"),
                                       QMessageBox::Yes|QMessageBox::No);
        if (ret==QMessageBox::No)
            return false;
    }*/

    QDir currentDirectory(QSettings().value("/Comp3D/LastProjectPath").toString());
    currentDirectory.cdUp();
    QString tmp_projectFilename=currentDirectory.filePath(tr("new"))+".comp";
    //std::cout<<"tmp_projectFilename: *"<<tmp_projectFilename.toCstr()<<"*"<<std::endl;
    tmp_projectFilename=QFileDialog::getSaveFileName(this, tr("New Comp3D project file"),
                                             tmp_projectFilename,
                                             tr("Comp3D (*.comp)"));
    if (tmp_projectFilename.isNull())
        return false;

    closeProject();

    setProject(new Project(tmp_projectFilename.toStdString()));

    return openConfig();
}


bool MainWindow::openProject(std::string filename)
{
    /*if (project)
    {
        int ret = QMessageBox::warning(this, tr("Open project"),
                                       tr("Are you sure you want to close current project and loose unsaved data?"),
                                       QMessageBox::Yes|QMessageBox::No);
        if (ret==QMessageBox::No)
            return false;
    }*/

    QString tmp_projectFilename;
    if (filename=="")
        tmp_projectFilename=QFileDialog::getOpenFileName(this, tr("Open Comp3D project file"),
                                                 QSettings().value("/Comp3D/LastProjectPath").toString(),
                                                 tr("Comp3D (*.comp)"));
    else
        tmp_projectFilename=filename.c_str();

    if (tmp_projectFilename.isNull())
        return false;

    closeProject();
    delete project;
    setProject(new Project(tmp_projectFilename.toStdString()));
    ui->configPushButton->setEnabled(true);
    ui->reloadPushButton->setEnabled(true);
    ui->actionProject_settings->setEnabled(true);
    ui->action_Reload->setEnabled(true);

    if (! project->read_config())
    {
        ui->statusbar->showMessage(tr("Impossible to read file: %1").arg(project->filename.c_str()));
        ErrorDialog dlg("",&(project->mInfo),"");
        dlg.show();
        dlg.exec();
        closeProject();
        return false;
    }
    if (!Project::theInfo()->isEmpty())
    {
        ui->statusbar->showMessage(tr("Warnings when reading file: %1").arg(project->filename.c_str()));
        ErrorDialog dlg("",Project::theInfo(),"");
        dlg.show();
        dlg.exec();
        //show config only if warnings are not about proj. If problem with proj, let the user change config OR preferences
        if (project->projection.type == PROJ_TYPE::PJ_UNINIT)
            return false;
        //return openConfig();
    }

    //ready to enter project configuration

    //if file open ok, save config
    QSettings().setValue("/Comp3D/LastProjectPath",project->filename.c_str());
    readyToCompute=false;
    ui->startComputPushButton->setEnabled(readyToCompute);
    ui->startComputPushButton->setText(startButtonText[project->config.compute_type]);
    ui->viewLogPushButton->setEnabled(true);
    ui->stop_pushButton->setEnabled(false);

    ui->action_Start->setEnabled(readyToCompute);
    ui->action_Start->setText(startButtonText[project->config.compute_type]);
    ui->actionProject_settings->setEnabled(true);
    ui->action_Reload->setEnabled(true);
    ui->action_View_Log->setEnabled(true);
    ui->action_Interrupt->setEnabled(false);

    connect(&(this->project->lsquares),SIGNAL(enterKernel()),this,SLOT(enterKernel()));
    connect(&(this->project->lsquares),SIGNAL(enterInvert()),this,SLOT(enterInvert()));
    connect(&(this->project->lsquares),SIGNAL(iterationDone(int,double)),this,SLOT(updateSigma0(int,double)));

    setWindowTitle(tr("Comp3D: %1").arg(tmp_projectFilename));

    return readData(true);//TODO: make function "setConfig"
}

bool MainWindow::reload()
{
    int ret = QMessageBox::warning(this, tr("Reload"),
                                   tr("Are you sure you want to close current project and loose unsaved data?"),
                                   QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
    if (ret==QMessageBox::No)
        return false;

    return openProject(project->filename);
    //return readData();
}

bool MainWindow::readData(bool firstRead)
{
    ui->configPushButton->setEnabled(true);
    ui->reloadPushButton->setEnabled(true);
    ui->startComputPushButton->setEnabled(false);
    ui->viewLogPushButton->setEnabled(false);
    ui->stop_pushButton->setEnabled(false);

    ui->actionProject_settings->setEnabled(true);
    ui->action_Reload->setEnabled(true);
    ui->action_Start->setEnabled(false);
    ui->action_View_Log->setEnabled(false);
    ui->action_Interrupt->setEnabled(false);

    bool readOk=false;
    Project::theInfo()->clear();
    if (!project) return false;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    readOk=project->readData();
    QApplication::restoreOverrideCursor();

    ErrorDialog dlg("",Project::theInfo(),"");
    dlg.show();
    dlg.exec();
    QApplication::setOverrideCursor(Qt::WaitCursor);

    project->mInfo.clear();

    if (project->dataRead)
        readOk=project->set_least_squares(false);

    QString message1="";
    QString message2=tr("Found %1 observations and %2 points.").arg(Project::theone()->totalNumberOfObs()).arg(project->points.size());
    QString message3="";
    if (project->config.compute_type==COMPUTE_TYPE::type_compensation)
        message3=tr(" Initial sigma0: %3.").arg(project->lsquares.sigma_0_init);
    if (!readOk)
    {
        message1=tr("Errors reading data. ");
        endOfComputation();
    }else if (project->hasWarning){
        message1=tr("Warnings reading data. ");
    }else{
        message1=tr("Data reading OK. ");
    }
    ui->statusbar->showMessage(message1+message2+message3);

    setAfterCompensation(false);
    ui->mainTables->updateContent(firstRead);

    readyToCompute=project->dataRead && readOk;

    ui->startComputPushButton->setEnabled(readyToCompute && (!project->points.empty()));
    ui->startComputPushButton->setText(startButtonText[project->config.compute_type]);
    //ui->convergenceDial->setValue(ui->convergenceDial->minimum());
    ui->configPushButton->setEnabled(true);
    ui->reloadPushButton->setEnabled(true);
    ui->viewLogPushButton->setEnabled(true);
    ui->stop_pushButton->setEnabled(false);

    ui->action_Start->setEnabled(readyToCompute);
    ui->action_Start->setText(startButtonText[project->config.compute_type]);
    ui->actionProject_settings->setEnabled(true);
    ui->action_Reload->setEnabled(true);
    ui->action_View_Log->setEnabled(true);
    ui->action_Interrupt->setEnabled(false);

    QApplication::restoreOverrideCursor();
    if (project->hasWarning&&(!Project::theInfo()->isEmpty()))
    {
        ErrorDialog dlg(tr("Setting messages: ").toStdString()+"<br/>",Project::theInfo(),"");
        dlg.show();
        dlg.exec();
        project->mInfo.clear();
    }
    return readyToCompute;
}

void MainWindow::askToStopComputation()
{
    if (project && readyToCompute)
    {
        std::cout<<"Asking to stop..."<<std::endl;
        project->lsquares.manual_interrupt=true;
        ui->stop_pushButton->setEnabled(false);
        ui->stop_pushButton->setText(tr("Will stop at next iteration..."));
    }
}

bool MainWindow::startComputation()
{
    if (!checkProject()) return false;
    if (!readyToCompute)
    {
        QMessageBox::warning(this, tr("Error"),tr("Project is not ready for computation."));
        return false;
    }

    Project::theInfo()->clear();
    project->lsquares.nbr_iterations=0;//TODO: clean it, mandatory for now because set_least_squares is used for every Monte Carlo iteration
    bool ok=project->set_least_squares(project->config.internalConstr);//remove it to "continue computation"

    if (!ok)
    {
        endOfComputation();
        return false;
    }

    Project::theInfo()->clear();

    if (m_calcThread!=nullptr)
    {
        m_calcThread->quit();
        while (!m_calcThread->isFinished() && m_calcThread->isRunning())
        {
            std::cout<<"Calc thread not finished, waiting..."<<std::endl;
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        std::cout<<"Calc thread finished, deleting..."<<std::endl;
        delete m_calcThread;
        m_calcThread=nullptr;
    }
    ui->statusbar->showMessage(tr("Computation started..."));

    m_calcThread=new CalcThread(project,this);
    connect(this->m_calcThread,SIGNAL(finished()),this,SLOT(endOfComputation()));

    ui->configPushButton->setEnabled(false);
    ui->reloadPushButton->setEnabled(false);
    ui->startComputPushButton->setEnabled(false);
    ui->viewLogPushButton->setEnabled(false);
    ui->stop_pushButton->setEnabled(true);
    ui->menubar->setEnabled(false);

    ui->action_Start->setEnabled(false);
    ui->actionProject_settings->setEnabled(false);
    ui->action_Reload->setEnabled(false);
    ui->action_View_Log->setEnabled(false);
    ui->action_Interrupt->setEnabled(true);

    ui->mainTables->disableInteraction(); // Will be renabled in endOfComputation with updateContents()

    m_calcThread->start();

    return true;
}


void MainWindow::changePreferences()
{
    DialogPreferences().exec();
}

void MainWindow::enterKernel()
{
    ui->statusbar->showMessage(tr("Suspected rank error in normal matrix.")+" "+tr("Begin kernel computation..."));
}

void MainWindow::enterInvert()
{
    ui->statusbar->showMessage(ui->statusbar->currentMessage()+" "+tr("Begin normal matrix inversion..."));
}

void MainWindow::updateSigma0(int number, double convergence)
{
    //std::cout<<"updateSigma0"<<std::endl;
    //ui->convergenceDial->setValue(-log(1+fabs(convergence))*1000*sign(convergence));
    //ui->convergenceProgressBar->setValue(-log(1+fabs(convergence))*1000*sign((double)convergence));
    ui->statusbar->showMessage(tr("Iteration number %1 finished. Sigma0=%2, convergence=%3.")
        .arg(number).arg((double)project->lsquares.all_sigma0.back()).arg((double)convergence));
}

void MainWindow::endOfComputation()
{
    std::cout<<"endOfComputation"<<std::endl;
    if (!Project::theInfo()->isEmpty())
    {
        ErrorDialog dlg(tr("Computation messages: ").toStdString()+"<br/>",Project::theInfo(),"");
        dlg.show();
        dlg.exec();
        project->mInfo.clear();
    }
    if (((project->config.compute_type==COMPUTE_TYPE::type_compensation)&&(!std::isfinite(project->lsquares.finalSigma0())))
            || project->lsquares.m_interrupted || project->lsquares.calculusError())
    {
        ui->statusbar->showMessage(tr("Unexpected end of computation"));
    }else{
        if (project->config.compute_type==COMPUTE_TYPE::type_compensation)
            ui->statusbar->showMessage(tr("End of Computation after %1 iterations, final sigma0=%2. Duration: %3")
                .arg(project->lsquares.nbr_iterations).arg(project->lsquares.finalSigma0())
                .arg(to_simple_string(project->lsquares.computation_end-project->lsquares.computation_start).c_str()));
        else
            ui->statusbar->showMessage(tr("End of Computation after %1 iterations. Duration: %2")
                .arg(project->lsquares.nbr_iterations)
                .arg(to_simple_string(project->lsquares.computation_end-project->lsquares.computation_start).c_str()));
    }
    ui->configPushButton->setEnabled(true);
    ui->reloadPushButton->setEnabled(true);
    ui->startComputPushButton->setEnabled(true);
    ui->viewLogPushButton->setEnabled(true);
    ui->stop_pushButton->setEnabled(false);
    ui->stop_pushButton->setText(tr("Interrupt"));
    ui->menubar->setEnabled(true);

    ui->action_Start->setEnabled(true);
    ui->action_Reload->setEnabled(true);
    ui->actionProject_settings->setEnabled(true);
    ui->action_View_Log->setEnabled(true);
    ui->action_Interrupt->setEnabled(false);
    ui->action_Interrupt->setText(tr("Interrupt"));

    setAfterCompensation(((project->config.compute_type==COMPUTE_TYPE::type_compensation)&&(project->compensationDone)));

    std::cout<<"Updating interface... "<<std::endl;
    ui->mainTables->updateContent();

    Project::prepareJson(project->filename,project->config.name);
}


bool MainWindow::openConfig()
{
    if (!checkProject()) return false;

    ConfigDialog confDlg(project);
    if (confDlg.exec())
    {
        project->clear();
        confDlg.updateProjectConfig();
        project->saveasJSON();

        Project::theInfo()->clear();
        bool res = openProject(project->config.config_file);
        project->saveasJSON();
        return res;
    }
    return false;
}


void MainWindow::showJson()
{
    if (!checkProject()) return;

    if (!afterCompensation) //if a compensation has been done, json is already saved
    {
        project->saveasJSON();
        Project::prepareJson(project->filename,project->config.name);
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(QString("%1.html").arg(project->filename.c_str())));
}

void MainWindow::resizeEvent(QResizeEvent *)
{
    ui->mainTables->updateGraphics();
}


//------------------------- Tools ---------------------------

bool MainWindow::sightMatrixToObs()
{
    SightMatrixDialog sightMatrixDlg(this);
    if (project)
        sightMatrixDlg.setProj(project->projection);
    sightMatrixDlg.show();
    sightMatrixDlg.exec();
    return false;
}

bool MainWindow::infinityAscToBas()
{
    InfinityAscToBasDialog infinityAscToBasDlg(this);
    infinityAscToBasDlg.show();
    infinityAscToBasDlg.exec();
    return false;
}

bool MainWindow::checkProject()
{
    if (!project)
    {
        QMessageBox::warning(this, tr("Error"),tr("You need to load a project first."));
        return false;
    }
    return true;
}

bool MainWindow::checkInverse()
{
    if (!checkProject()) return false;
    if (!project->invertedMatrix)
    {
        QMessageBox::warning(this, tr("Error"),tr("You need to invert normal matrix first."));
        return false;
    }
    return true;
}

bool MainWindow::exportCoordinates()
{
    if (!checkProject()) return false;
    ExportCoordDialog coordDlg(project, this);
    if (coordDlg.exec())
    {
        coordDlg.doExport();
    }
    return true;
}

bool MainWindow::exportVarCov()
{
    if (!checkInverse()) return false;

    ExportPointsDialog ptsDlg(&project->config, project->points, ExportPointsDialogType::varcov);
    if (ptsDlg.exec())
    {
        std::vector <Point*> selectedPoints=ptsDlg.getSelectedPoints();
        if (selectedPoints.size()==0)
            return false;
        ptsDlg.saveMetaData();
        project->saveasJSON();//save metadata
        QString filename=QFileDialog::getSaveFileName(this, tr("Export Variance-Covariance Matrix File"),
                                                      (project->config.config_file+"_var_covar.csv").c_str(),
                                                     tr("CSV (*.csv)"));
        if (!filename.isNull())
            project->exportVarCov(filename.toCstr(),selectedPoints,ptsDlg.getOptions()[0],ptsDlg.getOptions()[1]);
        return true;
    }
    return false;
}

bool MainWindow::exportSinex()
{
    if (!checkInverse()) return false;

    ExportPointsDialog ptsDlg(&project->config, project->points, ExportPointsDialogType::sinex);
    if (ptsDlg.exec())
    {
        std::vector <Point*> selectedPoints=ptsDlg.getSelectedPoints();
        if (selectedPoints.size()==0)
            return false;
        ptsDlg.saveMetaData();
        project->saveasJSON();//save metadata
        SinexDialog sinexDlg(&project->config);
        if (sinexDlg.exec())
        {
            sinexDlg.saveMetaData();
            project->saveasJSON();//save metadata

            QString filename=QFileDialog::getSaveFileName(this, tr("Export Sinex File"),
                                                          (project->filename+".SNX").c_str(),
                                                         tr("Sinex (*.SNX)"));
            if (!filename.isNull())
                project->exportSINEX(filename.toCstr(),selectedPoints);
            return true;
        }
    }
    return false;
}

bool MainWindow::exportRelPrec()
{
    if (!checkInverse()) return false;

    ExportPointsDialog ptsDlg(&project->config, project->points, ExportPointsDialogType::simple);
    if (ptsDlg.exec())
    {
        std::vector <Point*> selectedPoints=ptsDlg.getSelectedPoints();
        if (selectedPoints.size()==0)
            return false;

        ptsDlg.saveMetaData();
        project->saveasJSON();//save metadata

        QString filename=QFileDialog::getSaveFileName(this, tr("Export relative precision file"),
                                                      (project->filename+"_relprec.txt").c_str(),
                                                     tr("Text (*.txt)"));
        if (!filename.isNull())
            project->exportRelPrec(filename.toCstr(),selectedPoints);
        return true;
    }else{
        return false;
    }
}


bool MainWindow::exportParamVari()
{
    if (!checkProject()) return false;

    QString csv_filename;
    csv_filename=QFileDialog::getSaveFileName(this,tr("Parameters variations file name"),
                       (project->config.config_file+"_param_var.csv").c_str(),
                       tr("CSV (*.csv);;All (*.*)"));
    if (!csv_filename.isNull())
    {
        uni_stream::ofstream variFile(csv_filename.toStdString());
        variFile.precision(12);
        for (auto & param :project->lsquares.all_parameters)
            variFile<<param->name<<"\t";
        variFile<<"\n";

        std::vector <tdouble> all_param_val=project->lsquares.all_param_init_val;

        for (auto & val: all_param_val)
            variFile<<val<<"\t";
        variFile<<"\n";

        for (unsigned int i=0;i<project->lsquares.all_dX.size();i++)
        {
            for (unsigned int j=0;j<project->lsquares.all_parameters.size();j++)
            {
                all_param_val[j]-=project->lsquares.all_dX[i][j];
                variFile<<all_param_val[j]<<"\t";
            }
            variFile<<"\n";
        }
        variFile.close();
    }
    return false;
}

bool MainWindow::exportSightMatrix()
{
    if (!checkProject()) return false;

    QString filename=QFileDialog::getSaveFileName(this, tr("Export Sight Matrix"),
                                                  (project->filename+"_sight.csv").c_str(),
                                                 tr("CSV (*.csv)"));
    if (!filename.isNull())
    {
        if (!project->exportSightMatrix(filename.toCstr()))
        {
            QMessageBox::warning(this, tr("Error"),
                                           tr("Impossible to write file: ")+filename,
                                           QMessageBox::Ok);
            return false;
        }
        return true;
    }
    return true;
}


//apply the transfo found in a xyz file to a cor file
bool MainWindow::applyTransfo()
{
    ApplyTransfoDialog applyTrDlg(this);
    applyTrDlg.exec();

    return true;
}

//export capt or compensated coordinates into cor file
bool MainWindow::export2cor()
{
    if (!checkProject()) return false;

    if (!afterCompensation)
    {
        int ret = QMessageBox::warning(this, tr("Not compensated"),
                                       tr("There is no compensation. Would you like to export initial coordinates?"),
                                       QMessageBox::Yes|QMessageBox::No);
        if (ret==QMessageBox::No)
            return false;
    }

    QString cor_filename;
    cor_filename=QFileDialog::getSaveFileName(this,tr("Output COR file name"),
                       (project->config.get_root_COR_absolute_filename()+"_export.cor").c_str(),
                       tr("Coord (*.cor);;All (*.*)"));
    if (!cor_filename.isNull())
    {
        bool withConstrainedPts = true;

        if (afterCompensation)
            withConstrainedPts =QMessageBox::question(this, tr("Export to COR"),
                                       tr("Do you want the constrained points to be included in the COR file?")+"\n"
                                       +tr("Constrained coordinates will be kept to read ones."),
                                       QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)==QMessageBox::Yes;

        uni_stream::ofstream file_cor;
        file_cor.open(cor_filename.toStdString().c_str());
        std::cout<<"Write file "<<cor_filename.toCstr()<<std::endl;
        project->get_cor_root().writeCORcompensated(&file_cor, !afterCompensation, withConstrainedPts);
        file_cor.close();

        QString endMessage=tr("Done.");
        //if (afterCompensation && withConstrainedPts)
        //    endMessage+="\n"+tr("Make sure to check constrained points coordinates!");
        QMessageBox::warning(this, tr("Export to COR"),
                                       endMessage,
                                       QMessageBox::Ok);
        return true;
    }
    return false;
}

bool MainWindow::convertCoordCartProj()
{
    ConversionDialog conversionDlg(this);
    if (project)
        conversionDlg.setProj(project->projection);
    conversionDlg.exec();

    return true;
}

void MainWindow::projectTemplate()
{
    QString filename;
    filename=QFileDialog::getSaveFileName(this,tr("Output Comp3D file name"),
                       QSettings().value("/Comp3D/LastProjectPath").toString(),
                       tr("Comp3D project (*.comp);;All (*.*)"));
    if (!filename.isNull())
    {
        Project::prepareJson(filename.toStdString(),"Auto Comp");
        std::string json_data_str=Project::createTemplate(filename.toStdString());
        uni_stream::ofstream fileout(filename.toStdString().c_str());
        //std::string outputConfig = writer.write( root );
        fileout << "data=\n";
        fileout << json_data_str<<std::endl;
        fileout.close();

        QMessageBox::information(this, tr("Project template"),
                                       tr("Project template saved in ")+filename,
                                       QMessageBox::Ok);
    }
}

// Directory of user doc.
// Must be defined at compile time
// If COMP2D_DOC_DIR is not an absolute path, it will be prepended
//    with the executable dir path
#ifndef COMP3D_DOC_DIR
#  error Macro "COMP3D_DOC_DIR" must be defined at compile time
#  define COMP3D_DOC_DIR ""
#endif

// Index file of doc, relative to COMP3D_DOC_DIR
// Can be overloaded at compile time. "%1" will be replaced by the locale ("en", "fr", ...)
#ifndef COMP3D_DOC_INDEX
#  define COMP3D_DOC_INDEX    "%1/html/index.html"
#endif


void MainWindow::showHelp()
{

    QString doc_dir = COMP3D_DOC_DIR;

    if ( QFileInfo(doc_dir).isRelative())
        doc_dir = qApp->applicationDirPath() + "/" + doc_dir;
    if (std::getenv("APPDIR"))                  // Env var APPDIR to work with AppImage
        doc_dir = std::getenv("APPDIR") + doc_dir;
    QString doc_path_tpl = doc_dir + "/" + COMP3D_DOC_INDEX;

    QString lang = QSettings().value("/Comp3D/Language","en").toString();
    QString doc_path_locale = doc_path_tpl.arg(lang);
    if (!QFile(doc_path_locale).exists())
    {
        doc_path_locale = doc_path_tpl.arg("en");
        if (!QFile(doc_path_locale).exists())
        {
            QMessageBox::critical(this, tr("No documentation"),
                                  tr("Internal error: documentation not found!<p><p>"
                                     "Documentation should have been installed in '%1'.").arg(doc_dir),
                                  QMessageBox::Ok);
            return;
        }
    }
    QString doc_full_path = QFileInfo(doc_path_locale).canonicalFilePath();
    ui->statusbar->showMessage(tr("Opening %1").arg(doc_full_path));
    QDesktopServices::openUrl(QUrl::fromLocalFile(doc_full_path));
}
