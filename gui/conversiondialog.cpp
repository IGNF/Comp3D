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

#include "conversiondialog.h"
#include "ui_conversiondialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>
#include <QtGlobal>
#include <vector>
#include "uni_stream.h"
#include <sstream>
#include <regex>

#include "point.h"
#include "project.h"

ConversionDialog::ConversionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConversionDialog)
{
    ui->setupUi(this);

    QDir currentDirectory(QSettings().value("/Comp3D/LastProjectPath").toString());
    currentDirectory.cdUp();
    directory=currentDirectory.absolutePath();

    ui->digitsSpinBox->setValue(4);
    connect(this->ui->toCartesianButton,SIGNAL(pressed()),this,SLOT(toCartesian()));
    connect(this->ui->toProjButton,SIGNAL(pressed()),this,SLOT(toProj()));
    connect(this->ui->pushButtonCOR,SIGNAL(pressed()),this,SLOT(selectCOR()));
    connect(this->ui->pushButtonXYZ,SIGNAL(pressed()),this,SLOT(selectXYZ()));

    adjustSize();
    setFixedSize(size());
}

ConversionDialog::~ConversionDialog()
{
    delete ui;
}

void ConversionDialog::setProj(Projection &_proj)
{
    ui->groupBoxFrame->importProj(_proj);
}

bool ConversionDialog::toCartesian()
{
    Project project;
    project.config.nbDigits=ui->digitsSpinBox->value();
    bool ok=true;
    ok = ui->groupBoxFrame->exportProj(project.projection);
    if (!ok)
    {
        QMessageBox::warning(this, tr("Conversion"),Project::theInfo()->toStrFormat().c_str());
        return false;
    }

    //new suffix file
    QString suffix = QInputDialog::getText(this, tr("Choose output files suffix"),
                                                             tr("Suffix:"), QLineEdit::Normal,
                                                             "_toCart.txt", &ok);
    if (!ok)
        return false;

    int line_num=1;
    int nb_pts=0;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QStringList allFilesNamesList=ui->CORFile->text().split(";",Qt::SkipEmptyParts);
#else
    QStringList allFilesNamesList=ui->CORFile->text().split(";",QString::SkipEmptyParts);
#endif
    for (auto &filename:allFilesNamesList)
    {
        //read input file
        uni_stream::ifstream file;
        file.open((directory+"/"+filename).toStdString().c_str());
        auto dataFile = CORFile::create(filename.toCstr());

        bool isCOR=
                ((filename.lastIndexOf(".cor")==filename.length()-4)
                ||(filename.lastIndexOf(".COR")==filename.length()-4));

        uni_stream::ofstream file_3d;
        file_3d.open((directory+"/"+filename+suffix).toStdString().c_str());

        if (file.is_open())
        {
            for (std::string line; std::getline(file, line); )
            {
                //test if line is empty or just a comment
                std::regex regex_line("^[[:space:]]*([^\\*]*)(.*)$");
                std::smatch what;
                if(std::regex_match(line, what, regex_line))
                {
                    // what[0] contains the whole string
                    // what[1] contains the data part
                    // what[2] contains the comment part
                    if ((what[1]).length()>1)//not just a comment
                    {
                        Point pt(dataFile.get(),line_num);
                        if (pt.read_point(what[1],!isCOR))
                        {
                            project.projection.georefToSpherical(pt.coord_read,pt.coord_init_spher);
                            Coord coord_compensated_cartesian;
                            project.projection.sphericalToGlobalCartesian(pt.coord_init_spher,coord_compensated_cartesian);
                            file_3d<<"  "<<pt.name<<" "<<coord_compensated_cartesian.toString()<<"\n";
                            nb_pts++;
                            if (nb_pts%100==0)
                            {
                               ui->labelNbPts->setText(QString(tr("Points processed: %1")).arg(nb_pts));
                                ui->labelNbPts->repaint();
                            }
                        }else{
                            ok=false;
                            break;
                        }
                    }
                }
                line_num++;
            }
            file.close();
            file_3d.close();
        }else{
            QMessageBox::warning(this, tr("Conversion"),tr("Error: impossible to open file %1").arg((directory+"/"+filename)));
            return false;
        }
    }
    ui->labelNbPts->setText(QString(tr("Points processed: %1")).arg(nb_pts));
    if (ok)
        QMessageBox::information(this, tr("Conversion"),tr("%1 points successfully converted.").arg(nb_pts));
    else
        QMessageBox::warning(this, tr("Conversion"),Project::theInfo()->toStrFormat().c_str());
    return ok;
}

bool ConversionDialog::toProj()
{
    Project project;
    project.config.nbDigits=ui->digitsSpinBox->value();
    bool ok=true;
    ok = ui->groupBoxFrame->exportProj(project.projection);
    if (!ok)
    {
        QMessageBox::warning(this, tr("Conversion"),Project::theInfo()->toStrFormat().c_str());
        return false;
    }

    //new suffix file
    QString suffix = QInputDialog::getText(this, tr("Choose output files suffix"),
                                                             tr("Suffix:"), QLineEdit::Normal,
                                                             "_toProj.txt", &ok);
    if (!ok)
        return false;

    int line_num=1;
    int nb_pts=0;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QStringList allFilesNamesList=ui->XYZFile->text().split(";",Qt::SkipEmptyParts);
#else
    QStringList allFilesNamesList=ui->XYZFile->text().split(";",QString::SkipEmptyParts);
#endif
    for (auto &filename:allFilesNamesList)
    {
        //read input file
        uni_stream::ifstream file;
        file.open((directory+"/"+filename).toStdString().c_str());
        auto dataFile = NewFile::create(filename.toCstr());

        uni_stream::ofstream file_proj;
        file_proj.open((directory+"/"+filename+suffix).toStdString().c_str());

        if (file.is_open())
        {
            for (std::string line; std::getline(file, line); )
            {
                //test if line is empty or just a comment
                std::regex regex_line("^[[:space:]]*([^\\*]*)(.*)$");
                std::smatch what;
                if(std::regex_match(line, what, regex_line))
                {
                    // what[0] contains the whole string
                    // what[1] contains the data part
                    // what[2] contains the comment part
                    if ((what[1]).length()>1)//not just a comment
                    {
                        Point pt(dataFile.get(),line_num);
                        if (pt.read_point(what[1],true))
                        {
                            project.projection.globalCartesianToSpherical(pt.coord_read,pt.coord_init_spher);
                            Coord coord_proj;
                            project.projection.sphericalToGeoref(pt.coord_init_spher,coord_proj);
                            file_proj<<"  "<<pt.name<<" "<<coord_proj.toString()<<"\n";
                            nb_pts++;
                            if (nb_pts%100==0)
                            {
                                ui->labelNbPts->setText(QString(tr("Points processed: %1")).arg(nb_pts));
                                ui->labelNbPts->repaint();
                            }
                        }else{
                            ok=false;
                            break;
                        }
                    }
                }
                line_num++;
            }
            file.close();
            file_proj.close();
        }else{
            QMessageBox::warning(this, tr("Conversion"),tr("Error: impossible to open file %1").arg((directory+"/"+filename)));
            return false;
        }
    }
    ui->labelNbPts->setText(QString(tr("Points processed: %1")).arg(nb_pts));
    if (ok)
        QMessageBox::information(this, tr("Conversion"),tr("%1 points successfully converted.").arg(nb_pts));
    else
        QMessageBox::warning(this, tr("Conversion"),Project::theInfo()->toStrFormat().c_str());
    return ok;

}

bool ConversionDialog::selectCOR()
{
    QStringList filenames=QFileDialog::getOpenFileNames(this, tr("Select proj coord files"),
                                             directory,
                                             tr("Proj coord (*.cor *.new *.txt);;All (*.*)"));
    if (filenames.isEmpty())
    {
        ui->CORFile->setText("");
        ui->toCartesianButton->setEnabled(false);
        return false;
    }

    QDir dir(directory);
    QString relative_filenames="";
    for (auto & filename:filenames)
    {
        QString relative_filename=dir.relativeFilePath(filename);
        relative_filenames+=relative_filename+";";
    }
    ui->CORFile->setText(relative_filenames);
    ui->toCartesianButton->setEnabled(true);
    return true;
}

bool ConversionDialog::selectXYZ()
{
    QStringList filenames=QFileDialog::getOpenFileNames(this, tr("Select cartesian coord files"),
                                             directory,
                                             tr("Cartesian (*.txt *.xyz *.XYZ);;All (*.*)"));
    if (filenames.isEmpty())
    {
        ui->XYZFile->setText("");
        ui->toProjButton->setEnabled(false);
        return false;
    }

    QDir dir(directory);
    QString relative_filenames="";
    for (auto & filename:filenames)
    {
        QString relative_filename=dir.relativeFilePath(filename);
        relative_filenames+=relative_filename+";";
    }
    ui->XYZFile->setText(relative_filenames);
    ui->toProjButton->setEnabled(true);
    return true;
}
