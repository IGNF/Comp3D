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

#include <QMenu>
#include <QTimer>
#include <QDesktopServices>
#include <QUrl>
#include "maintables.h"
#include "ui_maintables.h"
#include "project.h"
#include "point.h"
#include "customtablewidgetitem.h"

#define RESID_STAND_WARN 2.5
#define RESID_STAND_ERR  4.1

#define RESID_NORM_WARN  3
#define RESID_NORM_ERR   6

#define RESID_ERR_COLOR  QColor(255,199,199)
#define RESID_WARN_COLOR QColor(255,249,212)

enum PointsRowName{PTS_NUM=0,PTS_CODE,PTS_NAME, PTS_X, PTS_Y, PTS_Z, PTS_NBO, PTS_SX, PTS_SY, PTS_SZ};
enum ObsRowName{OBS_NUM=0,OBS_ACTIVE,OBS_FROM, OBS_TO, OBS_CODE, OBS_SIGMA, OBS_RESIDUAL, OBS_RESIDUALMM, OBS_TAR_NBO, OBS_REDONDANCY};

static std::map<CR_CODE, const char *> point_code_desc = {
    {CR_CODE::FORBIDDEN,QT_TRANSLATE_NOOP("MainTables","?") },
    {CR_CODE::FREE,QT_TRANSLATE_NOOP("MainTables","3D free point")},
    {CR_CODE::CR_XYZ,QT_TRANSLATE_NOOP("MainTables","3D point with known X,Y,Z")},
    {CR_CODE::CR_XY,QT_TRANSLATE_NOOP("MainTables","3D point with known X and Y")},
    {CR_CODE::CR_Z,QT_TRANSLATE_NOOP("MainTables","3D point with known Z")},
    {CR_CODE::NIV_FREE,QT_TRANSLATE_NOOP("MainTables","1D free point")},
    {CR_CODE::NIV_CR,QT_TRANSLATE_NOOP("MainTables","1D point with known Z ")},
    {CR_CODE::PLANI_FREE,QT_TRANSLATE_NOOP("MainTables","2D free point")},
    {CR_CODE::PLANI_CR,QT_TRANSLATE_NOOP("MainTables","2D point with known X and Y")},
    {CR_CODE::PLANI_FAR_FREE,QT_TRANSLATE_NOOP("MainTables","Far 2D free point")},
    {CR_CODE::PLANI_FAR_CR,QT_TRANSLATE_NOOP("MainTables","Far 2D point with known X and Y")},
    {CR_CODE::UNDEFINED,QT_TRANSLATE_NOOP("MainTables","?") }
};


Q_DECLARE_METATYPE(Obs*);

MainTables::MainTables(QWidget *parent) :
    QSplitter(parent),
    project(nullptr),afterCompensation(false), showCoord(true),showObs(true),
    interactive(true),
    ui(new Ui::MainTables)
{
    qApp->setStyle(new CenteredBoxProxy);

    ui->setupUi(this);

    QPalette p = ui->pointsTable->palette();
    p.setColor(QPalette::Inactive, QPalette::Highlight, p.color(QPalette::Active, QPalette::Highlight));
    p.setColor(QPalette::Inactive, QPalette::HighlightedText , p.color(QPalette::Active, QPalette::HighlightedText));
    ui->pointsTable->setPalette(p);

    p = ui->obsTable->palette();
    p.setColor(QPalette::Inactive, QPalette::Highlight, p.color(QPalette::Active, QPalette::Highlight));
    p.setColor(QPalette::Inactive, QPalette::HighlightedText , p.color(QPalette::Active, QPalette::HighlightedText));
    ui->obsTable->setPalette(p);

    handle(1)->setToolTip(tr("Resize/Hide Tables"));
    setStretchFactor(0,2);
    setStretchFactor(1,3);

    //prepare growing factors for each table col
    //num name code x y z obs ex ey ez
    std::vector<double> size_minimums1{40,50,50,70,70,70,50,70,70,70};
    std::vector<double> size_factors1{1,0,3,4,4,4,3,4,4,4};
    ui->pointsTable->set_size_info(size_minimums1,size_factors1);
    //num act from to type sigma nres resmm obstarget redon
    std::vector<double> size_minimums2{40,50,55,55,65,60,60,80,60,60};
    std::vector<double> size_factors2{0,0,2,2,0,2,1,2,1,2};
    ui->obsTable->set_size_info(size_minimums2,size_factors2);

    connect(this,SIGNAL(splitterMoved (int,int)),this,SLOT(splitterMoved()));
    connect(this->ui->obsTable,SIGNAL(cellClicked (int,int)),this,SLOT(obsClicked(int,int)));
// CM: La connexion par SIGNAL ne marche pas ! Pourqoui ?
//    connect(this->ui->obsTable,SIGNAL(customContextMenuRequested(const QPoint &pos)),this,SLOT(obsContextMenu(const Qpoint &pos)));
    connect(this->ui->obsTable,&QWidget::customContextMenuRequested,this,&MainTables::obsContextMenu);
    ui->obsTable->setContextMenuPolicy(Qt::CustomContextMenu);
}

MainTables::~MainTables()
{
    delete ui;
}

void MainTables::disableSort()
{
    ui->pointsTable->setSortingEnabled(false);
    ui->obsTable->setSortingEnabled(false);
}

void MainTables::enableSort()
{
    ui->pointsTable->setSortingEnabled(true);
    ui->pointsTable->resort();
    ui->obsTable->setSortingEnabled(true);
    ui->obsTable->resort();
}

void MainTables::updateGraphics()
{
    ui->pointsTable->resize();
    ui->obsTable->resize();
}

void MainTables::updateContent(bool initSort)
{
    //std::cout<<ui->pointsTable->horizontalHeader()->sortIndicatorOrder()<<"\n";
    //std::cout<<ui->pointsTable->horizontalHeader()->sortIndicatorSection()<<"\n";

    ui->pointsTable->clearContents();
    ui->obsTable->clearContents();
    ui->pointsTable->setRowCount(0);
    ui->obsTable->setRowCount(0);
    if (project==nullptr)
        return;

    interactive = true;
    int nbDigMm = project->config.nbDigits-3;
    if (nbDigMm<0) nbDigMm = 0;

    if (afterCompensation)
    {
        ui->pointsLabel->setText(tr("Final Points"));
        ui->obsLabel->setText(tr("Final Observations"));
    }else{
        ui->pointsLabel->setText(tr("Initial Points"));
        ui->obsLabel->setText(tr("Initial Observations"));
    }

    //num name code x y z obs ex ey ez
    QStringList pointsHeaders;
    QStringList  pointsToolTips;
    pointsHeaders<<" "<<"c"<<tr("Name");
    pointsToolTips << tr("Point rank") << tr("Point code") << tr("Point name");
    if (afterCompensation) {
        if (project->projection.type==PROJ_TYPE::PJ_GEO) {
            pointsHeaders<<"ΔE m"<<"ΔN m";
            pointsToolTips << tr("Difference between final and initial East coordinate (m)");
            pointsToolTips << tr("Difference between final and initial North coordinate (m)");
        } else {
            pointsHeaders<<"Δx m"<<"Δy m";
            pointsToolTips << tr("Difference between final and initial x (m)");
            pointsToolTips << tr("Difference between final and initial y (m)");
        }
        if (project->config.useEllipsHeight) {
            pointsHeaders<<tr("ΔEh m");
            pointsToolTips << tr("Difference between final and initial ellipsoidal height (m)");
        } else {
            pointsHeaders<<tr("ΔGh m");
            pointsToolTips << tr("Difference between final and initial orthometric height (m)");
        }
    } else {
        if (project->projection.type==PROJ_TYPE::PJ_GEO) {
            pointsHeaders<<"E m"<<"N m";
            pointsToolTips << tr("East (m)") << tr("North (m)");
        } else {
            pointsHeaders<<"x m"<<"y m";
            pointsToolTips << "x (m)" << "y (m)";
        }
        if (project->config.useEllipsHeight) {
            pointsHeaders<<tr("Eh m");
            pointsToolTips << tr("Ellipsoidal height (m)");
        } else {
            pointsHeaders<<tr("Gh m");
            pointsToolTips << tr("Orthometric height (m)");
        }
    }
    if (!showObs) {
        pointsHeaders<<tr("NbObs");
        pointsToolTips << tr("Number of active observations for this point");
        if (project->config.compute_type==COMPUTE_TYPE::type_monte_carlo)
        {
            pointsHeaders<<tr("MSE x mm")<<tr("MSE y mm")<<tr("MSE z mm");
            pointsToolTips << tr("Mean squared error x (mm)") << tr("Mean squared error y (mm)") << tr("Mean squared error z (mm)");
        } else {
            pointsHeaders<<tr("σ̂x mm")<<tr("σ̂y mm")<<tr("σ̂z mm");
            pointsToolTips << tr("A posteriori sigma x (mm)") << tr("A posteriori sigma y (mm)") << tr("A posteriori sigma z (mm)");
        }
    }
    ui->pointsTable->setColumnCount(pointsHeaders.count());
    ui->pointsTable->setHorizontalHeaderLabels(pointsHeaders);
    for (int i=0; i < pointsHeaders.count(); i++)
        ui->pointsTable->horizontalHeaderItem(i)->setToolTip(pointsToolTips[i]);
    ui->pointsTable->horizontalHeader()->setTextElideMode(Qt::ElideRight);
    //ui->pointsTable->horizontalHeader()->setMovable(true);//qt4
    //ui->pointsTable->horizontalHeader()->setSectionsMovable(true);//qt5

    //num from to type sigma nres resmm redon
    QStringList obsHeaders;
    QStringList obsToolTips;

    obsHeaders<<" "<<tr("Act")<<tr("Origin")<<tr("Target")<<tr("Type")<<tr("σ")<<tr("Res σ");
    obsToolTips << tr("Observation rank")
                << tr("Activate or deactivate observations")
                << tr("From point")
                << tr("To point")
                << tr("Observation type")
                << tr("A priori sigma");
    if (!afterCompensation) {
        obsToolTips << tr("Initial normalized residual");
        obsToolTips << tr("Initial Residual (mm)");
    } else {
        if (project->invertedMatrix)
            obsToolTips << tr("Computed standardized residual, depends on redondancy");
        else
            obsToolTips << tr("Computed normalized residual");
        obsToolTips << tr("Computed Residual (mm)");
    }
    obsToolTips << tr("Number of active observations for the 'To' point")
                << tr("Observations redundancy");
    if (!showCoord)
    {
        obsHeaders<<tr("Res mm")<<tr("NbObs To")<<tr("Red %");
    }
    ui->obsTable->setColumnCount(obsHeaders.count());
    ui->obsTable->setHorizontalHeaderLabels(obsHeaders);

    for (int i=0; i < obsHeaders.count(); i++)
        ui->obsTable->horizontalHeaderItem(i)->setToolTip(obsToolTips[i]);
    //ui->obsTable->horizontalHeader()->setMovable(true);//qt4
    //ui->obsTable->horizontalHeader()->setSectionsMovable(true);//qt5
    ui->obsTable->horizontalHeader()->setTextElideMode(Qt::ElideRight);

    updateGraphics();

    ui->pointsTable->setRowCount(project->points.size());
    //ui->obsTable->setRowCount(project->numberOfObs());
    ui->obsTable->setRowCount(Project::theone()->totalNumberOfObs());

    disableSort();

    ui->pointsLabel->setToolTip("Point");
    ui->obsLabel->setToolTip("Observation");

    int row=0;
    double sigma0 = project->lsquares.all_sigma0.empty()?1.0:project->lsquares.all_sigma0.back();
    for (auto & point : project->points) {
        //std::cout<<"Table add point "<<point->name<<std::endl;

        QTableWidgetItem *num_item=new QTableWidgetItem();
        //num_item->setText(QString("%1").arg(point->getPointNumber()));
        num_item->setData(Qt::DisplayRole, (int)point.getPointNumber());//to have numerical sort!
        if (point.file)
            num_item->setToolTip(tr("From file '%1', line: %2").arg(point.file->get_name().c_str()).arg(point.lineNumber));
        else
            num_item->setToolTip(tr("Auto initialized point"));
        ui->pointsTable->setItem(point.getPointNumber(),PTS_NUM,num_item);

        QTableWidgetItem *pt_code_item=new QTableWidgetItem();
        pt_code_item->setText(point.code_name.c_str());
        pt_code_item->setToolTip(tr(point_code_desc.at(point.code)));
        //pt_code_item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
        ui->pointsTable->setItem(point.getPointNumber(),PTS_CODE,pt_code_item);

        QTableWidgetItem *name_item=new QTableWidgetItem();
        name_item->setText(point.name.c_str());
        name_item->setToolTip(pointsToolTips[PTS_NAME]);
        ui->pointsTable->setItem(point.getPointNumber(),PTS_NAME,name_item);

        FloatTableWidgetItem *x_item=new FloatTableWidgetItem();
        x_item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
        x_item->setToolTip(pointsToolTips[PTS_X]);
        FloatTableWidgetItem *y_item=new FloatTableWidgetItem();
        y_item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
        y_item->setToolTip(pointsToolTips[PTS_Y]);
        FloatTableWidgetItem *z_item=new FloatTableWidgetItem();
        z_item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
        z_item->setToolTip(pointsToolTips[PTS_Z]);
        if (afterCompensation)
        {
            x_item->setText(QString("%1").arg(point.shift.x(),4,'f',project->config.nbDigits));
            y_item->setText(QString("%1").arg(point.shift.y(),4,'f',project->config.nbDigits));
            z_item->setText(QString("%1").arg(point.shift.z(),4,'f',project->config.nbDigits));
        }else{
            x_item->setText(QString("%1").arg(point.coord_read.x(),4,'f',project->config.nbDigits));
            y_item->setText(QString("%1").arg(point.coord_read.y(),4,'f',project->config.nbDigits));
            z_item->setText(QString("%1").arg(point.coord_read.z(),4,'f',project->config.nbDigits));
        }
        ui->pointsTable->setItem(point.getPointNumber(),PTS_X,x_item);
        ui->pointsTable->setItem(point.getPointNumber(),PTS_Y,y_item);
        ui->pointsTable->setItem(point.getPointNumber(),PTS_Z,z_item);

        if (!showObs)
        {
            FloatTableWidgetItem *nbo_item=new FloatTableWidgetItem();
            nbo_item->setText(QString("%1").arg(point.nbActiveObs));
            nbo_item->setToolTip(pointsToolTips[PTS_NBO]);
            nbo_item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
            ui->pointsTable->setItem(point.getPointNumber(),PTS_NBO,nbo_item);
            if (project->MonteCarloDone)
            {
                FloatTableWidgetItem *ellx_item=new FloatTableWidgetItem();
                ellx_item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
                ellx_item->setToolTip(pointsToolTips[PTS_SX]);
                ellx_item->setText(QString("%1").arg(point.MC_shift_sq_average.x()*1000,4,'f',nbDigMm));
                FloatTableWidgetItem *elly_item=new FloatTableWidgetItem();
                elly_item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
                elly_item->setToolTip(pointsToolTips[PTS_SY]);
                elly_item->setText(QString("%1").arg(point.MC_shift_sq_average.y()*1000,4,'f',nbDigMm));
                FloatTableWidgetItem *ellz_item=new FloatTableWidgetItem();
                ellz_item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
                ellz_item->setToolTip(pointsToolTips[PTS_SZ]);
                ellz_item->setText(QString("%1").arg(point.MC_shift_sq_average.z()*1000,4,'f',nbDigMm));

                ui->pointsTable->setItem(point.getPointNumber(),PTS_SX,ellx_item);
                ui->pointsTable->setItem(point.getPointNumber(),PTS_SY,elly_item);
                ui->pointsTable->setItem(point.getPointNumber(),PTS_SZ,ellz_item);
            }else if (point.ellipsoid.isSet())
            {
                FloatTableWidgetItem *ellx_item=new FloatTableWidgetItem();
                ellx_item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
                ellx_item->setToolTip(pointsToolTips[PTS_SX]);
                ellx_item->setText(QString(" -- "));
                FloatTableWidgetItem *elly_item=new FloatTableWidgetItem();
                elly_item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
                elly_item->setToolTip(pointsToolTips[PTS_SY]);
                elly_item->setText(QString(" -- "));
                FloatTableWidgetItem *ellz_item=new FloatTableWidgetItem();
                ellz_item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
                ellz_item->setToolTip(pointsToolTips[PTS_SZ]);
                ellz_item->setText(QString(" -- "));

                if (!point.isXfixed) ellx_item->setText(QString("%1").arg(point.ellipsoid.get_variance(0)*sigma0*1000,4,'f',nbDigMm));
                if (!point.isYfixed) elly_item->setText(QString("%1").arg(point.ellipsoid.get_variance(1)*sigma0*1000,4,'f',nbDigMm));
                if (!point.isZfixed) ellz_item->setText(QString("%1").arg(point.ellipsoid.get_variance(2)*sigma0*1000,4,'f',nbDigMm));

                ui->pointsTable->setItem(point.getPointNumber(),PTS_SX,ellx_item);
                ui->pointsTable->setItem(point.getPointNumber(),PTS_SY,elly_item);
                ui->pointsTable->setItem(point.getPointNumber(),PTS_SZ,ellz_item);
            }
        }
    }

    for (auto & station : project->stations) {
        for (auto & obs : station->observations) {
            //std::cout<<"Write line "<<obs.getObsNumber()<<" with "<<obs.from->name<<" ... "<<obs.toString()<<"  row "<<row<<std::endl;
            QTableWidgetItem *nbobs_item=new QTableWidgetItem();
            //num_item->setText(QString("%1").arg(obs.getObsNumber()));
            nbobs_item->setData(Qt::DisplayRole, (int)obs.getObsNumber());//to have numerical sort!
            //nbobs_item->setFlags(nbobs_item->flags() & ~Qt::ItemIsSelectable);
            nbobs_item->setToolTip(tr("From file '%1', line: %2").arg(obs.file->get_name().c_str()).arg(obs.lineNumber));
            nbobs_item->setData(Qt::UserRole,QVariant::fromValue(&obs));
            ui->obsTable->setItem(row,OBS_NUM,nbobs_item);

            QTableWidgetItem *active_item=new DoubleCheckBoxTableWidgetItem();
            active_item->setData(DoubleCheckBoxTableWidgetItem::AlignmentRole,Qt::AlignCenter);
            active_item->setToolTip(obsToolTips[OBS_ACTIVE]);
            ui->obsTable->setItem(row,OBS_ACTIVE,active_item);

            QTableWidgetItem *from_item=new QTableWidgetItem();
            from_item->setText(obs.from->name.c_str());
            from_item->setToolTip(toFromToolTip(obs.from).c_str());
            ui->obsTable->setItem(row,OBS_FROM,from_item);

            QTableWidgetItem *to_item=new QTableWidgetItem();
            to_item->setText(obs.to->name.c_str());
            to_item->setToolTip(toFromToolTip(obs.to).c_str());
            ui->obsTable->setItem(row,OBS_TO,to_item);

            QTableWidgetItem *obs_code_item=new QTableWidgetItem();
            obs_code_item->setText(Obs::getTypeName(obs.code).c_str());
            obs_code_item->setToolTip(obsToolTips[OBS_CODE]);
            ui->obsTable->setItem(row,OBS_CODE,obs_code_item);

            QTableWidgetItem *sigma_item=new QTableWidgetItem();
            sigma_item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
            sigma_item->setText(QString("%1 %2").arg(obs.sigmaTotal/obs.unitFactor,4,'f',project->config.nbDigits).arg(obs.unitStr.c_str()));
            sigma_item->setToolTip(obsToolTips[OBS_SIGMA]);
            ui->obsTable->setItem(row,OBS_SIGMA,sigma_item);

            AbsFloatTableWidgetItem *residual_item=new AbsFloatTableWidgetItem();
            residual_item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
            residual_item->setToolTip(obsToolTips[OBS_RESIDUAL]);
            if (!afterCompensation)
                residual_item->setText(QString::fromUtf8("%1").arg(obs.initialNormalizedResidual,4,'f',nbDigMm));
            else {
                if (project->invertedMatrix)
                {
                    if (!obs.active)
                        residual_item->setText(NA);
                    else if (!std::isnan(obs.standardizedResidual))
                    {
                        residual_item->setText(QString::fromUtf8("%1").arg(obs.standardizedResidual,4,'f',nbDigMm));
                        if (fabs(obs.standardizedResidual) >= RESID_STAND_ERR)
                            residual_item->setBackground(RESID_ERR_COLOR);
                        else if (fabs(obs.standardizedResidual) >= RESID_STAND_WARN)
                            residual_item->setBackground(RESID_WARN_COLOR);
                    }
                    else
                        residual_item->setText("");
                } else {
                    residual_item->setText(QString::fromUtf8("%1").arg(obs.normalizedResidual,4,'f',nbDigMm));
                    if (fabs(obs.normalizedResidual) >= RESID_NORM_ERR)
                        residual_item->setBackground(RESID_ERR_COLOR);
                    else if (fabs(obs.normalizedResidual) >= RESID_NORM_WARN)
                        residual_item->setBackground(RESID_WARN_COLOR);
                }
            }
            ui->obsTable->setItem(row,OBS_RESIDUAL,residual_item);

            if (!showCoord)
            {
                AbsFloatTableWidgetItem *residualmm_item=new AbsFloatTableWidgetItem();
                residualmm_item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
                residualmm_item->setToolTip(obsToolTips[OBS_RESIDUALMM]);

                double res = 0.0;
                if (!afterCompensation)
                    res = obs.initialResidual;
                else
                    res = obs.residual;

                if (obs.unitStr=="m")
                    res *= 1000.0;
                else
                    res *= obs.D * 1000.0;

                residualmm_item->setText(QString::fromUtf8("%1").arg(res,4,'f',nbDigMm));
                ui->obsTable->setItem(row,OBS_RESIDUALMM,residualmm_item);

                FloatTableWidgetItem *tar_nbo_item=new FloatTableWidgetItem();
                tar_nbo_item->setText(QString("%1").arg(obs.to->nbActiveObs));
                tar_nbo_item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
                tar_nbo_item->setToolTip(obsToolTips[OBS_TAR_NBO]);
                ui->obsTable->setItem(row,OBS_TAR_NBO,tar_nbo_item);

                AbsFloatTableWidgetItem *redond_item=new AbsFloatTableWidgetItem();
                redond_item->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
                redond_item->setToolTip(obsToolTips[OBS_REDONDANCY]);
                if (afterCompensation && project->invertedMatrix)
                {
                    if (obs.active)
                        redond_item->setText(QString::fromUtf8("%1").arg(obs.obsRedondancy,4,'f',1));
                    else
                        redond_item->setText(NA);
                }
                ui->obsTable->setItem(row,OBS_REDONDANCY,redond_item);
            }
            setObsActiveStyle(row,obs.active,obs.active!=obs.activeRead);
            row++;
        }
    }

    enableSort();
    if (initSort)
    {
        ui->pointsTable->sortByColumn(0,Qt::AscendingOrder);
        ui->obsTable->sortByColumn(0,Qt::AscendingOrder);
    }
}

std::string MainTables::toFromToolTip(const Point* point)
{
    #define SW std::setw(15)
    std::ostringstream oss;
    oss.precision(Project::theone()?Project::theone()->config.nbDigits:8);
    oss.setf(std::ios::fixed);

    oss<<"<style type=\"text/css\"> td { padding-right: 15px; padding-left: 15px; align: center }</style>";

    oss<<"<h2>"<<point->name<<"</h2>";
    oss<<"<table><tr><th>"<<point->code_name<<"</th> ";
    if (Project::theone()->projection.type==PROJ_TYPE::PJ_GEO)
        oss << "<th>E</th> <th>N</th>";
    else
        oss << "<th>x</th> <th>y</th>";
    if (Project::theone()->config.useEllipsHeight)
        oss << "<th>Eh</th>";
    else
        oss << "<th>Gh</th>";
    oss << "</tr>";

    if (afterCompensation)
    {
        const Coord &finalCoord=Project::theone()->projection.type==PROJ_TYPE::PJ_GEO?point->coord_compensated_georef:point->coord_comp;
        oss<<"<tr><th>"<<QObject::tr("Final").toCstr()<<"</th> <td>"<<SW<<finalCoord.x()<<"</td> <td>"<<SW<<finalCoord.y()<<"</td> <td>"<<SW<<finalCoord.z()<<"</td></tr>";
        oss<<"<tr><th>"<<QObject::tr("Δ").toCstr()<<"</th> <td>"<<SW<<point->shift.x()<<"</td> <td>"<<SW<<point->shift.y()<<"</td> <td>"<<SW<<point->shift.z()<<"</td></tr>";
    }
    else
        oss<<"<tr><th>"<<QObject::tr("Init").toCstr()<<"</th> <td>"<<SW<<point->coord_read.x()<<"</td> <td>"<<SW<<point->coord_read.y()<<"</td> <td>"<<SW<<point->coord_read.z()<<"</td></tr>";
    oss<<"<tr><th>"<<QObject::tr("σ").toCstr()<<"</th> <td>"<<SW<<point->sigmas_init.x()<<"</td> <td>"<<SW<<point->sigmas_init.y()<<"</td> <td>"<<SW<<point->sigmas_init.z()<<"</td></tr>";
    if (point->ellipsoid.isSet())
    {
        switch (point->dimension) {
        case 1:
            oss<<"<tr><th>"<<QObject::tr("σ̂").toCstr()<<"</th> <td>--</td> <td>--</td> <td>"<<SW<<point->ellipsoid.get_variance(0)<<"</td></tr>";
            break;
        case 2:
            oss<<"<tr><th>"<<QObject::tr("σ̂").toCstr()<<"</th> <td>"<<SW<<point->ellipsoid.get_variance(0)<<"</td> <td>"<<SW<<point->ellipsoid.get_variance(1)<<"</td> <td>--</td></tr>";
            break;
        case 3:
            oss<<"<tr><th>"<<QObject::tr("σ̂").toCstr()<<"</th> <td>"<<SW<<point->ellipsoid.get_variance(0)<<"</td> <td>"<<SW<<point->ellipsoid.get_variance(1)<<"</td> <td>"<<SW<<point->ellipsoid.get_variance(2)<<"</td></tr>";
            break;
        }
    }
    oss<<"</table>\n";

    oss<<"<p>"<<QObject::tr("Nb active obs: ").toCstr()<<point->nbActiveObs<<"</p>";
    if (point->comment.size()>1)
        oss<<"<p>"<<QObject::tr("Comment: ").toCstr()<<point->comment<<"</p>";

    //std::cout<<oss.str()<<"\n";
    return oss.str();
}


void MainTables::setObsActiveStyle(int row, bool active, bool modified)
{
    QFont font = ui->obsTable->font();
    QBrush brush = ui->obsTable->palette().windowText();

    if (! active)
    {
        font.setItalic(true);
        brush = QBrush(Qt::gray);
    }
    for (int i=0;i<ui->obsTable->columnCount();i++)
    {
        ui->obsTable->item(row,i)->setFont(font);
        ui->obsTable->item(row,i)->setForeground(brush);
    }
    if (modified)
        ui->obsTable->item(row,OBS_ACTIVE)->setBackground(QColor(129, 199, 195));
    else
        ui->obsTable->item(row,OBS_ACTIVE)->setBackground(ui->obsTable->item(row,OBS_NUM)->background());

    if (modified) {
        ui->obsTable->item(row,OBS_ACTIVE)->setFlags(ui->obsTable->item(row,OBS_ACTIVE)->flags() & ~Qt::ItemIsSelectable);
    } else {
        ui->obsTable->item(row,OBS_ACTIVE)->setFlags(ui->obsTable->item(row,OBS_ACTIVE)->flags() | Qt::ItemIsSelectable);
        ui->obsTable->item(row,OBS_ACTIVE)->setSelected(ui->obsTable->item(row,OBS_NUM)->isSelected());
    }

    ui->obsTable->item(row,OBS_ACTIVE)->setCheckState(active ? Qt::Checked : Qt::Unchecked);
    QString modifText = modified?tr("\nThis observation has been modified"):"";

    static_cast<DoubleCheckBoxTableWidgetItem*>(ui->obsTable->item(row,OBS_ACTIVE))->secondBool = modified;

    if (active)
        ui->obsTable->item(row,OBS_ACTIVE)->setToolTip(tr("Click to deactivate this observation")+modifText);
    else
        ui->obsTable->item(row,OBS_ACTIVE)->setToolTip(tr("Click to activate this observation")+modifText);
}

void MainTables::setObsActive(Obs* obs, bool active)
{
    if (active == obs->active)
        return;
    obs->active = active;
    if (active) {
        obs->from->nbActiveObs+=obs->numberOfBasicObs();
        if (obs->from != obs->to)
            obs->to->nbActiveObs+=obs->numberOfBasicObs();
    } else {
        obs->from->nbActiveObs-=obs->numberOfBasicObs();
        if (obs->from != obs->to)
            obs->to->nbActiveObs-=obs->numberOfBasicObs();
    }
    if (!showObs) {
        QTableWidgetItem *item = ui->pointsTable->item(obs->from->getPointNumber(),PTS_NBO);
        item->setText(QString("%1").arg(obs->from->nbActiveObs));
        item = ui->pointsTable->item(obs->to->getPointNumber(),PTS_NBO);
        item->setText(QString("%1").arg(obs->to->nbActiveObs));
    }
    for (int rowObs=0; rowObs<ui->obsTable->rowCount(); rowObs++) {
        Obs *o = ui->obsTable->item(rowObs,OBS_NUM)->data(Qt::UserRole).value<Obs*>();
        if (o->from == obs->to || o->from == obs->from) {
            QTableWidgetItem *item = ui->obsTable->item(rowObs,OBS_FROM);
            item->setToolTip(toFromToolTip(o->from).c_str());
        }
        if (o->to == obs->from || o->to == obs->to) {
            QTableWidgetItem *item = ui->obsTable->item(rowObs,OBS_TO);
            item->setToolTip(toFromToolTip(o->to).c_str());
            if (!showCoord) {
                item = ui->obsTable->item(rowObs,OBS_TAR_NBO);
                item->setText(QString("%1").arg(o->to->nbActiveObs));
            }
        }
    }
}


void MainTables::obsClicked(int row, int column)
{
    Obs *obs = ui->obsTable->item(row,OBS_NUM)->data(Qt::UserRole).value<Obs*>();

    switch (column) {
    case OBS_FROM:
    case OBS_TO:
    {
        int pt_idx = column == OBS_FROM ? obs->from->getPointNumber() :  obs->to->getPointNumber();
        for (int i=0; i<ui->pointsTable->rowCount(); i++) {
            QTableWidgetItem *twi = ui->pointsTable->item(i,0);
            if (twi && twi->data(Qt::DisplayRole).toInt() == pt_idx) {
                ui->pointsTable->clearSelection();
                ui->pointsTable->setCurrentCell(i,0);
                twi->setSelected(true);
                ui->pointsTable->scrollToItem(twi);
                return;
            }
        }
        return;
    }
    case OBS_ACTIVE:
        if (!interactive)
                return;
        disableSort();
        setObsActive(obs, ! obs->active);
        setObsActiveStyle(row,obs->active,obs->active!=obs->activeRead);
        enableSort();

        break;
    }
}

void MainTables::openObsFile(QString path)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainTables::setObsSelectionActiveState(int state)
{
    QModelIndexList selection=ui->obsTable->selectionModel()->selectedRows();
    if (selection.empty())
        return;
    disableSort();
    for (const auto& sel : selection) {
        Obs *obs = ui->obsTable->item(sel.row(),0)->data(Qt::UserRole).value<Obs*>();
        switch(state) {
        case 0: setObsActive(obs, false); break;
        case 1: setObsActive(obs, true);  break;
        case 2: setObsActive(obs, !obs->active); ; break;
        }
        setObsActiveStyle(sel.row(),obs->active,obs->active!=obs->activeRead);
    }
    enableSort();
}

void MainTables::obsContextMenu(const QPoint& pos)
{
    QModelIndexList selection=ui->obsTable->selectionModel()->selectedRows();
    QMenu menu(this);
    QAction *action;

    action = menu.addAction(tr("Activate selection"));
    action->setDisabled(selection.empty() || !interactive);
    connect (action,&QAction::triggered,[this](){setObsSelectionActiveState(1);});

    action = menu.addAction(tr("Deactivate selection"));
    action->setDisabled(selection.empty()  || !interactive);
    connect (action,&QAction::triggered,[this](){setObsSelectionActiveState(0);});

    action = menu.addAction(tr("Toggle selection activation"));
    action->setDisabled(selection.empty() || !interactive);
    connect (action,&QAction::triggered,[this](){setObsSelectionActiveState(2);});

    Obs *obs = ui->obsTable->item(ui->obsTable->itemAt(pos)->row(),0)->data(Qt::UserRole).value<Obs*>();
    if (obs->file)
    {
        QString menuText = tr("Open file %1 line %2").arg(obs->file->get_name().c_str()).arg(obs->lineNumber);
        action = menu.addAction(menuText);
        std::cout<<ui->obsTable->itemAt(pos)->row()<<std::endl;
        connect (action,&QAction::triggered,[this, obs](){openObsFile((obs->file->get_path()+"/"+obs->file->get_name()).c_str());});
    }

    menu.exec(ui->obsTable->mapToGlobal(pos));
}

void MainTables::splitterMoved()
{
    bool prevShowCoord=showCoord;
    bool prevShowObs=showObs;
    showCoord = sizes()[0] > 0;
    showObs = sizes()[1] > 0;
    if ((showCoord!=prevShowCoord) || (showObs!=prevShowObs)) {
        int ptsTopRow = ui->pointsTable->itemAt(0,0)->row();
        int obsTopRow = ui->obsTable->itemAt(0,0)->row();
        updateContent();//add or remove cols
        QTimer::singleShot(0,[this,ptsTopRow](){this->ui->pointsTable->scrollToItem(ui->pointsTable->item(ptsTopRow,0),QAbstractItemView::PositionAtTop);});
        QTimer::singleShot(0,[this,obsTopRow ](){this->ui->obsTable->scrollToItem(ui->obsTable->item(obsTopRow ,0),QAbstractItemView::PositionAtTop);
;});
    } else {
        updateGraphics();//only resize cols
    }
    if ((!showCoord) && prevShowCoord) //coord just hidden
        setStyleSheet("QSplitter::handle{image:url(:/gui/splitter_L.svg);}");
    if ((!showObs) && prevShowObs) //obs just hidden
        setStyleSheet("QSplitter::handle{image:url(:/gui/splitter_R.svg);}");
    if ((showCoord && showObs) && ( (!prevShowCoord)||(!prevShowObs))) //all just shown
        setStyleSheet("QSplitter::handle{image:url(:/gui/splitter_M.svg);}");

}

void MainTables::setProject(const Project *aProject)
{
    project = aProject;
}

void MainTables::setAfterCompensation(bool after)
{
    afterCompensation = after;
}

void MainTables::disableInteraction()
{
    interactive = false;
    for (int row=0; row<ui->obsTable->rowCount(); row++) {
        Qt::ItemFlags flag=ui->obsTable->item(row,OBS_ACTIVE)->flags();
        flag &= ~Qt::ItemIsEnabled;
        ui->obsTable->item(row,OBS_ACTIVE)->setToolTip(tr("Waiting for compensation to finish"));
        ui->obsTable->item(row,OBS_ACTIVE)->setFlags(flag);
        ui->obsTable->item(row,OBS_ACTIVE)->setForeground(QBrush(Qt::gray));
    }
}

