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

#ifndef MATHTOOLS_H
#define MATHTOOLS_H

#include "compile.h"
#include "coord.h"

#include <cmath>
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Sparse>
#include <string>
#include <vector>

using MatX = Eigen::Matrix<tdouble, Eigen::Dynamic, Eigen::Dynamic>;
using VectX = Eigen::Matrix<tdouble, Eigen::Dynamic, 1>;

using SpMat = Eigen::SparseMatrix<tdouble>;
using SpVect = Eigen::SparseVector<tdouble>;

enum class ANGLE_UNIT
{
    RAD=0,
    GRAD,
    DEG,
    //DMS, //DMS is not represented as double, use special functions //DMS is a tdouble with format dd.mmssuuuuu... (where uuuuu... are seconds decimals)
    ARCSEC
};

struct Angles
{
    static std::map<ANGLE_UNIT, std::string> names;
};


constexpr double neginfinity = -std::numeric_limits<double>::infinity();

tdouble chi2_min(tdouble confidence,int num_freedom);
tdouble chi2_max(tdouble confidence,int num_freedom);

extern const tdouble PI;

bool arc(tdouble Xa,tdouble Ya,tdouble Xb,tdouble Yb,tdouble &C, tdouble &S, tdouble radius);
bool arc_cartesian(tdouble Xa,tdouble Ya,tdouble Xb,tdouble Yb,tdouble &C, tdouble &S);

tdouble azimuth(tdouble Xa, tdouble Ya, tdouble Xb, tdouble Yb, tdouble C, tdouble radius);//<angle from north

tdouble gauss_random(tdouble m, tdouble s);

tdouble toRad(tdouble ang, ANGLE_UNIT unit);
tdouble fromRad(tdouble ang, ANGLE_UNIT unit);
tdouble convertAngle(tdouble ang, ANGLE_UNIT unit_from, ANGLE_UNIT unit_to);

struct DMSAngle { //TODO(jmm): refactor
    DMSAngle() : degrees(0),minutes(0),secondes(0),isNegative(false) {}
    DMSAngle(int d, int m, tdouble s) : degrees(d<0 ? -d : d),minutes(m),secondes(s),isNegative(d<0) {}
    DMSAngle(tdouble ang, ANGLE_UNIT unit, bool forcePositive);
    int degrees,minutes;
    tdouble secondes;
    bool isNegative;
};

std::string angToDMSString(tdouble angle, ANGLE_UNIT unit, bool forcePositive, int nb_dec=10);
void normalizeAngle(tdouble &a);

tdouble sqr(tdouble x);
tdouble sign(tdouble x);

void abc2R(tdouble a, tdouble b, tdouble c, tdouble theta, Mat3 &R);
void R2abc(const Mat3 &R, tdouble &theta, tdouble &a, tdouble &b, tdouble &c);
bool Thomson_Shut(const std::vector<Coord> &Xm, const std::vector<Coord> &Xt, Mat3 &R, Vect3 &T, tdouble &scale);
Mat3 axiator(const Vect3 &V);
Mat3 angEN2Rot_small_angles(tdouble ang_e, tdouble ang_n);
Mat3 vertDeflection2Rot(tdouble eta, tdouble xi);//<computes rotation from ellipsoid to geoid
Mat3 global2vertRot(const Coord &center_spherical, tdouble eta, tdouble xi);//<computes rotation matrix from ground cartesian to cartesian with vert aligned on one point (given in spherical)

Json::Value R2json(Mat3 R);

bool pointsAreAligned(const std::vector<Coord> &pts);

#endif // MATHTOOLS_H
