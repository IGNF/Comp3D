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

#include "customtablewidgetitem.h"

#include <iostream>
#include <cmath>

bool AbsFloatTableWidgetItem::operator <(const QTableWidgetItem &other) const
{
    if (text()==NA) return true;
    if (text().isEmpty())
        return !(other.text()==NA);
    return fabs(text().toFloat()) < fabs(other.text().toFloat());
}

bool FloatTableWidgetItem::operator <(const QTableWidgetItem &other) const
{
    return text().toFloat() < other.text().toFloat();
}

bool CheckBoxTableWidgetItem::operator<(const QTableWidgetItem &other) const
{
    if (checkState() == other.checkState())
        return false;
    if (checkState() != Qt::Checked)
        return false;
    return true;
}

bool DoubleCheckBoxTableWidgetItem::operator<(const QTableWidgetItem &other) const
{
    if (secondBool) return true; //top priority if second bool
    const DoubleCheckBoxTableWidgetItem * other_double = dynamic_cast<const DoubleCheckBoxTableWidgetItem*>(&other);
    if (other_double)
        if (other_double->secondBool) return false;
    return (checkState() < other.checkState());
}

QRect CenteredBoxProxy::subElementRect(QStyle::SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    const QRect baseRes = QProxyStyle::subElementRect(element, option, widget);
    if(element==SE_ItemViewItemCheckIndicator){
        const QStyleOptionViewItem* const itemOpt = qstyleoption_cast<const QStyleOptionViewItem*>(option) ;
        Q_ASSERT(itemOpt);
        const QVariant alignData = itemOpt->index.data(CheckBoxTableWidgetItem::AlignmentRole);
        if(alignData.isNull())
            return baseRes;
        const QRect itemRect = option->rect;
        //            Q_ASSERT(itemRect.width()>baseRes.width() && itemRect.height()>baseRes.height());
        const int alignFlag = alignData.toInt();
        int x=0;
        if(alignFlag & Qt::AlignLeft){
            x=baseRes.x();
        }
        else if(alignFlag & Qt::AlignRight){
            x=itemRect.x() + itemRect.width() - (baseRes.x()-itemRect.x())-baseRes.width();
        }
        else if(alignFlag & Qt::AlignHCenter){
            x=itemRect.x() + (itemRect.width()/2)-(baseRes.width()/2);
        }
        return QRect(QPoint(x,baseRes.y()), baseRes.size());
    }
    return baseRes;
}
