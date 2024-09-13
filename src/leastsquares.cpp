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

#include "leastsquares.h"

#include <iostream>
#include <algorithm>
#include <typeindex>
#include "obs.h"
#include "mathtools.h"
#include "project.h"
#include "compile.h"
#include "point.h"

#ifdef MATRIX_A_IMAGE
    #include <fstream>
#endif

const int LeastSquares::B_index;
const int LeastSquares::no_param_index;

LeastSquares::LeastSquares(Project * _project):
    project(_project),all_parameters(),A(),Q(),P(),B(),sigma_0(NAN),sigma_0_init(NAN),m_calculusError(true),
    computation_start(boost::posix_time::not_a_date_time),computation_end(boost::posix_time::not_a_date_time),
    nbr_iterations(0),nbActiveObs(0),manual_interrupt(false),m_interrupted(false),m_internalConstr(false),
    rankDeficiency(0),matrixOrdering(project),solverName(std::type_index(typeid(spSolver)).name())
{
    std::cout<<"LeastSquares use Solver type "
             <<solverName<<".\n";
}

LeastSquares::~LeastSquares()
{
    clear();
}

void LeastSquares::clear()
{
    computation_start=boost::posix_time::not_a_date_time;
    computation_end=boost::posix_time::not_a_date_time;
    nbr_iterations=0;
    rankDeficiency=0;
    all_parameters.clear();
    all_sigma0.clear();
    all_param_init_val.clear();
    all_dX.clear();
    clear_matrices();
    sigma_0=NAN;
    sigma_0_init=NAN;
    project->invertedMatrix=false;
    project->compensationDone=false;
    project->MonteCarloDone=false;
}

void LeastSquares::clear_matrices()
{
    static unsigned int old_nbActiveObs = 0;
    if (old_nbActiveObs != nbActiveObs)
    {
        old_nbActiveObs = nbActiveObs;
        A.resize(nbActiveObs,all_parameters.size());
        B.resize(nbActiveObs);
        Q.resize(nbActiveObs,nbActiveObs);
        P.resize(nbActiveObs,nbActiveObs);
    }

    for (auto & station : project->stations)
        for (auto & obs : station->observations)
            obs.reset(); //rank=-1 means obs not used in calculation
    all_active_obs.clear();
    ICobs.clear();

    A.setZero();
    B.setZero();
    Q.setZero();
    P.setZero();
}


void LeastSquares::show_info()
{
  #ifdef SHOW_LS
    std::cout<<"all_parameters.size(): "<<all_parameters.size()<<std::endl;
    show_residuals(B,"B");
  #endif
  #ifdef SHOW_MATRICES
    std::cout<<"A:\n"<<MatX(A)<<std::endl;
    std::cout<<"B:\n"<<MatX(B)<<std::endl;
    std::cout<<"Q:\n"<<MatX(Q)<<std::endl;
    std::cout<<"P:\n"<<MatX(P)<<std::endl;
    //show_residuals(B,"B");
    //std::cout<<"invAtPA:\n"<<invAtPA<<std::endl;
  #endif
}

//update rank of every parameter
bool LeastSquares::create_unknowns()
{
    if (project->points.size()==0) return false;

    unsigned int current_rank=0;

    all_parameters.clear();
    nbActiveObs = project->updateNumberOfActiveObs();
  #ifdef REORDER
    //compute ordering...
    matrixOrdering.computeRelations();
    matrixOrdering.orderRelations();
    std::vector<Point*> pointsDefaultOrder;
    pointsDefaultOrder.reserve(project->points.size());
    for (auto & pt: project->points) //TODO: why transform not working?
        pointsDefaultOrder.push_back(&pt);
    //std::transform(project->points.begin(), project->points.end(), pointsDefaultOrder.begin(), [](auto &x){return &x;});
    std::vector<Point*> pointsNewOrder;
    pointsNewOrder.reserve(project->points.size());
    for (unsigned int i=0;i<pointsDefaultOrder.size();i++)
        pointsNewOrder.push_back(pointsDefaultOrder[matrixOrdering.newOrder[i]]);
  #else
    std::vector<Point*> pointsNewOrder;
    std::transform(project->points.begin(), project->points.end(),
                   back_inserter(pointsNewOrder), [](auto &p) { return &p; });
  #endif
    for (auto & point: pointsNewOrder)
    {
        for (auto &param:point->params)
        {
            if (param.rank==LeastSquares::no_param_index)
                continue;//not a parameter to optimize

            param.rank=current_rank++;
            all_parameters.push_back(&param);
            //std::cout<<"Unknown rank "<<param.name<<": "<<param.rank<<std::endl;
        }
        for (auto &station:point->stations_) //add stations param just after point params to imporve matrix order
            for (auto &param:station->params)
            {
                param.rank=current_rank++;
                all_parameters.push_back(&param);
                //std::cout<<"Pt "<<point->name<<" station: ";
                //std::cout<<"Unknown "<<param.name<<" rank: "<<param.rank<<std::endl;
            }
    }
    for (auto &station:project->stations) //add stations param for non point-based stations
        if (!station->origin())
            for (auto &param:station->params)
            {
                param.rank=current_rank++;
                all_parameters.push_back(&param);
                //std::cout<<"Unknown "<<param.name<<" rank: "<<param.rank<<std::endl;
            }
    std::cout<<QObject::tr("Total: ").toCstr()<<current_rank<<QObject::tr(" unknowns.").toCstr()<<std::endl;
    return true;
}

bool LeastSquares::initialize(COMPUTE_TYPE compute_type, bool internalConstr)
{
    //no call to clear() for Monte-Carlo
    project->invertedMatrix=false;
    m_internalConstr=internalConstr;
    all_param_init_val.clear();
    all_param_init_val.reserve(all_parameters.size());
    if (!create_unknowns()) return false;

    if ((compute_type==COMPUTE_TYPE::type_propagation)&&(internalConstr))
    {

        if (!update_design_matrix_internalConstr())
        {
            Project::theInfo()->error(INFO_LS,1,QT_TRANSLATE_NOOP("QObject","Error on observations, "
                                                                            "compensation canceled."));
            m_calculusError = true;
            return false;
        }
    }else{
        if (!update_design_matrix(true))
        {
            Project::theInfo()->error(INFO_LS,1,QT_TRANSLATE_NOOP("QObject","Error on observations, "
                                                                            "compensation canceled."));
            m_calculusError = true;
            return false;
        }
    }

    std::cout<<"A->innerSize(): "<<A.innerSize()<<std::endl;
    std::cout<<"A->outerSize(): "<<A.outerSize()<<std::endl;

    if (compute_type==COMPUTE_TYPE::type_propagation)
        sigma_0_init=1.0;
    else if (nbActiveObs<=all_parameters.size())
        sigma_0_init=NAN;
    else
    {
        MatX sigma0_mat=MatX(MatX(B).transpose()*P*B);
        if (compute_type==COMPUTE_TYPE::type_monte_carlo)
            sigma_0_init=sqrt(sigma0_mat(0,0)/WEIGHT_FACTOR/(nbActiveObs));//residuals are before iteration
        else
            sigma_0_init=sqrt(sigma0_mat(0,0)/WEIGHT_FACTOR/(nbActiveObs-all_parameters.size()));
    }

    for (auto &param: all_parameters)
        all_param_init_val.push_back(*param->value);

    if (all_sigma0.empty()) //initialize is called on each Monte-Carlo iteration
    {
        std::cout<<"sigma_0_init="<<sigma_0_init<<std::endl;
        all_sigma0.push_back(sigma_0_init);
    }
    return true;
}

bool LeastSquares::update_design_matrix(bool initialResidual)
{
    bool ok=true;

    clear_matrices();

    //set every observation
    for (auto & station : project->stations)
        ok = station->set_obs(this,initialResidual,false) && ok;
    if (project->coord_cov)
        ok = add_covariance(project->coord_cov->getObservations(), project->coord_cov->getMatrix()) && ok; //TODO: make ok as a LeastSquares attribute to update it automatically
    if (!ok) {
        Project::theInfo()->error(INFO_LS,1,QT_TRANSLATE_NOOP("QObject","Error setting observations."));
        return ok;
    }

#ifdef MATRIX_A_IMAGE
    if (initialResidual)
    {
        std::ofstream fileout;
        fileout.open(project->filename + "_matrixAimage.pbm");
        fileout << "P1\n";
        fileout << "# Matrix A representation:\n";
        fileout << "# null val -> 0 (white) & non-null val -> 1 (black)\n";

        fileout << "########### columns: all_parameters ##########\n";
        auto n_param = 0;
        for (auto &param:this->all_parameters)
        {
            fileout << "#   col " << n_param << ": " << param->name << "\n";
            ++n_param;
        }
        fileout << "########### rows: all_active_obs #############\n";
        auto n_obs = 0;
        for (auto &obs:this->all_active_obs)
        {
            fileout << "#   row " << n_obs << ": obs num " << obs->toString() << "\n";
            ++n_obs;
        }
        fileout << "##############################################\n";

        fileout << this->A.cols() << " " << this->A.rows() << "\n";
        auto n = 0;
        for (int i=0; i< this->A.rows(); ++i){
            for(int j=0; j < this->A.cols(); ++j)
            {
                ++n;
                // 1 for non-null values & 0 for null values
                auto val = (this->A.coeff(i,j)!=0.0) ? 1 : 0;
                fileout << val << " ";
                if ((n%30) == 0)
                    fileout << "\n";
             }
        }
        fileout.close();
    }
#endif

    ok = ok & computePfromQ();
    return ok;
}

bool LeastSquares::computePfromQ()
{
    std::cout<<QObject::tr("Compute").toCstr()<<" P..."<<std::endl;
    Eigen::SimplicialLDLT<SpMat> solverQ;
    solverQ.compute(Q);
    if(solverQ.info()!=Eigen::Success) {
      Project::theInfo()->error(INFO_LS,1,QT_TRANSLATE_NOOP("QObject","Error: Q matrix decomposition failed!"));
      return false;
    }
    SpMat IQ(Q.rows(),Q.rows());
    IQ.setIdentity();
    P = solverQ.solve(IQ);
    if(solverQ.info()!=Eigen::Success) {
      Project::theInfo()->error(INFO_LS,1,QT_TRANSLATE_NOOP("QObject","Error: P matrix solving failed!"));
      return false;
    }
    return true;
}

//TODO: call update_design_matrix
bool LeastSquares::update_design_matrix_internalConstr()
{
    bool ok=true;

    bool has_bubble=false;
    bool has_hz=false;
    bool has_dist=false;
    bool only_levelling=true;
    bool only_plani=true;

    for (auto & point : project->points)
    {
        if (point.dimension!=2)
            only_plani=false;
    }
    for (auto & station : project->stations)
        for (auto & obs : station->observations)
            if (obs.active)
            {
                if (!obs.isInternal())
                    continue;
                only_levelling = only_levelling && obs.isOnlyLeveling();
                has_bubble|=obs.isBubbuled();
                has_hz|=obs.isHz();
                has_dist|=obs.isDistance();
            }

    if (only_plani) has_bubble=true;

    nbActiveObs = project->updateNumberOfActiveObs(true);
    //count the internal constraints
    if (only_levelling) nbActiveObs+=1;//only sum(dz)=0 to add
    else if (only_plani) nbActiveObs+=3;//only sum(dx)=0 sum(dx)=0 rot(z) to add
    else
    {
        nbActiveObs+=7;//max 7 constraints to add
        if (has_bubble) nbActiveObs-=2;
        if (has_hz) nbActiveObs-=1;
        if (has_dist) nbActiveObs-=1;
    }


    std::cout<<"Number of internal active obs: "<<nbActiveObs<<std::endl;
    std::cout<<"has_bubble: "<<has_bubble<<std::endl;
    std::cout<<"has_hz: "<<has_hz<<std::endl;
    std::cout<<"has_dist: "<<has_dist<<std::endl;
    std::cout<<"only_levelling: "<<only_levelling<<std::endl;
    std::cout<<"only_plani: "<<only_plani<<std::endl;

    clear_matrices();

    //set every observation
    for (auto & station : project->stations)
        ok = station->set_obs(this,false,true) && ok;

    if (!ok)
    {
        Project::theInfo()->error(INFO_LS,1,QT_TRANSLATE_NOOP("QObject","Error setting observations."));
        return ok;
    }

    std::cout<<"Add the internal constraints obs"<<std::endl;

    std::vector<tdouble> relations;
    std::vector<int> positions;

    //compute barycenter
    Coord barycenter(0,0,0);
    for (auto & point : project->points)
    {
        barycenter+=point.coord_comp;
    }
    barycenter/=(tdouble)project->points.size();


    if (!only_plani)
    {
        //add sum(dz)=0
        relations.resize(0);
        relations.push_back(0);
        positions.resize(0);
        positions.push_back(B_index);
        for (auto & point : project->points)
        {
            if (point.code>=CR_CODE::PLANI_FAR_FREE)
                continue;
            relations.push_back(1);
            positions.push_back(point.params.at(2).rank);
        }
        ICobs.push_back(std::make_unique<Obs>(nullptr,nullptr,nullptr,OBS_CODE::INTCONST_DZ,true,0,
                                              INTERNALCONSTR_SIGMA,0,0,0,0,1,"",nullptr,""));
        add_constraint(ICobs.back().get(),relations,positions,INTERNALCONSTR_SIGMA,true);
    }

    if (!only_levelling)
    {
        //add sum(dx)=0
        relations.resize(0);
        relations.push_back(0);
        positions.resize(0);
        positions.push_back(B_index);
        for (auto & point : project->points)
        {
            if (point.code>=CR_CODE::PLANI_FAR_FREE)
                continue;
            if (point.dimension>1)
            {
                relations.push_back(1);
                positions.push_back(point.params.at(0).rank);
            }
        }
        ICobs.push_back(std::make_unique<Obs>(nullptr,nullptr,nullptr,OBS_CODE::INTCONST_DX,true,0,
                                              INTERNALCONSTR_SIGMA,0,0,0,0,1,"",nullptr,""));
        add_constraint(ICobs.back().get(),relations,positions,INTERNALCONSTR_SIGMA,true);

        //add sum(dy)=0
        relations.resize(0);
        relations.push_back(0);
        positions.resize(0);
        positions.push_back(B_index);
        for (auto & point : project->points)
        {
            if (point.code>=CR_CODE::PLANI_FAR_FREE)
                continue;
            if (point.dimension>1)
            {
                relations.push_back(1);
                positions.push_back(point.params.at(1).rank);
            }
        }
        ICobs.push_back(std::make_unique<Obs>(nullptr,nullptr,nullptr,OBS_CODE::INTCONST_DY,true,0,
                                              INTERNALCONSTR_SIGMA,0,0,0,0,1,"",nullptr,""));
        add_constraint(ICobs.back().get(),relations,positions,INTERNALCONSTR_SIGMA,true);

        if (!has_hz)
        {
            //add d rotationZ=0
            relations.resize(0);
            relations.push_back(0);
            positions.resize(0);
            positions.push_back(B_index);
            for (auto & point : project->points)
            {
                if (point.code>=CR_CODE::PLANI_FAR_FREE)
                    continue;
                if (point.dimension>1)
                {
                    relations.push_back(point.coord_comp.y()-barycenter.y());
                    positions.push_back(point.params.at(0).rank);
                    relations.push_back(-(point.coord_comp.x()-barycenter.x()));
                    positions.push_back(point.params.at(1).rank);
                }
            }
            ICobs.push_back(std::make_unique<Obs>(nullptr,nullptr,nullptr,OBS_CODE::INTCONST_RZ,true,0,
                                                  INTERNALCONSTR_SIGMA,0,0,0,0,1,"",nullptr,""));
            add_constraint(ICobs.back().get(),relations,positions,INTERNALCONSTR_SIGMA,true);
        }

        if (!has_bubble)
        {
            //add d rotationX=0
            relations.resize(0);
            relations.push_back(0);
            positions.resize(0);
            positions.push_back(B_index);
            for (auto & point : project->points)
            {
                if (point.code>=CR_CODE::PLANI_FAR_FREE)
                    continue;
                if (point.dimension==3)
                {
                    relations.push_back(point.coord_comp.z()-barycenter.z());
                    positions.push_back(point.params.at(1).rank);
                    relations.push_back(-(point.coord_comp.y()-barycenter.y()));
                    positions.push_back(point.params.at(2).rank);
                }
            }
            ICobs.push_back(std::make_unique<Obs>(nullptr,nullptr,nullptr,OBS_CODE::INTCONST_RX,true,0,
                                                  INTERNALCONSTR_SIGMA,0,0,0,0,1,"",nullptr,""));
            add_constraint(ICobs.back().get(),relations,positions,INTERNALCONSTR_SIGMA,true);

            //add d rotationY=0
            relations.resize(0);
            relations.push_back(0);
            positions.resize(0);
            positions.push_back(B_index);
            for (auto & point : project->points)
            {
                if (point.code>=CR_CODE::PLANI_FAR_FREE)
                    continue;
                if (point.dimension==3)
                {

                    relations.push_back(-(point.coord_comp.z()-barycenter.z()));
                    positions.push_back(point.params.at(0).rank);
                    relations.push_back(point.coord_comp.x()-barycenter.x());
                    positions.push_back(point.params.at(2).rank);
                }
            }
            ICobs.push_back(std::make_unique<Obs>(nullptr,nullptr,nullptr,OBS_CODE::INTCONST_RY,true,0,
                                                  INTERNALCONSTR_SIGMA,0,0,0,0,1,"",nullptr,""));
            add_constraint(ICobs.back().get(),relations,positions,INTERNALCONSTR_SIGMA,true);

        }

        if (!has_dist)
        {
            //add d scale=0
            relations.resize(0);
            relations.push_back(0);
            positions.resize(0);
            positions.push_back(B_index);
            for (auto & point : project->points)
            {
                if (point.code>=CR_CODE::PLANI_FAR_FREE)
                    continue;
                if (point.dimension==3)
                {
                    relations.push_back(point.coord_comp.x()-barycenter.x());
                    positions.push_back(point.params.at(0).rank);
                    relations.push_back(point.coord_comp.y()-barycenter.y());
                    positions.push_back(point.params.at(1).rank);
                    relations.push_back(point.coord_comp.z()-barycenter.z());
                    positions.push_back(point.params.at(2).rank);
                }
            }
            ICobs.push_back(std::make_unique<Obs>(nullptr,nullptr,nullptr,OBS_CODE::INTCONST_SC,true,0,
                                                  INTERNALCONSTR_SIGMA,0,0,0,0,1,"",nullptr,""));
            add_constraint(ICobs.back().get(),relations,positions,INTERNALCONSTR_SIGMA,true);
        }
    }

    ok = ok & computePfromQ();
    return ok;
}


bool LeastSquares::computeInverse()
{
    std::cout<<"computeInverse"<<std::endl;
#ifdef USE_QT
    emit enterInvert();
#endif
    spSolver solver;
    if (!prepareSolve(solver))
        return false;

  #ifdef SPARSE_INVERT
    //TODO: make it work!
    /*
    //compute inverse with sparse matrix <=> solve A*X=I
    SpMat I(all_parameters.size(),all_parameters.size());
    I.setIdentity();
    Eigen::Solve< Eigen::SparseQR, Eigen::Rhs > invAtPA_sparse=solver.solve(I);
    std::cout<<"invAtPA_sparse:****\n"<<invAtPA_sparse<<std::endl;
    std::cout<<"****"<<std::cout;
    //invAtPA= MatX(invAtPA_sparse);
    MatX AtPA_dense = MatX(AtPA);
    invAtPA=AtPA_dense.inverse();*/
  #else
    /*std::cout<<QObject::tr("Compute").toCstr()<<" det(AtPA)..."<<std::endl;
    tdouble det=AtPA_dense.determinant();
    std::cout<<"det(AtPA)="<<det<<std::endl;
    if ((fabs(det)<0.000001)||(det==neginfinity))
    {
        std::cout<<"ERROR: system can't be solved!"<<std::endl;
        error_msg<<QObject::tr("ERROR: system can't be solved!").toCstr()<<std::endl;
        return false;
    }*/
    std::cout<<QObject::tr("Compute").toCstr()<<" Qxx=inv(AtPA)..."<<std::endl;
    Qxx=AtPA_dense.inverse();
    //other method using SPD: faster but a little less accurate...
    //MatX I = MatX::Identity(AtPA_dense.rows(), AtPA_dense.cols());
    //Qxx=AtPA_dense.llt().solve(I);

    if (!Qxx.allFinite())
    {
        Project::theInfo()->error(INFO_LS,1,QT_TRANSLATE_NOOP("QObject","Qxx has infinite values!"));
        return false;
    }

  #ifdef SHOW_MATRICES
    std::cout<<"Qxx:\n"<<Qxx<<std::endl;
    {
        MatX inv_Qxx=Qxx.inverse();
        //MatX inv_Qxx=Qxx.llt().solve(I);
        std::cout<<"inv_Qxx:\n"<<inv_Qxx<<std::endl;
        std::cout<<"AtPA - inv_Qxx:\n"<<AtPA - inv_Qxx<<std::endl;
    }
  #endif

    std::cout<<QObject::tr("Compute").toCstr()<<" Qll=A*Qxx*At..."<<std::endl;
    Qll=A*Qxx*(A.transpose());

    /*std::cout<<"A:\n"<<MatX(*A)<<std::endl;
    std::cout<<"Qxx:\n"<<Qxx<<std::endl;
    std::cout<<"Qll:\n"<<Qll<<std::endl;*/

    //compute standard residuals for each obs
    for (auto & obs : all_active_obs)
    {
        obs->sigmaAposteriori=sqrt(Qll(obs->getObsRank(),obs->getObsRank())*WEIGHT_FACTOR)/obs->appliedNormalisation;
        obs->residualStd=obs->residual/obs->sigmaAposteriori;
        obs->obsRedondancy=100*(1-sqr(obs->sigmaAposteriori/obs->sigmaTotal));
        if (obs->obsRedondancy>1)//at least a little redondancy is required
        {
            obs->sigmaResidual=sqrt(sqr(obs->sigmaTotal)-sqr(obs->sigmaAposteriori));
            obs->standardizedResidual=obs->residual/obs->sigmaResidual;
        }else{
            obs->sigmaResidual=NAN;//undefined
            obs->standardizedResidual=NAN;//undefined
        }
    }

 /*#ifdef SHOW_MATRICES
    std::cout<<"AtPA_dense:\n"<<AtPA_dense<<std::endl;
    std::cout<<"Qxx:\n"<<Qxx<<std::endl;
 #endif*/

 #endif
    project->invertedMatrix=true;
    return true;
}


bool LeastSquares::prepareSolve(spSolver &solver)
{
    std::cout<<QObject::tr("Compute").toCstr()<<" AtP..."<<std::endl;
    AtP=(A.transpose())*(P);//reduce computation duration by 30%!
    std::cout<<QObject::tr("Compute").toCstr()<<" AtPA..."<<std::endl;
    AtPA=AtP*(A);
    AtPA_dense = MatX(AtPA);
    if (!AtPA_dense.allFinite()) //TODO: make this verification in add_constraint to avoid creating dense AtPA?
    {
        Project::theInfo()->error(INFO_LS,1,
                                  QT_TRANSLATE_NOOP("QObject","Impossible to solve system "
                                                              "(not all values are finite)!"));
        return false;
    }

#ifdef SHOW_MATRICES
    //std::cout<<"AtP:\n"<<MatX(AtP)<<std::endl;
    //std::cout<<"AtPA:\n"<<MatX(AtPA)<<std::endl;
    //std::cout<<"B:\n"<<MatX(*B)<<std::endl;
    //std::cout<<"AtPB:\n"<<MatX(AtPB)<<std::endl;
#endif

    std::cout<<QObject::tr("Compute SparseQR decomposition...").toCstr()<<std::endl;

    solver.compute(AtPA);
    if(solver.info()!=Eigen::Success) {
        computeKernel();
        Project::theInfo()->error(INFO_LS,1,
                                  QT_TRANSLATE_NOOP("QObject","Normal matrix stability test failed!"));
        return false;
    }
#ifdef USE_QR_SOLVER
    if (!testStability(solver))
    {
        Project::theInfo()->error(INFO_LS,1,
                                  QT_TRANSLATE_NOOP("QObject","Normal matrix stability test failed!"));
        return false;
    }
#endif

    return true;
}

bool LeastSquares::solve()
{
    spSolver solver;
    if (!prepareSolve(solver))
        return false;
    std::cout<<QObject::tr("Compute").toCstr()<<" AtPB..."<<std::endl;
    VectX AtPB(AtP*B);

    std::cout<<QObject::tr("Compute SparseQR solving...").toCstr()<<std::endl;
    VectX dX = solver.solve(AtPB);
    all_dX.push_back(dX);

    if(solver.info()!=Eigen::Success) {
        // solving failed
        Project::theInfo()->error(INFO_LS,1,QT_TRANSLATE_NOOP("QObject","SparseQR solving failed."));
        return false;
    }

    //apply dX
    for (unsigned int i=0;i<all_parameters.size();i++)
    {
      #ifdef SHOW_LS
        std::cout<<"Param "<<all_parameters.at(i)->name<<": "<<*(all_parameters.at(i)->value)<<"+"<<-dX(i,0)<<std::endl;
      #endif
        *(all_parameters.at(i)->value)-=dX(i);
    }

    //test if points are too far
    for (auto &point:project->points)
        if ((fabs(point.coord_comp.x())>MAX_HZ_SIZE_M)||(fabs(point.coord_comp.y())>MAX_HZ_SIZE_M))
            Project::theInfo()->error(INFO_LS,1,
                                      QT_TRANSLATE_NOOP("QObject","Point %s got too far away..."),
                                      point.name.c_str());

  #ifdef SHOW_LS
    //Eigen::MatrixXld residuals=(*B)-(*A)*dX;
    //std::cout<<"residuals:\n"<<residuals<<std::endl;
    //show_residuals(&residuals,"linearized");
  #endif

    return true;
}

bool LeastSquares::iterate(bool invert, COMPUTE_TYPE compute_type)
{
    if (all_sigma0.empty())
    {
        Project::theInfo()->error(INFO_LS,1,
                                  QT_TRANSLATE_NOOP("QObject","Error: LeastSquares not initialized!"));
        return false;
    }

    computation_start=boost::posix_time::microsec_clock::local_time();
    m_interrupted=false;
    manual_interrupt=false;
    tdouble convergence=project->config.convergenceCriterion;
    int force_iter=project->config.forceIterations;

    nbr_iterations=0;
    all_sigma0.resize(1);//keep sigma0 init
    all_dX.clear();
    tdouble previous_sigma_0=all_sigma0.back();
    sigma_0=previous_sigma_0;

    m_calculusError=false;
    bool began_forced_iterations=false;
    bool internalConstr_applied=!m_internalConstr;

    if (A.cols()==0)
    {
        Project::theInfo()->warning(INFO_LS,1,
                                    QT_TRANSLATE_NOOP("QObject","No parameters to optimize."));
        computation_end=boost::posix_time::microsec_clock::local_time();
        //do this before emit computationDone
        project->compensationDone=(compute_type==COMPUTE_TYPE::type_compensation);
        return true;
    }

    //if propagation simulation, do only one iteration
    if (compute_type==COMPUTE_TYPE::type_propagation)
    {
        m_calculusError=!computeInverse();
        computation_end=boost::posix_time::microsec_clock::local_time();
        if (m_calculusError)
            m_interrupted = true;
        //std::cout<<error_msg.str()<<std::flush;
        //project->outputConversion();
        m_calculusError = m_calculusError || !project->updateEllipsoids();
        return !m_calculusError;
    }

    do
    {
        //if Monte-Carlo simulation, reset points positions
        if (compute_type==COMPUTE_TYPE::type_monte_carlo)
            project->set_least_squares(false);
      
        if (nbr_iterations>=project->config.maxIterations)
        {
            if (compute_type!=COMPUTE_TYPE::type_monte_carlo)
            {
                Project::theInfo()->warning(INFO_LS,1,
                                            QT_TRANSLATE_NOOP("QObject","Maximum iterations reached, "
                                                                        "computation interrupted."));
                m_interrupted=true;
            }
            break;
        }
        nbr_iterations++;
        std::cout<<"------------- Iteration "<<nbr_iterations<<" -------------"<<std::endl;

        show_info();

        //test if internalConstr_applied (to end loop)
        if ((force_iter<0) && (!internalConstr_applied))
            internalConstr_applied=true;

        if (!solve())
        {
            m_calculusError=true;
            m_interrupted=true;
            break;
        }

        //update all stations with new params values
        for (auto &station:project->stations)
            station->update();

        //if Monte-Carlo simulation, update data
        if (compute_type==COMPUTE_TYPE::type_monte_carlo)
            for (auto & point : project->points)
                point.update_MonteCarlo();


        //update design matrix for next iteration and to get new sigma0
        if (!update_design_matrix(false))
        {
            Project::theInfo()->error(INFO_LS,1,
                                      QT_TRANSLATE_NOOP("QObject","Error on observations, compensation canceled."));
            m_calculusError = true;
            return false;
        }

        previous_sigma_0=sigma_0;


        if (compute_type==COMPUTE_TYPE::type_propagation)
            sigma_0=1.0;
        else if (nbActiveObs<=all_parameters.size())
            sigma_0=NAN;
        else
        {
            MatX sigma0_mat=MatX(MatX(B).transpose()*P*B);
        #ifdef SHOW_MATRICES
            std::cout<<"BtPB:\n"<<MatX(sigma0_mat)<<std::endl;
        #endif
            if (compute_type==COMPUTE_TYPE::type_monte_carlo)
                sigma_0=sqrt(sigma0_mat(0,0)/WEIGHT_FACTOR/(nbActiveObs));//residuals are before iteration
            else
                sigma_0=sqrt(sigma0_mat(0,0)/WEIGHT_FACTOR/(nbActiveObs-all_parameters.size()));
        }

        std::cout<<"sigma_0="<<sigma_0<<std::endl;

        all_sigma0.push_back(sigma_0);

        tdouble newConvergence=((previous_sigma_0-sigma_0)/sigma_0);
        if (!std::isfinite(newConvergence))
            newConvergence=0.0;

        std::cout<<QObject::tr("Convergence: ").toCstr()<<newConvergence<<std::endl;

        if ((compute_type==COMPUTE_TYPE::type_compensation) && (!std::isfinite(sigma_0)))//only for compensation, to continue Monte-Carlo without redondancy
        {
            m_interrupted=true;
            if (!began_forced_iterations)
            {
                if (nbActiveObs-all_parameters.size()==0)
                    Project::theInfo()->error(INFO_LS,1,
                                              QT_TRANSLATE_NOOP("QObject","No redondancy!"));
                Project::theInfo()->error(INFO_LS,1,
                                          QT_TRANSLATE_NOOP("QObject","sigma_0 is NaN, end of normal iterations."));
            }else{
                Project::theInfo()->error(INFO_LS,1,
                                          QT_TRANSLATE_NOOP("QObject","sigma_0 is NaN."));
            }
            //force_iter=-1;
            //error=true; //let's say no redondancy is not an error, it stops iterations, but outputs compensated coordinates
        }

        if (((compute_type==COMPUTE_TYPE::type_compensation) && (nbr_iterations>1) && (!began_forced_iterations)
             &&(fabs(newConvergence)<convergence)) || (!std::isfinite(sigma_0)))
        {
            std::cout<<QObject::tr("Convergence reached!").toCstr()<<std::endl;
            began_forced_iterations=true;
        }

        if ((compute_type==COMPUTE_TYPE::type_compensation) && began_forced_iterations && (fabs(newConvergence)>convergence))
        {
            std::cout<<QObject::tr("Convergence unreached! Continue iterations...").toCstr()<<std::endl;
            began_forced_iterations = false;
            force_iter=project->config.forceIterations;
        }

        if (began_forced_iterations)
            force_iter--;

        //test if last iteration with internal constraints
        if ((force_iter<0) && (!internalConstr_applied))
        {
            //asked to use internal constraints
            //on last iterations, change observations

            if (!update_design_matrix_internalConstr())
            {
                Project::theInfo()->error(INFO_LS,1,
                                          QT_TRANSLATE_NOOP("QObject","Error on observations, compensation canceled."));
                //std::cerr<<error_msg.str()<<std::endl;
                m_calculusError = true;
                return false;
            }
            internalConstr_applied=true;
            show_info();
        }


        #ifdef USE_QT
            emit iterationDone(nbr_iterations,newConvergence);
        #endif
    }while( (!manual_interrupt) && !((force_iter<0) && internalConstr_applied) );

    if (manual_interrupt)
    {
        m_interrupted=true;
        m_calculusError=true;
        Project::theInfo()->warning(INFO_LS,1,
                                    QT_TRANSLATE_NOOP("QObject","Computation manually interrupted."));
    }

    //if Monte-Carlo simulation, update data
    if ((!m_calculusError) && (compute_type==COMPUTE_TYPE::type_monte_carlo))
    {
        project->MonteCarloDone = true;
        for (auto & point : project->points)
            point.finish_MonteCarlo(nbr_iterations);
    }

    if (!m_calculusError)
    {
        if (invert)
        {
            m_calculusError=!computeInverse();
            if (m_calculusError)
                m_interrupted=true;
        }
    }

    computation_end=boost::posix_time::microsec_clock::local_time();

    //std::cout<<error_msg.str()<<std::flush;

    //do this before emit computationDone
    project->compensationDone=(!m_calculusError)&&(compute_type==COMPUTE_TYPE::type_compensation);
    if (!m_calculusError)
    {
        project->outputConversion();
        if (invert)
        {
            m_calculusError = m_calculusError || !project->updateEllipsoids();
        }
    }
    return !m_calculusError;
}


//add new constraints to A, B and P matrices
//returns false if out of system
//normalize is used only for internal constraints, since relations can be very big
bool LeastSquares::add_constraint(Obs* obs, const std::vector<tdouble>& relations,
                                  const std::vector<int> & positions,tdouble sigma, bool normalize)
{
  #ifdef SHOW_MATRICES
    std::cout<<"Add constraint: "<<obs->toString()<<"  ->  "<<std::endl;
  #endif
    bool has_B_val=false;
    tdouble B_value;

    if (relations.size()!=positions.size())
    {
        std::cout<<"Error in observation setting (relations.size()!=positions.size())."<<std::endl;
        Project::theInfo()->error(INFO_OBS,1,
                                  QT_TRANSLATE_NOOP("QObject","Error in observation setting "
                                                              "(relations.size()!=positions.size()): %s"),
                                  obs->toString().c_str());
        return false;
    }
    if (sigma<=0)
    {
        std::cout<<"Error in observation setting (sigma<=0)."<<std::endl;
        Project::theInfo()->error(INFO_OBS,1,
                                  QT_TRANSLATE_NOOP("QObject","Error in observation setting "
                                                              "(sigma negative: %f): %s"),
                                  sigma,obs->toString().c_str());
        return false;
    }
    //test if all finite
    for (auto &v:relations)
    {
        if (!std::isfinite(v))
        {
            Project::theInfo()->error(INFO_OBS,1,
                                      QT_TRANSLATE_NOOP("QObject","Error in observation setting (not finite value): %s"),
                                      obs->toString().c_str());
            return false;
        }
    }

    if (normalize)
    {
        tdouble norm=0;
        for (auto &v:relations)
            norm+=v*v;
        norm=sqrt(norm);
        if (norm<1e-6)
        {
            std::cout<<"Error: observation norm too low."<<std::endl;
            Project::theInfo()->error(INFO_OBS,1,
                                      QT_TRANSLATE_NOOP("QObject","Error: observation norm too low (%f): %s"),
                                      norm,obs->toString().c_str());

            return false;
        }
        obs->appliedNormalisation=norm;
        //std::cout<<"norm: "<<obs->appliedNormalisation<<"\n";
        sigma*=obs->appliedNormalisation;
    }

    for (unsigned int i=0;i<relations.size();i++)
    {
      #ifdef SHOW_MATRICES
        std::cout<<"("<<positions.at(i)<<": "<<relations.at(i)<<") ";
      #endif

        if (positions.at(i)==no_param_index)
            continue;

        if (positions.at(i)==B_index)
        {
            has_B_val=true;
            B_value=relations.at(i);
            continue;
        }

        if ((positions.at(i)<0)||(positions.at(i)>=(int)all_parameters.size()))
        {
            std::cout<<"Error in observation setting (positions out of model)."<<std::endl;
            Project::theInfo()->error(INFO_OBS,1,
                                      QT_TRANSLATE_NOOP("QObject","Error in observation setting "
                                                                  "(positions out of model): %s"),
                                      obs->toString().c_str());
            return false;
        }
      #ifdef SHOW_MATRICES
        std::cout<<"Try to insert value in A: ("<<all_active_obs.size()<<","<<positions.at(i)<<") => "<<relations.at(i)<<std::endl;
      #endif
        A.insert(all_active_obs.size(),positions.at(i))=relations.at(i);
    }

  #ifdef SHOW_MATRICES
    //std::cout<<" (w: "<<weight<<")"<<std::endl;
  #endif

    if (has_B_val)
    {
        B.insert(all_active_obs.size(),0)=B_value;
        Q.insert(all_active_obs.size(),all_active_obs.size())=sqr(sigma)/WEIGHT_FACTOR;
        obs->obsRank=all_active_obs.size();
        //std::cout<<"Obs rank "<<obs->obsRank<<": "<<obs->toString()<<std::endl;
        all_active_obs.push_back(obs);
      #ifdef SHOW_MATRICES
        std::cout<<"All obs, add obs "<<obs->toString()<<"  : "<<all_active_obs.size()<<std::endl;
      #endif
        return true;
    }else{
        std::cerr<<"Error in observation: no B part."<<std::endl;
        std::cout<<"Error in observation: no B part."<<std::endl;
        Project::theInfo()->error(INFO_OBS,1,
                                  QT_TRANSLATE_NOOP("QObject","Error in observation: no B part: %s"),
                                  obs->toString().c_str());
        return false;
    }
}

//add covariances to P matrices
//returns false if error
bool LeastSquares::add_covariance(const std::vector<Obs *> &obs_list, const MatX &sigmas2)
{
  #ifdef SHOW_MATRICES
    std::cout<<"Add covariance: ";
    for (auto &obs:obs_list) std::cout<<obs->getObsRank()<<" ";
    std::cout<<std::endl;
    std::cout<<"Q before cov:\n"<<MatX(Q)<<std::endl;
  #endif
    if (((unsigned)sigmas2.rows()!=obs_list.size())
            ||((unsigned)sigmas2.cols()!=obs_list.size()))
    {
        Project::theInfo()->error(INFO_OBS,1,
                                  QT_TRANSLATE_NOOP("QObject","Error in add_covariance: "
                                                              "obs_list size and sigmas size not coherent"));
        return false;
    }

    for (unsigned int j=0;j<obs_list.size();j++)
    {
        if (obs_list[j]->obsRank<0) continue;
        for (unsigned int i=j+1;i<obs_list.size();i++)
        {
            if (obs_list[i]->obsRank<0) continue;
            Q.insert(obs_list[i]->obsRank,obs_list[j]->obsRank)=sigmas2(j,i)/WEIGHT_FACTOR;
            Q.insert(obs_list[j]->obsRank,obs_list[i]->obsRank)=sigmas2(i,j)/WEIGHT_FACTOR;
        }
    }

  #ifdef SHOW_MATRICES
    std::cout<<"Q after cov:\n"<<MatX(Q)<<std::endl;
  #endif

    return true;
}

Json::Value LeastSquares::toJson(FileRefJson& filesRef) const
{
    Json::Value val;
    //std::cout<<"computation_start "<<to_simple_string(computation_start)<<std::endl;
    val["computation_start"]=to_simple_string(computation_start);
    val["computation_duration"]=to_simple_string(computation_end-computation_start);
    val["internal_constraints"]=m_internalConstr;
    val["rank_deficiency"]=rankDeficiency;
    val["nbr_iterations"]=nbr_iterations;
    val["nbr_active_obs"]=(int)nbActiveObs;
    val["nbr_all_obs"]=(int)Project::theone()->totalNumberOfObs();
    val["nbr_parameters"]=(int)all_parameters.size();
    val["interrupted"]=m_interrupted;
    val["kernel"]=kernelBaseJson;

    if (Project::theone()->config.compute_type==COMPUTE_TYPE::type_compensation)
    {
        val["sigma_0_init"]=(double)sigma_0_init;
        val["sigma_0_final"]=(double)sigma_0;
    }

    val["all_sigma0"]=Json::arrayValue;
    for (unsigned int i=0;i<all_sigma0.size();i++)
        val["all_sigma0"].append((double)all_sigma0.at(i));

    val["all_parameters_names"]=Json::arrayValue;
    for (auto & param :all_parameters)
        val["all_parameters_names"].append(param->name);

    Json::Value js_chi2;
    tdouble confidence=0.99;
    int DOF=all_active_obs.size()-all_parameters.size();
    js_chi2["confidence"]=(double)confidence;
    js_chi2["DOF"]=(int)(DOF);
    if (DOF>0)
    {
        js_chi2["min"]=(double)chi2_min(confidence,DOF);
        js_chi2["max"]=(double)chi2_max(confidence,DOF);
    }else{
        js_chi2["min"]=NAN;
        js_chi2["max"]=NAN;
    }
    val["chi2_test"]=js_chi2;

    val["internal_constraints_obs"]=Json::arrayValue;
    for (auto & obs : ICobs)
        val["internal_constraints_obs"].append(obs->toJson(filesRef));

    val["solver_name"]=solverName;

    return val;
}

void LeastSquares::show_residuals(const SpVect &residuals, const std::string &text)
{
    MatX vect(residuals);
    std::cout<<text<<" residuals: ----------\n";
    for (unsigned int i=0;i<all_active_obs.size();i++)
        if (all_active_obs.at(i))
            std::cout<<vect(i)<<" -> "<<all_active_obs.at(i)->toString()<<std::endl;

    std::cout<<"end "<<text<<" residuals ---\n";
}

//returns true if dim(kern)>0
bool LeastSquares::computeKernel()
{
    kernelBaseJson.clear();
    kernelBaseJson=Json::nullValue;//to make sure the value is null if kdim=0, event after kernel error on previous computation
    if (AtPA.rows()==0) return false;
#ifdef USE_QT
    emit enterKernel();
#endif
    std::cout<<QObject::tr("Compute kernel...").toCstr()<<std::endl;
    AtPA_kernel = AtPA_dense.fullPivLu().kernel();
    //TODO: use SVD decomposition?
    /* https://eigen.tuxfamily.org/dox/classEigen_1_1FullPivLU.html
     * This LU decomposition is very stable and well tested with large matrices.
     * However there are use cases where the SVD decomposition is inherently more
     * stable and/or flexible. For example, when computing the kernel of a matrix,
     * working with the SVD allows to select the smallest singular values of the matrix,
     * something that the LU decomposition doesn't see.*/

    rankDeficiency = AtPA_kernel.cols();//kernel computation seems more precise than simple rank
    if (AtPA_kernel.col(0).squaredNorm()==0.0)
        rankDeficiency = 0;
    bool isKern = (rankDeficiency>0);
    if (isKern)
    {
        kernelMessage();
        Project::theInfo()->error(INFO_LS,1,
                                  QT_TRANSLATE_NOOP("QObject","The system has infinite solutions!"));
    }
    return isKern;
}

bool compare_kern_val(std::pair<tdouble,int> a,std::pair<tdouble,int> b)
{
    return (fabs(a.first)>fabs(b.first));
}
void LeastSquares::kernelMessage()
{
    Project::theInfo()->error(INFO_LS,1,
                              QT_TRANSLATE_NOOP("QObject","Linear application rank deficiency or imbalanced matrix "
                                                          "(%d constraints are missing)!"),rankDeficiency);
    Project::theInfo()->error(INFO_LS,1,
                              QT_TRANSLATE_NOOP("QObject","Kernel base:"));

    for (int j=0;j<AtPA_kernel.cols();j++)
    {
        std::ostringstream oss;
        oss.precision(2);
        Json::Value kernelVector;
        int k=0;
        std::vector <std::pair<tdouble,int> >all_vals;
        all_vals.reserve(AtPA_kernel.rows());
        for (int i=0;i<AtPA_kernel.rows();i++)
        {
            tdouble val=AtPA_kernel(i,j);
            Json::Value jsonVal;
            jsonVal["val"] = val;
            jsonVal["num_pt"] = all_parameters[i]->on_point?all_parameters[i]->on_point->getPointNumber():-1;
            all_vals.push_back(std::pair<tdouble,int>(val,i));
            if (fabs(val)>0.0001)
            {
                kernelVector[all_parameters[i]->name]=jsonVal;
            }
        }

        std::sort(all_vals.begin(),all_vals.end(),compare_kern_val);
        for (auto &v:all_vals)
        {
            if (fabs(v.first)>0.0001)
            {
                if (v.first>0)
                {
                    if (k>0) oss<<"+ ";
                }else oss<<"- ";

                if (fabs(v.first-1)>0.0001) oss<<fabs(v.first)<<"*";
                oss<<all_parameters[v.second]->name<<" ";
                k++;
            }
        }
        Project::theInfo()->error(INFO_LS,3,
                                  QT_TRANSLATE_NOOP("QObject","• %s"),oss.str().c_str());
        kernelBaseJson.append(kernelVector);
    }
}

#ifdef USE_QR_SOLVER
bool LeastSquares::testStability(spSolver &solver)
{
    std::cout<<QObject::tr("Normal matrix cols: ").toCstr()<<solver.cols()<<"\n";
    std::cout<<QObject::tr("Normal matrix rank: ").toCstr()<<solver.rank()<<"\n";
    rankDeficiency=solver.cols()-solver.rank();
    if (rankDeficiency>0)
    {
        if (computeKernel())
            return false;
    }
    return rankDeficiency==0; //rankDeficiency may be updated by computeKernel()
}
#endif
