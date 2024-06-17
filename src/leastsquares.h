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

#ifndef LEASTSQUARES_H
#define LEASTSQUARES_H


#include <vector>
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Sparse>
#include "json/json.h"
#include "filerefjson.h"
#include <boost/date_time.hpp>

#include "parameter.h"
#include "compile.h"
#include "project_config.h"
#include "matrixordering.h"



class Project;
class Obs;

//typedef Eigen::SparseMatrix<tdouble,Eigen::RowMajor> SpMat;
//SparseQR is slower than SimplicialCholesky, but it better for least squares, and more stable in our case

//using spSolver = Eigen::SparseQR<SpMat,Eigen::AMDOrdering<int> >;
//using spSolver = Eigen::SparseQR<SpMat,Eigen::COLAMDOrdering<int> >;
//using spSolver = Eigen::SimplicialLDLT<SpMat, Eigen::Lower, Eigen::AMDOrdering<int> >; //fastest
//using spSolver = Eigen::SimplicialLDLT<SpMat, Eigen::Lower, Eigen::NaturalOrdering<int> >;
//using spSolver = Eigen::SparseLU<SpMat,Eigen::NaturalOrdering<int> >;

#ifdef USE_QR_SOLVER
using spSolver = Eigen::SparseQR<SpMat,Eigen::NaturalOrdering<int> >;
#else
using spSolver = Eigen::SimplicialLDLT<SpMat, Eigen::Lower, Eigen::AMDOrdering<int> >;
#endif

/**
  Observations matrix (all_obs) is recorded as a vector of vector of tdouble
  not directly a Eigen matrix since size is unknown until ever obs is used.

  **/

class LeastSquares
#ifdef USE_QT
        : public QObject
#endif
{
#ifdef USE_QT
    Q_OBJECT
#endif
public:

    explicit LeastSquares(Project * _project);

    ~LeastSquares();
    void clear();
    void clear_matrices();

    //add a new record in all_obs vector
    bool add_constraint(Obs *obs, const std::vector<tdouble> &relations,
                        const std::vector<int> &positions, tdouble sigma, bool normalize=false); //< return false if out of system
    bool add_covariance(const std::vector<Obs *> &obs_list, const MatX &sigmas2);//< obs order must be the same as matrix. Do not add variances, obs.sigma_total must be correct!

    bool initialize(COMPUTE_TYPE compute_type, bool internalConstr);

    bool iterate(bool invert, COMPUTE_TYPE compute_type);//call solve() until convergence. Can force some more interations

    void show_info();

    void show_residuals(const SpVect &residuals, const std::string &text);

    bool calculusError() const { return m_calculusError; }
    tdouble finalSigma0() const { return (m_calculusError || all_sigma0.empty()) ? NAN : all_sigma0.back();}

    Json::Value toJson(FileRefJson &filesRef) const;

    Project * project;
    std::vector <Parameter*> all_parameters;
    std::vector <Obs*> all_active_obs;
    static const int B_index=-1;//index of the B par of equation
    static const int no_param_index=-2;//index if not a parameter

    SpMat A,Q,P;//design and weight matrices fill Q, then compute P=inv(Q)
    SpMat AtP;
    SpMat AtPA;//normal matrix (member data to access it in computeInverse())
    MatX AtPA_dense; //used to check if NaN values
    SpVect B;//constants vector
    MatX Qxx;//invAtPA variance-covariance matrix of parameters
    MatX Qll;//compensated variance-covariance matrix of observation

    MatX AtPA_kernel;//kernel of application: base of indeterminations
    Json::Value kernelBaseJson;

    tdouble sigma_0;
    tdouble sigma_0_init;
    bool m_calculusError;//<when computation results are not correct
    std::vector <tdouble> all_sigma0;
    std::vector <tdouble> all_param_init_val;
    std::vector <VectX> all_dX;
    boost::posix_time::ptime computation_start;
    boost::posix_time::ptime computation_end;
    int nbr_iterations;
    unsigned int nbActiveObs;
    bool manual_interrupt;
    bool m_interrupted;//<if computatio interrupted by max iter, no redondancy or user interruption (button)
    bool m_internalConstr;//m_internalConstr is update during initialize(), and used in iterate
    int rankDeficiency;//< if AtPA rank is not equal to cols

protected:
    bool prepareSolve(spSolver &solver);
    bool solve();//one iteration
    bool computeInverse();//not solving, only inv(atpa), if not simul_propagation matrices must be ready
    bool computePfromQ();
    bool create_unknowns();//fill all_parameters looking at points and stations
    bool update_design_matrix(bool initialResidual);//fill matrix using all_obs
    bool update_design_matrix_internalConstr();//fill matrix using obs without ext constr

#ifdef USE_QR_SOLVER
    bool testStability(spSolver &solver);
#endif

    bool computeKernel();//returns true if dim(kern)>0
    void kernelMessage();
    MatrixOrdering matrixOrdering;
    std::vector <std::unique_ptr<Obs> > ICobs;//internal constraints obs
    std::string solverName;

#ifdef USE_QT
signals:
    void iterationDone(int number, double convergence);
    void enterKernel();
    void enterInvert();
#endif

};

#endif // LEASTSQUARES_H
