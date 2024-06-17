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

#ifndef MULTISORTTABLE_H
#define MULTISORTTABLE_H

#include <QTableWidget>

// Use nb_sort_saved sorts at the same time (ex: ordered by obs type then by residual)
const int nb_sort_saved=3;
struct ExtendedSort
{
    int logicalIndices[nb_sort_saved];
    Qt::SortOrder orders[nb_sort_saved];
    ExtendedSort()
    {
        for (int i=0;i<nb_sort_saved;i++)
        {
            logicalIndices[i]=0;
            orders[i]=Qt::AscendingOrder;
        }
    }
    void addSort(int logicalIndex, Qt::SortOrder order)
    {
        for (int i=0;i<nb_sort_saved-1;i++)
        {
            logicalIndices[i]=logicalIndices[i+1];
            orders[i]=orders[i+1];
        }
        logicalIndices[nb_sort_saved-1]=logicalIndex;
        orders[nb_sort_saved-1]=order;
    }
};


class MultiSortTable : public QTableWidget
{
    Q_OBJECT

public:
    explicit MultiSortTable(QWidget *parent = nullptr);
    bool set_size_info(std::vector<double> size_minimums,std::vector<double> size_factors);
    void resize();
    void resort();

protected:
    std::vector<double> mSizeMinimums;
    std::vector<double> mSizeFactors;
    ExtendedSort sort_info;

public slots:
    void orderChanged(int logicalIndex, Qt::SortOrder order);
};

#endif // MULTISORTTABLE_H
