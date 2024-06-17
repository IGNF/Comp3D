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

#ifndef ERRORDIALOG_H
#define ERRORDIALOG_H

#include <QDialog>
#include <QTimer>

#include "info.h"
#include <sstream>


namespace Ui {
class ErrorDialog;
}

class ErrorDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ErrorDialog(const std::string &textBefore, Info *info, const std::string &textAfter, QWidget *parent = 0);
    ~ErrorDialog();
    
private:
    Ui::ErrorDialog *ui;
    Info *mInfo;
    QTimer *timer;
    double scroll;

public slots:
    void timer_timeout();//a timer is needed just to scroll up after the window is created.

};

#endif // ERRORDIALOG_H
