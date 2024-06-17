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

#include "multisorttable.h"
#include <iostream>
#include <QScrollBar>
#include <QHeaderView>

MultiSortTable::MultiSortTable(QWidget *parent):
    QTableWidget(parent),mSizeMinimums({100}),mSizeFactors({1}),sort_info()
{
    connect(this->horizontalHeader(),SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)),this,SLOT(orderChanged(int,Qt::SortOrder)));
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setMinimumSectionSize(0);       // Force use of our own minima
}

bool MultiSortTable::set_size_info(std::vector<double> size_minimums,std::vector<double> size_factors)
{
    if (size_minimums.size()!=size_factors.size())
    {
        std::cerr<<"Error in table resize info! "<<size_minimums.size()<<" "<<size_factors.size()<<std::endl;
        return false;
    }
    mSizeMinimums=size_minimums;
    mSizeFactors=size_factors;
    return true;
}

void MultiSortTable::resize()
{
    if (columnCount()<1)
        return;

    setWordWrap (false);

    double sum_minimums=0;
    unsigned int nb_col_used=columnCount();
    if (mSizeMinimums.size()<nb_col_used)
        nb_col_used=mSizeMinimums.size();

    for (unsigned int i=0;i<nb_col_used;i++)
        sum_minimums+=mSizeMinimums[i];

    double sum_factors=0;
    for (unsigned int i=0;i<nb_col_used;i++)
        sum_factors+=mSizeFactors[i];

    int realWidth=width()-nb_col_used-sum_minimums;

    for (unsigned int i=0;i<nb_col_used-1;i++)
    {
        int col_width = mSizeMinimums.at(i) + mSizeFactors.at(i)*realWidth/sum_factors;
        setColumnWidth(i, col_width);
    }
    setColumnWidth(nb_col_used-1, 1);  // Force auto resize of last column
}

void MultiSortTable::orderChanged(int logicalIndex, Qt::SortOrder order)
{
    //std::cout<<"Sort: "<<logicalIndex<<" - "<<order<<std::endl;
    sort_info.addSort(logicalIndex,order);
}

void MultiSortTable::resort()
{
    disconnect(this->horizontalHeader(),SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)),this,SLOT(orderChanged(int,Qt::SortOrder)));
    //qDebug("Resorting... ");
    for (int i=0;i<nb_sort_saved;i++)
    {
        //qDebug(" - by %d %d",sort_info.logicalIndices[i],sort_info.orders[i]);
        sortByColumn(sort_info.logicalIndices[i],sort_info.orders[i]);
    }
    connect(this->horizontalHeader(),SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)),this,SLOT(orderChanged(int,Qt::SortOrder)));
}
