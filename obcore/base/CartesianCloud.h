#ifndef CARTESIANCLOUD_H
#define CARTESIANCLOUD_H

#include <vector>
#include <map>
#include <math.h>

#include <gsl/gsl_matrix.h>
#include "obcore/math/Matrix.h"

using namespace std;

/**
 * @namespace obvious
 */
namespace obvious
{

  enum EnumPointAttribute { ePointAttrValid = 0x1};

  enum EnumSourceInfo{eSOURCEROWS=0, eSOURCECOLS=1};

  /**
   * @class CartesianCloud3D
   * @brief Represents a cloud of points in 3D space
   * @author Stefan May
   **/
  class CartesianCloud3D
  {
  public:
    /**
     * Constructor;
     */
    CartesianCloud3D(unsigned int size, double* coords, unsigned char* rgb, double* normals);

    /**
     * Constructor;
     */
    CartesianCloud3D(unsigned int size, bool withInfo = false);

    /**
     * Destructor
     */
    ~CartesianCloud3D();

    /**
     * Copy constructor
     */
    CartesianCloud3D(CartesianCloud3D* cloud);

    /**
     * Accessor to each point via array indices.
     * @param i point index
     */
    double* operator [] (unsigned int i);

    /**
     * Get access to array of coordinates
     * @return pointer to matrix instance
     */
    gsl_matrix* getCoords();

    void setNormals(gsl_matrix* normals);

    gsl_matrix* getNormals();

    /**
     * Get access to array of colors
     * @return pointer to matrix instance
     */
    unsigned char* getColors();

    /**
     * Get access to array of attributes
     * @return pointer pointer to array of attributes
     */
    int* getAttributes();

    /**
     * Get access to array of indices. Indices are related to the time of instantiation.
     * The cloud might be modified through a method, e.g., removeInvalidPoints, though, indices
     * are not necessarily in ascending order.
     * @return pointer pointer to array of indices
     */
    int* getIndices();

    /**
     * Shows presence of additional point info
     * @return flag of presence
     */
    int hasInfo();

    /**
     * Add additional source information
     * @param eSourceInfo source information identifier
     * @param lValue the info
     */
    void addSourceInfo(EnumSourceInfo eSourceInfo, long lValue);

    /**
     * Shows presence of additional source information. This is e.g. a row and column width.
     * @return flag of presence
     */
    int hasSourceInfo();

    /**
     * Accessor to additional source information
     * @param eSourceInfo source information identifier
     * @param plValue the info
     * @return success
     */
    int getSourceInfo(EnumSourceInfo eSourceInfo, long* plValue);

    /**
     * Clear source info map
     */
    void clearSourceInfo();

    void maskPoints(bool* mask);
    void maskEmptyNormals();

    /**
     * Remove points which have an invalid flag in attributes
     */
    void removeInvalidPoints();

    /**
     * Sub-sample point cloud
     * @param step subsampling step
     */
    void subsample(unsigned int step);

    /**
     * Get size of CartesianCloud
     * @return number of points
     */
    unsigned int size();

    /**
     * Transform the point cloud, i.e. translate and/or rotate it
     * @param matrix a 4x4 translation matrix
     */
    void transform(gsl_matrix* T);
    void transform(Matrix* T);
    void transform(double T[16]);

    /**
     * Create a perspective projection depending on a projection matrix
     * @param pImage projective image
     * @param pMask mask to be considered
     * @param P point coordinates
     * @param nW width of image
     * @param nH height of image
     */
    void createProjection(unsigned char* pImage, unsigned char* pMask, gsl_matrix* P, int nW, int nH);

    /**
     * Create a z-Buffer depending on a projection matrix
     * @param pImage projective image
     * @param zbuffer z-Buffer
     * @param P point coordinates
     * @param nW width of image
     * @param nH height of image
     */
    void createZBuffer(unsigned char* pImage, double* zbuffer, gsl_matrix* P, int nW, int nH);

  private:

    void init(unsigned int size, bool withInfo);

    /**
     * point container
     */
    gsl_matrix* _coords;
    gsl_matrix* _normals;
    unsigned char* _colors;
    int* _attributes;
    int* _indices;

    /**
     * Info flag signs presence of additional point information
     */
    int _hasInfo;

    /**
     * Info flag signs presence of additional point information
     */
    int _hasNormals;

    /**
     * Source info map
     */
    map<int, long> _mSourceInfo;

  };

}

#endif /*CARTESIANCLOUD_H*/
