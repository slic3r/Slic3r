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

#include <glib.h>

#include "point.h"
#include "edge.h"
#include "triangle.h"
#include "vedge.h"

#include "cdt-flipfix.h"


static P2trEdge*  p2tr_cdt_try_flip  (P2trCDT   *self,
                                      P2trEdge  *to_flip);

/* This function implements "Lawson's algorithm", also known as "The
 * diagonal swapping algorithm". This algorithm takes a CDT, and a list
 * of triangles that were formed by the insertion of a new point into
 * the triangular mesh, and makes the triangulation a CDT once more. Its
 * logic is explained below:
 *
 *   If a point is added to an existing triangular mesh then
 *   circumcircles are formed for all new triangles formed. If any of
 *   the neighbours lie inside the circumcircle of any triangle, then a
 *   quadrilateral is formed using the triangle and its neighbour. The
 *   diagonals of this quadrilateral are swapped to give a new
 *   triangulation. This process is continued till there are no more
 *   faulty triangles and no more swaps are required.
 *
 * The description above may seem slightly inaccurate, as it does not
 * consider the case were the diagonals can not be swapped since the
 * quadrilateral is concave (then swapping the diagonals would result
 * in a diagonal outside the quad, which is undesired).
 *
 * However, the description above is accurate. If the opposite point
 * is inside the circular cut created by the edge, then since the
 * circular cut is inside the infinite area created by extending the
 * two other edges of the triangle, therefor the point is also inside
 * that area - meaning that the quadrilateral is not concave!
 */

void
p2tr_cdt_flip_fix (P2trCDT     *self,
                   P2trVEdgeSet *candidates)
{
  P2trEdge *edge;
  P2trVEdge *vedge;

  while (p2tr_vedge_set_pop (candidates, &vedge))
    {
      if (! p2tr_vedge_try_get_and_unref (vedge, &edge))
        continue;

      if (! edge->constrained
          /* TODO: we probably don't need this check... */
          && ! p2tr_edge_is_removed (edge))
        {
          /* If the edge is not constrained, then it should be
           * a part of two triangles */
          P2trPoint *A  = P2TR_EDGE_START(edge), *B = edge->end;
          P2trPoint *C1 = p2tr_triangle_get_opposite_point (edge->tri, edge, FALSE);
          P2trPoint *C2 = p2tr_triangle_get_opposite_point (edge->mirror->tri, edge->mirror, FALSE);

          P2trEdge *flipped = p2tr_cdt_try_flip (self, edge);
          if (flipped != NULL)
            {
              p2tr_vedge_set_add (candidates, p2tr_point_get_edge_to (A, C1, TRUE));
              p2tr_vedge_set_add (candidates, p2tr_point_get_edge_to (A, C2, TRUE));
              p2tr_vedge_set_add (candidates, p2tr_point_get_edge_to (B, C1, TRUE));
              p2tr_vedge_set_add (candidates, p2tr_point_get_edge_to (B, C2, TRUE));
              p2tr_edge_unref (flipped);
            }
        }

      p2tr_edge_unref (edge);
    }
}

/**
 * Try to flip a given edge, If successfull, return the new edge (reffed!),
 * otherwise return NULL
 */
P2trEdge*
p2tr_cdt_try_flip (P2trCDT   *self,
                   P2trEdge  *to_flip)
{
  /*    C
   *  / | \
   * B-----A    to_flip: A->B
   *  \ | /     to_flip.Tri: ABC
   *    D
   */
  P2trPoint *A, *B, *C, *D;
  P2trEdge *AB, *CA, *AD, *DB, *BC, *DC;

  g_assert (! to_flip->constrained && ! to_flip->delaunay);

  A = P2TR_EDGE_START (to_flip);
  B = to_flip->end;
  C = p2tr_triangle_get_opposite_point (to_flip->tri, to_flip, FALSE);
  D = p2tr_triangle_get_opposite_point (to_flip->mirror->tri, to_flip->mirror, FALSE);

  AB = to_flip;

  /* Check if the quadriliteral ADBC is concave (because if it is, we
   * can't flip the edge) */
  if (p2tr_triangle_circumcircle_contains_point (AB->tri, &D->c) != P2TR_INCIRCLE_IN)
    return NULL;

  CA = p2tr_point_get_edge_to (C, A, FALSE);
  AD = p2tr_point_get_edge_to (A, D, FALSE);
  DB = p2tr_point_get_edge_to (D, B, FALSE);
  BC = p2tr_point_get_edge_to (B, C, FALSE);

  p2tr_edge_remove (AB);

  DC = p2tr_mesh_new_edge (self->mesh, D, C, FALSE);

  p2tr_triangle_unref (p2tr_mesh_new_triangle (self->mesh,
      CA, AD, DC));

  p2tr_triangle_unref (p2tr_mesh_new_triangle (self->mesh,
      DB, BC, DC->mirror));

  return DC;
}



