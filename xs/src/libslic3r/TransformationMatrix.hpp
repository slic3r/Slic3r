#ifndef slic3r_TriangleMatrix_hpp_
#define slic3r_TriangleMatrix_hpp_

#include "libslic3r.h"
#include "Point.hpp"

namespace Slic3r {

class TransformationMatrix
{
public:
    TransformationMatrix();

    TransformationMatrix(const std::vector<double> &entries_row_maj);

    TransformationMatrix(
        double m11, double m12, double m13, double m14,
        double m21, double m22, double m23, double m24,
        double m31, double m32, double m33, double m34);

    TransformationMatrix(const TransformationMatrix &other);
    TransformationMatrix& operator= (TransformationMatrix other);
    void swap(TransformationMatrix &other);

    /// matrix entries 
    double m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34;

    /// return the row-major form of the represented transformation matrix
    /// for admesh transform
    std::vector<double> matrix3x4f() const;

    /// return the determinante of the matrix
    double determinante() const;

    /// returns the inverse of the matrix
    bool inverse(TransformationMatrix &inverse) const;

    /// performs translation
    void translate(double x, double y, double z);
    void translate(Vectorf3 const &vector);
    void translateXY(Vectorf const &vector);

    /// performs uniform scale
    void scale(double factor);

    /// performs per-axis scale
    void scale(double x, double y, double z);

    /// performs per-axis scale via vector
    void scale(Vectorf3 const &vector);
    
    /// performs mirroring along given axis
    void mirror(const Axis &axis);

    /// performs mirroring along given axis
    void mirror(const Vectorf3 &normal);

    /// performs rotation around given axis
    void rotate(double angle_rad, const Axis &axis);

    /// performs rotation around arbitrary axis
    void rotate(double angle_rad, const Vectorf3 &axis);

    /// performs rotation defined by unit quaternion
    void rotate(double q1, double q2, double q3, double q4);

    /// multiplies the parameter-matrix from the left (this=left*this)
    void applyLeft(const TransformationMatrix &left);

    /// multiplies the parameter-matrix from the left (out=left*this)
    TransformationMatrix multiplyLeft(const TransformationMatrix &left);

    /// multiplies the parameter-matrix from the right (this=this*right)
    void applyRight(const TransformationMatrix &right);

    /// multiplies the parameter-matrix from the right (out=this*right)
    TransformationMatrix multiplyRight(const TransformationMatrix &right);

    /// generates an eye matrix.
    static TransformationMatrix mat_eye();

    /// generates a per axis scaling matrix
    static TransformationMatrix mat_scale(double x, double y, double z);

    /// generates an uniform scaling matrix
    static TransformationMatrix mat_scale(double scale);

    /// generates a reflection matrix by coordinate axis
    static TransformationMatrix mat_mirror(const Axis &axis);

    /// generates a reflection matrix by arbitrary vector
    static TransformationMatrix mat_mirror(const Vectorf3 &normal);

    /// generates a translation matrix
    static TransformationMatrix mat_translation(double x, double y, double z);

    /// generates a rotation matrix around coodinate axis
    static TransformationMatrix mat_rotation(double angle_rad, const Axis &axis);

    /// generates a rotation matrix defined by unit quaternion q1*i + q2*j + q3*k + q4
    static TransformationMatrix mat_rotation(double q1, double q2, double q3, double q4);

    /// generates a rotation matrix around arbitrary axis
    static TransformationMatrix mat_rotation(double angle_rad, const Vectorf3 &axis);

    /// generates a rotation matrix by specifying a vector (origin) that is to be rotated 
    /// to be colinear with another vector (target)
    static TransformationMatrix mat_rotation(Vectorf3 origin, Vectorf3 target);

    /// performs a matrix multiplication
    static TransformationMatrix multiply(const TransformationMatrix &left, const TransformationMatrix &right);
};

}

#endif
