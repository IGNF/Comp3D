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

#ifndef CUSTOMTABLEWIDGETITEM_H
#define CUSTOMTABLEWIDGETITEM_H

#include <QTableWidgetItem>
#include <QProxyStyle>

#define NA "--"

/*****************

custom TableWidgetItem in order to have correct ordering
http://stackoverflow.com/questions/7848683/how-to-sort-datas-in-qtablewidget

*******************/

class AbsFloatTableWidgetItem : public QTableWidgetItem
{
    bool operator <(const QTableWidgetItem &other) const; 
};

class FloatTableWidgetItem : public QTableWidgetItem
{
    bool operator <(const QTableWidgetItem &other) const;
};

class CheckBoxTableWidgetItem : public QTableWidgetItem
{
public:
    CheckBoxTableWidgetItem() : QTableWidgetItem() {}
    bool operator<(const QTableWidgetItem &other) const override;
    enum { AlignmentRole = Qt::UserRole + Qt::CheckStateRole + Qt::TextAlignmentRole };
};

class DoubleCheckBoxTableWidgetItem : public CheckBoxTableWidgetItem //checkbox with an other bool recorded (used for obs is active and is modified)
{
public:
    DoubleCheckBoxTableWidgetItem() : CheckBoxTableWidgetItem(), secondBool(false) {}
    bool operator<(const QTableWidgetItem &other) const override;
    bool secondBool;
};


// Proxy style to center checkbox in CheckBoxTableWidgetItem
// Use:
//   QTableWidgetItem *item = new CheckBoxTableWidgetItem()
//   item->setData(CheckBoxTableWidgetItem::AlignmentRole,Qt::AlignCenter);
class CenteredBoxProxy : public QProxyStyle {
public:
    QRect subElementRect(QStyle::SubElement element, const QStyleOption *option, const QWidget *widget) const override;
};
#endif // CUSTOMTABLEWIDGETITEM_H
