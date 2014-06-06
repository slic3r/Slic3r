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

#include "utils.h"
#include "rmath.h"

#include "point.h"
#include "edge.h"
#include "triangle.h"
#include "mesh.h"

void
p2tr_validate_edges_can_form_tri (P2trEdge *AB,
                                  P2trEdge *BC,
                                  P2trEdge *CA)
{
  if (AB != AB->mirror->mirror ||
      BC != BC->mirror->mirror ||
      CA != CA->mirror->mirror)
    {
      p2tr_exception_programmatic ("Bad edge mirroring!");
    }

  if (AB->end != P2TR_EDGE_START(BC)
      || BC->end != P2TR_EDGE_START(CA)
      || CA->end != P2TR_EDGE_START(AB))
    {
      p2tr_exception_programmatic ("Unexpected edge sequence for a triangle!");
    }

  if (AB == BC->mirror || BC == CA->mirror || CA == AB->mirror)
    {
      p2tr_exception_programmatic ("Repeated edge in a triangle!");
    }
}

P2trTriangle*
p2tr_triangle_new (P2trEdge *AB,
                   P2trEdge *BC,
                   P2trEdge *CA)
{
  gint i;
  P2trTriangle *self = g_slice_new (P2trTriangle);

  self->refcount = 0;

#ifndef P2TC_NO_LOGIC_CHECKS
  p2tr_validate_edges_can_form_tri (AB, BC, CA);
#endif

  switch (p2tr_math_orient2d(&CA->end->c, &AB->end->c, &BC->end->c))
    {
      case P2TR_ORIENTATION_CCW:
        self->edges[0] = CA->mirror;
        self->edges[1] = BC->mirror;
        self->edges[2] = AB->mirror;
        break;

      case P2TR_ORIENTATION_CW:
        self->edges[0] = AB;
        self->edges[1] = BC;
        self->edges[2] = CA;
        break;

      case P2TR_ORIENTATION_LINEAR:
        p2tr_exception_geometric ("Can't make a triangle of linear points!");
    }

#ifndef P2TC_NO_LOGIC_CHECKS
  p2tr_validate_edges_can_form_tri (self->edges[0], self->edges[1], self->edges[2]);

  if (p2tr_math_orient2d (&P2TR_TRIANGLE_GET_POINT(self,0)->c,
                          &P2TR_TRIANGLE_GET_POINT(self,1)->c,
                          &P2TR_TRIANGLE_GET_POINT(self,2)->c) != P2TR_ORIENTATION_CW)
    {
      p2tr_exception_programmatic ("Bad ordering!");
    }
#endif

  for (i = 0; i < 3; i++)
    {
#ifndef P2TC_NO_LOGIC_CHECKS
      if (self->edges[i]->tri != NULL)
        p2tr_exception_programmatic ("This edge is already in use by "
            "another triangle!");
#endif
      self->edges[i]->tri = self;
      p2tr_edge_ref (self->edges[i]);
      p2tr_triangle_ref (self);
    }

  return p2tr_triangle_ref (self);
}

P2trTriangle*
p2tr_triangle_ref (P2trTriangle *self)
{
  ++self->refcount;
  return self;
}

void
p2tr_triangle_unref (P2trTriangle *self)
{
  g_assert (self->refcount > 0);
  if (--self->refcount == 0)
    p2tr_triangle_free (self);
}

void
p2tr_triangle_free (P2trTriangle *self)
{
  g_assert (p2tr_triangle_is_removed (self));
  g_slice_free (P2trTriangle, self);
}

void
p2tr_triangle_remove (P2trTriangle *self)
{
  gint i;
  P2trMesh *mesh;
  
  if (p2tr_triangle_is_removed (self))
    return;

  mesh = p2tr_triangle_get_mesh (self);
  
  if (mesh != NULL)
    {
      p2tr_mesh_on_triangle_removed (mesh, self);
      p2tr_mesh_unref (mesh); /* The get function reffed it */
    }

  for (i = 0; i < 3; i++)
  {
    self->edges[i]->tri = NULL;
    p2tr_edge_unref (self->edges[i]);
    self->edges[i] = NULL;
    /* Although we lost reference to the triangle 3 rows above, we must
     * not unref it untill here since we still use the struct until this
     * line! */
    p2tr_triangle_unref (self);
  }
}

P2trMesh*
p2tr_triangle_get_mesh (P2trTriangle *self)
{
  if (self->edges[0] != NULL)
    return p2tr_edge_get_mesh (self->edges[0]);
  else
    return NULL;
}

gboolean 
p2tr_triangle_is_removed (P2trTriangle *self)
{
  return self->edges[0] == NULL;
}

P2trPoint*
p2tr_triangle_get_opposite_point (P2trTriangle *self,
                                  P2trEdge     *e,
                                  gboolean      do_ref)
{
  P2trPoint *pt;
  if (self->edges[0] == e || self->edges[0]->mirror == e)
    pt = self->edges[1]->end;
  else if (self->edges[1] == e || self->edges[1]->mirror == e)
    pt = self->edges[2]->end;
  else if (self->edges[2] == e || self->edges[2]->mirror == e)
    pt = self->edges[0]->end;
  else
    p2tr_exception_programmatic ("The edge is not in the triangle!");

  return do_ref ? p2tr_point_ref (pt) : pt;
}

P2trEdge*
p2tr_triangle_get_opposite_edge (P2trTriangle *self,
                                 P2trPoint    *p)
{
  if (self->edges[0]->end == p)
    return p2tr_edge_ref (self->edges[2]);
  if (self->edges[1]->end == p)
    return p2tr_edge_ref (self->edges[0]);
  if (self->edges[2]->end == p)
    return p2tr_edge_ref (self->edges[1]);

  p2tr_exception_programmatic ("The point is not in the triangle!");
}

/**
 * Angles return by this function are always in the range [0,180]
 */
gdouble
p2tr_triangle_get_angle_at (P2trTriangle *self,
                            P2trPoint    *p)
{
  if (p == self->edges[0]->end)
    return p2tr_edge_angle_between (self->edges[0], self->edges[1]);
  else if (p == self->edges[1]->end)
    return p2tr_edge_angle_between (self->edges[1], self->edges[2]);
  else if (p == self->edges[2]->end)
    return p2tr_edge_angle_between (self->edges[2], self->edges[0]);

  p2tr_exception_programmatic ("Can't find the point!");
}

gdouble
p2tr_triangle_smallest_non_constrained_angle (P2trTriangle *self)
{
    gdouble result = G_MAXDOUBLE, angle;
    
    if (! self->edges[0]->constrained || !self->edges[1]->constrained)
      {
        angle = p2tr_edge_angle_between(self->edges[0], self->edges[1]);
        result = MIN(result, angle);
      }

    if (!self->edges[1]->constrained || !self->edges[2]->constrained)
      {
        angle = p2tr_edge_angle_between(self->edges[1], self->edges[2]);
        result = MIN(result, angle);
      }

    if (!self->edges[2]->constrained || !self->edges[0]->constrained)
      {
        angle = p2tr_edge_angle_between(self->edges[2], self->edges[0]);
        result = MIN(result, angle);
      }

    return result;
}

void
p2tr_triangle_get_circum_circle (P2trTriangle *self,
                                 P2trCircle   *circle)
{
  p2tr_math_triangle_circumcircle (
      &P2TR_TRIANGLE_GET_POINT(self,0)->c,
      &P2TR_TRIANGLE_GET_POINT(self,1)->c,
      &P2TR_TRIANGLE_GET_POINT(self,2)->c,
      circle);
}

P2trInCircle
p2tr_triangle_circumcircle_contains_point (P2trTriangle      *self,
                                           const P2trVector2  *pt)
{
  /* Points must be given in CCW order! */
  return p2tr_math_incircle (
      &P2TR_TRIANGLE_GET_POINT(self,2)->c,
      &P2TR_TRIANGLE_GET_POINT(self,1)->c,
      &P2TR_TRIANGLE_GET_POINT(self,0)->c,
      pt);
}

P2trInTriangle
p2tr_triangle_contains_point  (P2trTriangle      *self,
                               const P2trVector2 *pt)
{
  return p2tr_math_intriangle (
      &P2TR_TRIANGLE_GET_POINT(self,0)->c,
      &P2TR_TRIANGLE_GET_POINT(self,1)->c,
      &P2TR_TRIANGLE_GET_POINT(self,2)->c,
      pt);
}

P2trInTriangle
p2tr_triangle_contains_point2 (P2trTriangle      *self,
                               const P2trVector2 *pt,
                               gdouble           *u,
                               gdouble           *v)
{
  return p2tr_math_intriangle2 (
      &P2TR_TRIANGLE_GET_POINT(self,0)->c,
      &P2TR_TRIANGLE_GET_POINT(self,1)->c,
      &P2TR_TRIANGLE_GET_POINT(self,2)->c,
      pt, u, v);
}
