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

#include "applytransfodialog.h"
#include "ui_applytransfodialog.h"

#include <regex>
#include <boost/filesystem.hpp>
#include <boost/date_time.hpp>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

#include "station_bascule.h"
#include "errordialog.h"

ApplyTransfoDialog::ApplyTransfoDialog(QWidget *parent):
    QDialog(parent),
    ui(new Ui::ApplyTransfoDialog)

{
    ui->setupUi(this);

    QDir currentDirectory(QSettings().value("/Comp3D/LastProjectPath").toString());
    currentDirectory.cdUp();
    m_directory=currentDirectory.absolutePath();

    connect(this->ui->pushButtonApplyDirect,SIGNAL(pressed()),this,SLOT(applyTransfoDirect()));
    connect(this->ui->pushButtonApplyIndirect,SIGNAL(pressed()),this,SLOT(applyTransfoIndirect()));
    connect(this->ui->pushButtonInputFile,SIGNAL(pressed()),this,SLOT(selectCart()));
    connect(this->ui->pushButtonTransfoFile,SIGNAL(pressed()),this,SLOT(selectXYZ()));

    adjustSize();
    setFixedSize(size());
}

ApplyTransfoDialog::~ApplyTransfoDialog()
{
    delete ui;
}

void ApplyTransfoDialog::selectXYZ()
{
    QString filename=QFileDialog::getOpenFileName(this, tr("Select transformation XYZ file"),
                                             m_directory,
                                             tr("XYZ (*.xyz)"));
    if (filename.isNull())
        return;

    /*QDir dir(m_project->config.working_directory.c_str());
    QString relative_filename=dir.relativeFilePath(filename);
    ui->mainCOREdit->setText(relative_filename);*/
    ui->lineEdit_XYZ->setText(filename);
}

void ApplyTransfoDialog::selectCart()
{
    QString filename=QFileDialog::getOpenFileName(this, tr("Select cartesian file to transform"),
                                             m_directory,
                                             tr("Cartesian (*.txt *.xyz *.XYZ);;All (*.*)"));
    if (filename.isNull())
        return;

    /*QDir dir(m_project->config.working_directory.c_str());
    QString relative_filename=dir.relativeFilePath(filename);
    ui->mainOBSEdit->setText(relative_filename);*/
    ui->lineEdit_3D->setText(filename);
}

void ApplyTransfoDialog::applyTransfo(bool direct)
{
    bool ok;
    Project tmpProject;
    tmpProject.projection.initLocal(0,Coord(0,0,0),1.);

    auto newFile=NewFile::create(ui->lineEdit_3D->text().toCstr());
    ok=newFile->read();
    if (!ok)
    {
        ErrorDialog dlg("",Project::theInfo(),"");
        dlg.show();
        dlg.exec();
        return;
    }

  //  tmpProject.points=newFile.get_points();
    auto *station = Station::create<Station_Bascule>(nullptr);      // origin will be created by readXYZrotation()

    ok=station->readXYZrotation(ui->lineEdit_XYZ->text().toCstr());
    if (!ok)
    {
        ErrorDialog dlg("",Project::theInfo(),"");
        dlg.show();
        dlg.exec();
        return;
    }

    QString filename=QFileDialog::getSaveFileName(this, tr("Output file"),
                                             m_directory,
                                             tr("Any (*.*)"));
    if (filename.isNull())
        return;
    uni_stream::ofstream outFile;
    outFile.open(filename.toStdString().c_str());
    outFile<<COMP3D_VERSION<<"\n"
          <<to_simple_string(boost::posix_time::microsec_clock::local_time())<<"\n"
          <<"Conversion Ground to local from file \""
          <<ui->lineEdit_3D->text().toCstr()<<"\"\n to local frame of file \""
          <<ui->lineEdit_XYZ->text().toCstr()<<"\".\n"<<std::endl;
    for (auto & pt: newFile->get_points())
    {
        Coord coord_cartesian=pt->coord_read;

        outFile<<pt->name<<" ";
        if (direct)
            outFile<<station->ptCartGlobal2Instr(coord_cartesian,true).toString(ui->spinBoxNbDigits->value())<<" ";
        else
            outFile<<station->mes2GlobalCart(coord_cartesian,true).toString(ui->spinBoxNbDigits->value())<<" ";

        if (pt->sigmas_init.x()>0)
        {
            Coord sigmaLocal;
            if (direct)
                sigmaLocal.varCovar2sigma(station->sigmaGlobal2Instr(pt->sigmas_init.sigma2varCovar()));
            else
                sigmaLocal.varCovar2sigma(station->sigmaInstr2Global(pt->sigmas_init.sigma2varCovar()));
            outFile<<sigmaLocal.toString(ui->spinBoxNbDigits->value());
        }
        outFile<<std::endl;
    }
    outFile.close();
    QMessageBox::information(this, tr("Write OK"),tr("Points written to ")+filename);
}

void ApplyTransfoDialog::applyTransfoDirect()
{
    applyTransfo(true);
}

void ApplyTransfoDialog::applyTransfoIndirect()
{
    applyTransfo(false);
}


