#ifndef SENSOR_POLAR_2D_H
#define SENSOR_POLAR_2D_H

#include "Sensor.h"

namespace obvious
{

/**
 * @class SensorPolar2D
 * @brief Generic class for 2D measurement units using polar sampling
 * @author Stefan May
 */
class SensorPolar2D : public Sensor
{
public:

  /**
   * Standard constructor
   * @param[in] beams number of beams
   * @param[in] angularRes angular resolution, i.e. angle between beams in rad
   * @param[in] phiMin minimum angle from which beams are counted positive counter-clockwisely (rad)
   * @param[in] maxRange maximum range
   */
  SensorPolar2D(unsigned int beams, double angularRes, double phiMin, double maxRange=NAN);

  /**
   * Destructor
   */
  ~SensorPolar2D();

  /**
   * Calculate ray of specific beam
   * @param[in] beam beam index
   * @param[out] ray vector
   */
  void calcRay(unsigned int beam, double ray[2]);

  /**
   * Assign an arbitrary 2D coordinate to a measurement beam
   * @param[in] coordinate vector
   * @return beam index
   */
  int backProject(double data[2]);

  /**
   * Parallel version of back projection
   * @param[in] M matrix of homogeneous 2D coordinates
   * @param[out] indices vector of beam indices
   */
  void backProject(Matrix* M, int* indices);

  /**
   * Get angular resolution
   * @return angular resolution
   */
  double angularRes(void);

  /**
   * Get the minimum angle
   * @return minimum angle
   */
  double phiMin(void);
private:

  double _angularRes;

  double _phiMin;

  double _phiLowerBound;

  double _phiUpperBound;

};

}

#endif
