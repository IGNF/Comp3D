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

#include "sightmatrixdialog.h"
#include "ui_sightmatrixdialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <iostream>
#include <sstream>
#include "project.h"
#include "errordialog.h"
#include "station_hz.h"

SightMatrixDialog::SightMatrixDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SightMatrixDialog)
{
    ui->setupUi(this);

    QDir currentDirectory(QSettings().value("/Comp3D/LastProjectPath").toString());
    currentDirectory.cdUp();
    workingDir=currentDirectory.absolutePath();

    connect(ui->pushButtonApply,SIGNAL(pressed()),this,SLOT(matrix2obs()));
    connect(ui->pushButtonHelp,SIGNAL(pressed()),this,SLOT(showHelp()));
    connect(ui->groupBoxComputeVal,SIGNAL(toggled(bool)),this,SLOT(updateApply()));
    connect(ui->pushButtonCorFile,SIGNAL(pressed()),this,SLOT(selectCorFile()));
    connect(ui->pushButtonMatFile,SIGNAL(pressed()),this,SLOT(selectMatFile()));

    ui->pushButtonApply->setEnabled(false);
    ui->groupBoxComputeVal->setChecked(false);

    adjustSize();
    setFixedSize(size());
}

SightMatrixDialog::~SightMatrixDialog()
{
    delete ui;
}

void SightMatrixDialog::setProj(Projection &_proj)
{
    ui->groupBoxFrame->importProj(_proj);
}

void SightMatrixDialog::updateApply()
{
    ui->pushButtonApply->setEnabled(
        (ui->lineEditMatrix->text().size()>0)
        &&((!ui->groupBoxComputeVal->isChecked())||(ui->lineEditCORFile->text().size()>0)));
}

void SightMatrixDialog::selectMatFile()
{
    QString filename=QFileDialog::getOpenFileName(this, tr("Select Matrix File"),
                                             workingDir,
                                             tr("Matrix (*.mat *.txt *.csv);;All (*.*)"));
    if (filename.isNull())
        return;

    //QDir dir(m_project->config.working_directory.c_str());
    //QString relative_filename=dir.relativeFilePath(filename);
    ui->lineEditMatrix->setText(filename);
    updateApply();

    QDir currentDirectory(filename);
    currentDirectory.cdUp();
    workingDir=currentDirectory.absolutePath();
}

void SightMatrixDialog::selectCorFile()
{
    QString filename=QFileDialog::getOpenFileName(this, tr("Select COR File"),
                                             workingDir,
                                             tr("Proj coord (*.cor);;All (*.*)"));
    if (filename.isNull())
        return;

    //QDir dir(m_project->config.working_directory.c_str());
    //QString relative_filename=dir.relativeFilePath(filename);
    ui->lineEditCORFile->setText(filename);
    updateApply();

    QDir currentDirectory(filename);
    currentDirectory.cdUp();
    workingDir=currentDirectory.absolutePath();
}

void SightMatrixDialog::matrix2obs()
{
    Project tmpProject;
    tmpProject.config.compute_type=COMPUTE_TYPE::type_compensation;
    tmpProject.config.nbDigits=ui->digitsSpinBox->value();
    tdouble angleUnitFactor=toRad(1.0,Project::theone()->config.filesUnit);
    if (ui->groupBoxComputeVal->isChecked())
    {
        bool ok;
        tmpProject.cor_root = CORFile::create(ui->lineEditCORFile->text().toCstr(),"");
        ok = ui->groupBoxFrame->exportProj(tmpProject.projection);
        ok = ok && tmpProject.cor_root->read();
        tmpProject.config.set_root_COR_abs_path(ui->lineEditCORFile->text().toCstr());

        if (!ok)
        {
            ErrorDialog dlg("",Project::theInfo(),"");
            dlg.show();
            dlg.exec();
            return;
        }
    }

    QString matFileName=ui->lineEditMatrix->text();
    uni_stream::ifstream matfile;
    matfile.open(matFileName.toCstr());
    if (!matfile)
    {
        QMessageBox::critical(this, tr("Error"),
                           tr("Can't open file ")+matFileName,
                           QMessageBox::Ok);
        return;
    }

    double angHSigmaAbs=ui->doubleSpinBoxAngleHAbs->value();
    double angHSigmaRel=ui->doubleSpinBoxAngleHRel->value();
    double angZSigmaAbs=ui->doubleSpinBoxAngleZAbs->value();
    double angZSigmaRel=ui->doubleSpinBoxAngleZRel->value();
    double distSigmaAbs=ui->doubleSpinBoxDistAbs->value();
    double distSigmaRel=ui->doubleSpinBoxDistRel->value();

    std::string line,tmp;
    std::vector <std::string> all_targets;
    std::vector <Point*> mat_points;
    //first line is targets
    std::getline(matfile, line);
    std::istringstream iss(line);
    iss>>tmp;//unused 1st column for first line
    while (iss>>tmp)
    {
        all_targets.push_back(tmp);
        if (ui->groupBoxComputeVal->isChecked())
        {
            Point* pt=Project::theone()->getPoint(tmp,false);
            if (!pt)
            {
                QMessageBox::critical(this, tr("Error"),
                                   tr("Point %1 not found in COR file.").arg(tmp.c_str()),
                                   QMessageBox::Ok);
                return;
            }
            mat_points.push_back(pt);
        }
    }

    if (ui->groupBoxComputeVal->isChecked() && mat_points.empty())
    {
        QMessageBox::critical(this, tr("Error"),
                           tr("No point found."),
                           QMessageBox::Ok);
        return;
    }

    QString obsFilename=QFileDialog::getSaveFileName(this, tr("New Obs File"),
                                             workingDir+"/obs.obs",
                                             tr("obs (*.obs)"));
    if (obsFilename.isNull())
        return;

    QDir currentDirectory(obsFilename);
    currentDirectory.cdUp();
    workingDir=currentDirectory.absolutePath();
    std::cout<<"Write obs to "<<obsFilename.toCstr()<<std::endl;


    uni_stream::ofstream obsfile;
    obsfile.open(obsFilename.toCstr());
    obsfile<<std::setiosflags(std::ios_base::fixed);
    int nbl=2;

    Station_Hz* fakeStation=nullptr;
    if (ui->groupBoxComputeVal->isChecked())
    {
        fakeStation=Station::create<Station_Hz>(mat_points[0]); //destroyed by project
        fakeStation->g0=0.0;
    }

    //other lines are stations
    while (std::getline(matfile, line))
    {
        std::cout<<line<<std::endl;
        std::istringstream iss2(line);
        std::string stationName;
        iss2>>stationName;
        if (stationName.empty()) continue;
        Point* station=0;
        if (ui->groupBoxComputeVal->isChecked())
        {
            station=Project::theone()->getPoint(stationName,false);
            if (!station)
            {
                QMessageBox::critical(this, tr("Error"),
                                   tr("Point %1 not found in COR file.").arg(stationName.c_str()),
                                   QMessageBox::Ok);
                return;
            }
        }
        bool atLeastOneObs=false;
        unsigned int i=0;
        std::string isSeenStr;
        while (iss2>>isSeenStr)
        {
            bool isSeen = (isSeenStr=="1");
            if (i>=all_targets.size())
            {
                QMessageBox::critical(this, tr("Error"),
                                   tr("Too many columns on line %1: ").arg(nbl)+line.c_str(),
                                   QMessageBox::Ok);
                return;
            }
            if (isSeen)
            {
                if (stationName==all_targets.at(i))
                {
                    i++;
                    if (QMessageBox::warning(this, tr("Error"),
                                       tr("Observation from %1 to itself is ignored.").arg(stationName.c_str()),
                                       QMessageBox::Ok|QMessageBox::Abort,QMessageBox::Ok)
                            == QMessageBox::Abort)
                        return;

                    continue;
                }

                if (ui->groupBoxComputeVal->isChecked())
                {
                    Obs obs5(station,mat_points.at(i),fakeStation,atLeastOneObs?OBS_CODE::HZ_ANG:OBS_CODE::HZ_REF,true,0,angHSigmaAbs,angHSigmaRel,0,0,0,angleUnitFactor,"",0,"");
                    Obs obs6(station,mat_points.at(i),fakeStation,OBS_CODE::ZEN,true,0,angZSigmaAbs,angZSigmaRel,0,0,0,angleUnitFactor,"",0,"");
                    Obs obs3(station,mat_points.at(i),fakeStation,OBS_CODE::DIST,true,0,distSigmaAbs,distSigmaRel,0,0,0,1,"",0,"");
                    bool obs5ok = obs5.computeInfos(false);
                    bool obs6ok = obs6.computeInfos(false);
                    bool obs3ok = obs3.computeInfos(false);
                    if (ui->checkBoxAddNoise->isChecked())
                    {
                        obs5.computedValue+=gauss_random(0,obs5.sigmaTotal);
                        obs6.computedValue+=gauss_random(0,obs6.sigmaTotal);
                        obs3.computedValue+=gauss_random(0,obs3.sigmaTotal);
                    }
                    if (obs5ok) obsfile<<obs5.toObsFile(true);
                    if (obs6ok) obsfile<<obs6.toObsFile(true);
                    if (obs3ok) obsfile<<obs3.toObsFile(true);

                    if (obs5ok) atLeastOneObs=true;
                }else {
                    obsfile<<(atLeastOneObs?" 5 ":" 7 ");
                    obsfile<<stationName<<" "<<all_targets[i]<<" 0 "<<std::setprecision(ui->doubleSpinBoxAngleHAbs->decimals())<<angHSigmaAbs
                          <<" "<<std::setprecision(ui->doubleSpinBoxAngleHRel->decimals())<<angHSigmaRel<<" 0 0"<<std::endl;
                    obsfile<<" 6 "<<stationName<<" "<<all_targets[i]<<" 0 "<<std::setprecision(ui->doubleSpinBoxAngleHAbs->decimals())<<angZSigmaAbs
                          <<" "<<std::setprecision(ui->doubleSpinBoxAngleHRel->decimals())<<angZSigmaRel<<" 0 0"<<std::endl;
                    obsfile<<" 3 "<<stationName<<" "<<all_targets[i]<<" 0 "<<std::setprecision(ui->doubleSpinBoxDistAbs->decimals())<<distSigmaAbs
                          <<" "<<std::setprecision(ui->doubleSpinBoxDistRel->decimals())<<distSigmaRel<<" 0 0"<<std::endl;
                    atLeastOneObs=true;
                }
                std::cout<<"From "<<stationName<<" to "<<all_targets[i]<<std::endl;
            }
            i++;
        }
        if ((i>0)&&(i<all_targets.size()))
        {
            QMessageBox::critical(this, tr("Error"),
                               tr("Not enough columns on line %1: ").arg(nbl)+line.c_str(),
                               QMessageBox::Ok);
            return;
        }
        if (atLeastOneObs)
            obsfile<<std::endl;
        nbl++;
    }

    obsfile.close();
    QMessageBox::information(this, tr("Info"),
                       tr("Observations written to ")+obsFilename,
                       QMessageBox::Ok);
}

void SightMatrixDialog::showHelp()
{
    QMessageBox::information(this, tr("Matrix File Example"),
                             "station\\target    A    B    P1   P2\n       A                0     1     1     1\n       B                1     0     1     1",
                       QMessageBox::Ok);
}
