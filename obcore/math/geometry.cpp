#include "geometry.h"
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include <iostream>

using namespace std;

namespace obvious
{

void calculatePerspective(Matrix* P, CartesianCloud3D* cloud, int nW, int nH, int subsample)
{

  int points = cloud->size() / subsample + 1;

  Matrix* A = new Matrix(2 * points, 11);
  double* b = new double[2*points];

  int k = 0;
  int* indices = cloud->getIndices();

  for (unsigned int i = 0; i < cloud->size(); i += subsample)
  {
    int idx = indices[i];
    double* point = (*cloud)[i];
    double ik = (double) (idx % nW);
    double jk = (double) (idx / nW);
    double xk = point[0];
    double yk = point[1];
    double zk = point[2];

    (*A)[k][0] = xk;
    (*A)[k][1] = yk;
    (*A)[k][2] = zk;
    (*A)[k][3] = 1.0;
    (*A)[k][4] = 0.0;
    (*A)[k][5] = 0.0;
    (*A)[k][6] = 0.0;
    (*A)[k][7] = 0.0;
    (*A)[k][8] = -ik * xk;
    (*A)[k][9] = -ik * yk;
    (*A)[k][10] = -ik * zk;

    (*A)[k+1][0] = 0.0;
    (*A)[k+1][1] = 0.0;
    (*A)[k+1][2] = 0.0;
    (*A)[k+1][3] = 0.0;
    (*A)[k+1][4] = xk;
    (*A)[k+1][5] = yk;
    (*A)[k+1][6] = zk;
    (*A)[k+1][7] = 1.0;
    (*A)[k+1][8] = -jk * xk;
    (*A)[k+1][9] = -jk * yk;
    (*A)[k+1][10] = -jk * zk;

    b[k] = ik;
    b[k+1] = jk;

    k += 2;
  }

  double x[11];
  A->solve(b, x);

  for (int i = 0; i < 11; i++)
    (*P)[i/4][i%4] = x[i];
  (*P)[2][3] = 1.0;
}

bool axisAngle(Matrix M, double* axis, double* angle)
{
  int rows = M.getRows();
  int cols = M.getCols();

  if((rows != 3 && cols != 3) && (rows != 4 && cols != 4))
  {
    printf("WARNING Matrix::axisAngle: axis angle representation only valid for 3x3 or 4x4 matrices\n");
    return false;
  }

  double trace = M.trace();
  if(rows==4) trace -= M[3][3];

  /**
   * Fix for imprecise rotation matrices
   * ToDo: proof correctness
   */
  if(trace>3.0) trace = 3.0;
  if(trace<-1.0) trace = -1.0;

  *angle = acos((trace - 1.0)/2.0);

  double w = 1.0 / (2.0 * sin(*angle));
  double a1 = w * (M[2][1] - M[1][2]);
  double a2 = w * (M[0][2] - M[2][0]);
  double a3 = w * (M[1][0] - M[0][1]);
  axis[0] = a1;
  axis[1] = a2;
  axis[2] = a3;

  return true;
}

}
