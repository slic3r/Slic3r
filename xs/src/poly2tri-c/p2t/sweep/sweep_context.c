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

#include "sweep_context.h"
#include "advancing_front.h"

void
p2t_sweepcontext_basin_init (P2tSweepContextBasin* THIS)
{
  p2t_sweepcontext_basin_clear (THIS);
}

void
p2t_sweepcontext_basin_clear (P2tSweepContextBasin* THIS)
{
  THIS->left_node = NULL;
  THIS->bottom_node = NULL;
  THIS->right_node = NULL;
  THIS->width = 0.0;
  THIS->left_highest = FALSE;
}

void
p2t_sweepcontext_edgeevent_init (P2tSweepContextEdgeEvent* THIS)
{
  THIS->constrained_edge = NULL;
  THIS->right = FALSE;
}

void
p2t_sweepcontext_init (P2tSweepContext* THIS, P2tPointPtrArray polyline)
{
  int i;

  THIS->front_ = NULL;
  THIS->head_ = NULL;
  THIS->tail_ = NULL;

  THIS->af_head_ = NULL;
  THIS->af_middle_ = NULL;
  THIS->af_tail_ = NULL;

  THIS->edge_list = g_ptr_array_new ();
  THIS->triangles_ = g_ptr_array_new ();
  THIS->map_ = NULL;

  p2t_sweepcontext_basin_init (&THIS->basin);
  p2t_sweepcontext_edgeevent_init (&THIS->edge_event);

  THIS->points_ = g_ptr_array_sized_new (polyline->len);
  for (i = 0; i < polyline->len; i++)
    g_ptr_array_add (THIS->points_, point_index (polyline, i));

  p2t_sweepcontext_init_edges (THIS, THIS->points_);
}

P2tSweepContext*
p2t_sweepcontext_new (P2tPointPtrArray polyline)
{
  P2tSweepContext* THIS = g_new (P2tSweepContext, 1);
  p2t_sweepcontext_init (THIS, polyline);
  return THIS;
}

void
p2t_sweepcontext_destroy (P2tSweepContext* THIS)
{
  GList* iter;
  int i;
  /* Clean up memory */

  p2t_point_free (THIS->head_);
  p2t_point_free (THIS->tail_);
  p2t_advancingfront_free (THIS->front_);
  p2t_node_free (THIS->af_head_);
  p2t_node_free (THIS->af_middle_);
  p2t_node_free (THIS->af_tail_);

  g_ptr_array_free (THIS->points_, TRUE);
  g_ptr_array_free (THIS->triangles_, TRUE);

  for (iter = g_list_first (THIS->map_); iter != NULL; iter = g_list_next (iter))
    {
      P2tTriangle* ptr = triangle_val (iter);
      g_free (ptr);
    }

  g_list_free (THIS->map_);

  for (i = 0; i < THIS->edge_list->len; i++)
    {
      p2t_edge_free (edge_index (THIS->edge_list, i));
    }

  g_ptr_array_free (THIS->edge_list, TRUE);

}

void
p2t_sweepcontext_delete (P2tSweepContext* THIS)
{
  p2t_sweepcontext_destroy (THIS);
  g_free(THIS);
}

void
p2t_sweepcontext_add_hole (P2tSweepContext *THIS, P2tPointPtrArray polyline)
{
  int i;
  p2t_sweepcontext_init_edges (THIS, polyline);
  for (i = 0; i < polyline->len; i++)
    {
      g_ptr_array_add (THIS->points_, point_index (polyline, i));
    }
}

void
p2t_sweepcontext_add_point (P2tSweepContext *THIS, P2tPoint* point)
{
  g_ptr_array_add (THIS->points_, point);
}

P2tTrianglePtrArray
p2t_sweepcontext_get_triangles (P2tSweepContext *THIS)
{
  return THIS->triangles_;
}

P2tTrianglePtrList
p2t_sweepcontext_get_map (P2tSweepContext *THIS)
{
  return THIS->map_;
}

void
p2t_sweepcontext_init_triangulation (P2tSweepContext *THIS)
{
  int i;
  double xmax = point_index (THIS->points_, 0)->x, xmin = point_index (THIS->points_, 0)->x;
  double ymax = point_index (THIS->points_, 0)->y, ymin = point_index (THIS->points_, 0)->y;
  double dx, dy;

  /* Calculate bounds. */
  for (i = 0; i < THIS->points_->len; i++)
    {
      P2tPoint* p = point_index (THIS->points_, i);
      if (p->x > xmax)
        xmax = p->x;
      if (p->x < xmin)
        xmin = p->x;
      if (p->y > ymax)
        ymax = p->y;
      if (p->y < ymin)
        ymin = p->y;
    }

  dx = kAlpha * (xmax - xmin);
  dy = kAlpha * (ymax - ymin);
  THIS->head_ = p2t_point_new_dd (xmax + dx, ymin - dy);
  THIS->tail_ = p2t_point_new_dd (xmin - dx, ymin - dy);

  /* Sort points along y-axis */
  g_ptr_array_sort (THIS->points_, p2t_point_cmp);
}

void
p2t_sweepcontext_init_edges (P2tSweepContext *THIS, P2tPointPtrArray polyline)
{
  int i;
  int num_points = polyline->len;
  g_ptr_array_set_size (THIS->edge_list, THIS->edge_list->len + num_points); /* C-OPTIMIZATION */
  for (i = 0; i < num_points; i++)
    {
      int j = i < num_points - 1 ? i + 1 : 0;
      g_ptr_array_add (THIS->edge_list, p2t_edge_new (point_index (polyline, i), point_index (polyline, j)));
    }
}

P2tPoint*
p2t_sweepcontext_get_point (P2tSweepContext *THIS, const int index)
{
  return point_index (THIS->points_, index);
}

void
p2t_sweepcontext_add_to_map (P2tSweepContext *THIS, P2tTriangle* triangle)
{
  THIS->map_ = g_list_append (THIS->map_, triangle);
}

P2tNode*
p2t_sweepcontext_locate_node (P2tSweepContext *THIS, P2tPoint* point)
{
  /* TODO implement search tree */
  return p2t_advancingfront_locate_node (THIS->front_, point->x);
}

void
p2t_sweepcontext_create_advancingfront (P2tSweepContext *THIS, P2tNodePtrArray nodes)
{
  /* Initial triangle */
  P2tTriangle* triangle = p2t_triangle_new (point_index (THIS->points_, 0), THIS->tail_, THIS->head_);

  THIS->map_ = g_list_append (THIS->map_, triangle);

  THIS->af_head_ = p2t_node_new_pt_tr (p2t_triangle_get_point (triangle, 1), triangle);
  THIS->af_middle_ = p2t_node_new_pt_tr (p2t_triangle_get_point (triangle, 0), triangle);
  THIS->af_tail_ = p2t_node_new_pt (p2t_triangle_get_point (triangle, 2));
  THIS->front_ = p2t_advancingfront_new (THIS->af_head_, THIS->af_tail_);

  /* TODO: More intuitiv if head is middles next and not previous?
   *       so swap head and tail */
  THIS->af_head_->next = THIS->af_middle_;
  THIS->af_middle_->next = THIS->af_tail_;
  THIS->af_middle_->prev = THIS->af_head_;
  THIS->af_tail_->prev = THIS->af_middle_;
}

void
p2t_sweepcontext_remove_node (P2tSweepContext *THIS, P2tNode* node)
{
  g_free (node);
}

void
p2t_sweepcontext_map_triangle_to_nodes (P2tSweepContext *THIS, P2tTriangle* t)
{
  int i;
  for (i = 0; i < 3; i++)
    {
      if (!p2t_triangle_get_neighbor (t, i))
        {
          P2tNode* n = p2t_advancingfront_locate_point (THIS->front_, p2t_triangle_point_cw (t, p2t_triangle_get_point (t, i)));
          if (n)
            n->triangle = t;
        }
    }
}

void
p2t_sweepcontext_remove_from_map (P2tSweepContext *THIS, P2tTriangle* triangle)
{
  THIS->map_ = g_list_remove (THIS->map_, triangle);
}

void
p2t_sweepcontext_mesh_clean (P2tSweepContext *THIS, P2tTriangle* triangle)
{
  GQueue triangles = G_QUEUE_INIT;
  int i;

  g_queue_push_tail (&triangles, triangle);

  while (! g_queue_is_empty (&triangles))
    {
      P2tTriangle* t = (P2tTriangle*) g_queue_pop_tail (&triangles);

      if (t != NULL && !p2t_triangle_is_interior (t))
        {
          p2t_triangle_is_interior_b (t, TRUE);
          g_ptr_array_add (THIS->triangles_, t);
          for (i = 0; i < 3; i++)
            {
              if (! t->constrained_edge[i])
                g_queue_push_tail (&triangles, p2t_triangle_get_neighbor (t, i));
            }
        }
    }
}

P2tAdvancingFront*
p2t_sweepcontext_front (P2tSweepContext *THIS)
{
  return THIS->front_;
}

int
p2t_sweepcontext_point_count (P2tSweepContext *THIS)
{
  return THIS->points_->len;
}

void
p2t_sweepcontext_set_head (P2tSweepContext *THIS, P2tPoint* p1)
{
  THIS->head_ = p1;
}

P2tPoint*
p2t_sweepcontext_head (P2tSweepContext *THIS)
{
  return THIS->head_;
}

void
p2t_sweepcontext_set_tail (P2tSweepContext *THIS, P2tPoint* p1)
{
  THIS->tail_ = p1;
}

P2tPoint*
p2t_sweepcontext_tail (P2tSweepContext *THIS)
{
  return THIS->tail_;
}
