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

#include "framewidget.h"
#include "ui_framewidget.h"

FrameWidget::FrameWidget(QWidget *parent) :
    QGroupBox(parent),
    ui(new Ui::FrameWidget)
{
    ui->setupUi(this);
    connect(this->ui->radioButton_local,SIGNAL(toggled(bool)),this,SLOT(updateEnabled()));
    connect(this->ui->radioButton_proj,SIGNAL(toggled(bool)),this,SLOT(updateEnabled()));
    connect(this->ui->comboBoxProj,SIGNAL(currentIndexChanged(int)),this,SLOT(CRSchanged()));
    connect(this->ui->CRSLineEdit,SIGNAL(textEdited(QString)),this,SLOT(defChanged()));

    ui->latitudeDoubleSpinBox->setValue(45);
    ui->localManualXSpinBox->setValue(0);
    ui->localManualYSpinBox->setValue(0);

    for (unsigned int i=0;i<Projection::allCRS().size();i++)
    {
        CRSproj proj=Projection::allCRS()[i];
        ui->comboBoxProj->addItem(QString("EPSG %1: %2").arg(proj.EPSGcode).arg(proj.name.c_str()));
    }
    updateEnabled();
}

FrameWidget::~FrameWidget()
{
    delete ui;
}

double FrameWidget::getLatitude() const
{
    return ui->latitudeDoubleSpinBox->value();
}

int FrameWidget::getLocalCenterX() const
{
    return ui->localManualXSpinBox->value();
}

int FrameWidget::getLocalCenterY() const
{
    return ui->localManualYSpinBox->value();
}

void FrameWidget::importProj(const Projection &_proj)
{
    ui->radioButton_proj->setChecked(_proj.type==PROJ_TYPE::PJ_GEO);
    ui->radioButton_local->setChecked(_proj.type!=PROJ_TYPE::PJ_GEO);
    ui->latitudeDoubleSpinBox->setValue(_proj.latitude);
    if (_proj.type==PROJ_TYPE::PJ_GEO)
    {
        ui->localManualXSpinBox->setValue(_proj.centerGeoref.x());
        ui->localManualYSpinBox->setValue(_proj.centerGeoref.y());
    }else{
        ui->localManualXSpinBox->setValue(_proj.centerStereo.x());
        ui->localManualYSpinBox->setValue(_proj.centerStereo.y());
    }

    int currentIndex=-1;
    for (unsigned int i=0;i<Projection::allCRS().size();i++)
    {
        CRSproj proj=Projection::allCRS()[i];
        if (proj.def==_proj.projDef)
            currentIndex=i;
    }
    if (currentIndex>=0)
    {
        ui->comboBoxProj->setCurrentIndex(currentIndex);
        ui->CRSLineEdit->setText(Projection::allCRS()[currentIndex].def.c_str());
    }else{
        ui->comboBoxProj->setCurrentIndex(0);
        ui->CRSLineEdit->setText(_proj.projDef.c_str());
    }
    updateEnabled();
}

bool FrameWidget::exportProj(Projection &_proj) const
{
    if (ui->radioButton_local->isChecked())
        return _proj.initLocal(getLatitude(),
                               Coord(getLocalCenterX(),
                                     getLocalCenterY(),0),
                               1.0);
    else
        return _proj.initGeo(Coord(getLocalCenterX(),
                                   getLocalCenterY(),0),
                             ui->CRSLineEdit->text().toStdString());
}

void FrameWidget::importConfig(const Project_Config &conf)
{
    ui->latitudeDoubleSpinBox->setValue(conf.centerLatitude);
    ui->radioButton_proj->setChecked(conf.useProj);
    ui->radioButton_local->setChecked(!conf.useProj);

    ui->localManualXSpinBox->setValue(conf.localCenter.x());
    ui->localManualYSpinBox->setValue(conf.localCenter.y());

    int currentIndex=-1;
    for (unsigned int i=0;i<Projection::allCRS().size();i++)
    {
        CRSproj proj=Projection::allCRS()[i];
        if (proj.def==conf.projDef)
            currentIndex=i;
    }
    if (currentIndex>=0)
    {
        ui->comboBoxProj->setCurrentIndex(currentIndex);
        ui->CRSLineEdit->setText(Projection::allCRS()[currentIndex].def.c_str());
    }else{
        ui->comboBoxProj->setCurrentIndex(0);
        ui->CRSLineEdit->setText(conf.projDef.c_str());
    }
    updateEnabled();
}

void FrameWidget::exportConfig(Project_Config &conf) const
{
    conf.centerLatitude=ui->latitudeDoubleSpinBox->value();
    conf.localCenter.setx(ui->localManualXSpinBox->value());
    conf.localCenter.sety(ui->localManualYSpinBox->value());
    conf.useProj=ui->radioButton_proj->isChecked();
    conf.projDef=ui->CRSLineEdit->text().trimmed().toStdString();
}

void FrameWidget::updateEnabled()
{
    if (ui->radioButton_local->isChecked())
    {
        ui->label_11->setText(QObject::tr("X"));
        ui->label_12->setText(QObject::tr("Y"));
    }else{
        ui->label_11->setText(QObject::tr("E"));
        ui->label_12->setText(QObject::tr("N"));
    }
    
    ui->label_17->setEnabled(ui->radioButton_local->isChecked());
    ui->latitudeDoubleSpinBox->setEnabled(ui->radioButton_local->isChecked());
    ui->CRSLineEdit->setEnabled(!ui->radioButton_local->isChecked());
    ui->label_18->setEnabled(!ui->radioButton_local->isChecked());
    ui->label->setEnabled(!ui->radioButton_local->isChecked());
    ui->comboBoxProj->setEnabled(!ui->radioButton_local->isChecked());
}

void FrameWidget::CRSchanged()
{
    if (ui->comboBoxProj->currentIndex()>0) //0 is user CRS
        ui->CRSLineEdit->setText(Projection::allCRS().at(ui->comboBoxProj->currentIndex()).def.c_str());
}

void FrameWidget::defChanged()
{
    ui->comboBoxProj->setCurrentIndex(0); //0 is user CRS
}
