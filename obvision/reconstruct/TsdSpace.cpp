/*
 * Tsd_space.cpp
 *
 *  Created on: 20.07.2012
 *      Author: phil
 */

#include <iostream>
#include <cstring>
#include <cmath>
#include <cstdio>
#include "TsdSpace.h"
#include "TsdSpace_utils.h"
#include "obcore/base/tools.h"
#include "obcore/math/Matrix.h"
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "Projection.h"
#include "obcore/base/System.h"
#include "obcore/math/mathbase.h"

#include "obcore/base/Logger.h"

#define ROW_MAX 480            // Sensor depth-array-dimensions
#define COL_MAX 640
#define MAXWEIGHT 1.6e308     // maximum weight (near end of double)
#define RGB_MAX 255           // 2⁸-1
#define UNUSED 0.999
#define M_UNUSED -0.999
#define SCALE 1.//0.25        // fixed scale for raycaster

#define KINFU_WEIGHTING 1
#define CORRECT_SIGN_CHANGE 1

namespace obvious
{

TsdSpace::TsdSpace(const unsigned int height, const unsigned int width,	const unsigned int depth,  const double voxelDim, double* perspective)
{
	// determine number of voxels in each dimension
	_xDim = round(width  / voxelDim);
	_yDim = round(height / voxelDim);
	_zDim = round(depth  / voxelDim);

	LOGMSG(DBG_DEBUG, "Dimensions are (x/y/z) (" << _xDim << "/" << _yDim << "/" << _zDim << ")");
	LOGMSG(DBG_DEBUG, "Creating TsdVoxel Space...");

	// allocate Space Voxel ***
	System<TsdVoxel>::allocate(_zDim, _yDim, _xDim, _space);

	LOGMSG(DBG_DEBUG, "Space for " << _xDim * _yDim * _zDim	<< " TsdVoxels has been created");

	// init _space Voxel***
	for (unsigned int z = 0; z < _zDim; z++)
	{
		for (unsigned y = 0; y < _yDim; y++)
		{
			for (unsigned int x = 0; x < _xDim; x++)
			{
				_space[z][y][x].tsdf   = 1.0;
#if KINFU_WEIGHTING
				_space[z][y][x].weight = 0.0;
#else
				_space[z][y][x].weight = 1.0;
#endif
			}
		}
	}

	_voxelDim = voxelDim;
	_height   = height;
	_width    = width;
	_depth    = depth;

	_projection = new Projection(perspective);

	_T = new Matrix(4, 4);
	_T->setIdentity();

	_Tinv = new Matrix(4, 4);
	_Tinv->setIdentity();

	//Initialize space for depth image
	_depthImage = new double[COL_MAX * ROW_MAX];

	//initialize homogeneous point at origin
	_zero_h = new Matrix(4, 1);
	_zero_h->setZero();
	(*_zero_h)[3][0] = 1.0;
}

TsdSpace::~TsdSpace(void)
{
	delete [] _space;
	delete _projection;
	delete _T;
	delete _Tinv;
	delete [] _depthImage;
	delete _zero_h;
}

MSG TsdSpace::push(double *depthImage)
{
	//store current depth image in member variable
	LOGMSG(DBG_DEBUG, "Store current depth image...");
	memcpy(_depthImage, depthImage, COL_MAX * ROW_MAX * sizeof(double));

	/*
	 * Room for Multithreading shall use up to 12 Threads which calculate a slide each
	 */
	for (unsigned int i = 0; i < _zDim; i++)
	{
		if (depthSlice(i) != OK)
			LOGMSG(DBG_ERROR, "Error slice nr. " << i);
	}

	return (OK);
}

void TsdSpace::setMaxTruncation(double val)
{
	if(val < 2 * _voxelDim)
	{
		LOGMSG(DBG_WARN, "Truncation radius must be at 2 x voxel dimension. Setting minimum size.");
		val = 2 * _voxelDim;
	}
	_maxTruncation = val;
}

MSG TsdSpace::depthSlice(const unsigned int depth)
{
	//Variables and Pointers
	double coordVoxel[4];
	Matrix mCoordVoxel(4, 1);
	TsdVoxel *voxel;

	// center point of all voxels in given z-slice
	coordVoxel[2] = ((double) depth + 0.5) * _voxelDim;
	coordVoxel[3] = 1.0;

	// Loop over the z-slice given in depth
	for (unsigned int row = 0; row < _yDim; row++)
	{
		for (unsigned int col = 0; col < _xDim; col++)
		{
			// calculate center point of current voxel
			coordVoxel[0] = ((double) col + 0.5) * _voxelDim;
			coordVoxel[1] = ((double) row + 0.5) * _voxelDim;
			mCoordVoxel.setData(coordVoxel);

			// Transform coordinates of current voxel in sensor coordinate system
			mCoordVoxel = *_Tinv * mCoordVoxel;

			// Get current Pixel
			unsigned int u;
			unsigned int v;
			_projection->get_pxl(&mCoordVoxel, &u, &v);

			if((v < 0) || (v >= ROW_MAX)) continue;
			if((u < 0) || (u >= COL_MAX)) continue;

			//invalid point-->test?
			if (_depthImage[((ROW_MAX - 1) - v) * COL_MAX + u] < 0.001) continue;

			// current voxel in Kinect frustrum -> Calculate TSDF
			// get current translation_vector to kinect
			double t[3];
			for (unsigned int i = 0; i < 3; i++)
				t[i] = (*_T)[i][3];

			// calculate distance of current voxel to kinect
			double distance = euklideanDistance<double>((double*) t, (double*) coordVoxel, 3);

			// calculate signed distance function
			double sdf=distance-((_depthImage[((ROW_MAX-1)-v)*COL_MAX+u]));     //sdf(i)=abs(t(i)-v(g))-D(i)(p) /Kinect Z-Image = millimeters!FUCK!!

			/*float xl = (u - (*_projection)[0][2]) / (*_projection)[0][0];
			float yl = (v - (*_projection)[1][2]) / (*_projection)[1][1];
			float lambda_inv = 1. / sqrt(xl * xl + yl * yl + 1.);
			sdf = distance * lambda_inv	- ((_depthImage[((ROW_MAX - 1) - v) * COL_MAX + u])); //mm*/

			sdf *= -1.0;

			if(sdf >= -_maxTruncation)
			{
				//calculate truncated sdf
				//get ptr to current TsdVoxel
				voxel = &_space[depth][(_yDim - 1) - row][col]; //turn row idx bsp. _yDim=480, row =0 -> y_idx=479-0 = 479

				//truncate
				double tsdf = sdf / _maxTruncation; //determine whether sdf/max_truncation = ]-1;1[ otherwise continue

				if(tsdf > 1.0) tsdf = 1.0;

#if KINFU_WEIGHTING
				voxel->tsdf = (voxel->tsdf * voxel->weight + 1.0 * tsdf) / (voxel->weight + 1.0);
				voxel->weight += 1.0;
				if(voxel->weight > MAXWEIGHT) voxel->weight = MAXWEIGHT;
#else
				//weight it with depth of the voxel
				//increment weight
				if (voxel->tsdf > UNUSED) //First hit? no need to weight it
					voxel->tsdf = tsdf;
				else
				{
					double* weight_var = &voxel->weight;
					if (*weight_var < MAXWEIGHT)
						*weight_var += 1.0;
					voxel->tsdf = (voxel->tsdf * (*weight_var - 1.0) + tsdf) / (*weight_var);
				}
#endif
			}
		}
	}

	return (OK);
}

MSG TsdSpace::buildSliceImage(const unsigned int depthIndex, unsigned char* image)
{
	unsigned char R[_xDim * _yDim];
	unsigned char G[_xDim * _yDim];
	unsigned char B[_xDim * _yDim];
	unsigned int ctr = 0;
	unsigned im_ctr = 0;
	TsdVoxel *c_vxlPtr;
	double c_tsdf;

	// initialize arrays for RGB
	for (unsigned int i = 0; i < _xDim * _yDim; i++)
	{
		R[i] = 0;
		G[i] = 0;
		B[i] = 0;
	}

	// iterate over given slice, generate 2D-Picture
	for (int row = 0; row < _yDim; row++)
	{
		for (int col = 0; col < _xDim; col++)
		{
			// Get current tsdf
			c_tsdf = _space[depthIndex][row][col].tsdf;

			// Blue for depth behind Voxel
			if (c_tsdf > 0)
			{
				if (c_tsdf > UNUSED)
					G[im_ctr++] = (unsigned char) 150; //row*_xDim+col
				else
					B[im_ctr++] = (unsigned char) (c_tsdf * RGB_MAX + 0.5); //row*_xDim+col
			}

			// Red for depth in front of Voxel
			else if (c_tsdf < 0)
			{
				if (c_tsdf < M_UNUSED)
					G[im_ctr++] = (unsigned char) 50;
				else
					R[im_ctr++] =
							(unsigned char) ((c_tsdf * -1.0) * RGB_MAX + 0.5); //row*_xDim+col
			}
		}
	}

	//put components together to complete picture
	for (unsigned int i = 0; i < _xDim * _yDim * 3; i++)
	{
		image[i]   = R[ctr];
		image[++i] = G[ctr];
		image[++i] = B[ctr];
		ctr++;
	}

	return (OK);
}

unsigned int TsdSpace::getXDimension()
{
	return _xDim;
}

unsigned int TsdSpace::getYDimension()
{
	return _yDim;
}

unsigned int TsdSpace::getZDimension()
{
	return _zDim;
}

MSG TsdSpace::setTransformation(double *transM_data)
{
	//Variables and Pointers
	Matrix var_M(4, 4);
	Matrix h_var_M(4, 4);

	//_T->setData(transM_data);
	var_M.setData(transM_data);
	(*_T) *= var_M;
	h_var_M = *_T;
	h_var_M.invert();
	*_Tinv = h_var_M;

	return (OK);
}

MSG TsdSpace::peak(unsigned int row, unsigned int col, unsigned int *nbr, double **coordinates)
{
	//Variables and pointers
	double c_tsdf = 0.0;
	double n_tsdf = 0.0;
	const double X_POS = ((double) col + 0.5) * _voxelDim; //const vars to store x,y coordinates of peak
	const double Y_POS = ((double) row + 0.5) * _voxelDim;
	const unsigned int Y_IND = (_yDim - 1) - row;
	const unsigned int Z_MAX = _zDim - 1;

	/*
	 * Main loop
	 */
	//Iterate through peak
	for (unsigned int i = 0; i < Z_MAX; i++)
	{
		//compare the signs
		c_tsdf = _space[i][Y_IND][col].tsdf;
		n_tsdf = _space[i + 1][Y_IND][col].tsdf;

		if ((c_tsdf > 0 && c_tsdf < UNUSED) && (n_tsdf < 0 && n_tsdf > M_UNUSED))
		{
			//Store coordinates
			(*coordinates)[(*nbr)++] = X_POS;
			(*coordinates)[(*nbr)++] = Y_POS;
			(*coordinates)[(*nbr)++] = ((double) i + 0.5) * _voxelDim;
		}
	}
	return (OK);
}

MSG TsdSpace::generatePointcloud(double **cloud, unsigned int *nbr)
{
	//initialize
	//allocate space for pointcloud
	*cloud = new double[_xDim * _yDim * _zDim * 3];

	/*
	 * Room for Multithreading shall use up to 12 Threads which calculate a peak each
	 */

	// send peaks through space
	for (unsigned int row = 0; row < _yDim; row++)
	{
		for (unsigned int col = 0; col < _xDim; col++)
		{
			if ((peak(row, col, nbr, cloud)) != OK)
			{
				LOGMSG(DBG_ERROR, "Peak in : (row/col) (" << row << "/" << col << ")");
				continue;
			}
		}
	}
	return (OK);
}

MSG TsdSpace::calcRay(const unsigned int row, const unsigned int col, double **dir_vec, double **foot_point)
{
	Matrix dirp_vecM(4, 1);
	Matrix dirf_vecM(4, 1);
	double abs_vec = 0.0;

	//get point out of given pixel
	_projection->get_point3(col, row, SCALE, &dirp_vecM);

	//bring peakpoint in Map-coordinate-system
	dirp_vecM = *_T * dirp_vecM;

	//calculate foot point
	dirf_vecM = *_T * (*_zero_h); //bring footpoint in Map-coordinate-system
	dirf_vecM.getData(*foot_point); //give data to calling function

	//calculate amount of vector
	abs_vec = abs3D(&dirf_vecM, &dirp_vecM);

	//calculate vector normalized to voxeldimension
	//((PEAK-FOOT)/ABS)*VOXELDIMENSION)
	for (unsigned int i = 0; i < 3; i++)
		(*dir_vec)[i] = ((dirp_vecM[i][0] - dirf_vecM[i][0]) / abs_vec)	* _voxelDim;

	return (OK);
}

MSG TsdSpace::getModel(double **model_pcl, unsigned int *ctr)
{
	double *p_var    = new double[3];
	bool found       = 0;
	double depth_var = 0.0;

	// allocate space for pointcloud
	*model_pcl = new double[ROW_MAX * COL_MAX * 3];
	LOGMSG(DBG_DEBUG, "Space allocated. Starting Raytracing...");

	//reset counter
	*ctr = 0;

	/*
	 * Room for Multithreading shall use up to 12 Threads which calculate a ray each
	 */
	for (unsigned int row = 0; row < ROW_MAX; row++)
	{
		for (unsigned int col = 0; col < COL_MAX; col++)
		{
			if (rayTrace(row, col, &p_var, &depth_var) == OK) //Ray returned with coordinates
			{
				found = 1;
				for (unsigned int i = 0; i < 3; i++)
					(*model_pcl)[(*ctr)++] = p_var[i];
			}
		}
	}
	if (found) --(*ctr);

	LOGMSG(DBG_DEBUG, "Raytracing finished! Found " << *ctr << " coordinates.");

	delete p_var;
	return (OK);
}

Matrix* TsdSpace::getTransformation()
{
	return(_T);
}

#if CORRECT_SIGN_CHANGE
MSG TsdSpace::rayTrace(const unsigned int row, const unsigned int col, double **coordinates, double *depth)
{
	double *dir_vec = new double[3];
	double *foot_point = new double[4];
	double *position = new double[3];
	double *position_prev = new double[3];
	int x_idx = 0;
	int y_idx = 0;
	int z_idx = 0;
	unsigned int ctr = 0;

	foot_point[3] = 1.0;
	if (calcRay(row, col, &dir_vec, &foot_point) != OK)
	{
		LOGMSG(DBG_DEBUG, "Error calculating ray (row/col) (" << row << "/" << col << ")");
		delete dir_vec;
		delete foot_point;
		return (ERROR);
	}

	// Interpolation weight
	double interp;

	while (1)
	{
		// calculate current position
		memcpy(position_prev, position, 3 * sizeof(*position));

		for (unsigned int i = 0; i < 3; i++)
			position[i] = foot_point[i] + ((double) ctr) * (dir_vec[i]);

		// calculate current indices
		x_idx = (int) (position[0] / _voxelDim);
		y_idx = (_yDim - 1) - (int) (position[1] / _voxelDim);
		z_idx = (int) (position[2] / _voxelDim);

		// check whether tracer is in space or not
		if ((x_idx >= _xDim) || (x_idx < 0) || (y_idx >= _yDim) || (y_idx < 0) || (z_idx >= _zDim)) // raytracer reached edge of space
		{
			delete dir_vec;
			delete foot_point;
			delete position;
			return EDGE;
		}

		if(z_idx < 0) // raytracer is not in space
		{
			ctr++;
			continue;
		}

		if(ctr>100)
		{
			delete dir_vec;
			delete foot_point;
			delete position;
			return EDGE;
		}

		double tsdf;
		if (interpolateTrilineary(&position, &tsdf) != OK)
		{
			ctr++;
			continue;
		}

		double tsdf_prev;
		if (interpolateTrilineary(&position_prev, &tsdf_prev) != OK)
		{
			ctr++;
			continue;
		}

		// check sign change
		if(tsdf_prev > 0 && tsdf < 0)
		{
			interp = tsdf_prev / (tsdf_prev - tsdf);
			break;
		}

		// increment counter
		ctr++;
	}

	// interpolate between voxels when sign change happened
	for (unsigned int i = 0; i < 3; i++)
		(*coordinates)[i] = position_prev[i] + dir_vec[i] * interp;

	delete dir_vec;
	delete foot_point;
	delete position;
	return (OK);
}

#else
MSG TsdSpace::rayTrace(const unsigned int row, const unsigned int col, double **coordinates, double *depth)
{
	double *dir_vec = new double[3];
	double *foot_point = new double[4];
	double *position = new double[3];
	double *position_prev = new double[3];
	double c_tsdf = 1.0;
	double l_tsdf = 1.0;
	int x_idx = 0;
	int y_idx = 0;
	int z_idx = 0;
	unsigned int ctr = 0;

	//initialize
	//homogeneous coordinates
	foot_point[3] = 1.0;
	//calc direction vector
	if (calcRay(row, col, &dir_vec, &foot_point) != OK)
	{
		LOGMSG(DBG_DEBUG, "Error calculating ray (row/col) (" << row << "/" << col << ")");
		delete dir_vec;
		delete foot_point;
		return (ERROR);
	}

	// Interpolation weight
	double interp;

	while (1)
	{
		//calculate current position
		memcpy(position_prev, position, 3 * sizeof(*position));

		for (unsigned int i = 0; i < 3; i++)
			position[i] = foot_point[i] + ((double) ctr) * (dir_vec[i]);

		//calculate current indices
		x_idx = (int) (position[0] / _voxelDim);
		y_idx = (_yDim - 1) - (int) (position[1] / _voxelDim);
		z_idx = (int) (position[2] / _voxelDim);

		//check whether tracer is in space or not
		if ((x_idx >= _xDim) || (x_idx < 0) || (y_idx >= _yDim) || (y_idx < 0) || (z_idx >= _zDim) || (z_idx < 0)) //ratraycer reached edge of space
		{
			delete dir_vec;
			delete foot_point;
			delete position;
			return (EDGE);
		}

		//get current tsdf
		c_tsdf = _space[z_idx][y_idx][x_idx].tsdf;

		if(l_tsdf < 0 && c_tsdf > 0) return (EDGE);

		//check sign change
		//if ((l_tsdf > 0 && l_tsdf < UNUSED)	&& (c_tsdf < 0 && c_tsdf > M_UNUSED))
		if(l_tsdf > 0 && c_tsdf < 0)
		{
			double tsdf;
			if (interpolateTrilineary(&position, &tsdf) != OK)
			{
				delete dir_vec;
				delete foot_point;
				delete position;
				return ERROR;
			}

			double tsdf_prev;
			if (interpolateTrilineary(&position_prev, &tsdf_prev) != OK)
			{
				delete dir_vec;
				delete foot_point;
				delete position;
				return ERROR;
			}

			/*if(tsdf > 0 && tsdf_prev > 0)
			{
				l_tsdf = tsdf;
				ctr++;
				continue;
				//return ERROR;
			}

			if(tsdf < 0 && tsdf_prev < 0)
			{
				return ERROR;
				//interp = tsdf_prev / (tsdf_prev + tsdf);
				//break;
			}*/

			interp = tsdf_prev / (tsdf_prev - tsdf);

			break;
		}
		//store current tsdf in last tsdf
		l_tsdf = c_tsdf;

		//increment counter
		ctr++;
	}

	// interpolate between voxels when sign change happened
	for (unsigned int i = 0; i < 3; i++)
		(*coordinates)[i] = position_prev[i] + dir_vec[i] * interp;

	delete dir_vec;
	delete foot_point;
	delete position;
	return (OK);
}
#endif

MSG TsdSpace::interpolateTrilineary(double **coordinates, double *tsdf)
{
	int x_idx = 0;
	int y_idx = 0;
	int z_idx = 0;
	double w_x = 0;
	double w_y = 0;
	double w_z = 0;
	double abs = 0;
	double str_cr_val = 0;
	Point c_vxl_mp;

	// initialize
	// get voxel indices
	x_idx = (int) (((*coordinates)[0] / _voxelDim));
	y_idx = (int) (((*coordinates)[1] / _voxelDim));
	z_idx = (int) (((*coordinates)[2] / _voxelDim));

	// check edges / 0 is edge because of voxelfinetuning
	if ((x_idx >= (_xDim - 2)) || (x_idx < 1) || (y_idx >= (_yDim - 2)) || (y_idx < 1) || (z_idx >= (_zDim - 2)) || (z_idx < 1))
		return (EDGE);

	// get current voxel middlepoint
	c_vxl_mp.x = (double(x_idx) + 0.5) * _voxelDim;
	c_vxl_mp.y = (double(y_idx) + 0.5) * _voxelDim;
	c_vxl_mp.z = (double(z_idx) + 0.5) * _voxelDim;

	// voxel fine tuning
	if ((*coordinates)[0] < c_vxl_mp.x)
	{
		x_idx--;
		c_vxl_mp.x = (double(x_idx) + 0.5) * _voxelDim;
	}
	if ((*coordinates)[1] < c_vxl_mp.y)
	{
		y_idx--;
		c_vxl_mp.y = (double(y_idx) + 0.5) * _voxelDim;
	}
	if ((*coordinates)[2] < c_vxl_mp.z)
	{
		z_idx--;
		c_vxl_mp.z = (double(z_idx) + 0.5) * _voxelDim;
	}

	// turn y-axis
	y_idx = (_yDim - 1) - y_idx;

	if (_space[z_idx + 0][y_idx + 0][x_idx + 0].tsdf > UNUSED) return EDGE;
	if (_space[z_idx + 1][y_idx + 0][x_idx + 0].tsdf > UNUSED) return EDGE;
	if (_space[z_idx + 0][y_idx - 1][x_idx + 0].tsdf > UNUSED) return EDGE;
	if (_space[z_idx + 1][y_idx - 1][x_idx + 0].tsdf > UNUSED) return EDGE;
	if (_space[z_idx + 0][y_idx + 0][x_idx + 1].tsdf > UNUSED) return EDGE;
	if (_space[z_idx + 1][y_idx + 0][x_idx + 1].tsdf > UNUSED) return EDGE;
	if (_space[z_idx + 0][y_idx - 1][x_idx + 1].tsdf > UNUSED) return EDGE;
	if (_space[z_idx + 1][y_idx - 1][x_idx + 1].tsdf > UNUSED) return EDGE;

	if (_space[z_idx + 0][y_idx + 0][x_idx + 0].tsdf < M_UNUSED) return EDGE;
	if (_space[z_idx + 1][y_idx + 0][x_idx + 0].tsdf < M_UNUSED) return EDGE;
	if (_space[z_idx + 0][y_idx - 1][x_idx + 0].tsdf < M_UNUSED) return EDGE;
	if (_space[z_idx + 1][y_idx - 1][x_idx + 0].tsdf < M_UNUSED) return EDGE;
	if (_space[z_idx + 0][y_idx + 0][x_idx + 1].tsdf < M_UNUSED) return EDGE;
	if (_space[z_idx + 1][y_idx + 0][x_idx + 1].tsdf < M_UNUSED) return EDGE;
	if (_space[z_idx + 0][y_idx - 1][x_idx + 1].tsdf < M_UNUSED) return EDGE;
	if (_space[z_idx + 1][y_idx - 1][x_idx + 1].tsdf < M_UNUSED) return EDGE;

	// get weights
	w_x = ((*coordinates)[0] - c_vxl_mp.x) / _voxelDim;
	w_y = ((*coordinates)[1] - c_vxl_mp.y) / _voxelDim;
	w_z = ((*coordinates)[2] - c_vxl_mp.z) / _voxelDim;

	// Interpolate
	*tsdf =   _space[z_idx + 0][y_idx + 0][x_idx + 0].tsdf * (1 - w_x) * (1 - w_y) * (1 - w_z)
        	+ _space[z_idx + 1][y_idx + 0][x_idx + 0].tsdf * (1 - w_x) * (1 - w_y) * w_z
        	+ _space[z_idx + 0][y_idx - 1][x_idx + 0].tsdf * (1 - w_x) * w_y * (1 - w_z)
        	+ _space[z_idx + 1][y_idx - 1][x_idx + 0].tsdf * (1 - w_x) * w_y * w_z
        	+ _space[z_idx + 0][y_idx + 0][x_idx + 1].tsdf * w_x * (1 - w_y) * (1 - w_z)
        	+ _space[z_idx + 1][y_idx + 0][x_idx + 1].tsdf * w_x * (1 - w_y) * w_z
        	+ _space[z_idx + 0][y_idx - 1][x_idx + 1].tsdf * w_x * w_y * (1 - w_z)
        	+ _space[z_idx + 1][y_idx - 1][x_idx + 1].tsdf * w_x * w_y * w_z;

	return (OK);
}

} // end namespace
