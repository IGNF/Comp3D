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

#include "ellipsoid.h"
#include <sstream>
#include "project.h"
#include "mathtools.h"

Ellipsoid::Ellipsoid() : matrix(MatX::Zero(3,3)),eigen_values(),azimuts(),sites(),isset(false)
{
}

tdouble Ellipsoid::get_variance(int i) const
{
    if ((i>=0)&&(i<3))
    {
        return sqrt(matrix(i,i));
    }else{
        std::cerr<<"Ellipsoid::get_variance: i must be 0<=i<3"<<std::endl;
        return nan("");
    }
}

tdouble Ellipsoid::get_ellipsAxe(int i) const
{
    if ((i>=0)&&(i<3))
    {
        return eigen_values[i];
    }else{
        std::cerr<<"Ellipsoid::get_ellipsAxe: i must be 0<=i<3"<<std::endl;
        return nan("");
    }
}


std::string Ellipsoid::sigmasToString(tdouble sigma0)
{
    std::ostringstream oss;
    oss.precision(Project::theone()->config.nbDigits+2);
    oss<<std::fixed;
    for (int i=0;i<3;i++)
    {
        oss<<get_variance(i)*sigma0<<" ";
    }
    return oss.str();
}


void Ellipsoid::compute_eigenvalues(tdouble sigma0)
{
    Eigen::EigenSolver<MatX> es(matrix);
#ifdef SHOW_MATRICES
    std::cout << "The eigenvalues of A are:" << std::endl << es.eigenvalues() << std::endl;
    std::cout << "The matrix of eigenvectors, V, is:" << std::endl << es.eigenvectors() << std::endl << std::endl;
    std::cout<<"matrix:\n"<<matrix<<"\n";
#endif
    Eigen::Matrix<std::complex<tdouble>, Eigen::Dynamic, 1> values=es.eigenvalues();
    Eigen::Matrix<std::complex<tdouble>, Eigen::Dynamic, Eigen::Dynamic> vectors=es.eigenvectors();
    for (int i=0;i<3;i++)
    {
            eigen_values[i]=sqrt(values[i].real())*sigma0;
            azimuts[i] = atan2(vectors(0,i).real(),vectors(1,i).real());
            sites[i] = asin(vectors(2,i).real());
    }
    /*if (dimension==1)
    {
        eigen_values[0]=sqrt(values[0].real())*sigma0;
        azimuts[0] = 0;
        sites[0] = PI/2;
    }
    else if (dimension==2)
    {
        for (int i=0;i<dimension;i++)
        {
            eigen_values[i]=sqrt(values[i].real())*sigma0;
            azimuts[i] = atan2(vectors(0,i).real(),vectors(1,i).real());
            sites[i] = 0;
        }
    }
    else if (dimension==3)
    {
        for (int i=0;i<dimension;i++)
        {
                eigen_values[i]=sqrt(values[i].real())*sigma0;
                azimuts[i] = atan2(vectors(0,i).real(),vectors(1,i).real());
                sites[i] = asin(vectors(2,i).real());
        }
    }else{
        std::cout<<"ERRROR: no compute_eigenvalues for this dimension!\n";
    }*/

    //order axis
    tdouble tmp;
    for (int j=0;j<2;j++)
        for (int i=2;i>0;i--)
        {
            if (eigen_values[i]>eigen_values[i-1])
            {
                tmp=eigen_values[i-1];
                eigen_values[i-1]=eigen_values[i];
                eigen_values[i]=tmp;

                tmp=azimuts[i-1];
                azimuts[i-1]=azimuts[i];
                azimuts[i]=tmp;

                tmp=sites[i-1];
                sites[i-1]=sites[i];
                sites[i]=tmp;
            }
        }

    //force azimuts and sites in [0;Pi[
    for (int i=0;i<3;i++)
    {
        if(azimuts[i]<0)
            azimuts[i]+=PI;
        if(azimuts[i]>PI)
            azimuts[i]-=PI;
        if(sites[i]<0)
            sites[i]+=PI;
        if(sites[i]>PI)
            sites[i]-=PI;
    }
    //std::cout<<toString();
    isset=true;
}


Json::Value Ellipsoid::toJson(tdouble sigma0) const
{
    Json::Value val;
    if (isset)
    {
        Json::Value val_matrix;
        for (int i=0;i<3;i++)
            for (int j=i;j<3;j++)
                /*if (isnan(matrix(i,j)))
                    val_matrix.append("nan");
                else*/
                    val_matrix.append((double)matrix(i,j));
        val["matrix"]=val_matrix;
        Json::Value val_half_axes;
        Json::Value val_half_axe[3];
        for (int i=0;i<3;i++)
        {
            val_half_axe[i].append((double)eigen_values[i]);
            val_half_axe[i].append((double)fromRad(azimuts[i],Project::theone()->config.filesUnit));
            val_half_axe[i].append((double)fromRad(sites[i],Project::theone()->config.filesUnit));
            val_half_axes.append(val_half_axe[i]);
        }
        val["axes"]=val_half_axes;

        Json::Value val_sigmas;
        for (int i=0;i<3;i++)
            val_sigmas.append((double)get_variance(i)*sigma0);
        val["sigmas"]=val_sigmas;
    }
    return val;
}

std::string Ellipsoid::toString() const
{
    std::ostringstream oss;
    oss.precision(Project::theone()->config.nbDigits);
    oss<<std::fixed;
    //display
    for (int i=0;i<3;i++)
        oss<<"axe: "<<eigen_values[i]<<" "<<fromRad(azimuts[i],Project::theone()->config.filesUnit)
          <<" "<<fromRad(sites[i],Project::theone()->config.filesUnit)<<std::endl;
    return oss.str();
}


