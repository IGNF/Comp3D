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

#ifndef CONVERSIONDIALOG_H
#define CONVERSIONDIALOG_H

#include <QDialog>
#include "projection.h"

namespace Ui {
class ConversionDialog;
}

class ConversionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConversionDialog(QWidget *parent);
    ~ConversionDialog();
    void setProj(Projection& _proj);

private:
    Ui::ConversionDialog *ui;
    QString directory;

public slots:
    bool selectCOR();
    bool selectXYZ();
    bool toCartesian();
    bool toProj();
};

#endif // CONVERSIONDIALOG_H
