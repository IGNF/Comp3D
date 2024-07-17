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

#include "configdialog.h"
#include "ui_configdialog.h"

#include <QDir>
#include <QFileDialog>

ConfigDialog::ConfigDialog(Project *project, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigDialog),m_project(project)
{
    ui->setupUi(this);

    ui->tabWidget->setCurrentIndex(0);
    ui->comboBoxLogLang->addItems(SUPPORTED_LANG_NAME);

    connect(this->ui->invertCheckBox,SIGNAL(stateChanged(int)),this,SLOT(updateInvertCheckBox()));
    connect(this->ui->compensationRadioButton,SIGNAL(toggled(bool)),this,SLOT(updateInvertCheckBox()));
    connect(this->ui->propagationRadioButton,SIGNAL(toggled(bool)),this,SLOT(updateInvertCheckBox()));
    connect(this->ui->monteCarloRadioButton,SIGNAL(toggled(bool)),this,SLOT(updateInvertCheckBox()));

    connect(this->ui->compensationRadioButton,SIGNAL(toggled(bool)),this,SLOT(updateIterationsLabel()));
    connect(this->ui->propagationRadioButton,SIGNAL(toggled(bool)),this,SLOT(updateIterationsLabel()));
    connect(this->ui->monteCarloRadioButton,SIGNAL(toggled(bool)),this,SLOT(updateIterationsLabel()));

    connect(this->ui->pushButtonCOR,SIGNAL(pressed()),this,SLOT(selectCOR()));
    connect(this->ui->pushButtonOBS,SIGNAL(pressed()),this,SLOT(selectOBS()));
    connect(this->ui->pushButtonCoordCov,SIGNAL(pressed()),this,SLOT(selectCoordCov()));

    connect(this->ui->mainCOREdit,SIGNAL(textChanged(QString)),this,SLOT(updateOkButton()));
    connect(this->ui->mainOBSEdit,SIGNAL(textChanged(QString)),this,SLOT(updateOkButton()));

    //initialize with project data
    ui->mainCOREdit->setText(m_project->config.get_root_COR_relative_filename().c_str());
    ui->mainOBSEdit->setText(m_project->config.get_root_OBS_relative_filename().c_str());
    ui->mainCoordCovEdit->setText(m_project->config.get_coord_cov_relative_filename().c_str());
    ui->projectDescEdit->setText(m_project->config.description.c_str());
    ui->projectNameEdit->setText(m_project->config.name.c_str());
    ui->compensationRadioButton->setChecked(m_project->config.compute_type==COMPUTE_TYPE::type_compensation);
    ui->propagationRadioButton->setChecked(m_project->config.compute_type==COMPUTE_TYPE::type_propagation);
    ui->monteCarloRadioButton->setChecked(m_project->config.compute_type==COMPUTE_TYPE::type_monte_carlo);
    ui->convergenceDoubleSpinBox->setValue(m_project->config.convergenceCriterion);
    ui->refractionDoubleSpinBox->setValue(m_project->config.refraction);
    ui->maxIterSpinBox->setValue(m_project->config.maxIterations);
    ui->checkBoxEllipsHeight->setChecked(m_project->config.useEllipsHeight);
    ui->checkBoxCleanOutputs->setChecked(m_project->config.cleanOutputs);
    ui->checkBoxDisplayMap->setChecked(m_project->config.displayMap);
    ui->forcedIterSpinBox->setValue(m_project->config.forceIterations);
    ui->digitsSpinBox->setValue(m_project->config.nbDigits);
    ui->comboBoxLogLang->setCurrentIndex(QStringList(SUPPORTED_LANG_CODE).lastIndexOf(m_project->config.lang.c_str()));
    ui->invertCheckBox->setChecked(m_project->config.invert);
    ui->internalConstrCheckBox->setChecked(m_project->config.internalConstr);

    ui->groupBoxFrame->importConfig(m_project->config);

    updateInvertCheckBox();
    updateIterationsLabel();

    updateOkButton();

    adjustSize();
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::updateProjectConfig()
{
    //initialize with dialog data
    QDir dir(m_project->config.working_directory.c_str());
    QString _root_COR_filename=dir.relativeFilePath(ui->mainCOREdit->text());
    m_project->config.set_root_COR_filename(_root_COR_filename.toCstr());
    QString _root_OBS_filename=dir.relativeFilePath(ui->mainOBSEdit->text());
    m_project->config.set_root_OBS_filename(_root_OBS_filename.toCstr());
    QString _coord_cov_filename=dir.relativeFilePath(ui->mainCoordCovEdit->text());
    m_project->config.set_coord_cov_filename(_coord_cov_filename.toCstr());
    m_project->config.description=ui->projectDescEdit->toPlainText().toCstr();
    m_project->config.name=ui->projectNameEdit->text().toCstr();
    if (ui->compensationRadioButton->isChecked()) m_project->config.compute_type=COMPUTE_TYPE::type_compensation;
    else if (ui->propagationRadioButton->isChecked()) m_project->config.compute_type=COMPUTE_TYPE::type_propagation;
    else m_project->config.compute_type=COMPUTE_TYPE::type_monte_carlo;
    m_project->config.convergenceCriterion=ui->convergenceDoubleSpinBox->value();
    m_project->config.refraction=ui->refractionDoubleSpinBox->value();
    m_project->config.maxIterations=ui->maxIterSpinBox->value();
    m_project->config.forceIterations=ui->forcedIterSpinBox->value();
    m_project->config.nbDigits=ui->digitsSpinBox->value();
    m_project->config.lang=QStringList(SUPPORTED_LANG_CODE)[ui->comboBoxLogLang->currentIndex()].toStdString();
    m_project->config.useEllipsHeight=ui->checkBoxEllipsHeight->isChecked();
    m_project->config.cleanOutputs=ui->checkBoxCleanOutputs->isChecked();
    m_project->config.displayMap=ui->checkBoxDisplayMap->isChecked();

    ui->groupBoxFrame->exportConfig(m_project->config);
    m_project->config.invert=ui->invertCheckBox->checkState()==Qt::CheckState::Checked;
    m_project->config.internalConstr=ui->internalConstrCheckBox->checkState()==Qt::CheckState::Checked;

    updateInvertCheckBox();
    updateIterationsLabel();
}

void ConfigDialog::selectCOR()
{
    QString filename=QFileDialog::getOpenFileName(this, tr("Select main COR file"),
                                             m_project->config.working_directory.c_str(),
                                             tr("COR (*.cor)"));
    if (filename.isNull())
        return;

    QDir dir(m_project->config.working_directory.c_str());
    QString relative_filename=dir.relativeFilePath(filename);
    ui->mainCOREdit->setText(relative_filename);
}

void ConfigDialog::selectOBS()
{
    QString filename=QFileDialog::getOpenFileName(this, tr("Select main OBS file"),
                                             m_project->config.working_directory.c_str(),
                                             tr("OBS (*.obs)"));
    if (filename.isNull())
        return;

    QDir dir(m_project->config.working_directory.c_str());
    QString relative_filename=dir.relativeFilePath(filename);
    ui->mainOBSEdit->setText(relative_filename);
}

void ConfigDialog::selectCoordCov()
{
    QString filename=QFileDialog::getOpenFileName(this, tr("Select COR covariance matrix file"),
                                             m_project->config.working_directory.c_str(),
                                             tr("CSV (*.csv)"));
    if (filename.isNull())
        return;

    QDir dir(m_project->config.working_directory.c_str());
    QString relative_filename=dir.relativeFilePath(filename);
    ui->mainCoordCovEdit->setText(relative_filename);
}

void ConfigDialog::updateOkButton()
{
    ui->pushButtonOk->setEnabled( (ui->mainCOREdit->text().size()>1) && (ui->mainOBSEdit->text().size()>1) );
}

void ConfigDialog::updateInvertCheckBox()
{
    if (ui->propagationRadioButton->isChecked())
    {
        ui->invertCheckBox->setEnabled(false);
        ui->invertCheckBox->setChecked(true);
    }else if (ui->monteCarloRadioButton->isChecked())
    {
        ui->invertCheckBox->setEnabled(false);
        ui->invertCheckBox->setChecked(false);
        ui->internalConstrCheckBox->setEnabled(false);
        ui->internalConstrCheckBox->setChecked(false);
    }
    else ui->invertCheckBox->setEnabled(true);

    ui->internalConstrCheckBox->setEnabled(ui->invertCheckBox->isChecked());
}

void ConfigDialog::updateIterationsLabel()
{
    if (ui->monteCarloRadioButton->isChecked())
        ui->iterationsLabel->setText(tr("Number of draws"));
    else
        ui->iterationsLabel->setText(tr("Maximum Iterations"));

    ui->groupBoxIterations->setEnabled(!ui->propagationRadioButton->isChecked());
    ui->convergenceDoubleSpinBox->setEnabled(ui->compensationRadioButton->isChecked());
    ui->convergenceLabel->setEnabled(ui->compensationRadioButton->isChecked());
    ui->forcedIterLabel->setEnabled(ui->compensationRadioButton->isChecked());
    ui->forcedIterSpinBox->setEnabled(ui->compensationRadioButton->isChecked());
}
