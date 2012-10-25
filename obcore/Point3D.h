/*********************************************************************************/
/*                                                                               */
/* class Point		                                                         */
/*                                                                               */
/*      Author: Ch. Merkl                                                        */
/*  Created on: 25.10.2012                                                       */
/* Description:	class for 3D points                                              */
/*                                                                               */
/*********************************************************************************/
#ifndef __POINT_3D__
#define __POINT_3D__

namespace obvious {

class Point3D
{
public:
    //! default constructor
    Point3D(const double& x = double(0.0), const double& y = double(0.0), const double& z = double(0,0)) : m_x(x), m_y(y), m_z(z) { }

    //! copy constructor
    Point3D(const Point3D& point) : m_x(point.m_x), m_y(point.m_y), m_x(point.m_z) { }

    //! get x value
    const double& x(void) const { return m_x; }
    //! get y value
    const double& y(void) const { return m_y; }
    //! get z value
    const double& z(void) const { return m_z; }

    //! set x value
    void setX(const double& x) { m_x = x; }
    //! set y value
    void setY(const double& y) { m_y = y; }
    //! set z value
    void setZ(const double& z) { m_z = z; }

    //! operator +=
    Point3D& operator+=(const Point3D& right) { m_x += right.m_x; m_y += right.m_y; m_z += right.m_z; return *this; }
    //! operator -=
    Point3D& operator-=(const Point3D& right) { m_x -= right.m_x; m_y -= right.m_y; m_z -= right.m_z; return *this; }
    //! operator =
    Point3D& operator= (const Point3D& right) { m_x  = right.m_x; m_y  = right.m_y; m_z  = right.m_z; return *this; }
    //! operator +
    Point3D  operator+ (const Point3D& right) { return Point3D(m_x + right.m_x, m_y + right.m_y, m_z + right.m_z);  }
    //! operator -
    Point3D  operator- (const Point3D& right) { return Point3D(m_x - right.m_x, m_y - right.m_y, m_z + right.m_z);  }

protected:
    double m_x;
    double m_y;
    double m_z;
};

}

#endif
