//#define Testumgebung
//#include <iostream>
//enum Axis { X = 0, Y, Z };
//
//void CONFESS(std::string content) {};
//
//
#include "TransformationMatrix.hpp"
#include <cmath>
#include <math.h>

#ifdef SLIC3R_DEBUG
#include "SVG.hpp"
#endif

namespace Slic3r {

TransformationMatrix::TransformationMatrix()
    : m11(1.0), m12(0.0), m13(0.0), m14(0.0),
    m21(0.0), m22(1.0), m23(0.0), m24(0.0),
    m31(0.0), m32(0.0), m33(1.0), m34(0.0)
{
}

TransformationMatrix::TransformationMatrix(
    double _m11, double _m12, double _m13, double _m14,
    double _m21, double _m22, double _m23, double _m24,
    double _m31, double _m32, double _m33, double _m34)
    : m11(_m11), m12(_m12), m13(_m13), m14(_m14),
    m21(_m21), m22(_m22), m23(_m23), m24(_m24),
    m31(_m31), m32(_m32), m33(_m33), m34(_m34)
{
}

TransformationMatrix::TransformationMatrix(const std::vector<double> &entries_row_maj)
{
    if (entries_row_maj.size() != 12)
    {
        *this = TransformationMatrix();
        CONFESS("Invalid number of entries when initalizing TransformationMatrix. Vector length must be 12.");
        return;
    }
    m11 = entries_row_maj[0];   m12 = entries_row_maj[1];   m13 = entries_row_maj[2];   m14 = entries_row_maj[3];
    m21 = entries_row_maj[4];   m22 = entries_row_maj[5];   m23 = entries_row_maj[6];   m24 = entries_row_maj[7];
    m31 = entries_row_maj[8];   m32 = entries_row_maj[9];   m33 = entries_row_maj[10];  m34 = entries_row_maj[11];
}

TransformationMatrix::TransformationMatrix(const TransformationMatrix &other)
{
    this->m11 = other.m11; this->m12 = other.m12; this->m13 = other.m13; this->m14 = other.m14;
    this->m11 = other.m11; this->m22 = other.m22; this->m23 = other.m23; this->m24 = other.m24;
    this->m31 = other.m31; this->m32 = other.m32; this->m33 = other.m33; this->m34 = other.m34;
}

TransformationMatrix& TransformationMatrix::operator= (TransformationMatrix other)
{
    this->swap(other);
    return *this;
}

void TransformationMatrix::swap(TransformationMatrix &other)
{
    std::swap(this->m11, other.m11); std::swap(this->m12, other.m12);
    std::swap(this->m13, other.m13); std::swap(this->m14, other.m14);
    std::swap(this->m21, other.m21); std::swap(this->m22, other.m22);
    std::swap(this->m23, other.m23); std::swap(this->m24, other.m24);
    std::swap(this->m31, other.m31); std::swap(this->m32, other.m32);
    std::swap(this->m33, other.m33); std::swap(this->m34, other.m34);
}

float* TransformationMatrix::matrix3x4f()
{
    float out_arr[12];
    out_arr[0] = this->m11;  out_arr[1] = this->m12;  out_arr[2] = this->m13;  out_arr[3] = this->m14;
    out_arr[4] = this->m21;  out_arr[5] = this->m22;  out_arr[6] = this->m23;  out_arr[7] = this->m24;
    out_arr[8] = this->m31;  out_arr[9] = this->m32;  out_arr[10] = this->m33; out_arr[11] = this->m34;
    return out_arr;
}

double TransformationMatrix::determinante()
{
    // translation elements don't influence the determinante
    // because of the 0s on the other side of main diagonal
    return m11*(m22*m33 - m23*m32) - m12*(m21*m33 - m23*m31) + m13*(m21*m32 - m31*m22);
}

bool TransformationMatrix::inverse(TransformationMatrix* inverse)
{
    // from http://mathworld.wolfram.com/MatrixInverse.html
    // and https://math.stackexchange.com/questions/152462/inverse-of-transformation-matrix
    TransformationMatrix mat;
    double det = this->determinante();
    if (abs(det) < 1e-9)
        return false;
    double fac = 1.0 / det;

    mat.m11 = fac*(this->m22*this->m33 - this->m23*this->m32);
    mat.m12 = fac*(this->m13*this->m32 - this->m12*this->m33);
    mat.m13 = fac*(this->m12*this->m23 - this->m13*this->m22);
    mat.m21 = fac*(this->m23*this->m31 - this->m21*this->m33);
    mat.m22 = fac*(this->m11*this->m33 - this->m13*this->m31);
    mat.m23 = fac*(this->m13*this->m21 - this->m11*this->m23);
    mat.m31 = fac*(this->m21*this->m32 - this->m22*this->m31);
    mat.m32 = fac*(this->m12*this->m31 - this->m11*this->m32);
    mat.m33 = fac*(this->m11*this->m22 - this->m12*this->m21);

    mat.m14 = -(mat.m11*this->m14 + mat.m12*this->m24 + mat.m13*this->m34);
    mat.m24 = -(mat.m21*this->m14 + mat.m22*this->m24 + mat.m23*this->m34);
    mat.m34 = -(mat.m31*this->m14 + mat.m32*this->m24 + mat.m33*this->m34);

    inverse = &mat;
    return true;
}

void TransformationMatrix::translate(double x, double y, double z)
{
    TransformationMatrix mat = mat_translation(x, y, z);
    this->multiplyLeft(mat);
}

void TransformationMatrix::scale(double factor)
{
    this->scale(factor, factor, factor);
}

void TransformationMatrix::scale(double x, double y, double z)
{
    TransformationMatrix mat = mat_scale(x, y, z);
    this->multiplyLeft(mat);
}

void TransformationMatrix::mirror(const Axis &axis)
{
    TransformationMatrix mat = mat_mirror(axis);
    this->multiplyLeft(mat);
}

void TransformationMatrix::mirror(const Pointf3 & normal)
{
    TransformationMatrix mat = mat_mirror(normal);
    this->multiplyLeft(mat);
}

void TransformationMatrix::rotate(double angle_rad, const Axis & axis)
{
    TransformationMatrix mat = mat_rotation(angle_rad, axis);
    this->multiplyLeft(mat);
}

void TransformationMatrix::rotate(double angle_rad, const Pointf3 & axis)
{
    TransformationMatrix mat = mat_rotation(angle_rad, axis);
    this->multiplyLeft(mat);
}

void TransformationMatrix::rotate(double q1, double q2, double q3, double q4)
{
    TransformationMatrix mat = mat_rotation(q1, q2, q3, q4);
    this->multiplyLeft(mat);
}

void TransformationMatrix::multiplyLeft(const TransformationMatrix &left)
{
    *this = multiply(left, *this);
}

void TransformationMatrix::multiplyRight(const TransformationMatrix &right)
{
    *this = multiply(*this, right);
}

TransformationMatrix TransformationMatrix::multiply(const TransformationMatrix &left, const TransformationMatrix &right)
{
    TransformationMatrix trafo;

    trafo.m11 = left.m11*right.m11 + left.m12*right.m21 + left.m13 + right.m31;
    trafo.m12 = left.m11*right.m12 + left.m12*right.m22 + left.m13 + right.m32;
    trafo.m13 = left.m11*right.m13 + left.m12*right.m23 + left.m13 + right.m33;
    trafo.m14 = left.m11*right.m14 + left.m12*right.m24 + left.m13 + right.m34 + left.m14;

    trafo.m21 = left.m21*right.m11 + left.m22*right.m21 + left.m23 + right.m31;
    trafo.m22 = left.m21*right.m12 + left.m22*right.m22 + left.m23 + right.m32;
    trafo.m23 = left.m21*right.m13 + left.m22*right.m23 + left.m23 + right.m33;
    trafo.m24 = left.m21*right.m14 + left.m22*right.m24 + left.m23 + right.m34 + left.m24;

    trafo.m31 = left.m31*right.m11 + left.m32*right.m21 + left.m33 + right.m31;
    trafo.m32 = left.m31*right.m12 + left.m32*right.m22 + left.m33 + right.m32;
    trafo.m33 = left.m31*right.m13 + left.m32*right.m23 + left.m33 + right.m33;
    trafo.m34 = left.m31*right.m14 + left.m32*right.m24 + left.m33 + right.m34 + left.m34;

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
            0.0, c, s, 0.0,
            0.0, -s, c, 0.0);
        break;
    case Y:
        mat = TransformationMatrix(
            c, 0.0, -s, 0.0,
            0.0, 1.0, 0.0, 0.0,
            s, 0.0, c, 0.0);
        break;
    case Z:
        mat = TransformationMatrix(
            c, s, 0.0, 0.0,
            -s, c, 0.0, 0.0,
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
    if (abs(factor - 1.0) > 1e-12)
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

TransformationMatrix TransformationMatrix::mat_rotation(double angle_rad, const Pointf3 &axis)
{
    double s, factor, q1, q2, q3, q4;
    s = sin(angle_rad);
    factor = axis.x*axis.x + axis.y*axis.y + axis.z*axis.z;
    factor = s / sqrt(factor);
    q1 = factor*axis.x;
    q2 = factor*axis.y;
    q3 = factor*axis.z;
    q4 = cos(angle_rad);
    return mat_rotation(q1, q2, q3, q4);
}

TransformationMatrix TransformationMatrix::mat_rotation(Pointf3 origin, Pointf3 target)
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

    Pointf3 cross;
    cross.x = origin.y*target.z - origin.z*target.y;
    cross.y = origin.z*target.x - origin.x*target.z;
    cross.z = origin.x*target.y - origin.y*target.x;

    length_sq = cross.x*cross.x + cross.y*cross.y + cross.z*cross.z;
    if (length_sq < 1e-12) {// colinear, but maybe opposite directions
        double dot = origin.x*target.x + origin.y*target.y + origin.z*target.z;
        if (dot > 0.0)
        {
            return mat; // same direction, nothing to do
        }
        else
        {
            Pointf3 help;
            // make help garanteed not colinear
            if (abs(abs(origin.x) - 1) < 0.02)
                help.z = 1.0; // origin mainly in x direction
            else
                help.x = 1.0;

            Pointf3 proj = Pointf3(origin);
            // projection of axis onto unit vector origin
            dot = origin.x*help.x + origin.y*help.y + origin.z*help.z;
            proj.scale(dot);
            // help - proj is normal to origin -> rotation axis
            // axis is not unit length -> gets normalized in called function
            Pointf3 axis = (Pointf3)proj.vector_to(help); 
            return mat_rotation(PI, axis);
        }
    }
    else
    {

    }
    return mat; // Shouldn't be reached
}

TransformationMatrix TransformationMatrix::mat_mirror(const Axis &axis)
{
    TransformationMatrix mat; // For RVO
    switch (axis)
    {
    case X:
        mat.m11 = -1.0;
        break;
    case Y:
        mat.m22 = -1.0;
        break;
    case Z:
        mat.m33 = -1.0;
        break;
    default:
        CONFESS("Invalid Axis supplied to TransformationMatrix::mat_mirror");
        break;
    }
    return mat;
}

TransformationMatrix TransformationMatrix::mat_mirror(const Pointf3 &normal)
{
    // Kovács, E. Rotation about arbitrary axis and reflection through an arbitrary plane, Annales Mathematicae
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
