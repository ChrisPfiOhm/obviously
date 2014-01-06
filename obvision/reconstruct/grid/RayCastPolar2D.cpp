#include "RayCastPolar2D.h"

#include <string.h>

#include "obcore/base/System.h"
#include "obcore/math/mathbase.h"
#include "obcore/base/Timer.h"
#include "obcore/base/Logger.h"

namespace obvious
{

RayCastPolar2D::RayCastPolar2D()
{

}

RayCastPolar2D::~RayCastPolar2D()
{

}

void RayCastPolar2D::calcCoordsFromCurrentView(TsdGrid* grid, SensorPolar2D* sensor, double* coords, double* normals, unsigned int* cnt)
{
  Timer t;
  *cnt = 0;

  double c[2];
  double n[2];
  Matrix M(3,1);
  Matrix N(3,1);
  Matrix* T = sensor->getPose();
  Matrix Ti(3, 3);
  Ti = T->getInverse();
  M(2,0) = 1.0;
  N(2,0) = 0.0; // no translation for normals

  for (unsigned int beam = 0; beam < sensor->getRealMeasurementSize(); beam++)
  {
    if (rayCastFromCurrentView(grid, sensor, beam, c, n)) // Ray returned with coordinates
    {
      M(0,0) = c[0];
      M(1,0) = c[1];
      N(0,0) = n[0];
      N(1,0) = n[1];
      M       = Ti * M;
      N       = Ti * N;
      for (unsigned int i = 0; i < 2; i++)
      {
        coords[*cnt] = M(i,0);
        normals[(*cnt)++] = N(i,0);
      }
    }
  }

  LOGMSG(DBG_DEBUG, "Elapsed TSDF projection: " << t.getTime() << "ms");
  LOGMSG(DBG_DEBUG, "Ray casting finished! Found " << *cnt << " coordinates");
}


bool RayCastPolar2D::rayCastFromCurrentView(TsdGrid* grid, SensorPolar2D* sensor, const unsigned int beam, double coordinates[2], double normal[2])
{
  int xDim = grid->getCellsX();
  int yDim = grid->getCellsY();
  double cellSize = grid->getCellSize();

  double tr[2];
  sensor->getPosition(tr);
  double maxRange = sensor->getMaximumRange();

  double ray[2];
  double position[2];

  sensor->calcRay(beam, ray);
  ray[0] *= cellSize;
  ray[1] *= cellSize;

  // Interpolation weight
  double interp;

  double xmin   = -10e9;
  double ymin   = -10e9;
  if(fabs(ray[0])>10e-6) xmin = ((double)(ray[0] > 0.0 ? 0 : (xDim-1)*cellSize) - tr[0]) / ray[0];
  if(fabs(ray[1])>10e-6) ymin = ((double)(ray[1] > 0.0 ? 0 : (yDim-1)*cellSize) - tr[1]) / ray[1];
  double idxMin = max(xmin, ymin);
  idxMin        = max(idxMin, 0.0);

  double xmax   = 10e9;
  double ymax   = 10e9;
  if(fabs(ray[0])>10e-6) xmax = ((double)(ray[0] > 0.0 ? (xDim-1)*cellSize : 0) - tr[0]) / ray[0];
  if(fabs(ray[1])>10e-6) ymax = ((double)(ray[1] > 0.0 ? (yDim-1)*cellSize : 0) - tr[1]) / ray[1];
  double idxMax = min(xmax, ymax);

  if(!isnan(maxRange))
  {
    double idxRange = maxRange / cellSize + 0.5;
    idxMin = min(idxMin, idxRange) ;
    idxMax = min(idxMax, idxRange);
  }

  if (idxMin >= idxMax) return false;

  // Traverse partitions roughly to clip minimum index
  int partitionSize = grid->getPartitionSize();
  for(int i=idxMin; i<idxMax; i+=partitionSize)
  {
    double tsd_tmp;
    position[0] = tr[0] + i * ray[0];
    position[1] = tr[1] + i * ray[1];
    EnumTsdGridInterpolate retval = grid->interpolateBilinear(position, &tsd_tmp);
    if(retval!=INTERPOLATE_EMPTYPARTITION && retval!=INTERPOLATE_INVALIDINDEX)
      break;
    else
      idxMin = i;
  }

  double tsd_prev;
  position[0] = tr[0] + idxMin * ray[0];
  position[1] = tr[1] + idxMin * ray[1];
  if(grid->interpolateBilinear(position, &tsd_prev)!=INTERPOLATE_SUCCESS)
    tsd_prev = NAN;

  bool found = false;
  for(int i=idxMin; i<=idxMax; i++)
  {
    position[0] += ray[0];
    position[1] += ray[1];

    double tsd;
    if (grid->interpolateBilinear(position, &tsd)!=INTERPOLATE_SUCCESS)
    {
      tsd_prev = tsd;
      continue;
    }

    // Check sign change
    if(tsd_prev > 0 && tsd < 0)
    {
      interp = tsd_prev / (tsd_prev - tsd);
      found = true;
      break;
    }

    tsd_prev = tsd;
  }

  if(!found)
  {
    return false;
  }

  coordinates[0] = position[0] + ray[0] * (interp-1.0);
  coordinates[1] = position[1] + ray[1] * (interp-1.0);

  return grid->interpolateNormal(coordinates, normal);
}

void RayCastPolar2D::calcCoordsAligned(TsdGrid* grid, double* coords, double* normals, unsigned int* cnt)
{
  Timer t;
  *cnt = 0;

  double c[2];

  double cellSize = grid->getCellSize();
  for (unsigned int y=0; y<grid->getCellsY(); y++)
  {
    c[0] = cellSize * 0.5;
    c[1] = y*cellSize + cellSize * 0.5;
    double tsdf_prev;
    grid->interpolateBilinear(c, &tsdf_prev);
    for (unsigned int x=1; x<grid->getCellsX(); x++)
    {
      c[0] += cellSize;
      double tsdf;
      if(grid->interpolateBilinear(c, &tsdf))
      {
        // Check sign change
        if(tsdf_prev*tsdf < 0 && fabs(tsdf)<0.9999 && fabs(tsdf_prev)<0.99999)
        {
          double interp = 1-(tsdf_prev / (tsdf_prev - tsdf));
          coords[*cnt] = c[0] - interp * cellSize;
          coords[*cnt+1] = c[1];
          *cnt+=2;
        }
      }
      tsdf_prev = tsdf;
    }
  }

  for (unsigned int x=0; x<grid->getCellsX(); x++)
  {
    c[0] = x*cellSize + cellSize * 0.5;
    c[1] = cellSize * 0.5;
    double tsdf_prev;
    grid->interpolateBilinear(c, &tsdf_prev);
    for (unsigned int y=1; y<grid->getCellsY(); y++)
    {
      c[1] += cellSize;
      double tsdf;
      if(grid->interpolateBilinear(c, &tsdf))
      {
        // Check sign change
        if(tsdf_prev*tsdf < 0 && fabs(tsdf)<0.9999 && fabs(tsdf_prev)<0.99999)
        {
          double interp = 1-(tsdf_prev / (tsdf_prev - tsdf));
          coords[*cnt] = c[0];
          coords[*cnt+1] = c[1] - interp * cellSize;
          *cnt+=2;
        }
      }
      tsdf_prev = tsdf;
    }
  }
  LOGMSG(DBG_DEBUG, "Elapsed TSDF projection: " << t.getTime() << "ms");
  LOGMSG(DBG_DEBUG, "Ray casting finished! Found " << *cnt << " coordinates");
}

}

