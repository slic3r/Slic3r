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

    /// Return the row-major form of the represented transformation matrix
    /// for admesh transform
    std::vector<double> matrix3x4f() const;

    /// Return the determinante of the matrix
    double determinante() const;

    /// Returns the inverse of the matrix
    bool inverse(TransformationMatrix &inverse) const;

    /// Perform Translation
    void translate(double x, double y, double z);
    void translateXY(Slic3r::Pointf position);

    /// Perform uniform scale
    void scale(double factor);

    /// Perform per-axis scale
    void scale(double x, double y, double z);

    /// Perform mirroring along given axis
    void mirror(const Axis &axis);

    /// Perform mirroring along given axis
    void mirror(const Pointf3 &normal);

    /// Perform rotation around given axis
    void rotate(double angle_rad, const Axis &axis);

    /// Perform rotation around arbitrary axis
    void rotate(double angle_rad, const Pointf3 &axis);

    /// Perform rotation defined by unit quaternion
    void rotate(double q1, double q2, double q3, double q4);

    /// Multiplies the Parameter-Matrix from the left (this=left*this)
    void applyLeft(const TransformationMatrix &left);

    /// Multiplies the Parameter-Matrix from the left (out=left*this)
    TransformationMatrix multiplyLeft(const TransformationMatrix &left);

    /// Multiplies the Parameter-Matrix from the right (this=this*right)
    void applyRight(const TransformationMatrix &right);

    /// Multiplies the Parameter-Matrix from the right (out=this*right)
    TransformationMatrix multiplyRight(const TransformationMatrix &right);

    /// Generate an eye matrix.
    static TransformationMatrix mat_eye();

    /// Generate a per axis scaling matrix
    static TransformationMatrix mat_scale(double x, double y, double z);

    /// Generate a uniform scaling matrix
    static TransformationMatrix mat_scale(double scale);

    /// Generate a reflection matrix by coordinate axis
    static TransformationMatrix mat_mirror(const Axis &axis);

    /// Generate a reflection matrix by arbitrary vector
    static TransformationMatrix mat_mirror(const Pointf3 &normal);

    /// Generate a translation matrix
    static TransformationMatrix mat_translation(double x, double y, double z);

    /// Generate a rotation matrix around coodinate axis
    static TransformationMatrix mat_rotation(double angle_rad, const Axis &axis);

    /// Generate a rotation matrix defined by unit quaternion q1*i + q2*j + q3*k + q4
    static TransformationMatrix mat_rotation(double q1, double q2, double q3, double q4);

    /// Generate a rotation matrix around arbitrary axis
    static TransformationMatrix mat_rotation(double angle_rad, const Pointf3 &axis);

    /// Generate a rotation matrix by specifying a vector (origin) that is to be rotated 
    /// to be colinear with another vector (target)
    static TransformationMatrix mat_rotation(Pointf3 origin, Pointf3 target);

    /// Performs a matrix multiplication
    static TransformationMatrix multiply(const TransformationMatrix &left, const TransformationMatrix &right);
};

}

#endif
