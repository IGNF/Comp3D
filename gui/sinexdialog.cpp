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

#include "sinexdialog.h"
#include "ui_sinexdialog.h"

SinexDialog::SinexDialog(Project_Config *config, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SinexDialog), m_config(config)
{
    ui->setupUi(this);
    Json::Value jsonSINEX = config->miscMetadata["SINEX"];
    ui->fileAgencyEdit->setText(jsonSINEX.get("fileAgency", "").asString().c_str());
    ui->dataAgencyEdit->setText(jsonSINEX.get("dataAgency", "").asString().c_str());
    if (jsonSINEX.get("obsTime", "").isInt64())
        ui->obsTimeEdit->setDateTime(QDateTime::fromMSecsSinceEpoch(jsonSINEX.get("obsTime", 0).asInt64() * 1000).toUTC());
    else
        ui->obsTimeEdit->setDateTime(QDateTime::currentDateTimeUtc());
}

SinexDialog::~SinexDialog()
{
    delete ui;
}

void SinexDialog::saveMetaData()
{
    Json::Value jsonSINEX;
    jsonSINEX["fileAgency"]=ui->fileAgencyEdit->text().toStdString();
    jsonSINEX["dataAgency"]=ui->dataAgencyEdit->text().toStdString();
    jsonSINEX["obsTime"]= (int64_t)(ui->obsTimeEdit->dateTime().toMSecsSinceEpoch() / 1000);
    m_config->miscMetadata["SINEX"]=jsonSINEX;
}
