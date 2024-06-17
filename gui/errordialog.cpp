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

#include "errordialog.h"
#include "ui_errordialog.h"

#include <QScrollBar>
#include <QCursor>
#include <iostream>

ErrorDialog::ErrorDialog(const std::string &textBefore, Info *info, const std::string &textAfter, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ErrorDialog),mInfo(info),scroll(0)
{
    ui->setupUi(this);
    ui->messagesTextEdit->setHtml((textBefore+info->toStrFormat()+textAfter).c_str());
    connect(this->ui->okPushButton,SIGNAL(pressed()),this,SLOT(accept()));

    timer=new QTimer(this);
    timer->start(20);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(timer_timeout()));
}

ErrorDialog::~ErrorDialog()
{
    delete timer;
    delete ui;
}

void ErrorDialog::timer_timeout()
{
    scroll+=ui->messagesTextEdit->verticalScrollBar()->maximum()/100.0;
    ui->messagesTextEdit->verticalScrollBar()->setValue(scroll);
    timer->start(5);
    if (scroll>ui->messagesTextEdit->verticalScrollBar()->maximum())
    {
        disconnect(timer, SIGNAL(timeout()), this, SLOT(timer_timeout()));
        delete timer;
        timer=0;
    }
}
