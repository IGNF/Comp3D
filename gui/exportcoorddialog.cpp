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

#include "exportcoorddialog.h"
#include "ui_exportcoorddialog.h"

#include <QMessageBox>
#include "point.h"

ExportCoordDialog::ExportCoordDialog(Project *_project, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportCoordDialog),
    project(_project)
{
    ui->setupUi(this);
    if (project->projection.type!=PROJ_TYPE::PJ_GEO)
    {
        ui->groupBox_Latlong->setEnabled(false);
        ui->groupBox_Latlong->setChecked(false);
        ui->groupBox_Cart->setEnabled(true);
        ui->groupBox_Cart->setChecked(true);
        ui->checkBox_CartGlob->setChecked(true);
        ui->checkBox_CartGeocentric->setChecked(false);
        ui->checkBox_CartGeocentric->setEnabled(false);
    }
    if (!(project->compensationDone || project->MonteCarloDone || project->invertedMatrix))
    {
        ui->radioButton_AfterCoord->setChecked(false);
        ui->radioButton_AfterCoord->setEnabled(false);
        ui->radioButton_BeforeCoord->setChecked(true);
        ui->radioButton_BeforeCoord->setEnabled(true);
        ui->checkBox_WithPrecisions->setChecked(false);
        ui->checkBox_WithPrecisions->setEnabled(false);
    }
    if (!(project->invertedMatrix || project->MonteCarloDone))
        ui->checkBox_WithPrecisions->setChecked(false);
    update();
    connect(this->ui->radioButton_AfterCoord,SIGNAL(toggled(bool)),this,SLOT(update()));
}

ExportCoordDialog::~ExportCoordDialog()
{
    delete ui;
}

void ExportCoordDialog::update()
{
    ui->checkBox_WithPrecisions->setEnabled(
                ui->radioButton_AfterCoord->isChecked()
                && (project->invertedMatrix || project->MonteCarloDone)
    );
}

std::string ExportCoordDialog::exportOneType(Coord_type type_coord, bool isFinal)
{
    std::string status = isFinal ? "final" : "init";
    std::string coordNames[3];
    std::string filename;
    QString name_coord;
    switch(type_coord)
    {
    case geogr_degdec:
        name_coord = tr("Geographic degdec");
        filename = project->filename+"_geogr_degdec_"+status+".txt";
        coordNames[0]="lon";
        coordNames[1]="lat";
        coordNames[2]="up";
        break;
    case geogr_DMS:
        name_coord = tr("Geographic dms");
        filename = project->filename+"_geogr_dms_"+status+".txt";
        coordNames[0]="lon";
        coordNames[1]="lat";
        coordNames[2]="up";
        break;
    case cart_geo:
        name_coord = tr("Geocentric cartesian");
        filename = project->filename+"_cartgeo_"+status+".txt";
        coordNames[0]="X";
        coordNames[1]="Y";
        coordNames[2]="Z";
        break;
    case cart_glob:
        name_coord = tr("Global cartesian");
        filename = project->filename+"_cartglob_"+status+".txt";
        coordNames[0]="x";
        coordNames[1]="y";
        coordNames[2]="z";
        break;
    }

    tdouble sigma0 = isFinal ? project->lsquares.sigma_0 : NAN;
    if (project->MonteCarloDone)
        sigma0 = 1.;

    MatX R = project->projection.RotGlobal2Geocentric;
    MatX mat;

    bool with_prec = ui->checkBox_WithPrecisions->isEnabled() && ui->checkBox_WithPrecisions->isChecked();
    uni_stream::ofstream file_out;
    file_out.open(filename.c_str());
    if (!file_out.is_open())
    {
        QMessageBox::critical(this,tr("Export Coordinates"),
                              tr("Error: impossible to create file ")+filename.c_str());
        return "";
    }

    if (isFinal)
        name_coord += QString(tr(" after computation"));
    else
        name_coord += QString(tr(" before computation"));

    if (with_prec)
        name_coord += QString(tr(" with final precisions (factor applied %1)")).arg(sigma0);

    QString header = QString("* %1\n").arg(name_coord);
    header += QString(tr("* From project %1\n")).arg(project->config.config_file.c_str());
    if (project->compensationDone || project->MonteCarloDone || project->invertedMatrix)
        header +=  QString(tr("* project computed on %1 with Comp3D %2\n"))
                       .arg(to_simple_string(project->lsquares.computation_start).c_str()).arg(GIT_VERSION);
    header+="\n";
    file_out<<header.toStdString();
    std::string unit = (type_coord==geogr_degdec)?"°":"m";
    if (type_coord==geogr_DMS) unit = "°'\"";
    file_out<<"*name "<<coordNames[0]<<"("<<unit<<") "<<coordNames[1]<<"("<<unit<<") "<<coordNames[2]<<"(m)";
    if (with_prec)
        file_out<<" sigma_"<<coordNames[0]<<"("<<unit<<") sigma_"<<coordNames[1]<<"("<<unit<<") sigma_"<<coordNames[2]<<"(m)";
    file_out<<"\n";

    Coord spher, latlong, coord_exp, sigmas;
    for (auto &point:project->points)
    {
        spher = isFinal ? point.coord_comp : point.coord_init_spher;
        project->projection.sphericalToLatLong(spher,latlong);
        tdouble latitude = toRad(latlong[1],ANGLE_UNIT::DEG);
        tdouble w = sqrt(1-e2*sin(latitude)*sin(latitude));
        tdouble radius_meridian = semi_axis*(1-e2)/(w*w*w);
        tdouble radius_parallel = semi_axis*cos(latitude)/w;
        mat = point.ellipsoid.get_matrix();
        if (project->config.compute_type==COMPUTE_TYPE::type_monte_carlo)
        {
            mat(0,0) = sqr(point.MC_shift_sq_average.x());
            mat(1,1) = sqr(point.MC_shift_sq_average.y());
            mat(2,2) = sqr(point.MC_shift_sq_average.z());
        }

        switch(type_coord)
        {
        case geogr_degdec:
            project->projection.sphericalToLatLong(spher,coord_exp);
            if (with_prec)
            {
                sigmas.setx(sqrt(mat(0,0))*sigma0 / radius_parallel);
                sigmas.sety(sqrt(mat(1,1))*sigma0 / radius_meridian);
                sigmas.setz(sqrt(mat(2,2))*sigma0);
                sigmas.setx( fromRad(sigmas.x(), ANGLE_UNIT::DEG) );
                sigmas.sety( fromRad(sigmas.y(), ANGLE_UNIT::DEG) );
            }
            break;
        case geogr_DMS:
            project->projection.sphericalToLatLong(spher,coord_exp);
            if (with_prec)
            {
                sigmas.setx(sqrt(mat(0,0))*sigma0 / radius_parallel);
                sigmas.sety(sqrt(mat(1,1))*sigma0 / radius_meridian);
                sigmas.setz(sqrt(mat(2,2))*sigma0);
            }
            break;
        case cart_geo:
            project->projection.sphericalToCartGeocentric(spher,coord_exp);
            mat = point.ellipsoid.get_matrix();
            mat = R*mat*R.transpose();
            sigmas[0]=sqrt(mat(0,0))*sigma0;
            sigmas[1]=sqrt(mat(1,1))*sigma0;
            sigmas[2]=sqrt(mat(2,2))*sigma0;
            break;
        case cart_glob:
            project->projection.sphericalToGlobalCartesian(spher,coord_exp);
            if (with_prec)
            {
                sigmas.setx(sqrt(mat(0,0))*sigma0);
                sigmas.sety(sqrt(mat(1,1))*sigma0);
                sigmas.setz(sqrt(mat(2,2))*sigma0);
            }
            break;
        }
        
 
        switch(type_coord)
		{
        case geogr_DMS:
        {
            std::streamsize ss = std::cout.precision();
            file_out<<point.name
                    <<" "<<angToDMSString(coord_exp[0],ANGLE_UNIT::DEG,false,12)
                    <<" "<<angToDMSString(coord_exp[1],ANGLE_UNIT::DEG,false,12)
                    <<std::setprecision(15)<<std::fixed
                    <<" "<<coord_exp[2];
            if (with_prec)
                file_out<<" "<<angToDMSString(sigmas[0],ANGLE_UNIT::RAD,false,12)
                        <<" "<<angToDMSString(sigmas[1],ANGLE_UNIT::RAD,false,12)
                        <<" "<<sigmas[2];
            file_out<<std::setprecision(ss)<<"\n";
            break;
        }
        default:
        {
            file_out<<point.name<<std::setprecision(15)<<std::fixed;
            file_out<<" "<<coord_exp[0]<<" "<<coord_exp[1]<<" "<<coord_exp[2];
            if (with_prec)
                file_out<<" "<<sigmas[0]<<" "<<sigmas[1]<<" "<<sigmas[2];
            file_out<<"\n";
            break;
        }
		}
	}
    file_out.close();
    return filename;
}

void ExportCoordDialog::doExport()
{
    std::string allfilenames;
    std::string outname;

    if (ui->groupBox_Latlong->isChecked())
    {
        Coord_type coord_type = geogr_degdec;
        if (ui->radioButton_DMS->isChecked())
            coord_type = geogr_DMS;
        outname = exportOneType(coord_type, ui->radioButton_AfterCoord->isChecked());
        if (!outname.empty())
            allfilenames += "\n - "+outname;
    }

    if (ui->groupBox_Cart->isChecked() && ui->checkBox_CartGlob->isChecked())
    {
        Coord_type coord_type = cart_glob;
        outname = exportOneType(coord_type, ui->radioButton_AfterCoord->isChecked());
        if (!outname.empty())
            allfilenames += "\n - "+outname;
    }

    if (ui->groupBox_Cart->isChecked() && ui->checkBox_CartGeocentric->isChecked())
    {
        Coord_type coord_type = cart_geo;
        outname = exportOneType( coord_type, ui->radioButton_AfterCoord->isChecked());
        if (!outname.empty())
            allfilenames += "\n - "+outname;
    }

    if (allfilenames.empty())
        QMessageBox::warning(this,tr("Export Coordinates"),tr("Nothing to export!"));
    else
        QMessageBox::information(this,tr("Export Coordinates"),tr("Output files: ")+allfilenames.c_str());
}
