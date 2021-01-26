#ifndef slic3r_TransformationMatrix_hpp_
#define slic3r_TransformationMatrix_hpp_

#include "libslic3r.h"
#include "Point.hpp"

namespace Slic3r {

/*

    The TransformationMatrix class was created to keep track of the transformations applied
    to the objects from the GUI. Reloading these objects will now preserve their orientation.

    As usual in engineering vectors are treated as column vectors.
    
    Note that if they are treated as row vectors, the order is inversed:
    (') denotes the transponse and column vectors
    Column:         out' = M1 * M2 * in'
    Row:             out = in * M2' * M1'

    Every vector gets a 4th component w added,
    with positions lying in hyperplane w=1
    and direction vectors lying in hyperplane w=0

    Using this, affine transformations (scaling, rotating, shearing, translating and their combinations)
    can be represented as 4x4 Matrix.
    The 4th row equals [0 0 0 1] in order to not alter the w component.
    The other entries are represented by the class properties mij (i-th row [0,1,2], j-th column [0,1,2,3]).
    The 4th row is not explicitly stored, it is hard coded in the multiply function.

    Column vectors have to be multiplied from the right side.

 */


class TransformationMatrix
{
public:
    TransformationMatrix();

    TransformationMatrix(const std::vector<double> &entries_row_maj);

    TransformationMatrix(
        double m00, double m01, double m02, double m03,
        double m10, double m11, double m12, double m13,
        double m20, double m21, double m22, double m23);

    TransformationMatrix(const TransformationMatrix &other);
    TransformationMatrix& operator= (TransformationMatrix other);
    void swap(TransformationMatrix &other);

    bool operator== (const TransformationMatrix &other) const;
    bool operator!= (const TransformationMatrix &other) const { return !(*this == other); };

    /// matrix entries 
    double m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23;

    /// return the row-major form of the represented transformation matrix
    /// for admesh transform
    std::vector<double> matrix3x4f() const;

    /// return the determinante of the matrix
    double determinante() const;

    /// returns the inverse of the matrix
    TransformationMatrix inverse() const;

    /// multiplies the parameter-matrix from the left (this=left*this)
    void applyLeft(const TransformationMatrix &left);

    /// multiplies the parameter-matrix from the left (out=left*this)
    TransformationMatrix multiplyLeft(const TransformationMatrix &left) const;

    /// multiplies the parameter-matrix from the right (this=this*right)
    void applyRight(const TransformationMatrix &right);

    /// multiplies the parameter-matrix from the right (out=this*right)
    TransformationMatrix multiplyRight(const TransformationMatrix &right) const;

    /// multiplies the Point from the right (out=this*right)
    Pointf3 transform(const Pointf3 &point, coordf_t w = 1.0) const;

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

    /// generates a translation matrix
    static TransformationMatrix mat_translation(const Vectorf3 &vector);

    /// generates a rotation matrix around coordinate axis
    static TransformationMatrix mat_rotation(double angle_rad, const Axis &axis);

    /// generates a rotation matrix around arbitrary axis
    static TransformationMatrix mat_rotation(double angle_rad, const Vectorf3 &axis);

    /// generates a rotation matrix defined by unit quaternion q1*i + q2*j + q3*k + q4
    static TransformationMatrix mat_rotation(double q1, double q2, double q3, double q4);

    /// generates a rotation matrix by specifying a vector (origin) that is to be rotated 
    /// to be colinear with another vector (target)
    static TransformationMatrix mat_rotation(Vectorf3 origin, Vectorf3 target);

    /// performs a matrix multiplication
    static TransformationMatrix multiply(const TransformationMatrix &left, const TransformationMatrix &right);
};

}

#endif
