#ifndef slic3r_TriangleMatrix_hpp_
#define slic3r_TriangleMatrix_hpp_

#ifndef Testumgebung
#include "libslic3r.h"
#include "Point.hpp"
#endif

namespace Slic3r {

class TransformationMatrix
{
public:
    TransformationMatrix();
#ifndef Testumgebung
    TransformationMatrix(std::vector<float> &entries);
#endif
    TransformationMatrix(float m11, float  m12, float  m13, float  m14, float  m21, float  m22, float  m23, float  m24, float  m31, float  m32, float  m33, float  m34);
    TransformationMatrix(const TransformationMatrix &other);
    TransformationMatrix& operator= (TransformationMatrix other);
    void swap(TransformationMatrix &other);

    /// matrix entries 
    float m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34;

    /// Return the row-major form of the represented transformation matrix
    float* matrix3x4f();

    /// Return the determinante of the matrix
    float determinante();

    /// Returns the inverse of the matrix
    bool inverse(TransformationMatrix * inverse);

    /// Perform Translation
    void translate(float x, float y, float z);

    /// Perform uniform scale
    void scale(float factor);

    /// Perform per-axis scale
    void scale(float x, float y, float z);

    /// Perform mirroring along given axis
    void mirror(const Axis &axis);

    /// Perform mirroring along given axis
    void mirror(const Pointf3 &normal);

    /// Perform rotation around given axis
    void rotate(float angle_rad, const Axis &axis);

    /// Perform rotation around arbitrary axis
    void rotate(float angle_rad, const Pointf3 &axis);

    /// Perform rotation defined by unit quaternion
    void rotate(float q1, float q2, float q3, float q4);

    /// Multiplies the Parameter-Matrix from the left (this=left*this)
    void multiplyLeft(TransformationMatrix &left);

    /// Multiplies the Parameter-Matrix from the right (this=this*right)
    void multiplyRight(TransformationMatrix &right);

    /// Generate an eye matrix.
    static TransformationMatrix mat_eye();

    /// Generate a per axis scaling matrix
    static TransformationMatrix mat_scale(float x, float y, float z);

    /// Generate a uniform scaling matrix
    static TransformationMatrix mat_scale(float scale);

    /// Generate a reflection matrix by coordinate axis
    static TransformationMatrix mat_mirror(const Axis &axis);

    /// Generate a reflection matrix by arbitrary vector
    static TransformationMatrix mat_mirror(const Pointf3 &normal);

    /// Generate a translation matrix
    static TransformationMatrix mat_translation(float x, float y, float z);

    /// Generate a rotation matrix around coodinate axis
    static TransformationMatrix mat_rotation(float angle_rad, const Axis &axis);

    /// Generate a rotation matrix defined by unit quaternion q1*i + q2*j + q3*k + q4
    static TransformationMatrix mat_rotation(float q1, float q2, float q3, float q4);

    /// Generate a rotation matrix around arbitrary axis
    static TransformationMatrix mat_rotation(float angle_rad, const Pointf3 &axis);

    /// Performs a matrix multiplication
    static TransformationMatrix multiply(const TransformationMatrix &left, const TransformationMatrix &right);
};

}

#endif
