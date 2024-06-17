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

#ifndef FRAMEWIDGET_H
#define FRAMEWIDGET_H

#include <QGroupBox>
#include "projection.h"
#include "project_config.h"

namespace Ui {
class FrameWidget;
}

class FrameWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit FrameWidget(QWidget *parent = 0);
    ~FrameWidget();
    void importProj(const Projection& _proj);//<use proj to init widget
    bool exportProj(Projection& _proj) const;//<init proj with widget
    void importConfig(const Project_Config &conf);//<use config to init widget
    void exportConfig(Project_Config &conf) const;//<init config with widget
    double getLatitude() const;
    int getLocalCenterX() const;
    int getLocalCenterY() const;
private:
    Ui::FrameWidget *ui;
public slots:
    void updateEnabled();
    void CRSchanged();
    void defChanged();
};

#endif // FRAMEWIDGET_H
