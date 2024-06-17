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

#include "dialogpreferences.h"
#include "ui_dialogpreferences.h"
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>
#include "project.h"
#include "compile.h"

DialogPreferences::DialogPreferences(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogPreferences)
{
    ui->setupUi(this);

    ui->comboBoxLang->addItems(SUPPORTED_LANG_NAME);
    auto curVal = QSettings().value("/Comp3D/Language","en").toString();
    ui->comboBoxLang->setCurrentIndex(QStringList(SUPPORTED_LANG_CODE).lastIndexOf(curVal));

    connect(this,&QDialog::accepted,this,&DialogPreferences::savePref);
    adjustSize();
    setFixedSize(size());
    show();
}


DialogPreferences::~DialogPreferences()
{
    delete ui;
}

void DialogPreferences::savePref()
{
    QSettings settings;
    QString codeLang = QStringList(SUPPORTED_LANG_CODE)[ui->comboBoxLang->currentIndex()];
    settings.setValue("/Comp3D/Language",codeLang);
    Project::defaultLogLang = codeLang.toStdString();
    if (!settings.isWritable() || settings.value("/Comp3D/Language",codeLang).toString() != codeLang)
        QMessageBox::warning(this,tr("Preferences error"),tr("Can't save language preference"));
}
