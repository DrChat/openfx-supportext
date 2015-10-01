/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of openfx-supportext <https://github.com/devernay/openfx-supportext>,
 * Copyright (C) 2015 INRIA
 *
 * openfx-supportext is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * openfx-supportext is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-supportext.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

/*
 * OFX Matrix utilities.
 */

#ifndef openfx_supportext_ofxsMatrix2D_h
#define openfx_supportext_ofxsMatrix2D_h

#include <cmath>

namespace OFX {
// NEVER define a variable/constant in a header (said it 100 times already)
// an inline function is OK
inline double
ofxsPi()
{
    return 3.14159265358979323846264338327950288419717;
}

/**
 * @brief A simple 3 * 3 matrix class layed out as such:
 *  a b c
 *  d e f
 *  g h i
 **/

inline double
ofxsToDegrees(double rad)
{
    rad = rad * 180.0 / ofxsPi();

    return rad;
}

inline double
ofxsToRadians(double deg)
{
    deg = deg * ofxsPi() / 180.0;

    return deg;
}

struct Point3D
{
    double x,y,z;

    Point3D()
        : x(0), y(0), z(0)
    {
    }

    Point3D(double x,
            double y,
            double z)
        : x(x),y(y),z(z)
    {
    }

    Point3D(const Point3D & p)
        : x(p.x),y(p.y),z(p.z)
    {
    }

    bool operator==(const Point3D & other)
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct Point4D
{
    double x,y,z,w;

    Point4D()
        : x(0), y(0), z(0), w(0)
    {
    }

    Point4D(double x,
            double y,
            double z,
            double w)
        : x(x), y(y), z(z), w(w)
    {
    }

    Point4D(const Point4D & o)
        : x(o.x), y(o.y), z(o.z), w(o.w)
    {
    }

    double & operator() (int i)
    {
        switch (i) {
        case 0:

            return x;
        case 1:

            return y;
        case 2:

            return z;
        case 3:

            return w;
        default:
            assert(false);
            return x;
        }
    }

    double operator() (int i) const
    {
        switch (i) {
        case 0:

            return x;
        case 1:

            return y;
        case 2:

            return z;
        case 3:

            return w;
        default:
            assert(false);
            return x;
        }
    }

    bool operator==(const Point4D & o)
    {
        return x == o.x && y == o.y && z == o.z && w == o.w;
    }
};

struct Matrix3x3
{
    double a,b,c,d,e,f,g,h,i;

    Matrix3x3()
        : a(1), b(0), c(0), d(1), e(0), f(0), g(0), h(0), i(0)
    {
    }

    Matrix3x3(double a_,
              double b_,
              double c_,
              double d_,
              double e_,
              double f_,
              double g_,
              double h_,
              double i_)
        : a(a_),b(b_),c(c_),d(d_),e(e_),f(f_),g(g_),h(h_),i(i_)
    {
    }

    Matrix3x3(const Matrix3x3 & mat)
        : a(mat.a), b(mat.b), c(mat.c), d(mat.d), e(mat.e), f(mat.f), g(mat.g), h(mat.h), i(mat.i)
    {
    }

    Matrix3x3 & operator=(const Matrix3x3 & m)
    {
        a = m.a; b = m.b; c = m.c; d = m.d; e = m.e; f = m.f; g = m.g; h = m.h; i = m.i; return *this;
    }

    bool isIdentity() const
    {
        return a == 1 && b == 0 && c == 0 &&
               d == 0 && e == 1 && f == 0 &&
               g == 0 && h == 0 && i == 1;
    }
};

inline double ofxsMatDeterminant(const Matrix3x3 & M);
inline Matrix3x3 ofxsMatScaleAdjoint(const Matrix3x3 & M, double s);
inline Matrix3x3 ofxsMatInverse(const Matrix3x3 & M);
inline Matrix3x3 ofxsMatInverse(const Matrix3x3 & M,double det);
inline Matrix3x3 ofxsMatRotation(double rads);
inline Matrix3x3 ofxsMatRotationAroundPoint(double rads, double pointX, double pointY);
inline Matrix3x3 ofxsMatTranslation(double translateX, double translateY);
inline Matrix3x3 ofxsMatScale(double scaleX, double scaleY);
inline Matrix3x3 ofxsMatScale(double scale);
inline Matrix3x3 ofxsMatScaleAroundPoint(double scaleX, double scaleY, double pointX, double pointY);
inline Matrix3x3 ofxsMatSkewXY(double skewX, double skewY, bool skewOrderYX);

// matrix transform from destination to source, in canonical coordinates
inline Matrix3x3 ofxsMatInverseTransformCanonical(double translateX, double translateY, double scaleX, double scaleY, double skewX, double skewY, bool skewOrderYX, double rads, double centerX, double centerY);

// matrix transform from source to destination in canonical coordinates
inline Matrix3x3 ofxsMatTransformCanonical(double translateX, double translateY, double scaleX, double scaleY, double skewX, double skewY, bool skewOrderYX, double rads, double centerX, double centerY);

/// transform from pixel coordinates to canonical coordinates
inline Matrix3x3 ofxsMatPixelToCanonical(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
                                         double renderscaleX, //!< 0.5 for a half-resolution image
                                         double renderscaleY,
                                         bool fielded); //!< true if the image property kOfxImagePropField is kOfxImageFieldLower or kOfxImageFieldUpper (apply 0.5 field scale in Y
/// transform from canonical coordinates to pixel coordinates
inline Matrix3x3 ofxsMatCanonicalToPixel(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
                                         double renderscaleX, //!< 0.5 for a half-resolution image
                                         double renderscaleY,
                                         bool fielded); //!< true if the image property kOfxImagePropField is kOfxImageFieldLower or kOfxImageFieldUpper (apply 0.5 field scale in Y

// matrix transform from destination to source, in pixel coordinates
inline Matrix3x3 ofxsMatInverseTransformPixel(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
                                              double renderscaleX, //!< 0.5 for a half-resolution image
                                              double renderscaleY,
                                              bool fielded,
                                              double translateX, double translateY,
                                              double scaleX, double scaleY,
                                              double skewX,
                                              double skewY,
                                              bool skewOrderYX,
                                              double rads,
                                              double centerX, double centerY);

// matrix transform from source to destination in pixel coordinates
inline Matrix3x3 ofxsMatTransformPixel(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
                                       double renderscaleX, //!< 0.5 for a half-resolution image
                                       double renderscaleY,
                                       bool fielded,
                                       double translateX, double translateY,
                                       double scaleX, double scaleY,
                                       double skewX,
                                       double skewY,
                                       bool skewOrderYX,
                                       double rads,
                                       double centerX, double centerY);
inline Matrix3x3
operator*(const Matrix3x3 & m1,
          const Matrix3x3 & m2)
{
    return Matrix3x3(m1.a * m2.a + m1.b * m2.d + m1.c * m2.g,
                     m1.a * m2.b + m1.b * m2.e + m1.c * m2.h,
                     m1.a * m2.c + m1.b * m2.f + m1.c * m2.i,
                     m1.d * m2.a + m1.e * m2.d + m1.f * m2.g,
                     m1.d * m2.b + m1.e * m2.e + m1.f * m2.h,
                     m1.d * m2.c + m1.e * m2.f + m1.f * m2.i,
                     m1.g * m2.a + m1.h * m2.d + m1.i * m2.g,
                     m1.g * m2.b + m1.h * m2.e + m1.i * m2.h,
                     m1.g * m2.c + m1.h * m2.f + m1.i * m2.i);
}

inline Point3D
operator*(const Matrix3x3 & m,
          const Point3D & p)
{
    Point3D ret;

    ret.x = m.a * p.x + m.b * p.y + m.c * p.z;
    ret.y = m.d * p.x + m.e * p.y + m.f * p.z;
    ret.z = m.g * p.x + m.h * p.y + m.i * p.z;

    return ret;
}

struct Matrix4x4
{
    double data[16];

    Matrix4x4()
    {
        std::fill(data, data + 16, 0.);
    }

    Matrix4x4(const double d[16])
    {
        std::copy(d,d + 16,data);
    }

    Matrix4x4(const Matrix4x4 & o)
    {
        std::copy(o.data,o.data + 16,data);
    }

    double & operator()(int row,
                        int col)
    {
        assert(row >= 0 && row < 4 && col >= 0 && col < 4);

        return data[row * 4 + col];
    }

    double operator()(int row,
                      int col) const
    {
        assert(row >= 0 && row < 4 && col >= 0 && col < 4);

        return data[row * 4 + col];
    }
};

inline Matrix4x4
operator*(const Matrix4x4 & m1,
          const Matrix4x4 & m2)
{
    Matrix4x4 ret;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            for (int x = 0; x < 4; ++x) {
                ret(i,j) += m1(i,x) * m2(x,j);
            }
        }
    }

    return ret;
}

inline Point4D
operator*(const Matrix4x4 & m,
          const Point4D & p)
{
    Point4D ret;

    for (int i = 0; i < 4; ++i) {
        ret(i) = 0.;
        for (int j = 0; j < 4; ++j) {
            ret(i) += m(i,j) * p(j);
        }
    }

    return ret;
}

inline Matrix4x4
matrix4x4FromMatrix3x3(const Matrix3x3 & m)
{
    Matrix4x4 ret;

    ret(0,0) = m.a; ret(0,1) = m.b; ret(0,2) = m.c; ret(0,3) = 0.;
    ret(1,0) = m.d; ret(1,1) = m.e; ret(1,2) = m.f; ret(1,3) = 0.;
    ret(2,0) = m.g; ret(2,1) = m.h; ret(2,2) = m.i; ret(2,3) = 0.;
    ret(3,0) = 0.;  ret(3,1) = 0.;  ret(3,2) = 0.;  ret(3,3) = 1.;

    return ret;
}

////////////////////
// IMPLEMENTATION //
////////////////////

double
ofxsMatDeterminant(const Matrix3x3 & M)
{
    return M.a * (M.e * M.i - M.h * M.f)
           - M.b * (M.d * M.i - M.g * M.f)
           + M.c * (M.d * M.h - M.g * M.e);
}

Matrix3x3
ofxsMatScaleAdjoint(const Matrix3x3 & M,
                    double s)
{
    Matrix3x3 ret;

    ret.a = (s) * (M.e * M.i - M.h * M.f);
    ret.d = (s) * (M.f * M.g - M.d * M.i);
    ret.g = (s) * (M.d * M.h - M.e * M.g);

    ret.b = (s) * (M.c * M.h - M.b * M.i);
    ret.e = (s) * (M.a * M.i - M.c * M.g);
    ret.h = (s) * (M.b * M.g - M.a * M.h);

    ret.c = (s) * (M.b * M.f - M.c * M.e);
    ret.f = (s) * (M.c * M.d - M.a * M.f);
    ret.i = (s) * (M.a * M.e - M.b * M.d);

    return ret;
}

Matrix3x3
ofxsMatInverse(const Matrix3x3 & M)
{
    return ofxsMatScaleAdjoint( M, 1. / ofxsMatDeterminant(M) );
}

Matrix3x3
ofxsMatInverse(const Matrix3x3 & M,
               double det)
{
    return ofxsMatScaleAdjoint(M, 1. / det);
}

Matrix3x3
ofxsMatRotation(double rads)
{
    double c = std::cos(rads);
    double s = std::sin(rads);

    return Matrix3x3(c,s,0,-s,c,0,0,0,1);
}

Matrix3x3
ofxsMatRotationAroundPoint(double rads,
                           double px,
                           double py)
{
    return ofxsMatTranslation(px, py) * ( ofxsMatRotation(rads) * ofxsMatTranslation(-px, -py) );
}

Matrix3x3
ofxsMatTranslation(double x,
                   double y)
{
    return Matrix3x3(1., 0., x,
                     0., 1., y,
                     0., 0., 1.);
}

Matrix3x3
ofxsMatScale(double x,
             double y)
{
    return Matrix3x3(x,  0., 0.,
                     0., y,  0.,
                     0., 0., 1.);
}

Matrix3x3
ofxsMatScale(double s)
{
    return ofxsMatScale(s, s);
}

Matrix3x3
ofxsMatScaleAroundPoint(double scaleX,
                        double scaleY,
                        double px,
                        double py)
{
    return ofxsMatTranslation(px,py) * ( ofxsMatScale(scaleX,scaleY) * ofxsMatTranslation(-px, -py) );
}

Matrix3x3
ofxsMatSkewXY(double skewX,
              double skewY,
              bool skewOrderYX)
{
    return Matrix3x3(skewOrderYX ? 1. : (1. + skewX * skewY), skewX, 0.,
                     skewY, skewOrderYX ? (1. + skewX * skewY) : 1, 0.,
                     0., 0., 1.);
}

// matrix transform from destination to source
Matrix3x3
ofxsMatInverseTransformCanonical(double translateX,
                                 double translateY,
                                 double scaleX,
                                 double scaleY,
                                 double skewX,
                                 double skewY,
                                 bool skewOrderYX,
                                 double rads,
                                 double centerX,
                                 double centerY)
{
    ///1) We translate to the center of the transform.
    ///2) We scale
    ///3) We apply skewX and skewY in the right order
    ///4) We rotate
    ///5) We apply the global translation
    ///5) We translate back to the origin

    // since this is the inverse, oerations are in reverse order
    return ofxsMatTranslation(centerX, centerY) *
           ofxsMatScale(1. / scaleX, 1. / scaleY) *
           ofxsMatSkewXY(-skewX, -skewY, !skewOrderYX) *
           ofxsMatRotation(rads) *
           ofxsMatTranslation(-translateX,-translateY) *
           ofxsMatTranslation(-centerX,-centerY);
}

// matrix transform from source to destination
Matrix3x3
ofxsMatTransformCanonical(double translateX,
                          double translateY,
                          double scaleX,
                          double scaleY,
                          double skewX,
                          double skewY,
                          bool skewOrderYX,
                          double rads,
                          double centerX,
                          double centerY)
{
    ///1) We translate to the center of the transform.
    ///2) We scale
    ///3) We apply skewX and skewY in the right order
    ///4) We rotate
    ///5) We apply the global translation
    ///5) We translate back to the origin

    return ofxsMatTranslation(centerX, centerY) *
           ofxsMatTranslation(translateX, translateY) *
           ofxsMatRotation(-rads) *
           ofxsMatSkewXY(skewX, skewY, skewOrderYX) *
           ofxsMatScale(scaleX, scaleY) *
           ofxsMatTranslation(-centerX,-centerY);
}

// The transforms between pixel and canonical coordinated
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#MappingCoordinates

/// transform from pixel coordinates to canonical coordinates
Matrix3x3
ofxsMatPixelToCanonical(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
                        double renderscaleX, //!< 0.5 for a half-resolution image
                        double renderscaleY,
                        bool fielded) //!< true if the image property kOfxImagePropField is kOfxImageFieldLower or kOfxImageFieldUpper (apply 0.5 field scale in Y
{
    /*
       To map an X and Y coordinates from Pixel coordinates to Canonical coordinates, we perform the following multiplications...

       X' = (X * PAR)/SX
       Y' = Y/(SY * FS)
     */

    // FIXME: when it's the Upper field, showuldn't the first pixel start at canonical coordinate (0,0.5) ?
    return ofxsMatScale( pixelaspectratio / renderscaleX, 1. / ( renderscaleY * (fielded ? 0.5 : 1.0) ) );
}

/// transform from canonical coordinates to pixel coordinates
Matrix3x3
ofxsMatCanonicalToPixel(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
                        double renderscaleX, //!< 0.5 for a half-resolution image
                        double renderscaleY,
                        bool fielded) //!< true if the image property kOfxImagePropField is kOfxImageFieldLower or kOfxImageFieldUpper (apply 0.5 field scale in Y
{
    /*
       To map an X and Y coordinates from Canonical coordinates to Pixel coordinates, we perform the following multiplications...

       X' = (X * SX)/PAR
       Y' = Y * SY * FS
     */

    // FIXME: when it's the Upper field, showuldn't the first pixel start at canonical coordinate (0,0.5) ?
    return ofxsMatScale( renderscaleX / pixelaspectratio, renderscaleY * (fielded ? 0.5 : 1.0) );
}

// matrix transform from destination to source
Matrix3x3
ofxsMatInverseTransformPixel(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
                             double renderscaleX, //!< 0.5 for a half-resolution image
                             double renderscaleY,
                             bool fielded,
                             double translateX,
                             double translateY,
                             double scaleX,
                             double scaleY,
                             double skewX,
                             double skewY,
                             bool skewOrderYX,
                             double rads,
                             double centerX,
                             double centerY)
{
    ///1) We go from pixel to canonical
    ///2) we apply transform
    ///3) We go back to pixels

    return ofxsMatCanonicalToPixel(pixelaspectratio, renderscaleX, renderscaleY, fielded) *
           ofxsMatInverseTransformCanonical(translateX, translateY, scaleX, scaleY, skewX, skewY, skewOrderYX, rads, centerX, centerY) *
           ofxsMatPixelToCanonical(pixelaspectratio, renderscaleX, renderscaleY, fielded);
}

// matrix transform from source to destination
Matrix3x3
ofxsMatTransformPixel(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
                      double renderscaleX, //!< 0.5 for a half-resolution image
                      double renderscaleY,
                      bool fielded,
                      double translateX,
                      double translateY,
                      double scaleX,
                      double scaleY,
                      double skewX,
                      double skewY,
                      bool skewOrderYX,
                      double rads,
                      double centerX,
                      double centerY)
{
    ///1) We go from pixel to canonical
    ///2) we apply transform
    ///3) We go back to pixels

    return ofxsMatCanonicalToPixel(pixelaspectratio, renderscaleX, renderscaleY, fielded) *
           ofxsMatTransformCanonical(translateX, translateY, scaleX, scaleY, skewX, skewY, skewOrderYX, rads, centerX, centerY) *
           ofxsMatPixelToCanonical(pixelaspectratio, renderscaleX, renderscaleY, fielded);
}
};

#endif // openfx_supportext_ofxsMatrix2D_h
