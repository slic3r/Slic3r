/*
 * This file is a part of the C port of the Poly2Tri library
 * Porting to C done by (c) Barak Itkin <lightningismyname@gmail.com>
 * http://code.google.com/p/poly2tri-c/
 *
 * Poly2Tri Copyright (c) 2009-2010, Poly2Tri Contributors
 * http://code.google.com/p/poly2tri/
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

#include "shapes.h"
#include <stdio.h>
#include <stdlib.h>

/** Default constructor does nothing (for performance). */

void
p2t_point_init (P2tPoint* THIS)
{
  THIS->x = 0;
  THIS->y = 0;
  THIS->edge_list = g_ptr_array_new ();
}

P2tPoint*
p2t_point_new ()
{
  P2tPoint* THIS = g_slice_new (P2tPoint);
  p2t_point_init (THIS);
  return THIS;
}

/** Construct using coordinates. */

void
p2t_point_init_dd (P2tPoint* THIS, double x, double y)
{
  THIS->x = x;
  THIS->y = y;
  THIS->edge_list = g_ptr_array_new ();
}

P2tPoint*
p2t_point_new_dd (double x, double y)
{
  P2tPoint* THIS = g_slice_new (P2tPoint);
  p2t_point_init_dd (THIS, x, y);
  return THIS;
}

void
p2t_point_destroy (P2tPoint* THIS)
{
  g_ptr_array_free (THIS->edge_list, TRUE);
}

void
p2t_point_free (P2tPoint* THIS)
{
  p2t_point_destroy (THIS);
  g_slice_free (P2tPoint, THIS);
}

/** Constructor */

void
p2t_edge_init (P2tEdge* THIS, P2tPoint* p1, P2tPoint* p2)
{
  THIS->p = p1;
  THIS->q = p2;
  if (p1->y > p2->y)
    {
      THIS->q = p1;
      THIS->p = p2;
    }
  else if (p1->y == p2->y)
    {
      if (p1->x > p2->x)
        {
          THIS->q = p1;
          THIS->p = p2;
        }
      else if (p1->x == p2->x)
        {
          /* Repeat points */
          assert (FALSE);
        }
    }

  g_ptr_array_add (THIS->q->edge_list, THIS);
}

P2tEdge*
p2t_edge_new (P2tPoint* p1, P2tPoint* p2)
{
  P2tEdge* THIS = g_slice_new (P2tEdge);
  p2t_edge_init (THIS, p1, p2);
  return THIS;
}

void
p2t_edge_destroy (P2tEdge* THIS) { }

void
p2t_edge_free (P2tEdge* THIS)
{
  p2t_edge_destroy (THIS);
  g_slice_free (P2tEdge, THIS);
}

P2tTriangle*
p2t_triangle_new (P2tPoint* a, P2tPoint* b, P2tPoint* c)
{
  P2tTriangle *tr = g_new (P2tTriangle, 1);
  p2t_triangle_init (tr, a, b, c);
  return tr;
}

void
p2t_triangle_init (P2tTriangle* THIS, P2tPoint* a, P2tPoint* b, P2tPoint* c)
{
  THIS->points_[0] = a;
  THIS->points_[1] = b;
  THIS->points_[2] = c;
  THIS->neighbors_[0] = NULL;
  THIS->neighbors_[1] = NULL;
  THIS->neighbors_[2] = NULL;
  THIS->constrained_edge[0] = THIS->constrained_edge[1] = THIS->constrained_edge[2] = FALSE;
  THIS->delaunay_edge[0] = THIS->delaunay_edge[1] = THIS->delaunay_edge[2] = FALSE;
  THIS->interior_ = FALSE;

}
/* Update neighbor pointers */

void
p2t_triangle_mark_neighbor_pt_pt_tr (P2tTriangle* THIS, P2tPoint* p1, P2tPoint* p2, P2tTriangle* t)
{
  if ((p1 == THIS->points_[2] && p2 == THIS->points_[1]) || (p1 == THIS->points_[1] && p2 == THIS->points_[2]))
    THIS->neighbors_[0] = t;
  else if ((p1 == THIS->points_[0] && p2 == THIS->points_[2]) || (p1 == THIS->points_[2] && p2 == THIS->points_[0]))
    THIS->neighbors_[1] = t;
  else if ((p1 == THIS->points_[0] && p2 == THIS->points_[1]) || (p1 == THIS->points_[1] && p2 == THIS->points_[0]))
    THIS->neighbors_[2] = t;
  else
    assert (0);
}

/* Exhaustive search to update neighbor pointers */

void
p2t_triangle_mark_neighbor_tr (P2tTriangle* THIS, P2tTriangle *t)
{
  if (p2t_triangle_contains_pt_pt (t, THIS->points_[1], THIS->points_[2]))
    {
      THIS->neighbors_[0] = t;
      p2t_triangle_mark_neighbor_pt_pt_tr (t, THIS->points_[1], THIS->points_[2], THIS);
    }
  else if (p2t_triangle_contains_pt_pt (t, THIS->points_[0], THIS->points_[2]))
    {
      THIS->neighbors_[1] = t;
      p2t_triangle_mark_neighbor_pt_pt_tr (t, THIS->points_[0], THIS->points_[2], THIS);
    }
  else if (p2t_triangle_contains_pt_pt (t, THIS->points_[0], THIS->points_[1]))
    {
      THIS->neighbors_[2] = t;
      p2t_triangle_mark_neighbor_pt_pt_tr (t, THIS->points_[0], THIS->points_[1], THIS);
    }
}

/**
 * Clears all references to all other triangles and points
 */
void
p2t_triangle_clear (P2tTriangle* THIS)
{
  int i;
  P2tTriangle *t;
  for (i = 0; i < 3; i++)
    {
      t = THIS->neighbors_[i];
      if (t != NULL)
        {
          p2t_triangle_clear_neighbor_tr (t, THIS);
        }
    }
  p2t_triangle_clear_neighbors (THIS);
  THIS->points_[0] = THIS->points_[1] = THIS->points_[2] = NULL;
}

void
p2t_triangle_clear_neighbor_tr (P2tTriangle* THIS, P2tTriangle *triangle)
{
  if (THIS->neighbors_[0] == triangle)
    {
      THIS->neighbors_[0] = NULL;
    }
  else if (THIS->neighbors_[1] == triangle)
    {
      THIS->neighbors_[1] = NULL;
    }
  else
    {
      THIS->neighbors_[2] = NULL;
    }
}

void
p2t_triangle_clear_neighbors (P2tTriangle* THIS)
{
  THIS->neighbors_[0] = NULL;
  THIS->neighbors_[1] = NULL;
  THIS->neighbors_[2] = NULL;
}

void
p2t_triangle_clear_delunay_edges (P2tTriangle* THIS)
{
  THIS->delaunay_edge[0] = THIS->delaunay_edge[1] = THIS->delaunay_edge[2] = FALSE;
}

P2tPoint*
p2t_triangle_opposite_point (P2tTriangle* THIS, P2tTriangle* t, P2tPoint* p)
{
  P2tPoint *cw = p2t_triangle_point_cw (t, p);
  /*double x = cw->x;
  double y = cw->y;
  x = p->x;
  y = p->y;
  P2tPoint* ham = */p2t_triangle_point_cw (THIS, cw);
  return p2t_triangle_point_cw (THIS, cw);
}

/* Legalized triangle by rotating clockwise around point(0) */

void
p2t_triangle_legalize_pt (P2tTriangle* THIS, P2tPoint *point)
{
  THIS->points_[1] = THIS->points_[0];
  THIS->points_[0] = THIS->points_[2];
  THIS->points_[2] = point;
}

/* Legalize triagnle by rotating clockwise around oPoint */

void
p2t_triangle_legalize_pt_pt (P2tTriangle* THIS, P2tPoint *opoint, P2tPoint *npoint)
{
  if (opoint == THIS->points_[0])
    {
      THIS->points_[1] = THIS->points_[0];
      THIS->points_[0] = THIS->points_[2];
      THIS->points_[2] = npoint;
    }
  else if (opoint == THIS->points_[1])
    {
      THIS->points_[2] = THIS->points_[1];
      THIS->points_[1] = THIS->points_[0];
      THIS->points_[0] = npoint;
    }
  else if (opoint == THIS->points_[2])
    {
      THIS->points_[0] = THIS->points_[2];
      THIS->points_[2] = THIS->points_[1];
      THIS->points_[1] = npoint;
    }
  else
    {
      assert (0);
    }
}

int
p2t_triangle_index (P2tTriangle* THIS, const P2tPoint* p)
{
  if (p == THIS->points_[0])
    {
      return 0;
    }
  else if (p == THIS->points_[1])
    {
      return 1;
    }
  else if (p == THIS->points_[2])
    {
      return 2;
    }
  assert (0);
}

int
p2t_triangle_edge_index (P2tTriangle* THIS, const P2tPoint* p1, const P2tPoint* p2)
{
  if (THIS->points_[0] == p1)
    {
      if (THIS->points_[1] == p2)
        {
          return 2;
        }
      else if (THIS->points_[2] == p2)
        {
          return 1;
        }
    }
  else if (THIS->points_[1] == p1)
    {
      if (THIS->points_[2] == p2)
        {
          return 0;
        }
      else if (THIS->points_[0] == p2)
        {
          return 2;
        }
    }
  else if (THIS->points_[2] == p1)
    {
      if (THIS->points_[0] == p2)
        {
          return 1;
        }
      else if (THIS->points_[1] == p2)
        {
          return 0;
        }
    }
  return -1;
}

void
p2t_triangle_mark_constrained_edge_i (P2tTriangle* THIS, const int index)
{
  THIS->constrained_edge[index] = TRUE;
}

void
p2t_triangle_mark_constrained_edge_ed (P2tTriangle* THIS, P2tEdge* edge)
{
  p2t_triangle_mark_constrained_edge_pt_pt (THIS, edge->p, edge->q);
}

/* Mark edge as constrained */

void
p2t_triangle_mark_constrained_edge_pt_pt (P2tTriangle* THIS, P2tPoint* p, P2tPoint* q)
{
  if ((q == THIS->points_[0] && p == THIS->points_[1]) || (q == THIS->points_[1] && p == THIS->points_[0]))
    {
      THIS->constrained_edge[2] = TRUE;
    }
  else if ((q == THIS->points_[0] && p == THIS->points_[2]) || (q == THIS->points_[2] && p == THIS->points_[0]))
    {
      THIS->constrained_edge[1] = TRUE;
    }
  else if ((q == THIS->points_[1] && p == THIS->points_[2]) || (q == THIS->points_[2] && p == THIS->points_[1]))
    {
      THIS->constrained_edge[0] = TRUE;
    }
}

/* The point counter-clockwise to given point */

P2tPoint*
p2t_triangle_point_cw (P2tTriangle* THIS, P2tPoint* point)
{
  if (point == THIS->points_[0])
    {
      return THIS->points_[2];
    }
  else if (point == THIS->points_[1])
    {
      return THIS->points_[0];
    }
  else if (point == THIS->points_[2])
    {
      return THIS->points_[1];
    }
  assert (0);
}

/* The point counter-clockwise to given point */

P2tPoint*
p2t_triangle_point_ccw (P2tTriangle* THIS, P2tPoint* point)
{
  if (point == THIS->points_[0])
    {
      return THIS->points_[1];
    }
  else if (point == THIS->points_[1])
    {
      return THIS->points_[2];
    }
  else if (point == THIS->points_[2])
    {
      return THIS->points_[0];
    }
  assert (0);
}

/* The neighbor clockwise to given point */

P2tTriangle*
p2t_triangle_neighbor_cw (P2tTriangle* THIS, P2tPoint* point)
{
  if (point == THIS->points_[0])
    {
      return THIS->neighbors_[1];
    }
  else if (point == THIS->points_[1])
    {
      return THIS->neighbors_[2];
    }
  return THIS->neighbors_[0];
}

/* The neighbor counter-clockwise to given point */

P2tTriangle*
p2t_triangle_neighbor_ccw (P2tTriangle* THIS, P2tPoint* point)
{
  if (point == THIS->points_[0])
    {
      return THIS->neighbors_[2];
    }
  else if (point == THIS->points_[1])
    {
      return THIS->neighbors_[0];
    }
  return THIS->neighbors_[1];
}

gboolean
p2t_triangle_get_constrained_edge_ccw (P2tTriangle* THIS, P2tPoint* p)
{
  if (p == THIS->points_[0])
    {
      return THIS->constrained_edge[2];
    }
  else if (p == THIS->points_[1])
    {
      return THIS->constrained_edge[0];
    }
  return THIS->constrained_edge[1];
}

gboolean
p2t_triangle_get_constrained_edge_cw (P2tTriangle* THIS, P2tPoint* p)
{
  if (p == THIS->points_[0])
    {
      return THIS->constrained_edge[1];
    }
  else if (p == THIS->points_[1])
    {
      return THIS->constrained_edge[2];
    }
  return THIS->constrained_edge[0];
}

void
p2t_triangle_set_constrained_edge_ccw (P2tTriangle* THIS, P2tPoint* p, gboolean ce)
{
  if (p == THIS->points_[0])
    {
      THIS->constrained_edge[2] = ce;
    }
  else if (p == THIS->points_[1])
    {
      THIS->constrained_edge[0] = ce;
    }
  else
    {
      THIS->constrained_edge[1] = ce;
    }
}

void
p2t_triangle_set_constrained_edge_cw (P2tTriangle* THIS, P2tPoint* p, gboolean ce)
{
  if (p == THIS->points_[0])
    {
      THIS->constrained_edge[1] = ce;
    }
  else if (p == THIS->points_[1])
    {
      THIS->constrained_edge[2] = ce;
    }
  else
    {
      THIS->constrained_edge[0] = ce;
    }
}

gboolean
p2t_triangle_get_delunay_edge_ccw (P2tTriangle* THIS, P2tPoint* p)
{
  if (p == THIS->points_[0])
    {
      return THIS->delaunay_edge[2];
    }
  else if (p == THIS->points_[1])
    {
      return THIS->delaunay_edge[0];
    }
  return THIS->delaunay_edge[1];
}

gboolean
p2t_triangle_get_delunay_edge_cw (P2tTriangle* THIS, P2tPoint* p)
{
  if (p == THIS->points_[0])
    {
      return THIS->delaunay_edge[1];
    }
  else if (p == THIS->points_[1])
    {
      return THIS->delaunay_edge[2];
    }
  return THIS->delaunay_edge[0];
}

void
p2t_triangle_set_delunay_edge_ccw (P2tTriangle* THIS, P2tPoint* p, gboolean e)
{
  if (p == THIS->points_[0])
    {
      THIS->delaunay_edge[2] = e;
    }
  else if (p == THIS->points_[1])
    {
      THIS->delaunay_edge[0] = e;
    }
  else
    {
      THIS->delaunay_edge[1] = e;
    }
}

void
p2t_triangle_set_delunay_edge_cw (P2tTriangle* THIS, P2tPoint* p, gboolean e)
{
  if (p == THIS->points_[0])
    {
      THIS->delaunay_edge[1] = e;
    }
  else if (p == THIS->points_[1])
    {
      THIS->delaunay_edge[2] = e;
    }
  else
    {
      THIS->delaunay_edge[0] = e;
    }
}

/* The neighbor across to given point */

P2tTriangle*
p2t_triangle_neighbor_across (P2tTriangle* THIS, P2tPoint* opoint)
{
  if (opoint == THIS->points_[0])
    {
      return THIS->neighbors_[0];
    }
  else if (opoint == THIS->points_[1])
    {
      return THIS->neighbors_[1];
    }
  return THIS->neighbors_[2];
}

void
p2t_triangle_debug_print (P2tTriangle* THIS)
{
  printf ("%f,%f ", THIS->points_[0]->x, THIS->points_[0]->y);
  printf ("%f,%f ", THIS->points_[1]->x, THIS->points_[1]->y);
  printf ("%f,%f\n", THIS->points_[2]->x, THIS->points_[2]->y);
}

/* WARNING! the function for sorting a g_ptr_array expects to recieve
 *          pointers to the pointers (double indirection)! */

gint
p2t_point_cmp (gconstpointer a, gconstpointer b)
{
  P2tPoint *ap = *((P2tPoint**) a), *bp = *((P2tPoint**) b);
  if (ap->y < bp->y)
    {
      return -1;
    }
  else if (ap->y == bp->y)
    {
      /* Make sure q is point with greater x value */
      if (ap->x < bp->x)
        {
          return -1;
        }
      else if (ap->x == bp->x)
        return 0;
    }
  return 1;
}

/* gboolean operator == (const Point& a, const Point& b) */

gboolean
p2t_point_equals (const P2tPoint* a, const P2tPoint* b)
{
  return a->x == b->x && a->y == b->y;
}

P2tPoint*
p2t_triangle_get_point (P2tTriangle* THIS, const int index)
{
  return THIS->points_[index];
}

P2tTriangle*
p2t_triangle_get_neighbor (P2tTriangle* THIS, const int index)
{
  return THIS->neighbors_[index];
}

gboolean
p2t_triangle_contains_pt (P2tTriangle* THIS, P2tPoint* p)
{
  return p == THIS->points_[0] || p == THIS->points_[1] || p == THIS->points_[2];
}

gboolean
p2t_triangle_contains_ed (P2tTriangle* THIS, const P2tEdge* e)
{
  return p2t_triangle_contains_pt (THIS, e->p) && p2t_triangle_contains_pt (THIS, e->q);
}

gboolean
p2t_triangle_contains_pt_pt (P2tTriangle* THIS, P2tPoint* p, P2tPoint* q)
{
  return p2t_triangle_contains_pt (THIS, p) && p2t_triangle_contains_pt (THIS, q);
}

gboolean
p2t_triangle_is_interior (P2tTriangle* THIS)
{
  return THIS->interior_;
}

void
p2t_triangle_is_interior_b (P2tTriangle* THIS, gboolean b)
{
  THIS->interior_ = b;
}
