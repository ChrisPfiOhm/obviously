#ifndef RAYCASTPOLAR2D_H
#define RAYCASTPOLAR2D_H

#include <vector>
#include "obcore/math/linalg/linalg.h"
#include "obvision/reconstruct/grid/TsdGrid.h"
#include "obvision/reconstruct/grid/SensorPolar2D.h"

namespace obvious
{

/**
 * @class RayCastPolar2D
 * @brief
 * @author Stefan May
 */
class RayCastPolar2D
{
public:

  RayCastPolar2D();

  ~RayCastPolar2D();

  void calcCoordsFromCurrentView(TsdGrid* grid, SensorPolar2D* sensor, double* coords, double* normals, unsigned int* ctr);

  void calcCoordsFromCurrentViewMask(TsdGrid* grid, SensorPolar2D* sensor, double* coords, double* normals, bool* mask);

private:

  bool rayCastFromCurrentView(TsdGrid* grid, double tr[2], double ray[2], double coordinates[2], double normal[2]);

  void calcRayFromCurrentView(const unsigned int beam, double dirVec[2]);

  double _xmin;
  double _ymin;

  double _xmax;
  double _ymax;

  double _idxMin;
  double _idxMax;
};

}

#endif
