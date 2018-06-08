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
    {
        *this = mat_eye();
    }

    TransformationMatrix::TransformationMatrix(float m11, float  m12, float  m13, float  m14, float  m21, float  m22, float  m23, float  m24, float  m31, float  m32, float  m33, float  m34)
    {
        this->m11 = m11; this->m12 = m12; this->m13 = m13; this->m14 = m14;
        this->m21 = m21; this->m22 = m22; this->m23 = m23; this->m21 = m24;
        this->m31 = m31; this->m32 = m32; this->m33 = m33; this->m34 = m34;
    }
#ifndef Testumgebung
    TransformationMatrix::TransformationMatrix(std::vector<float> &entries_row_maj)
    {
        if (entries_row_maj.size() != 12)
        {
            *this = mat_eye();
            CONFESS("Invalid number of entries when initalizing TransformationMatrix. Vector length must be 12.");
            return;
        }
        m11 = entries_row_maj[0];   m12 = entries_row_maj[1];   m13 = entries_row_maj[2];   m14 = entries_row_maj[3];
        m21 = entries_row_maj[4];   m22 = entries_row_maj[5];   m23 = entries_row_maj[6];   m24 = entries_row_maj[7];
        m31 = entries_row_maj[8];   m32 = entries_row_maj[9];   m33 = entries_row_maj[10];  m34 = entries_row_maj[11];
    }
#endif
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

    void
        TransformationMatrix::swap(TransformationMatrix &other)
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
        float mat[12];
        mat[0] = this->m11;  mat[1] = this->m12;  mat[2] = this->m13;  mat[3] = this->m14;
        mat[4] = this->m21;  mat[5] = this->m22;  mat[6] = this->m23;  mat[7] = this->m24;
        mat[8] = this->m31;  mat[9] = this->m32;  mat[10] = this->m33; mat[11] = this->m34;
        return mat;
    }

    float TransformationMatrix::determinante()
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
        float det = this->determinante();
        if (abs(det) < 1e-9)
            return false;
        float fac = 1.0f / det;

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

    void TransformationMatrix::translate(float x, float y, float z)
    {
        TransformationMatrix mat = mat_translation(x, y, z);
        this->multiplyLeft(mat);
    }

    void TransformationMatrix::scale(float factor)
    {
        this->scale(factor, factor, factor);
    }

    void TransformationMatrix::scale(float x, float y, float z)
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

    void TransformationMatrix::rotate(float angle_rad, const Axis & axis)
    {
        TransformationMatrix mat = mat_rotation(angle_rad, axis);
        this->multiplyLeft(mat);
    }

    void TransformationMatrix::rotate(float angle_rad, const Pointf3 & axis)
    {
        TransformationMatrix mat = mat_rotation(angle_rad, axis);
        this->multiplyLeft(mat);
    }

    void TransformationMatrix::rotate(float q1, float q2, float q3, float q4)
    {
        TransformationMatrix mat = mat_rotation(q1, q2, q3, q4);
        this->multiplyLeft(mat);
    }

    void TransformationMatrix::multiplyLeft(TransformationMatrix &left)
    {
        *this = multiply(left, *this);
    }

    void TransformationMatrix::multiplyRight(TransformationMatrix &right)
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
        return TransformationMatrix(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f);
    }

    TransformationMatrix TransformationMatrix::mat_translation(float x, float y, float z)
    {
        return TransformationMatrix(
            1.0f, 0.0f, 0.0f, x,
            0.0f, 1.0f, 0.0f, y,
            0.0f, 0.0f, 1.0f, z);
    }

    TransformationMatrix TransformationMatrix::mat_scale(float x, float y, float z)
    {
        return TransformationMatrix(
            x, 0.0f, 0.0f, 0.0f,
            0.0f, y, 0.0f, 0.0f,
            0.0f, 0.0f, z, 0.0f);
    }

    TransformationMatrix TransformationMatrix::mat_scale(float scale)
    {
        return TransformationMatrix::mat_scale(scale, scale, scale);
    }

    TransformationMatrix TransformationMatrix::mat_rotation(float angle_rad, const Axis &axis)
    {
        float s = sin(angle_rad);
        float c = cos(angle_rad);
        TransformationMatrix mat; // For RVO
        switch (axis)
        {
        case X:
            mat = TransformationMatrix(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, c, s, 0.0f,
                0.0f, -s, c, 0.0f);
            break;
        case Y:
            mat = TransformationMatrix(
                c, 0.0f, -s, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                s, 0.0f, c, 0.0f);
            break;
        case Z:
            mat = TransformationMatrix(
                c, s, 0.0f, 0.0f,
                -s, c, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f);
            break;
        default:
            CONFESS("Invalid Axis supplied to TransformationMatrix::mat_rotation");
            mat = TransformationMatrix();
            break;
        }
        return mat;
    }

    TransformationMatrix TransformationMatrix::mat_rotation(float q1, float q2, float q3, float q4)
    {
        float factor = q1*q1 + q2*q2 + q3*q3 + q4*q4;
        if (abs(factor - 1.0f) > 1e-9)
        {
            factor = 1.0f / sqrtf(factor);
            q1 *= factor;
            q2 *= factor;
            q3 *= factor;
            q4 *= factor;
        }
        // https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation#Quaternion-derived_rotation_matrix
        return TransformationMatrix(
            1 - 2 * (q2*q2 + q3*q3), 2 * (q1*q2 - q3*q4), 2 * (q1*q3 + q2*q4), 0.0f,
            2 * (q1*q2 + q3*q4), 1 - 2 * (q1*q1 + q3*q3), 2 * (q2*q3 - q1*q4), 0.0f,
            2 * (q1*q3 - q2*q4), 2 * (q2*q3 + q1*q4), 1 - 2 * (q1*q1 + q2*q2), 0.0f);
    }

    TransformationMatrix TransformationMatrix::mat_rotation(float angle_rad, const Pointf3 &axis)
    {
        float s, factor, q1, q2, q3, q4;
        s = sin(angle_rad);
        TransformationMatrix mat; // For RVO
        factor = axis.x*axis.x + axis.y*axis.y + axis.z*axis.z;
        factor = 1.0f / sqrtf(factor);
        q1 = s*factor*axis.x;
        q2 = s*factor*axis.y;
        q3 = s*factor*axis.z;
        q4 = cos(angle_rad);
        return mat_rotation(q1, q2, q3, q4);
    }

    TransformationMatrix TransformationMatrix::mat_mirror(const Axis &axis)
    {
        TransformationMatrix mat; // For RVO
        switch (axis)
        {
        case X:
            mat = TransformationMatrix(
                -1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f);
            break;
        case Y:
            mat = TransformationMatrix(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, -1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f);
            break;
        case Z:
            mat = TransformationMatrix(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, -1.0f, 0.0f);
            break;
        default:
            CONFESS("Invalid Axis supplied to TransformationMatrix::mat_mirror");
            mat = TransformationMatrix();
            break;
        }
        return mat;
    }

    TransformationMatrix TransformationMatrix::mat_mirror(const Pointf3 &normal)
    {
        // Kovács, E. Rotation about arbitrary axis and reflection through an arbitrary plane, Annales Mathematicae
        // et Informaticae, Vol 40 (2012) pp 175-186
        // http://ami.ektf.hu/uploads/papers/finalpdf/AMI_40_from175to186.pdf
        float factor, c1, c2, c3;
        factor = normal.x*normal.x + normal.y*normal.y + normal.z*normal.z;
        factor = 1.0f / sqrtf(factor);
        c1 = factor*normal.x;
        c2 = factor*normal.y;
        c3 = factor*normal.z;
        return TransformationMatrix(
            1 - 2 * c1*c1, -2 * c2*c1, -2 * c3*c1, 0.0f,
            -2 * c2*c1, 1 - 2 * c2*c2, -2 * c2*c3, 0.0f,
            -2 * c1*c3, -2 * c2*c3, 1 - 2 * c3*c3, 0.0f);
    }

}
