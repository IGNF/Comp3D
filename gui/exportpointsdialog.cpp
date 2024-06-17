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

#include "exportpointsdialog.h"
#include "ui_exportpointsdialog.h"

QStringList ExportPointsDialog::techList({" - ", "C-Combi", "D-DORIS", "L-SLR", "M-LLR", "P-GNSS", "R-VLBI"});

auto CorCovOptions = {
    std::make_pair("add_stations_params", "Add stations params"),
    std::make_pair("add_other_params", "Add other params")
};
const std::map<ExportPointsDialogType,std::string> ExportPointsDialog::typesNames={
    {ExportPointsDialogType::simple,"Simple"},
    {ExportPointsDialogType::sinex,"Sinex"},
    {ExportPointsDialogType::varcov,"VarCov"},
};

ExportPointsDialog::ExportPointsDialog(Project_Config *config, std::list<Point> &points, ExportPointsDialogType type, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportPointsDialog),m_pointsFiltered(),m_config(config),m_type(type)
{
    ui->setupUi(this);
    connect(this->ui->pushButtonAll,SIGNAL(pressed()),this,SLOT(select_all()));
    connect(this->ui->pushButtonNone,SIGNAL(pressed()),this,SLOT(select_none()));

    //select points
    switch (m_type) {
    case ExportPointsDialogType::simple:
    case ExportPointsDialogType::varcov:
        for (auto &point:points)
            m_pointsFiltered.push_back(&point);
        break;
    case ExportPointsDialogType::sinex:
        for (auto &point:points)
            if (point.dimension==3)
                m_pointsFiltered.push_back(&point);
        break;
    }

    switch (m_type) {
    case ExportPointsDialogType::simple:
        fillSimple();
        break;
    case ExportPointsDialogType::sinex:
        fillSinex();
        break;
    case ExportPointsDialogType::varcov:
        fillVarCov();
        break;
    }
}

void ExportPointsDialog::fillSimple()
{
    unsigned int i=0;
    exportChecks.clear();
    lineCODE.clear();
    linePT.clear();
    lineDOMES.clear();
    lineDESCR.clear();
    ui->optionsLabel->hide();
    ui->optionsScrollArea->hide();
    Json::Value jsonExport = m_config->miscMetadata[std::string("export")+typesNames.at(m_type)];
    for (auto &point:m_pointsFiltered)

    {
        exportChecks.push_back(new QCheckBox(point->name.c_str(),ui->pointsScrollAreaWidgetContents));
        ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->addWidget(exportChecks.back(),i/3,i%3);
        exportChecks.back()->setChecked(jsonExport.get(point->name.c_str(), false).asBool());
        i++;
    }
}

void ExportPointsDialog::fillVarCov()
{
    unsigned int i=0;
    exportChecks.clear();
    optionsChecks.clear();
    lineCODE.clear();
    linePT.clear();
    lineDOMES.clear();
    lineDESCR.clear();
    Json::Value jsonExport = m_config->miscMetadata[std::string("export")+typesNames.at(m_type)];
    for (auto &point:m_pointsFiltered)
    {
        exportChecks.push_back(new QCheckBox(point->name.c_str(),ui->pointsScrollAreaWidgetContents));
        ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->addWidget(exportChecks.back(),i/3,i%3);
        exportChecks.back()->setChecked(jsonExport.get(point->name.c_str(), false).asBool());
        i++;
    }
    ui->optionsLabel->show();
    ui->optionsScrollArea->show();
    Json::Value jsonOptions = m_config->miscMetadata[std::string("export")+typesNames.at(m_type)+"Options"];
    i=0;
    for (auto& option:CorCovOptions)
    {
        std::cout<<option.second<<" "<<option.first<<" "<<jsonExport.get(option.first, false)<<std::endl;
        optionsChecks.push_back(new QCheckBox(tr(option.second),ui->optionsScrollAreaWidgetContents));
        ((QGridLayout*)(ui->optionsScrollAreaWidgetContents->layout()))->addWidget(optionsChecks.back(),i/3,i%3);
        optionsChecks.back()->setChecked(jsonOptions.get(option.first, false).asBool());
        i++;
    }
}

int ExportPointsDialog::tech2Num(QString str)
{
    for (int i=0;i<techList.size();i++)
        if (str[0]==techList[i][0])
            return i;
    return 0;
}

void ExportPointsDialog::fillSinex()
{
    unsigned int i=1;//fist line is for legend
    exportChecks.clear();
    lineCODE.clear();
    linePT.clear();
    lineDOMES.clear();
    comboTECH.clear();
    lineDESCR.clear();
    ui->optionsLabel->hide();
    ui->optionsScrollArea->hide();
    Json::Value jsonExport = m_config->miscMetadata[std::string("export")+typesNames.at(m_type)];
    Json::Value jsonCODE = m_config->miscMetadata["CODE"];
    Json::Value jsonPT = m_config->miscMetadata["PT"];
    Json::Value jsonDOMES = m_config->miscMetadata["DOMES"];
    Json::Value jsonTECH = m_config->miscMetadata["TECH"];
    Json::Value jsonDESCR = m_config->miscMetadata["DESCR"];

    //legend
    ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->addWidget(new QLabel("Export",ui->pointsScrollAreaWidgetContents),0,0);
    ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->addWidget(new QLabel("CODE",ui->pointsScrollAreaWidgetContents),0,1);
    ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->addWidget(new QLabel("PT",ui->pointsScrollAreaWidgetContents),0,2);
    ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->addWidget(new QLabel("DOMES",ui->pointsScrollAreaWidgetContents),0,3);
    ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->addWidget(new QLabel("TECH",ui->pointsScrollAreaWidgetContents),0,4);
    ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->addWidget(new QLabel("DESCRIPTION",ui->pointsScrollAreaWidgetContents),0,5);
    ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->setColumnStretch(0,1);
    ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->setColumnStretch(1,1);
    ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->setColumnStretch(2,1);
    ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->setColumnStretch(3,2);
    ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->setColumnStretch(4,1);
    ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->setColumnStretch(5,3);

    for (auto &point:m_pointsFiltered)
    {
        exportChecks.push_back(new QCheckBox(point->name.c_str(),ui->pointsScrollAreaWidgetContents));
        ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->addWidget(exportChecks.back(),i,0);
        exportChecks.back()->setChecked(jsonExport.get(point->name.c_str(), false).asBool());
        connect(exportChecks.back(),SIGNAL(stateChanged(int)),this,SLOT(update_enabled()));

        lineCODE.push_back(new QLineEdit("",ui->pointsScrollAreaWidgetContents));
        ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->addWidget(lineCODE.back(),i,1);
        lineCODE.back()->setEnabled(exportChecks.back()->isChecked());
        lineCODE.back()->setMaxLength(4);
        lineCODE.back()->setText(jsonCODE.get(point->name.c_str(), "").asString().c_str());

        linePT.push_back(new QLineEdit("",ui->pointsScrollAreaWidgetContents));
        ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->addWidget(linePT.back(),i,2);
        linePT.back()->setEnabled(exportChecks.back()->isChecked());
        linePT.back()->setMaxLength(2);
        linePT.back()->setText(jsonPT.get(point->name.c_str(), "").asString().c_str());

        lineDOMES.push_back(new QLineEdit("",ui->pointsScrollAreaWidgetContents));
        ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->addWidget(lineDOMES.back(),i,3);
        lineDOMES.back()->setEnabled(exportChecks.back()->isChecked());
        lineDOMES.back()->setMaxLength(9);
        lineDOMES.back()->setText(jsonDOMES.get(point->name.c_str(), "").asString().c_str());

        comboTECH.push_back(new QComboBox(ui->pointsScrollAreaWidgetContents));
        ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->addWidget(comboTECH.back(),i,4);
        comboTECH.back()->setEnabled(exportChecks.back()->isChecked());
        comboTECH.back()->addItems(techList);
        comboTECH.back()->setCurrentIndex(tech2Num(jsonTECH.get(point->name.c_str(), " ").asString().c_str()));

        lineDESCR.push_back(new QLineEdit("",ui->pointsScrollAreaWidgetContents));
        ((QGridLayout*)(ui->pointsScrollAreaWidgetContents->layout()))->addWidget(lineDESCR.back(),i,5);
        lineDESCR.back()->setEnabled(exportChecks.back()->isChecked());
        lineDESCR.back()->setMaxLength(22);
        lineDESCR.back()->setText(jsonDESCR.get(point->name.c_str(), "").asString().c_str());

        i++;
    }
}

ExportPointsDialog::~ExportPointsDialog()
{
    for (auto &check:exportChecks)
        delete check;
    exportChecks.clear();
    for (auto &check:optionsChecks)
        delete check;
    optionsChecks.clear();
    delete ui;
}

std::vector <Point*> ExportPointsDialog::getSelectedPoints()
{
    std::vector <Point*> selectedPoints;
    unsigned int i=0;
    for (auto &check:exportChecks)
    {
        if (check->isChecked())
            selectedPoints.push_back(m_pointsFiltered.at(i));
        i++;
    }
    return selectedPoints;
}

std::vector <bool> ExportPointsDialog::getOptions()
{
    std::vector <bool> options;
    for (auto &check:optionsChecks)
        options.push_back(check->isChecked());
    return options;
}

void ExportPointsDialog::saveMetaData()
{
    //try to fill everything, we will write only useful data for m_type at the end
    Json::Value jsonExport;
    Json::Value jsonOptions;
    Json::Value jsonCODE;
    Json::Value jsonPT;
    Json::Value jsonDOMES;
    Json::Value jsonTECH;
    Json::Value jsonDESCR;
    unsigned int i=0;
    for (auto &point:m_pointsFiltered)
    {
        if (lineCODE.size()>i)
            if (!lineCODE[i]->text().isEmpty())
                jsonCODE[point->name]=lineCODE[i]->text().toStdString();
        if (linePT.size()>i)
            if (!linePT[i]->text().isEmpty())
                jsonPT[point->name]=linePT[i]->text().toStdString();
        if (lineDOMES.size()>i)
            if (!lineDOMES[i]->text().isEmpty())
                jsonDOMES[point->name]=lineDOMES[i]->text().toStdString();
        if (comboTECH.size()>i)
            if (comboTECH[i]->currentIndex()>0)
            {
                jsonTECH[point->name]=comboTECH[i]->currentText().toStdString().substr(0,1);
                //std::cout<<point->name<<": save tech *"<<comboTECH[i]->currentText().toStdString()<<"* => *"
                //        <<comboTECH[i]->currentText().toStdString().substr(0,1)<<"*"<<std::endl;
            }
        if (lineDESCR.size()>i)
            if (!lineDESCR[i]->text().isEmpty())
                jsonDESCR[point->name]=lineDESCR[i]->text().toStdString();
        if (exportChecks.size()>i)
            if (exportChecks[i]->isChecked())
                jsonExport[point->name]=exportChecks[i]->isChecked();
        i++;
    }

    switch (m_type) {
    case ExportPointsDialogType::simple:
        m_config->miscMetadata[std::string("export")+typesNames.at(m_type)]=jsonExport;
        break;
    case ExportPointsDialogType::varcov:
        i=0;
        for (auto& option:CorCovOptions)
        {
            auto& check=optionsChecks[i];
            jsonOptions[option.first]=check->isChecked();
            i++;
        }
        m_config->miscMetadata[std::string("export")+typesNames.at(m_type)]=jsonExport;
        m_config->miscMetadata[std::string("export")+typesNames.at(m_type)+"Options"]=jsonOptions;
        break;
    case ExportPointsDialogType::sinex:
        m_config->miscMetadata[std::string("export")+typesNames.at(m_type)]=jsonExport;
        m_config->miscMetadata["CODE"]=jsonCODE;
        m_config->miscMetadata["PT"]=jsonPT;
        m_config->miscMetadata["DOMES"]=jsonDOMES;
        m_config->miscMetadata["TECH"]=jsonTECH;
        m_config->miscMetadata["DESCR"]=jsonDESCR;
        break;
    default:
        break;
    }
}

void ExportPointsDialog::select_all()
{
    for (auto &check:exportChecks)
        check->setChecked(true);
}

void ExportPointsDialog::select_none()
{
    for (auto &check:exportChecks)
        check->setChecked(false);
}

void ExportPointsDialog::update_enabled()
{
    for (unsigned int i=0;i<exportChecks.size();i++)
    {
        lineCODE[i]->setEnabled(exportChecks[i]->isChecked());
        linePT[i]->setEnabled(exportChecks[i]->isChecked());
        lineDOMES[i]->setEnabled(exportChecks[i]->isChecked());
        comboTECH[i]->setEnabled(exportChecks[i]->isChecked());
        lineDESCR[i]->setEnabled(exportChecks[i]->isChecked());
    }
}
