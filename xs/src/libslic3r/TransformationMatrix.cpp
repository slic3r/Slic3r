#include "TransformationMatrix.hpp"
#include <cmath>
#include <math.h>

namespace Slic3r {

TransformationMatrix::TransformationMatrix()
    : m00(1.0), m01(0.0), m02(0.0), m03(0.0),
    m10(0.0), m11(1.0), m12(0.0), m13(0.0),
    m20(0.0), m21(0.0), m22(1.0), m23(0.0)
{
}

TransformationMatrix::TransformationMatrix(
    double _m00, double _m01, double _m02, double _m03,
    double _m10, double _m11, double _m12, double _m13,
    double _m20, double _m21, double _m22, double _m23)
    : m00(_m00), m01(_m01), m02(_m02), m03(_m03),
    m10(_m10), m11(_m11), m12(_m12), m13(_m13),
    m20(_m20), m21(_m21), m22(_m22), m23(_m23)
{
}

TransformationMatrix::TransformationMatrix(const std::vector<double> &entries_row_maj)
{
    if (entries_row_maj.size() != 12)
    {
        *this = TransformationMatrix();
        CONFESS("Invalid number of entries when initializing TransformationMatrix. Vector length must be 12.");
        return;
    }
    m00 = entries_row_maj[0];   m01 = entries_row_maj[1];   m02 = entries_row_maj[2];   m03 = entries_row_maj[3];
    m10 = entries_row_maj[4];   m11 = entries_row_maj[5];   m12 = entries_row_maj[6];   m13 = entries_row_maj[7];
    m20 = entries_row_maj[8];   m21 = entries_row_maj[9];   m22 = entries_row_maj[10];  m23 = entries_row_maj[11];
}

TransformationMatrix::TransformationMatrix(const TransformationMatrix &other)
{
    this->m00 = other.m00; this->m01 = other.m01; this->m02 = other.m02; this->m03 = other.m03;
    this->m10 = other.m10; this->m11 = other.m11; this->m12 = other.m12; this->m13 = other.m13;
    this->m20 = other.m20; this->m21 = other.m21; this->m22 = other.m22; this->m23 = other.m23;
}

TransformationMatrix& TransformationMatrix::operator= (TransformationMatrix other)
{
    this->swap(other);
    return *this;
}

void TransformationMatrix::swap(TransformationMatrix &other)
{
    std::swap(this->m00, other.m00); std::swap(this->m01, other.m01);
    std::swap(this->m02, other.m02); std::swap(this->m03, other.m03);
    std::swap(this->m10, other.m10); std::swap(this->m11, other.m11);
    std::swap(this->m12, other.m12); std::swap(this->m13, other.m13);
    std::swap(this->m20, other.m20); std::swap(this->m21, other.m21);
    std::swap(this->m22, other.m22); std::swap(this->m23, other.m23);
}

bool TransformationMatrix::operator==(const TransformationMatrix &other) const
{
    double const eps = EPSILON; 
    bool is_equal = true;
    is_equal &= (std::abs(this->m00 - other.m00) < eps);
    is_equal &= (std::abs(this->m01 - other.m01) < eps);
    is_equal &= (std::abs(this->m02 - other.m02) < eps);
    is_equal &= (std::abs(this->m03 - other.m03) < eps);
    is_equal &= (std::abs(this->m10 - other.m10) < eps);
    is_equal &= (std::abs(this->m11 - other.m11) < eps);
    is_equal &= (std::abs(this->m12 - other.m12) < eps);
    is_equal &= (std::abs(this->m13 - other.m13) < eps);
    is_equal &= (std::abs(this->m20 - other.m20) < eps);
    is_equal &= (std::abs(this->m21 - other.m21) < eps);
    is_equal &= (std::abs(this->m22 - other.m22) < eps);
    is_equal &= (std::abs(this->m23 - other.m23) < eps);
    return is_equal;
}

std::vector<double> TransformationMatrix::matrix3x4f() const
{
    std::vector<double> out_arr(0);
    out_arr.reserve(12);
    out_arr.push_back(this->m00);
    out_arr.push_back(this->m01);
    out_arr.push_back(this->m02);
    out_arr.push_back(this->m03);
    out_arr.push_back(this->m10);
    out_arr.push_back(this->m11);
    out_arr.push_back(this->m12);
    out_arr.push_back(this->m13);
    out_arr.push_back(this->m20);
    out_arr.push_back(this->m21);
    out_arr.push_back(this->m22);
    out_arr.push_back(this->m23);
    return out_arr;
}

double TransformationMatrix::determinante() const
{
    // translation elements don't influence the determinante
    // because of the 0s on the other side of main diagonal
    return m00*(m11*m22 - m12*m21) - m01*(m10*m22 - m12*m20) + m02*(m10*m21 - m20*m11);
}

TransformationMatrix TransformationMatrix::inverse() const
{
    // from http://mathworld.wolfram.com/MatrixInverse.html
    // and https://math.stackexchange.com/questions/152462/inverse-of-transformation-matrix
    double det = this->determinante();
    if (std::abs(det) < 1e-9)
    {
        printf("Matrix (very close to) singular. Inverse cannot be computed");
        return TransformationMatrix();
    }
    double fac = 1.0 / det;

    TransformationMatrix inverse;

    inverse.m00 = fac * (this->m11 * this->m22 - this->m12 * this->m21);
    inverse.m01 = fac * (this->m02 * this->m21 - this->m01 * this->m22);
    inverse.m02 = fac * (this->m01 * this->m12 - this->m02 * this->m11);
    inverse.m10 = fac * (this->m12 * this->m20 - this->m10 * this->m22);
    inverse.m11 = fac * (this->m00 * this->m22 - this->m02 * this->m20);
    inverse.m12 = fac * (this->m02 * this->m10 - this->m00 * this->m12);
    inverse.m20 = fac * (this->m10 * this->m21 - this->m11 * this->m20);
    inverse.m21 = fac * (this->m01 * this->m20 - this->m00 * this->m21);
    inverse.m22 = fac * (this->m00 * this->m11 - this->m01 * this->m10);

    inverse.m03 = -(inverse.m00 * this->m03 + inverse.m01 * this->m13 + inverse.m02 * this->m23);
    inverse.m13 = -(inverse.m10 * this->m03 + inverse.m11 * this->m13 + inverse.m12 * this->m23);
    inverse.m23 = -(inverse.m20 * this->m03 + inverse.m21 * this->m13 + inverse.m22 * this->m23);

    return inverse;
}

void TransformationMatrix::applyLeft(const TransformationMatrix &left)
{
    TransformationMatrix temp = multiply(left, *this);
    *this = temp;
}

TransformationMatrix TransformationMatrix::multiplyLeft(const TransformationMatrix &left) const
{
    return multiply(left, *this);
}

void TransformationMatrix::applyRight(const TransformationMatrix &right)
{
    TransformationMatrix temp = multiply(*this, right);
    *this = temp;
}

TransformationMatrix TransformationMatrix::multiplyRight(const TransformationMatrix &right) const
{
    return multiply(*this, right);
}

Pointf3 TransformationMatrix::transform(const Pointf3 &point, coordf_t w) const
{
    Pointf3 out;
    out.x = this->m00 * point.x + this->m01 * point.y + this->m02 * point.z + this->m03 * w;
    out.y = this->m10 * point.x + this->m11 * point.y + this->m12 * point.z + this->m13 * w;
    out.z = this->m20 * point.x + this->m21 * point.y + this->m22 * point.z + this->m23 * w;
    return out;
}

TransformationMatrix TransformationMatrix::multiply(const TransformationMatrix &left, const TransformationMatrix &right)
{
    TransformationMatrix trafo;

    trafo.m00 = left.m00*right.m00 + left.m01*right.m10 + left.m02*right.m20;
    trafo.m01 = left.m00*right.m01 + left.m01*right.m11 + left.m02*right.m21;
    trafo.m02 = left.m00*right.m02 + left.m01*right.m12 + left.m02*right.m22;
    trafo.m03 = left.m00*right.m03 + left.m01*right.m13 + left.m02*right.m23 + left.m03;

    trafo.m10 = left.m10*right.m00 + left.m11*right.m10 + left.m12*right.m20;
    trafo.m11 = left.m10*right.m01 + left.m11*right.m11 + left.m12*right.m21;
    trafo.m12 = left.m10*right.m02 + left.m11*right.m12 + left.m12*right.m22;
    trafo.m13 = left.m10*right.m03 + left.m11*right.m13 + left.m12*right.m23 + left.m13;

    trafo.m20 = left.m20*right.m00 + left.m21*right.m10 + left.m22*right.m20;
    trafo.m21 = left.m20*right.m01 + left.m21*right.m11 + left.m22*right.m21;
    trafo.m22 = left.m20*right.m02 + left.m21*right.m12 + left.m22*right.m22;
    trafo.m23 = left.m20*right.m03 + left.m21*right.m13 + left.m22*right.m23 + left.m23;

    return trafo;
}

TransformationMatrix TransformationMatrix::mat_eye()
{
    return TransformationMatrix();
}

TransformationMatrix TransformationMatrix::mat_translation(double x, double y, double z)
{
    return TransformationMatrix(
        1.0, 0.0, 0.0, x,
        0.0, 1.0, 0.0, y,
        0.0, 0.0, 1.0, z);
}

TransformationMatrix TransformationMatrix::mat_translation(const Vectorf3 &vector)
{
    return TransformationMatrix(
        1.0, 0.0, 0.0, vector.x,
        0.0, 1.0, 0.0, vector.y,
        0.0, 0.0, 1.0, vector.z);
}

TransformationMatrix TransformationMatrix::mat_scale(double x, double y, double z)
{
    return TransformationMatrix(
        x, 0.0, 0.0, 0.0,
        0.0, y, 0.0, 0.0,
        0.0, 0.0, z, 0.0);
}

TransformationMatrix TransformationMatrix::mat_scale(double scale)
{
    return TransformationMatrix::mat_scale(scale, scale, scale);
}

TransformationMatrix TransformationMatrix::mat_rotation(double angle_rad, const Axis &axis)
{
    double s = sin(angle_rad);
    double c = cos(angle_rad);
    TransformationMatrix mat; // For RVO
    switch (axis)
    {
    case X:
        mat = TransformationMatrix(
            1.0, 0.0, 0.0, 0.0,
            0.0, c, -s, 0.0,
            0.0, s, c, 0.0);
        break;
    case Y:
        mat = TransformationMatrix(
            c, 0.0, s, 0.0,
            0.0, 1.0, 0.0, 0.0,
            -s, 0.0, c, 0.0);
        break;
    case Z:
        mat = TransformationMatrix(
            c, -s, 0.0, 0.0,
            s, c, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0);
        break;
    default:
        CONFESS("Invalid Axis supplied to TransformationMatrix::mat_rotation");
        mat = TransformationMatrix();
        break;
    }
    return mat;
}

TransformationMatrix TransformationMatrix::mat_rotation(double q1, double q2, double q3, double q4)
{
    double factor = q1*q1 + q2*q2 + q3*q3 + q4*q4;
    if (std::abs(factor - 1.0) > 1e-12)
    {
        factor = 1.0 / sqrt(factor);
        q1 *= factor;
        q2 *= factor;
        q3 *= factor;
        q4 *= factor;
    }
    // https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation#Quaternion-derived_rotation_matrix
    return TransformationMatrix(
        1.0 - 2.0 * (q2*q2 + q3*q3), 2.0 * (q1*q2 - q3*q4), 2.0 * (q1*q3 + q2*q4), 0.0,
        2.0 * (q1*q2 + q3*q4), 1.0 - 2.0 * (q1*q1 + q3*q3), 2.0 * (q2*q3 - q1*q4), 0.0,
        2.0 * (q1*q3 - q2*q4), 2.0 * (q2*q3 + q1*q4), 1.0 - 2.0 * (q1*q1 + q2*q2), 0.0);
}

TransformationMatrix TransformationMatrix::mat_rotation(double angle_rad, const Vectorf3 &axis)
{
    double s, factor, q1, q2, q3, q4;
    s = sin(angle_rad/2);
    factor = axis.x*axis.x + axis.y*axis.y + axis.z*axis.z;
    factor = s / sqrt(factor);
    q1 = factor*axis.x;
    q2 = factor*axis.y;
    q3 = factor*axis.z;
    q4 = cos(angle_rad/2);
    return mat_rotation(q1, q2, q3, q4);
}

TransformationMatrix TransformationMatrix::mat_rotation(Vectorf3 origin, Vectorf3 target)
{
    TransformationMatrix mat;
    double length_sq = origin.x*origin.x + origin.y*origin.y + origin.z*origin.z;
    double rec_length;
    if (length_sq < 1e-12)
    {
        CONFESS("0-length Vector supplied to TransformationMatrix::mat_rotation(origin,target)");
        return mat;
    }
    rec_length = 1.0 / sqrt(length_sq);
    origin.scale(rec_length);

    length_sq = target.x*target.x + target.y*target.y + target.z*target.z;
    if (length_sq < 1e-12)
    {
        CONFESS("0-length Vector supplied to TransformationMatrix::mat_rotation(origin,target)");
        return mat;
    }
    rec_length = 1.0 / sqrt(length_sq);
    target.scale(rec_length);

    Vectorf3 cross;
    cross.x = origin.y*target.z - origin.z*target.y;
    cross.y = origin.z*target.x - origin.x*target.z;
    cross.z = origin.x*target.y - origin.y*target.x;

    length_sq = cross.x*cross.x + cross.y*cross.y + cross.z*cross.z;
    double dot = origin.x*target.x + origin.y*target.y + origin.z*target.z;
    if (length_sq < 1e-12) {// colinear, but maybe opposite directions
        if (dot > 0.0)
        {
            return mat; // same direction, nothing to do
        }
        else
        {
            Vectorf3 help;
            // make help garanteed not colinear
            if (std::abs(std::abs(origin.x) - 1) < 0.02)
                help.z = 1.0; // origin mainly in x direction
            else
                help.x = 1.0;

            Vectorf3 proj = origin;
            // projection of axis onto unit vector origin
            dot = origin.x*help.x + origin.y*help.y + origin.z*help.z;
            proj.scale(dot);

            // help - proj is normal to origin -> rotation axis
            Vectorf3 axis = ((Pointf3)proj).vector_to((Pointf3)help);

            // axis is not unit length -> gets normalized in called function
            return mat_rotation(PI, axis);
        }
    }
    else
    {// not colinear, cross represents rotation axis so that angle is (0, 180)
        // dot's vectors have previously been normalized, so nothing to do except arccos
        double angle = acos(dot);

        // cross is (probably) not unit length -> gets normalized in called function
        return mat_rotation(angle, cross);
    }
    return mat; // Shouldn't be reached
}

TransformationMatrix TransformationMatrix::mat_mirror(const Axis &axis)
{
    TransformationMatrix mat; // For RVO
    switch (axis)
    {
    case X:
        mat.m00 = -1.0;
        break;
    case Y:
        mat.m11 = -1.0;
        break;
    case Z:
        mat.m22 = -1.0;
        break;
    default:
        CONFESS("Invalid Axis supplied to TransformationMatrix::mat_mirror");
        break;
    }
    return mat;
}

TransformationMatrix TransformationMatrix::mat_mirror(const Vectorf3 &normal)
{
    // Kov�cs, E. Rotation about arbitrary axis and reflection through an arbitrary plane, Annales Mathematicae
    // et Informaticae, Vol 40 (2012) pp 175-186
    // http://ami.ektf.hu/uploads/papers/finalpdf/AMI_40_from175to186.pdf
    double factor, c1, c2, c3;
    factor = normal.x*normal.x + normal.y*normal.y + normal.z*normal.z;
    factor = 1.0 / sqrt(factor);
    c1 = factor*normal.x;
    c2 = factor*normal.y;
    c3 = factor*normal.z;
    return TransformationMatrix(
        1.0 - 2.0 * c1*c1, -2 * c2*c1, -2 * c3*c1, 0.0,
        -2 * c2*c1, 1.0 - 2.0 * c2*c2, -2 * c2*c3, 0.0,
        -2 * c1*c3, -2 * c2*c3, 1.0 - 2.0 * c3*c3, 0.0);
}

}
