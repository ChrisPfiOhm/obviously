#include <iostream>
#include <sstream>
#include <math.h>
#include "obcore/base/tools.h"
#include "obcore/base/System.h"
#include "obcore/base/Logger.h"
#include "obcore/math/mathbase.h"

#include "obdevice/SickLMS100.h"

#include "obvision/reconstruct/TsdGrid.h"
#include "obvision/reconstruct/RayCastPolar2D.h"
#include "obvision/icp/icp_def.h"

#include "obgraphic/Obvious2D.h"

using namespace std;
using namespace obvious;

void callbackAssignment(double** m, double** s, unsigned int size)
{
  static unsigned int cnt = 0;
  char filename[64];
  sprintf(filename, "/tmp/assignment%05d.dat", cnt++);

  ofstream f;
  f.open(filename);


  for(unsigned int i=0; i<size; i++)
  {
    f << m[i][0] << " " << m[i][1] << " " << s[i][0] << " " << s[i][1] << endl;
  }

  f.close();

  delete [] m;
  delete [] s;
}

int main(int argc, char* argv[])
{
  LOGMSG_CONF("tsd_grid_test.log", Logger::file_off|Logger::screen_off, DBG_DEBUG, DBG_DEBUG);

  // Initialization of TSD grid
  const double dimX = 36.0;
  const double dimY = 36.0;
  const double cellSize = 0.06;

  // choose estimator type
  enum Est{PTP, PTL};
  Est _estType;
  if (argc >=1)
    _estType = PTL;
  else
    _estType = PTP;

  TsdGrid* grid = new TsdGrid(dimX, dimY, cellSize);
  grid->setMaxTruncation(4.0*cellSize);


  // Initialization of 2D viewer
  // image display is only possible for image dimensions divisible by 4
  const unsigned int w = grid->getCellsX();
  const unsigned int h = grid->getCellsY();
  unsigned char* image = new unsigned char[3*w*h];
  double ratio = double(w)/double(h);
  double screen_width = 1000;
  Obvious2D viewer(screen_width, screen_width/ratio, "tsd_grid_lms100");


  // Translation of sensor
  double tx = dimX/2.0;
  double ty = dimY/2.0;

  // Rotation about z-axis of sensor
  double phi = 0.0 * M_PI / 180.0;
  double tf[9] = {cos(phi), -sin(phi), tx,
                   sin(phi),  cos(phi), ty,
                           0,         0,        1};
  Matrix Tinit(3, 3);
  Tinit.setData(tf);

  Matrix LastScanPose = Tinit;

  // Sensor initialization
  SickLMS100 lms;
  int     rays       = lms.getNumberOfRays();
  double  angularRes = lms.getAngularRes();
  double  minPhi     = lms.getStartAngle();

  cout << "Rays: " << rays << " angular resolution: " << angularRes << " " << " min Phi: " << minPhi << endl;
  SensorPolar2D sensor(rays, angularRes, minPhi);


  RayCastPolar2D rayCaster;
  double* mCoords = new double[rays*2];
  double* mNormals = new double[rays*2];
  double* map      = new double[grid->getCellsX()*grid->getCellsY()*2];

  // Compose ICP modules
  int iterations                 = 25;
  PairAssignment* assigner       = (PairAssignment*)  new AnnPairAssignment(2);
  OutOfBoundsFilter2D* filterBounds = new OutOfBoundsFilter2D(grid->getMinX(), grid->getMaxX(), grid->getMinY(), grid->getMaxY());
  filterBounds->setPose(&Tinit);
  assigner->addPreFilter(filterBounds);
  DistanceFilter* filterDist = new DistanceFilter(2.0, 0.01, iterations);
  assigner->addPostFilter(filterDist);

//  // choose estimator
//  IRigidEstimator* estimator;
//  if (_estType == PTP)
//    estimator     = (IRigidEstimator*) new ClosedFormEstimator2D();
//  else
//    IRigidEstimator* estimator    = (IRigidEstimator*) new PointToLine2DEstimator();

  IRigidEstimator* estimator    = (IRigidEstimator*) new ClosedFormEstimator2D();


  Icp* icp = new Icp(assigner, estimator);
  icp->setMaxRMS(0.0);
  icp->setMaxIterations(iterations);
  icp->setConvergenceCounter(iterations);
  //icp->setAssignmentCallback(callbackAssignment);

  // Set first model
  lms.grab();
  sensor.setRealMeasurementData(lms.getRanges());
  sensor.transform(&Tinit);

  for(unsigned int i=0; i<10; i++)
    grid->push(&sensor);


  unsigned int initCount = 0;
  while(viewer.isAlive())
  {
    lms.grab();

    unsigned int mSize = 0;
    rayCaster.calcCoordsFromCurrentView(grid, &sensor, mCoords, mNormals, &mSize);
//    LOGMSG(DBG_DEBUG, "Raycast resulted in " << mSize << " coordinates");

    double* sCoords = lms.getCoords();

    Matrix* M = new Matrix(mSize/2, 2, mCoords);
    Matrix* N = new Matrix(mSize/2, 2, mNormals);
    Matrix* S = new Matrix(rays, 2, sCoords);

    icp->reset();
    icp->setModel(M->getBuffer(), N->getBuffer());
    icp->setScene(S->getBuffer());

    double rms;
    unsigned int pairs;
    unsigned int it;

    icp->iterate(&rms, &pairs, &it);
//    LOGMSG(DBG_DEBUG, "ICP result - RMS: " << rms << " pairs: " << pairs << " iterations: " << it << endl;)

    Matrix* T = icp->getFinalTransformation();

    double poseX  = gsl_matrix_get(T->getBuffer(), 0, 2);
    double poseY  = gsl_matrix_get(T->getBuffer(), 1, 2);
    double curPhi = acos(gsl_matrix_get(T->getBuffer(), 0, 0));
    double lastX  = gsl_matrix_get(LastScanPose.getBuffer(), 0, 2);
    double lastY  = gsl_matrix_get(LastScanPose.getBuffer(), 1, 2);
    double lastPhi= acos(gsl_matrix_get(LastScanPose.getBuffer(), 0, 0));

    double deltaX   = poseX - lastX;
    double deltaY   = poseY - lastY;
    double deltaPhi = fabs(curPhi - lastPhi);

    std::cout << deltaX << std::endl;

    //if(deltaX>0.1) abort();
    filterBounds->setPose(sensor.getPose());

    /*T->print();
    usleep(2000000);*/

    sensor.setRealMeasurementData(lms.getRanges());
    sensor.transform(T);

    std::cout << sqrt(deltaX*deltaX + deltaY*deltaY) << std::endl;

    if (sqrt(deltaX*deltaX + deltaY*deltaY) > 0.05 || deltaPhi > 0.05|| initCount < 100)
    {
      grid->push(&sensor);
      LastScanPose = *T;
      std::cout << "Pushed to grid" << std::endl;
      initCount++;
    }

    // Visualize data
    grid->grid2ColorImage(image);
    unsigned int mapSize;
    rayCaster.calcCoordsAligned(grid, map, NULL, &mapSize);
    for(unsigned int i=0; i<mapSize/2; i++)
    {
      double x = map[2*i];
      double y = map[2*i+1];
      int u = x / cellSize;
      int v = h-(y / cellSize);
      if(u>0 && u<(int)w && v>0 && v<(int)h)
      {
        int idx = 3*(v*w+u);
        image[idx] = 0;
        image[idx+1] = 0;
        image[idx+2] = 0;
      }
    }
    double position[2];
    sensor.getPosition(position);
    int x, y;
    double dx, dy;
    grid->coord2Cell(position, &x, &y, &dx, &dy);
    int idx = ((h-y)*w+x);
    image[3*idx]   = 255;
    image[3*idx+1] = 0;
    image[3*idx+2] = 0;
    viewer.draw(image, w, h, 3, 0, 0);

    delete M;
    delete N;
    delete S;
  }

  //output for grayscale data
  //serializePGM("/tmp/tsd_grid.pgm", image, w, h, true);

  delete [] image;
  delete [] mCoords;
  delete [] mNormals;
}


