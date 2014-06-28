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
#include "mesh.h"

#include "vtriangle.h"

P2trVTriangle*
p2tr_vtriangle_new (P2trTriangle *tri)
{
  P2trVTriangle *self = g_slice_new (P2trVTriangle);

  self->points[0] = p2tr_point_ref (tri->edges[0]->end);
  self->points[1] = p2tr_point_ref (tri->edges[1]->end);
  self->points[2] = p2tr_point_ref (tri->edges[2]->end);

  self->refcount = 1;

  return self;
}

P2trVTriangle*
p2tr_vtriangle_ref (P2trVTriangle *self)
{
  ++self->refcount;
  return self;
}

void
p2tr_vtriangle_unref (P2trVTriangle *self)
{
  g_assert (self->refcount > 0);
  if (--self->refcount == 0)
    p2tr_vtriangle_free (self);
}

void
p2tr_vtriangle_free (P2trVTriangle *self)
{
  p2tr_point_unref (self->points[0]);
  p2tr_point_unref (self->points[1]);
  p2tr_point_unref (self->points[2]);
  g_slice_free (P2trVTriangle, self);
}

P2trMesh*
p2tr_vtriangle_get_mesh (P2trVTriangle *self)
{
  return p2tr_point_get_mesh (self->points[0]);
}

P2trTriangle*
p2tr_vtriangle_is_real (P2trVTriangle *self)
{
  P2trEdge *e0, *e1, *e2;

  /* The triangle exists if and only if all the edges
   * still exist and they all are a part of the same
   * triangle. */
  if ((e0 = p2tr_point_has_edge_to (self->points[0], self->points[1])) &&
      (e1 = p2tr_point_has_edge_to (self->points[1], self->points[2])) &&
      (e2 = p2tr_point_has_edge_to (self->points[2], self->points[0])) &&
      e0->tri == e1->tri && e1->tri == e2->tri)
    return e0->tri;
  else
    return NULL;
}

void
p2tr_vtriangle_create (P2trVTriangle *self)
{
  P2trMesh     *mesh;
  P2trEdge     *e1, *e2, *e3;
  P2trTriangle *tri;

  g_assert (! p2tr_vtriangle_is_real (self));

  mesh = p2tr_vtriangle_get_mesh (self);
  e1 = p2tr_point_get_edge_to (self->points[0], self->points[1], FALSE);
  e2 = p2tr_point_get_edge_to (self->points[1], self->points[2], FALSE);
  e3 = p2tr_point_get_edge_to (self->points[2], self->points[0], FALSE);

  if (mesh != NULL)
    {
      tri = p2tr_mesh_new_triangle (mesh, e1, e2, e3);
      p2tr_mesh_unref (mesh);
    }
  else
    tri = p2tr_triangle_new (e1, e2, e3);

  p2tr_triangle_unref (tri);
}

void
p2tr_vtriangle_remove (P2trVTriangle *self)
{
  P2trTriangle *tri = p2tr_vtriangle_is_real (self);

  g_assert (tri != NULL);

  p2tr_triangle_remove (tri);
}

P2trTriangle*
p2tr_vtriangle_get (P2trVTriangle *self)
{
  P2trTriangle *real = p2tr_vtriangle_is_real (self);
  g_assert (real != NULL);
  return p2tr_triangle_ref (real);
}

