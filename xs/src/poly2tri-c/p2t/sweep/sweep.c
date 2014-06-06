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

#include <math.h>

#include "sweep.h"
#include "sweep_context.h"
#include "advancing_front.h"
#include "../common/utils.h"
#include "../common/shapes.h"

void
p2t_sweep_init (P2tSweep* THIS)
{
  THIS->nodes_ = g_ptr_array_new ();
}

P2tSweep*
p2t_sweep_new ()
{
  P2tSweep* THIS = g_new (P2tSweep, 1);
  p2t_sweep_init (THIS);
  return THIS;
}

/**
 * Destructor - clean up memory
 */
void
p2t_sweep_destroy (P2tSweep* THIS)
{
  int i;
  /* Clean up memory */
  for (i = 0; i < THIS->nodes_->len; i++)
    {
      p2t_node_free (node_index (THIS->nodes_, i));
    }

  g_ptr_array_free (THIS->nodes_, TRUE);
}

void
p2t_sweep_free (P2tSweep* THIS)
{
  p2t_sweep_destroy (THIS);
  g_free (THIS);
}

/* Triangulate simple polygon with holes */

void
p2t_sweep_triangulate (P2tSweep *THIS, P2tSweepContext *tcx)
{
  p2t_sweepcontext_init_triangulation (tcx);
  p2t_sweepcontext_create_advancingfront (tcx, THIS->nodes_);
  /* Sweep points; build mesh */
  p2t_sweep_sweep_points (THIS, tcx);
  /* Clean up */
  p2t_sweep_finalization_polygon (THIS, tcx);
}

void
p2t_sweep_sweep_points (P2tSweep *THIS, P2tSweepContext *tcx)
{
  int i, j;
  for (i = 1; i < p2t_sweepcontext_point_count (tcx); i++)
    {
      P2tPoint* point = p2t_sweepcontext_get_point (tcx, i);
      P2tNode* node = p2t_sweep_point_event (THIS, tcx, point);
      for (j = 0; j < point->edge_list->len; j++)
        {
          p2t_sweep_edge_event_ed_n (THIS, tcx, edge_index (point->edge_list, j), node);
        }
    }
}

void
p2t_sweep_finalization_polygon (P2tSweep *THIS, P2tSweepContext *tcx)
{
  /* Get an Internal triangle to start with */
  P2tTriangle* t = p2t_advancingfront_head (p2t_sweepcontext_front (tcx))->next->triangle;
  P2tPoint* p = p2t_advancingfront_head (p2t_sweepcontext_front (tcx))->next->point;
  while (!p2t_triangle_get_constrained_edge_cw (t, p))
    {
      t = p2t_triangle_neighbor_ccw (t, p);
    }

  /* Collect interior triangles constrained by edges */
  p2t_sweepcontext_mesh_clean (tcx, t);
}

P2tNode*
p2t_sweep_point_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tPoint* point)
{
  P2tNode* node = p2t_sweepcontext_locate_node (tcx, point);
  P2tNode* new_node = p2t_sweep_new_front_triangle (THIS, tcx, point, node);

  /* Only need to check +epsilon since point never have smaller
   * x value than node due to how we fetch nodes from the front */
  if (point->x <= node->point->x + EPSILON)
    {
      p2t_sweep_fill (THIS, tcx, node);
    }

  /*tcx.AddNode(new_node); */

  p2t_sweep_fill_advancingfront (THIS, tcx, new_node);
  return new_node;
}

void
p2t_sweep_edge_event_ed_n (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node)
{
  tcx->edge_event.constrained_edge = edge;
  tcx->edge_event.right = (edge->p->x > edge->q->x);

  if (p2t_sweep_is_edge_side_of_triangle (THIS, node->triangle, edge->p, edge->q))
    {
      return;
    }

  /* For now we will do all needed filling
   * TODO: integrate with flip process might give some better performance
   *       but for now this avoid the issue with cases that needs both flips and fills
   */
  p2t_sweep_fill_edge_event (THIS, tcx, edge, node);
  p2t_sweep_edge_event_pt_pt_tr_pt (THIS, tcx, edge->p, edge->q, node->triangle, edge->q);
}

void
p2t_sweep_edge_event_pt_pt_tr_pt (P2tSweep *THIS, P2tSweepContext *tcx, P2tPoint* ep, P2tPoint* eq, P2tTriangle* triangle, P2tPoint* point)
{
  P2tPoint *p1, *p2;
  P2tOrientation o1, o2;

  if (p2t_sweep_is_edge_side_of_triangle (THIS, triangle, ep, eq))
    {
      return;
    }

  p1 = p2t_triangle_point_ccw (triangle, point);
  o1 = p2t_orient2d (eq, p1, ep);
  if (o1 == COLLINEAR)
    {
      if (p2t_triangle_contains_pt_pt (triangle, eq, p1))
        {
          p2t_triangle_mark_constrained_edge_pt_pt (triangle, eq, p1);
          /* We are modifying the constraint maybe it would be better to
           * not change the given constraint and just keep a variable for the new constraint
           */
          tcx->edge_event.constrained_edge->q = p1;
          triangle = p2t_triangle_neighbor_across (triangle, point);
          p2t_sweep_edge_event_pt_pt_tr_pt (THIS, tcx, ep, p1, triangle, p1);
        }
      else
        {
          g_error ("EdgeEvent - collinear points not supported");
        }
      return;
    }

  p2 = p2t_triangle_point_cw (triangle, point);
  o2 = p2t_orient2d (eq, p2, ep);
  if (o2 == COLLINEAR)
    {
      if (p2t_triangle_contains_pt_pt (triangle, eq, p2))
        {
          p2t_triangle_mark_constrained_edge_pt_pt (triangle, eq, p2);
          /* We are modifying the constraint maybe it would be better to
           * not change the given constraint and just keep a variable for the new constraint
           */
          tcx->edge_event.constrained_edge->q = p2;
          triangle = p2t_triangle_neighbor_across (triangle, point);
          p2t_sweep_edge_event_pt_pt_tr_pt (THIS, tcx, ep, p2, triangle, p2);
        }
      else
        {
          g_error ("EdgeEvent - collinear points not supported");
        }
      return;
    }

  if (o1 == o2)
    {
      /* Need to decide if we are rotating CW or CCW to get to a triangle
       * that will cross edge */
      if (o1 == CW)
        {
          triangle = p2t_triangle_neighbor_ccw (triangle, point);
        }
      else
        {
          triangle = p2t_triangle_neighbor_cw (triangle, point);
        }
      p2t_sweep_edge_event_pt_pt_tr_pt (THIS, tcx, ep, eq, triangle, point);
    }
  else
    {
      /* This triangle crosses constraint so lets flippin start! */
      p2t_sweep_flip_edge_event (THIS, tcx, ep, eq, triangle, point);
    }
}

gboolean
p2t_sweep_is_edge_side_of_triangle (P2tSweep *THIS, P2tTriangle *triangle, P2tPoint* ep, P2tPoint* eq)
{
  int index = p2t_triangle_edge_index (triangle, ep, eq);

  if (index != -1)
    {
      P2tTriangle *t;
      p2t_triangle_mark_constrained_edge_i (triangle, index);
      t = p2t_triangle_get_neighbor (triangle, index);
      if (t)
        {
          p2t_triangle_mark_constrained_edge_pt_pt (t, ep, eq);
        }
      return TRUE;
    }
  return FALSE;
}

P2tNode*
p2t_sweep_new_front_triangle (P2tSweep *THIS, P2tSweepContext *tcx, P2tPoint* point, P2tNode *node)
{
  P2tTriangle* triangle = p2t_triangle_new (point, node->point, node->next->point);
  P2tNode *new_node;

  p2t_triangle_mark_neighbor_tr (triangle, node->triangle);
  p2t_sweepcontext_add_to_map (tcx, triangle);

  new_node = p2t_node_new_pt (point);
  g_ptr_array_add (THIS->nodes_, new_node);

  new_node->next = node->next;
  new_node->prev = node;
  node->next->prev = new_node;
  node->next = new_node;

  if (!p2t_sweep_legalize (THIS, tcx, triangle))
    {
      p2t_sweepcontext_map_triangle_to_nodes (tcx, triangle);
    }

  return new_node;
}

void
p2t_sweep_fill (P2tSweep *THIS, P2tSweepContext *tcx, P2tNode* node)
{
  P2tTriangle* triangle = p2t_triangle_new (node->prev->point, node->point, node->next->point);

  /* TODO: should copy the constrained_edge value from neighbor triangles
   *       for now constrained_edge values are copied during the legalize */
  p2t_triangle_mark_neighbor_tr (triangle, node->prev->triangle);
  p2t_triangle_mark_neighbor_tr (triangle, node->triangle);

  p2t_sweepcontext_add_to_map (tcx, triangle);

  /* Update the advancing front */
  node->prev->next = node->next;
  node->next->prev = node->prev;

  /* If it was legalized the triangle has already been mapped */
  if (!p2t_sweep_legalize (THIS, tcx, triangle))
    {
      p2t_sweepcontext_map_triangle_to_nodes (tcx, triangle);
    }

}

void
p2t_sweep_fill_advancingfront (P2tSweep *THIS, P2tSweepContext *tcx, P2tNode* n)
{

  /* Fill right holes */
  P2tNode* node = n->next;

  while (node->next)
    {
      /* if HoleAngle exceeds 90 degrees then break. */
      if (p2t_sweep_large_hole_dont_fill (THIS, node)) break;
      p2t_sweep_fill (THIS, tcx, node);
      node = node->next;
    }

  /* Fill left holes */
  node = n->prev;

  while (node->prev)
    {
      /* if HoleAngle exceeds 90 degrees then break. */
      if (p2t_sweep_large_hole_dont_fill (THIS, node)) break;
      p2t_sweep_fill (THIS, tcx, node);
      node = node->prev;
    }

  /* Fill right basins */
  if (n->next && n->next->next)
    {
      double angle = p2t_sweep_basin_angle (THIS, n);
      if (angle < PI_3div4)
        {
          p2t_sweep_fill_basin (THIS, tcx, n);
        }
    }
}

/* True if HoleAngle exceeds 90 degrees. */
gboolean
p2t_sweep_large_hole_dont_fill (P2tSweep *THIS, P2tNode* node)
{
  P2tNode* nextNode = node->next;
  P2tNode* prevNode = node->prev;
  P2tNode *next2Node, *prev2Node;
  if (! p2t_sweep_angle_exceeds_90_degrees (THIS, node->point, nextNode->point, prevNode->point))
    return FALSE;

  /* Check additional points on front. */
  next2Node = nextNode->next;
  /* "..Plus.." because only want angles on same side as point being added. */
  if ((next2Node != NULL) && !p2t_sweep_angle_exceeds_plus_90_degrees_or_is_negative (THIS, node->point, next2Node->point, prevNode->point))
    return FALSE;

  prev2Node = prevNode->prev;
  /* "..Plus.." because only want angles on same side as point being added. */
  if ((prev2Node != NULL) && !p2t_sweep_angle_exceeds_plus_90_degrees_or_is_negative (THIS, node->point, nextNode->point, prev2Node->point))
    return FALSE;

  return TRUE;
}

gboolean
p2t_sweep_angle_exceeds_90_degrees(P2tSweep* THIS, P2tPoint* origin, P2tPoint* pa, P2tPoint* pb)
{
  gdouble angle = p2t_sweep_angle (THIS, origin, pa, pb);
  gboolean exceeds90Degrees = ((angle > G_PI_2) || (angle < -G_PI_2));
  return exceeds90Degrees;
}

gboolean
p2t_sweep_angle_exceeds_plus_90_degrees_or_is_negative (P2tSweep* THIS, P2tPoint* origin, P2tPoint* pa, P2tPoint* pb)
{
  gdouble angle = p2t_sweep_angle (THIS, origin, pa, pb);
  gboolean exceedsPlus90DegreesOrIsNegative = (angle > G_PI_2) || (angle < 0);
  return exceedsPlus90DegreesOrIsNegative;
}

gdouble
p2t_sweep_angle (P2tSweep* THIS, P2tPoint* origin, P2tPoint* pa, P2tPoint* pb) {
  /* Complex plane
   * ab = cosA +i*sinA
   * ab = (ax + ay*i)(bx + by*i) = (ax*bx + ay*by) + i(ax*by-ay*bx)
   * atan2(y,x) computes the principal value of the argument function
   * applied to the complex number x+iy
   * Where x = ax*bx + ay*by
   *       y = ax*by - ay*bx
   */
  double px = origin->x;
  double py = origin->y;
  double ax = pa->x - px;
  double ay = pa->y - py;
  double bx = pb->x - px;
  double by = pb->y - py;
  double x = ax * by - ay * bx;
  double y = ax * bx + ay * by;
  double angle = atan2(x, y);
  return angle;
}

double
p2t_sweep_basin_angle (P2tSweep *THIS, P2tNode* node)
{
  double ax = node->point->x - node->next->next->point->x;
  double ay = node->point->y - node->next->next->point->y;
  return atan2 (ay, ax);
}

double
p2t_sweep_hole_angle (P2tSweep *THIS, P2tNode* node)
{
  /* Complex plane
   * ab = cosA +i*sinA
   * ab = (ax + ay*i)(bx + by*i) = (ax*bx + ay*by) + i(ax*by-ay*bx)
   * atan2(y,x) computes the principal value of the argument function
   * applied to the complex number x+iy
   * Where x = ax*bx + ay*by
   *       y = ax*by - ay*bx
   */
  double ax = node->next->point->x - node->point->x;
  double ay = node->next->point->y - node->point->y;
  double bx = node->prev->point->x - node->point->x;
  double by = node->prev->point->y - node->point->y;
  return atan2 (ax * by - ay * bx, ax * bx + ay * by);
}

gboolean
p2t_sweep_legalize (P2tSweep *THIS, P2tSweepContext *tcx, P2tTriangle *t)
{
  int i;
  /* To legalize a triangle we start by finding if any of the three edges
   * violate the Delaunay condition */
  for (i = 0; i < 3; i++)
    {
      P2tTriangle *ot;

      if (t->delaunay_edge[i])
        continue;

      ot = p2t_triangle_get_neighbor (t, i);

      if (ot)
        {
          P2tPoint* p = p2t_triangle_get_point (t, i);
          P2tPoint* op = p2t_triangle_opposite_point (ot, t, p);
          int oi = p2t_triangle_index (ot, op);
          gboolean inside;

          /* If this is a Constrained Edge or a Delaunay Edge(only during recursive legalization)
           * then we should not try to legalize */
          if (ot->constrained_edge[oi] || ot->delaunay_edge[oi])
            {
              t->constrained_edge[i] = ot->constrained_edge[oi];
              continue;
            }

          inside = p2t_sweep_incircle (THIS, p, p2t_triangle_point_ccw (t, p), p2t_triangle_point_cw (t, p), op);

          if (inside)
            {
              gboolean not_legalized;
              /* Lets mark this shared edge as Delaunay */
              t->delaunay_edge[i] = TRUE;
              ot->delaunay_edge[oi] = TRUE;

              /* Lets rotate shared edge one vertex CW to legalize it */
              p2t_sweep_rotate_triangle_pair (THIS, t, p, ot, op);

              /* We now got one valid Delaunay Edge shared by two triangles
               * This gives us 4 new edges to check for Delaunay */

              /* Make sure that triangle to node mapping is done only one time for a specific triangle */
              not_legalized = !p2t_sweep_legalize (THIS, tcx, t);
              if (not_legalized)
                {
                  p2t_sweepcontext_map_triangle_to_nodes (tcx, t);
                }

              not_legalized = !p2t_sweep_legalize (THIS, tcx, ot);
              if (not_legalized)
                p2t_sweepcontext_map_triangle_to_nodes (tcx, ot);

              /* Reset the Delaunay edges, since they only are valid Delaunay edges
               * until we add a new triangle or point.
               * XXX: need to think about this. Can these edges be tried after we
               *      return to previous recursive level? */
              t->delaunay_edge[i] = FALSE;
              ot->delaunay_edge[oi] = FALSE;

              /* If triangle have been legalized no need to check the other edges since
               * the recursive legalization will handles those so we can end here.*/
              return TRUE;
            }
        }
    }
  return FALSE;
}

gboolean
p2t_sweep_incircle (P2tSweep *THIS, P2tPoint* pa, P2tPoint* pb, P2tPoint* pc, P2tPoint* pd)
{
  double adx = pa->x - pd->x;
  double ady = pa->y - pd->y;
  double bdx = pb->x - pd->x;
  double bdy = pb->y - pd->y;

  double adxbdy = adx * bdy;
  double bdxady = bdx * ady;
  double oabd = adxbdy - bdxady;

  double cdx, cdy;
  double cdxady, adxcdy, ocad;

  double bdxcdy, cdxbdy;
  double alift, blift, clift;
  double det;

  if (oabd <= 0)
    return FALSE;

  cdx = pc->x - pd->x;
  cdy = pc->y - pd->y;

  cdxady = cdx * ady;
  adxcdy = adx * cdy;
  ocad = cdxady - adxcdy;

  if (ocad <= 0)
    return FALSE;

  bdxcdy = bdx * cdy;
  cdxbdy = cdx * bdy;

  alift = adx * adx + ady * ady;
  blift = bdx * bdx + bdy * bdy;
  clift = cdx * cdx + cdy * cdy;

  det = alift * (bdxcdy - cdxbdy) + blift * ocad + clift * oabd;

  return det > 0;
}

void
p2t_sweep_rotate_triangle_pair (P2tSweep *THIS, P2tTriangle *t, P2tPoint* p, P2tTriangle *ot, P2tPoint* op)
{
  P2tTriangle *n1, *n2, *n3, *n4;
  gboolean ce1, ce2, ce3, ce4;
  gboolean de1, de2, de3, de4;

  n1 = p2t_triangle_neighbor_ccw (t, p);
  n2 = p2t_triangle_neighbor_cw (t, p);
  n3 = p2t_triangle_neighbor_ccw (ot, op);
  n4 = p2t_triangle_neighbor_cw (ot, op);

  ce1 = p2t_triangle_get_constrained_edge_ccw (t, p);
  ce2 = p2t_triangle_get_constrained_edge_cw (t, p);
  ce3 = p2t_triangle_get_constrained_edge_ccw (ot, op);
  ce4 = p2t_triangle_get_constrained_edge_cw (ot, op);

  de1 = p2t_triangle_get_delunay_edge_ccw (t, p);
  de2 = p2t_triangle_get_delunay_edge_cw (t, p);
  de3 = p2t_triangle_get_delunay_edge_ccw (ot, op);
  de4 = p2t_triangle_get_delunay_edge_cw (ot, op);

  p2t_triangle_legalize_pt_pt (t, p, op);
  p2t_triangle_legalize_pt_pt (ot, op, p);

  /* Remap delaunay_edge */
  p2t_triangle_set_delunay_edge_ccw (ot, p, de1);
  p2t_triangle_set_delunay_edge_cw (t, p, de2);
  p2t_triangle_set_delunay_edge_ccw (t, op, de3);
  p2t_triangle_set_delunay_edge_cw (ot, op, de4);

  /* Remap constrained_edge */
  p2t_triangle_set_constrained_edge_ccw (ot, p, ce1);
  p2t_triangle_set_constrained_edge_cw (t, p, ce2);
  p2t_triangle_set_constrained_edge_ccw (t, op, ce3);
  p2t_triangle_set_constrained_edge_cw (ot, op, ce4);

  /* Remap neighbors
   * XXX: might optimize the markNeighbor by keeping track of
   *      what side should be assigned to what neighbor after the
   *      rotation. Now mark neighbor does lots of testing to find
   *      the right side. */
  p2t_triangle_clear_neighbors (t);
  p2t_triangle_clear_neighbors (ot);
  if (n1) p2t_triangle_mark_neighbor_tr (ot, n1);
  if (n2) p2t_triangle_mark_neighbor_tr (t, n2);
  if (n3) p2t_triangle_mark_neighbor_tr (t, n3);
  if (n4) p2t_triangle_mark_neighbor_tr (ot, n4);
  p2t_triangle_mark_neighbor_tr (t, ot);
}

void
p2t_sweep_fill_basin (P2tSweep *THIS, P2tSweepContext *tcx, P2tNode* node)
{
  if (p2t_orient2d (node->point, node->next->point, node->next->next->point) == CCW)
    {
      tcx->basin.left_node = node->next->next;
    }
  else
    {
      tcx->basin.left_node = node->next;
    }

  /* Find the bottom and right node */
  tcx->basin.bottom_node = tcx->basin.left_node;
  while (tcx->basin.bottom_node->next
         && tcx->basin.bottom_node->point->y >= tcx->basin.bottom_node->next->point->y)
    {
      tcx->basin.bottom_node = tcx->basin.bottom_node->next;
    }
  if (tcx->basin.bottom_node == tcx->basin.left_node)
    {
      /* No valid basin */
      return;
    }

  tcx->basin.right_node = tcx->basin.bottom_node;
  while (tcx->basin.right_node->next
         && tcx->basin.right_node->point->y < tcx->basin.right_node->next->point->y)
    {
      tcx->basin.right_node = tcx->basin.right_node->next;
    }
  if (tcx->basin.right_node == tcx->basin.bottom_node)
    {
      /* No valid basins */
      return;
    }

  tcx->basin.width = tcx->basin.right_node->point->x - tcx->basin.left_node->point->x;
  tcx->basin.left_highest = tcx->basin.left_node->point->y > tcx->basin.right_node->point->y;

  p2t_sweep_fill_basin_req (THIS, tcx, tcx->basin.bottom_node);
}

void
p2t_sweep_fill_basin_req (P2tSweep *THIS, P2tSweepContext *tcx, P2tNode* node)
{
  /* if shallow stop filling */
  if (p2t_sweep_is_shallow (THIS, tcx, node))
    {
      return;
    }

  p2t_sweep_fill (THIS, tcx, node);

  if (node->prev == tcx->basin.left_node && node->next == tcx->basin.right_node)
    {
      return;
    }
  else if (node->prev == tcx->basin.left_node)
    {
      P2tOrientation o = p2t_orient2d (node->point, node->next->point, node->next->next->point);
      if (o == CW)
        {
          return;
        }
      node = node->next;
    }
  else if (node->next == tcx->basin.right_node)
    {
      P2tOrientation o = p2t_orient2d (node->point, node->prev->point, node->prev->prev->point);
      if (o == CCW)
        {
          return;
        }
      node = node->prev;
    }
  else
    {
      /* Continue with the neighbor node with lowest Y value */
      if (node->prev->point->y < node->next->point->y)
        {
          node = node->prev;
        }
      else
        {
          node = node->next;
        }
    }

  p2t_sweep_fill_basin_req (THIS, tcx, node);
}

gboolean
p2t_sweep_is_shallow (P2tSweep *THIS, P2tSweepContext *tcx, P2tNode* node)
{
  double height;

  if (tcx->basin.left_highest)
    {
      height = tcx->basin.left_node->point->y - node->point->y;
    }
  else
    {
      height = tcx->basin.right_node->point->y - node->point->y;
    }

  /* if shallow stop filling */
  if (tcx->basin.width > height)
    {
      return TRUE;
    }
  return FALSE;
}

void
p2t_sweep_fill_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node)
{
  if (tcx->edge_event.right)
    {
      p2t_sweep_fill_right_above_edge_event (THIS, tcx, edge, node);
    }
  else
    {
      p2t_sweep_fill_left_above_edge_event (THIS, tcx, edge, node);
    }
}

void
p2t_sweep_fill_right_above_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node)
{
  while (node->next->point->x < edge->p->x)
    {
      /* Check if next node is below the edge */
      if (p2t_orient2d (edge->q, node->next->point, edge->p) == CCW)
        {
          p2t_sweep_fill_right_below_edge_event (THIS, tcx, edge, node);
        }
      else
        {
          node = node->next;
        }
    }
}

void
p2t_sweep_fill_right_below_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node)
{
  if (node->point->x < edge->p->x)
    {
      if (p2t_orient2d (node->point, node->next->point, node->next->next->point) == CCW)
        {
          /* Concave */
          p2t_sweep_fill_right_concave_edge_event (THIS, tcx, edge, node);
        }
      else
        {
          /* Convex */
          p2t_sweep_fill_right_convex_edge_event (THIS, tcx, edge, node);
          /* Retry this one */
          p2t_sweep_fill_right_below_edge_event (THIS, tcx, edge, node);
        }
    }
}

void
p2t_sweep_fill_right_concave_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node)
{
  p2t_sweep_fill (THIS, tcx, node->next);
  if (node->next->point != edge->p)
    {
      /* Next above or below edge? */
      if (p2t_orient2d (edge->q, node->next->point, edge->p) == CCW)
        {
          /* Below */
          if (p2t_orient2d (node->point, node->next->point, node->next->next->point) == CCW)
            {
              /* Next is concave */
              p2t_sweep_fill_right_concave_edge_event (THIS, tcx, edge, node);
            }
          else
            {
              /* Next is convex */
            }
        }
    }

}

void
p2t_sweep_fill_right_convex_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node)
{
  /* Next concave or convex? */
  if (p2t_orient2d (node->next->point, node->next->next->point, node->next->next->next->point) == CCW)
    {
      /* Concave */
      p2t_sweep_fill_right_concave_edge_event (THIS, tcx, edge, node->next);
    }
  else
    {
      /* Convex
       * Next above or below edge? */
      if (p2t_orient2d (edge->q, node->next->next->point, edge->p) == CCW)
        {
          /* Below */
          p2t_sweep_fill_right_convex_edge_event (THIS, tcx, edge, node->next);
        }
      else
        {
          /* Above */
        }
    }
}

void
p2t_sweep_fill_left_above_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node)
{
  while (node->prev->point->x > edge->p->x)
    {
      /* Check if next node is below the edge */
      if (p2t_orient2d (edge->q, node->prev->point, edge->p) == CW)
        {
          p2t_sweep_fill_left_below_edge_event (THIS, tcx, edge, node);
        }
      else
        {
          node = node->prev;
        }
    }
}

void
p2t_sweep_fill_left_below_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node)
{
  if (node->point->x > edge->p->x)
    {
      if (p2t_orient2d (node->point, node->prev->point, node->prev->prev->point) == CW)
        {
          /* Concave */
          p2t_sweep_fill_left_concave_edge_event (THIS, tcx, edge, node);
        }
      else
        {
          /* Convex */
          p2t_sweep_fill_left_convex_edge_event (THIS, tcx, edge, node);
          /* Retry this one */
          p2t_sweep_fill_left_below_edge_event (THIS, tcx, edge, node);
        }
    }
}

void
p2t_sweep_fill_left_convex_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node)
{
  /* Next concave or convex? */
  if (p2t_orient2d (node->prev->point, node->prev->prev->point, node->prev->prev->prev->point) == CW)
    {
      /* Concave */
      p2t_sweep_fill_left_concave_edge_event (THIS, tcx, edge, node->prev);
    }
  else
    {
      /* Convex
       * Next above or below edge? */
      if (p2t_orient2d (edge->q, node->prev->prev->point, edge->p) == CW)
        {
          /* Below */
          p2t_sweep_fill_left_convex_edge_event (THIS, tcx, edge, node->prev);
        }
      else
        {
          /* Above */
        }
    }
}

void
p2t_sweep_fill_left_concave_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node)
{
  p2t_sweep_fill (THIS, tcx, node->prev);
  if (node->prev->point != edge->p)
    {
      /* Next above or below edge? */
      if (p2t_orient2d (edge->q, node->prev->point, edge->p) == CW)
        {
          /* Below */
          if (p2t_orient2d (node->point, node->prev->point, node->prev->prev->point) == CW)
            {
              /* Next is concave */
              p2t_sweep_fill_left_concave_edge_event (THIS, tcx, edge, node);
            }
          else
            {
              /* Next is convex */
            }
        }
    }

}

void
p2t_sweep_flip_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tPoint* ep, P2tPoint* eq, P2tTriangle* t, P2tPoint* p)
{
  P2tTriangle* ot = p2t_triangle_neighbor_across (t, p);
  P2tPoint* op = p2t_triangle_opposite_point (ot, t, p);

  if (ot == NULL)
    {
      /* If we want to integrate the fillEdgeEvent do it here
       * With current implementation we should never get here
       *throw new RuntimeException( "[BUG:FIXME] FLIP failed due to missing triangle");
       */
      assert (0);
    }

  if (p2t_utils_in_scan_area (p, p2t_triangle_point_ccw (t, p), p2t_triangle_point_cw (t, p), op))
    {
      /* Lets rotate shared edge one vertex CW */
      p2t_sweep_rotate_triangle_pair (THIS, t, p, ot, op);
      p2t_sweepcontext_map_triangle_to_nodes (tcx, t);
      p2t_sweepcontext_map_triangle_to_nodes (tcx, ot);

      if (p == eq && op == ep)
        {
          if (p2t_point_equals (eq, tcx->edge_event.constrained_edge->q) && p2t_point_equals (ep, tcx->edge_event.constrained_edge->p))
            {
              p2t_triangle_mark_constrained_edge_pt_pt (t, ep, eq);
              p2t_triangle_mark_constrained_edge_pt_pt (ot, ep, eq);
              p2t_sweep_legalize (THIS, tcx, t);
              p2t_sweep_legalize (THIS, tcx, ot);
            }
          else
            {
              /* XXX: I think one of the triangles should be legalized here? */
            }
        }
      else
        {
          P2tOrientation o = p2t_orient2d (eq, op, ep);
          t = p2t_sweep_next_flip_triangle (THIS, tcx, (int) o, t, ot, p, op);
          p2t_sweep_flip_edge_event (THIS, tcx, ep, eq, t, p);
        }
    }
  else
    {
      P2tPoint* newP = p2t_sweep_next_flip_point (THIS, ep, eq, ot, op);
      p2t_sweep_flip_scan_edge_event (THIS, tcx, ep, eq, t, ot, newP);
      p2t_sweep_edge_event_pt_pt_tr_pt (THIS, tcx, ep, eq, t, p);
    }
}

P2tTriangle*
p2t_sweep_next_flip_triangle (P2tSweep *THIS, P2tSweepContext *tcx, int o, P2tTriangle *t, P2tTriangle *ot, P2tPoint* p, P2tPoint* op)
{
  int edge_index;

  if (o == CCW)
    {
      /* ot is not crossing edge after flip */
      int edge_index = p2t_triangle_edge_index (ot, p, op);
      ot->delaunay_edge[edge_index] = TRUE;
      p2t_sweep_legalize (THIS, tcx, ot);
      p2t_triangle_clear_delunay_edges (ot);
      return t;
    }

  /* t is not crossing edge after flip */
  edge_index = p2t_triangle_edge_index (t, p, op);

  t->delaunay_edge[edge_index] = TRUE;
  p2t_sweep_legalize (THIS, tcx, t);
  p2t_triangle_clear_delunay_edges (t);
  return ot;
}

P2tPoint*
p2t_sweep_next_flip_point (P2tSweep *THIS, P2tPoint* ep, P2tPoint* eq, P2tTriangle *ot, P2tPoint* op)
{
  P2tOrientation o2d = p2t_orient2d (eq, op, ep);
  if (o2d == CW)
    {
      /* Right */
      return p2t_triangle_point_ccw (ot, op);
    }
  else if (o2d == CCW)
    {
      /* Left */
      return p2t_triangle_point_cw (ot, op);
    }
  else
    {
      /*throw new RuntimeException("[Unsupported] Opposing point on constrained edge");*/
      assert (0);
    }
}

void
p2t_sweep_flip_scan_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tPoint* ep, P2tPoint* eq, P2tTriangle *flip_triangle,
                                P2tTriangle *t, P2tPoint* p)
{
  P2tTriangle* ot = p2t_triangle_neighbor_across (t, p);
  P2tPoint* op = p2t_triangle_opposite_point (ot, t, p);

  if (p2t_triangle_neighbor_across (t, p) == NULL)
    {
      /* If we want to integrate the fillEdgeEvent do it here
       * With current implementation we should never get here
       *throw new RuntimeException( "[BUG:FIXME] FLIP failed due to missing triangle");
       */
      assert (0);
    }

  if (p2t_utils_in_scan_area (eq, p2t_triangle_point_ccw (flip_triangle, eq), p2t_triangle_point_cw (flip_triangle, eq), op))
    {
      /* flip with new edge op->eq */
      p2t_sweep_flip_edge_event (THIS, tcx, eq, op, ot, op);
      /* TODO: Actually I just figured out that it should be possible to
       *       improve this by getting the next ot and op before the the above
       *       flip and continue the flipScanEdgeEvent here
       * set new ot and op here and loop back to inScanArea test
       * also need to set a new flip_triangle first
       * Turns out at first glance that this is somewhat complicated
       * so it will have to wait. */
    }
  else
    {
      P2tPoint* newP = p2t_sweep_next_flip_point (THIS, ep, eq, ot, op);
      p2t_sweep_flip_scan_edge_event (THIS, tcx, ep, eq, flip_triangle, ot, newP);
    }
}
