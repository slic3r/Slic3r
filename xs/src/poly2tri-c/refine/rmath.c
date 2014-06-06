/*
 * This file is a part of Poly2Tri-C
 * (c) Barak Itkin <lightningismyname@gmail.com>
 * http://code.google.com/p/poly2tri-c/
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of Poly2Tri nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <math.h>
#include <glib.h>
#include "rmath.h"

gdouble
p2tr_math_length_sq (gdouble x1, gdouble y1,
                     gdouble x2, gdouble y2)
{
  return P2TR_VECTOR2_DISTANCE_SQ2 (x1, y1, x2, y2);
}

gdouble
p2tr_math_length_sq2 (const P2trVector2 *pt1,
                      const P2trVector2 *pt2)
{
  return p2tr_math_length_sq (pt1->x, pt1->y, pt2->x, pt2->y);
}

static inline gdouble
p2tr_matrix_det2 (gdouble a00, gdouble a01,
                  gdouble a10, gdouble a11)
{
    return a00 * a11 - a10 * a01;
}

static inline gdouble
p2tr_matrix_det3 (gdouble a00, gdouble a01, gdouble a02,
                  gdouble a10, gdouble a11, gdouble a12,
                  gdouble a20, gdouble a21, gdouble a22)
{
    return
        + a00 * (a11 * a22 - a21 * a12)
        - a01 * (a10 * a22 - a20 * a12)
        + a02 * (a10 * a21 - a20 * a11);
}

static inline gdouble
p2tr_matrix_det4 (gdouble a00, gdouble a01, gdouble a02, gdouble a03,
                  gdouble a10, gdouble a11, gdouble a12, gdouble a13,
                  gdouble a20, gdouble a21, gdouble a22, gdouble a23,
                  gdouble a30, gdouble a31, gdouble a32, gdouble a33)
{
    return
        + a00 * p2tr_matrix_det3 (a11, a12, a13,
                                 a21, a22, a23,
                                 a31, a32, a33)
        - a01 * p2tr_matrix_det3 (a10, a12, a13,
                                  a20, a22, a23,
                                  a30, a32, a33)
        + a02 * p2tr_matrix_det3 (a10, a11, a13,
                                  a20, a21, a23,
                                  a30, a31, a33)
        - a03 * p2tr_matrix_det3 (a10, a11, a12,
                                  a20, a21, a22,
                                  a30, a31, a32);
}

void
p2tr_math_triangle_circumcircle (const P2trVector2 *A,
                                 const P2trVector2 *B,
                                 const P2trVector2 *C,
                                 P2trCircle    *circle)
{
  /*       | Ax Bx Cx |
   * D = + | Ay By Cy | * 2
   *       | +1 +1 +1 |
   *
   *       | Asq Bsq Csq |
   * X = + | Ay  By  Cy  | / D
   *       | 1   1   1   |
   *
   *       | Asq Bsq Csq |
   * Y = - | Ax  Bx  Cx  | / D
   *       | 1   1   1   |
   */
  gdouble Asq = P2TR_VECTOR2_LEN_SQ (A);
  gdouble Bsq = P2TR_VECTOR2_LEN_SQ (B);
  gdouble Csq = P2TR_VECTOR2_LEN_SQ (C);

  gdouble invD = 1 / (2 * p2tr_matrix_det3 (
      A->x, B->x, C->x,
      A->y, B->y, C->y,
      1,    1,    1));

  circle->center.x = + p2tr_matrix_det3 (
      Asq,  Bsq,  Csq,
      A->y, B->y, C->y,
      1,    1,    1) * invD;

  circle->center.y = - p2tr_matrix_det3 (
      Asq,  Bsq,  Csq,
      A->x, B->x, C->x,
      1,    1,    1) * invD;

  circle->radius = sqrt (P2TR_VECTOR2_DISTANCE_SQ (A, &circle->center));
}

/* The point in triangle test which is implemented below is based on the
 * algorithm which appears on:
 *
 *    http://www.blackpawn.com/texts/pointinpoly/default.html
 */
void
p2tr_math_triangle_barcycentric (const P2trVector2 *A,
                                 const P2trVector2 *B,
                                 const P2trVector2 *C,
                                 const P2trVector2 *P,
                                 gdouble           *u,
                                 gdouble           *v)
{
  gdouble dot00, dot01, dot02, dot11, dot12, invDenom;

  /* Compute the vectors offsetted so that A is the origin */
  P2trVector2 v0, v1, v2;
  p2tr_vector2_sub(C, A, &v0);
  p2tr_vector2_sub(B, A, &v1);
  p2tr_vector2_sub(P, A, &v2);

  /* Compute dot products */
  dot00 = P2TR_VECTOR2_DOT(&v0, &v0);
  dot01 = P2TR_VECTOR2_DOT(&v0, &v1);
  dot02 = P2TR_VECTOR2_DOT(&v0, &v2);
  dot11 = P2TR_VECTOR2_DOT(&v1, &v1);
  dot12 = P2TR_VECTOR2_DOT(&v1, &v2);

  /* Compute barycentric coordinates */
  invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
  *u = (dot11 * dot02 - dot01 * dot12) * invDenom;
  *v = (dot00 * dot12 - dot01 * dot02) * invDenom;
}

#define INTRIANGLE_EPSILON 0e-9

P2trInTriangle
p2tr_math_intriangle (const P2trVector2 *A,
                      const P2trVector2 *B,
                      const P2trVector2 *C,
                      const P2trVector2 *P)
{
  gdouble u, v;
  return p2tr_math_intriangle2 (A, B, C, P, &u, &v);
}

P2trInTriangle
p2tr_math_intriangle2 (const P2trVector2 *A,
                       const P2trVector2 *B,
                       const P2trVector2 *C,
                       const P2trVector2 *P,
                       gdouble           *u,
                       gdouble           *v)
{
  p2tr_math_triangle_barcycentric (A, B, C, P, u, v);
  
  /* Check if point is in triangle - i.e. whether (u + v) < 1 and both
   * u and v are positive */
  if ((*u >= INTRIANGLE_EPSILON) && (*v >= INTRIANGLE_EPSILON) && (*u + *v < 1 - INTRIANGLE_EPSILON))
    return P2TR_INTRIANGLE_IN;
  else if ((*u >= -INTRIANGLE_EPSILON) && (*v >= -INTRIANGLE_EPSILON) && (*u + *v <= 1 + INTRIANGLE_EPSILON))
    return P2TR_INTRIANGLE_ON;
  else
    return P2TR_INTRIANGLE_OUT;
}

/* The point in triangle circumcircle test, and the 3-point orientation
 * test, are both based on the work of Jonathan Richard Shewchuk. The
 * technique used here is described in his paper "Adaptive Precision
 * Floating-Point Arithmetic and Fast Robust Geometric Predicates"
 */

#define ORIENT2D_EPSILON 1e-9

P2trOrientation p2tr_math_orient2d (const P2trVector2 *A,
                                    const P2trVector2 *B,
                                    const P2trVector2 *C)
{
  /* We are trying to compute this determinant:
   * |Ax Ay 1|
   * |Bx By 1|
   * |Cx Cy 1|
   */
  gdouble result = p2tr_matrix_det3 (
      A->x, A->y, 1,
      B->x, B->y, 1,
      C->x, C->y, 1
  );

  if (result > ORIENT2D_EPSILON)
    return P2TR_ORIENTATION_CCW;
  else if (result < -ORIENT2D_EPSILON)
    return P2TR_ORIENTATION_CW;
  else
    return P2TR_ORIENTATION_LINEAR;
}

#define INCIRCLE_EPSILON 1e-9

/* Points must be given in CCW order!!!!! */
P2trInCircle
p2tr_math_incircle (const P2trVector2 *A,
                    const P2trVector2 *B,
                    const P2trVector2 *C,
                    const P2trVector2 *D)
{
  /* We are trying to compute this determinant:
   * |Ax Ay Ax^2+Ay^2 1|
   * |Bx By Bx^2+By^2 1|
   * |Cx Cy Cx^2+Cy^2 1|
   * |Dx Dy Dx^2+Dy^2 1|
   */
  gdouble result = p2tr_matrix_det4 (
      A->x, A->y, P2TR_VECTOR2_LEN_SQ(A), 1,
      B->x, B->y, P2TR_VECTOR2_LEN_SQ(B), 1,
      C->x, C->y, P2TR_VECTOR2_LEN_SQ(C), 1,
      D->x, D->y, P2TR_VECTOR2_LEN_SQ(D), 1
  );

  if (result > INCIRCLE_EPSILON)
    return P2TR_INCIRCLE_IN;
  else if (result < INCIRCLE_EPSILON)
    return P2TR_INCIRCLE_OUT;
  else
    return P2TR_INCIRCLE_ON;
}

/* The point inside diametral-circle test and the point inside diametral
 * lens test, are both based on the work of Jonathan Richard Shewchuk.
 * The techniques used here are partially described in his paper
 * "Delaunay Refinement Algorithms for Triangular Mesh Generation".
 *
 * W is inside the diametral circle (lens) of the line XY if and only if
 * the angle XWY is larger than 90 (120) degrees. We know how to compute
 * the cosine of that angle very easily like so:
 *
 * cos XWY = (WX * WY) / (|WX| * |WY|)
 *
 * Since XWY is larger than 90 (120) degrees, cos XWY <= 0 (-0.5) so:
 *
 * Diametral Circle               | Diametral Lens
 * -------------------------------+-----------------------------------
 * 0 >= (WX * WY) / (|WX| * |WY|) | - 0.5 >= (WX * WY) / (|WX| * |WY|)
 * 0 >= WX * WY                   | - 0.5 * |WX| * |WY| >= WX * WY
 */

gboolean
p2tr_math_diametral_circle_contains (const P2trVector2 *X,
                                     const P2trVector2 *Y,
                                     const P2trVector2 *W)
{
  P2trVector2 WX, WY;

  p2tr_vector2_sub(X, W, &WX);
  p2tr_vector2_sub(Y, W, &WY);

  return P2TR_VECTOR2_DOT(&WX, &WY) <= 0;
}

gboolean
p2tr_math_diametral_lens_contains (const P2trVector2 *X,
                                   const P2trVector2 *Y,
                                   const P2trVector2 *W)
{
  P2trVector2 WX, WY;

  p2tr_vector2_sub(X, W, &WX);
  p2tr_vector2_sub(Y, W, &WY);

  return P2TR_VECTOR2_DOT(&WX, &WY)
      <= 0.5 * p2tr_vector2_norm(&WX) * p2tr_vector2_norm(&WY);
}
