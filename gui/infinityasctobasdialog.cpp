/**
 * Copyright (c) Institut national de l'information géographique et forestière https://www.ign.fr/
 *
 * File main authors:
 *  - JM Muller
 *  - C Meynard
 *
 * This file is part of Comp3D: https://github.com/IGNF/Comp3D
 *
 * Comp3D is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Comp3D is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Comp3D. If not, see <https://www.gnu.org/licenses/>.
 */

#include "infinityasctobasdialog.h"
#include "ui_infinityasctobasdialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <sstream>
#include "uni_stream.h"
#include "filesystem_compat.h"


InfinityAscToBasDialog::InfinityAscToBasDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InfinityAscToBasDialog)
{
    ui->setupUi(this);

    QDir currentDirectory(QSettings().value("/Comp3D/LastProjectPath").toString());
    currentDirectory.cdUp();
    workingDir=currentDirectory.absolutePath();

    connect(ui->pushButtonHelp,&QPushButton::pressed,this,&InfinityAscToBasDialog::showHelp);
    connect(ui->pushButtonApply,&QPushButton::pressed,this,&InfinityAscToBasDialog::convert);
    connect(ui->pushButtonAscFile,&QPushButton::pressed,this,&InfinityAscToBasDialog::selectAscFile);

    connect(ui->lineEditAscFile,&QLineEdit::textChanged,this,&InfinityAscToBasDialog::updateApply);

    ui->pushButtonApply->setEnabled(false);

    adjustSize();
    setFixedSize(size());
}

InfinityAscToBasDialog::~InfinityAscToBasDialog()
{
    delete ui;
}

void InfinityAscToBasDialog::selectAscFile()
{
    QString defFile = ui->lineEditAscFile->text();
    if (defFile.isEmpty())
        defFile = workingDir;

    QString filename=QFileDialog::getOpenFileName(this, tr("Select ASC File"),
                                             defFile ,
                                             tr("Infinity ASC file(*.asc);;All (*.*)"));
    if (filename.isNull())
        return;

    ui->lineEditAscFile->setText(filename);

    QDir currentDirectory(filename);
    currentDirectory.cdUp();
    workingDir=currentDirectory.absolutePath();
}


void InfinityAscToBasDialog::updateApply()
{
    ui->pushButtonApply->setEnabled((ui->lineEditAscFile->text().size()>0));
    ui->textBrowserResult->clear();
}


void InfinityAscToBasDialog::showHelp()
{
    QMessageBox::information(this, tr("Infinity ASC to BAS help"),
                             tr("Converts an ASC file from Leica Infinity GNSS software "
                                "into a Comp3D GNSS baselines BAS file.\n\n"
                                "A BAS file is generated for each baselines "
                                "starting point from the ASC file.\n"
                                "If the input ASC file contains heights values, "
                                "it means that these heights were taken into "
                                "account when computing the baseline. "
                                "Therefore, the tool adds the heights as comments "
                                "and with 0 values in the output BAS file."),
                       QMessageBox::Ok);
}



struct BasBaseLine {
    std::string name;
    std::string dx,dy,dz;
    std::string height;
    std::string sta_height;
    std::string date;
    double cov_xx,cov_xy,cov_xz,cov_yy,cov_yz,cov_zz;
};

struct BasStation {
    std::string fileName;
    bool exist;
    bool ok;
    std::vector<BasBaseLine> baseLines;
};


typedef std::map<std::string,BasStation> BasObs;

static int readAscFile(const std::string& ascFileName, BasObs &basObs)
{
    std::string line;
    char char1,char2;
    std::string word1,word2;
    double K;
    BasBaseLine *basPt=nullptr;
    std::string basFileStem = fs::path(ascFileName).stem().string();

    uni_stream::ifstream ascFile(ascFileName);
    if (!ascFile)
        return -1;

    basObs.clear();
    while (1) {
        std::getline(ascFile,line);
        if (! ascFile)
            return ascFile.eof() ? basObs.size() : -2;
        std::stringstream ss(line);
        ss >> char1 >> char2 >> word1;
        if (char1 != '@' || word1.length() == 0)
            continue;
        switch(char2) {
        case '+':
            basObs[word1].fileName = basFileStem + "_" + word1 + ".bas";
            basObs[word1].baseLines.push_back(BasBaseLine());
            basPt = &basObs[word1].baseLines.back();
            basPt->date.clear();
            break;
        case '-':
            if (!basPt)
                continue;
            basPt->name = word1;
            ss >> basPt->dx >> basPt->dy >> basPt->dz;
            break;
        case '=':
            if (!basPt)
                continue;
            ss >> basPt->cov_xx >> basPt->cov_xy >> basPt->cov_xz;
            ss >> basPt->cov_yy >> basPt->cov_yz;
            ss >> basPt->cov_zz;
            K = std::stod(word1);
            basPt->cov_xx *= K*K;
            basPt->cov_xy *= K*K;
            basPt->cov_xz *= K*K;
            basPt->cov_yy *= K*K;
            basPt->cov_yz *= K*K;
            basPt->cov_zz *= K*K;
            break;
        case ':':
            if (! basPt)
                continue;
            basPt->sta_height = word1;
            break;
        case ';':
            if (!basPt)
                continue;
            basPt->height = word1;
            break;
        case '*':
            if (!basPt)
                continue;
            ss >> word2;
            basPt->date = word1 + " " + word2;
            break;
        }
    }
}


static void writeBasFile(const std::string& ascFileName, BasObs &basObs)
{
    fs::path basFilePath = ascFileName;
    fs::path basFileDir= basFilePath.parent_path();

    for (auto& piv : basObs) {
        piv.second.ok = true;
        uni_stream::ofstream basFile((basFileDir / piv.second.fileName).string(), std::ofstream::trunc);
        if (!basFile) {
            piv.second.ok = false;
            continue;
        }
        basFile << "* Imported from " << ascFileName << "\n";
        basFile << "* Station: " << piv.first << "\n";
        for (const auto& pt : piv.second.baseLines) {
            basFile << "\n";
            basFile << "* Station H: ";
            basFile << (pt.sta_height.size()==0 ? "None" : pt.sta_height) <<", ";
            basFile << "Target H: ";
            basFile << (pt.height.size()==0 ? "None" : pt.height) <<". ";
            if (pt.height.size() || pt.sta_height.size()) {
                if (pt.height.size() && pt.sta_height.size())
                    basFile << "These heights have been used in the baseline calculus.";
                else
                    basFile << "This height has been used in the baseline calculus.";
                basFile << "\n";
            }
            if (pt.date.size())
                basFile << "* Date: " << pt.date << "\n";
            basFile << pt.name << ' ';
            basFile << pt.dx << ' ' << ' ' << pt.dy << ' ' << pt.dz << ' ';
            basFile << pt.cov_xx << ' ' << pt.cov_xy << ' ' << pt.cov_xz << ' ';
            basFile << pt.cov_yy << ' ' << pt.cov_yz << ' ';
            basFile << pt.cov_zz << ' ';
            basFile << "0.0\n";
            if (!basFile) {
                piv.second.ok = false;
                continue;
            }
        }
    }
}

void InfinityAscToBasDialog::convert()
{
    int res;
    int nbPts = 0;
    int nbFail = 0;
    BasObs  basObs;
    QString resText;

    QString ascFileName = ui->lineEditAscFile->text();
    QString outDir = QString::fromStdString(fs::path(ascFileName.toStdString()).parent_path().string());

    res = readAscFile(ascFileName.toStdString(),basObs);
    if (res < 1) {
        switch (res) {
        case -2 : resText = tr("Error reading file '%1'").arg(ascFileName); break;
        case -1 : resText = tr("Can't open file '%1'").arg(ascFileName); break;
        case  0 : resText = tr("No observation found in '%1'").arg(ascFileName); break;
        }
        ui->textBrowserResult->setHtml("<font color=\"#aa0000\">" + resText + "</font>");
        return;
    }

    QString exists;
    for (const auto& piv : basObs) {
        QString fn = QString::fromStdString(piv.second.fileName);
        if (QFile::exists(outDir + "/" + fn)) {
            exists += fn + "<br>";
        }
    }
    if (exists.length()) {
        exists = tr("Overwrite files in %1:").arg(outDir) + "<br>" + exists + "?";
        if (QMessageBox::Yes != QMessageBox::question(this,
                                                      tr("Overwrite files"),
                                                      exists)) {
            ui->textBrowserResult->setHtml(tr("Cancelled."));
            return;
        }
    }

    writeBasFile(ascFileName.toStdString(),basObs);

    for (const auto& piv : basObs) {
        nbPts += piv.second.baseLines.size();
        if (! piv.second.ok)
            nbFail ++;
    }
    if (nbFail) {
        resText += tr("Error writting files in %1:").arg(outDir) + "<br>";
        for (const auto& piv : basObs) {
            if (! piv.second.ok)
                resText += QString::fromStdString(piv.second.fileName) + "<br>";
        }
        resText.chop(4);
        QMessageBox::information(this, tr("Write file error"), resText);
        resText = "<font color=\"#aa0000\">" +
                resText +
                "</font><p>";
    }

    resText += "<font color=\"#0000aa\">";
    resText += tr("%1 file(s) written in %2:")
            .arg(basObs.size() - nbFail)
            .arg(outDir);
    resText += "</font><br>";
    for (const auto& piv : basObs) {
        if (piv.second.ok)
            resText += QString::fromStdString(piv.second.fileName) + "<br>";
    }
    resText.chop(4);

    resText += "<p><font color=\"#0000aa\">" +
            tr("%1 pivot(s), %2 baseline(s):").arg(basObs.size()).arg(nbPts) +
            "</font><br>";

    for (const auto& piv : basObs) {
        resText +="<font color=\"#aa0000\">" +
                QString::fromStdString(piv.first) + " => " +
                "</font>";
        for (const auto& pt : piv.second.baseLines)
            resText += QString::fromStdString(pt.name) + ", ";
        if (piv.second.baseLines.size())
            resText.chop(2);
        resText += "<br>";
    }
    if (basObs.size())
        resText.chop(4);
    ui->textBrowserResult->setHtml(resText);
}
