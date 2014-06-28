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
#include "point.h"
#include "edge.h"

#include "cluster.h"

static gboolean p2tr_cluster_cw_tri_between_is_in_domain (P2trEdge *e1,
                                                          P2trEdge *e2);

gdouble
p2tr_cluster_shortest_edge_length (P2trCluster *self)
{
  gdouble min_length_sq = G_MAXDOUBLE, temp;
  GList *iter;
  
  for (iter = self->edges.head; iter != NULL; iter = iter->next)
    {
      temp = p2tr_edge_get_length_squared ((P2trEdge*)iter->data);
      min_length_sq = MIN(min_length_sq, temp);
    }
  return sqrt (min_length_sq);
}

/**
 * Return the edge cluster of the specified edge from the specified end
 * point. THE EDGES IN THE CLUSTER MUST BE UNREFFED!
 * @param[in] P The point which is shared between all edges of the cluster
 * @param[in] E The edge whose cluster should be returned
 * @return The cluster of @ref E from the point @ref P
 */
P2trCluster*
p2tr_cluster_get_for (P2trPoint   *P,
                      P2trEdge    *E)
{
  P2trCluster *cluster = g_slice_new (P2trCluster);
  gdouble temp_angle;
  P2trEdge *current, *next;
  
  cluster->min_angle = G_MAXDOUBLE;
  g_queue_init (&cluster->edges);

  if (P == E->end)
    E = E->mirror;
  else if (P != P2TR_EDGE_START (E))
    p2tr_exception_programmatic ("Unexpected point for the edge!");

  g_queue_push_head (&cluster->edges, p2tr_edge_ref (E));

  current = p2tr_edge_ref (E);
  next = p2tr_point_edge_cw (P, current);
  
  while (next != g_queue_peek_head (&cluster->edges)
      && (temp_angle = p2tr_edge_angle_between (current->mirror, next)) <= P2TR_CLUSTER_LIMIT_ANGLE
      && p2tr_cluster_cw_tri_between_is_in_domain (current, next))
    {
      g_queue_push_tail (&cluster->edges, p2tr_edge_ref (next));
      p2tr_edge_unref (current);
      current = next;
      next = p2tr_point_edge_cw (P, current);
      cluster->min_angle = MIN (cluster->min_angle, temp_angle);
    }
  p2tr_edge_unref (current);
  p2tr_edge_unref (next);

  current = p2tr_edge_ref (E);
  next = p2tr_point_edge_ccw (P, current);

  while (next != g_queue_peek_tail (&cluster->edges)
      && (temp_angle = p2tr_edge_angle_between (current->mirror, next)) <= P2TR_CLUSTER_LIMIT_ANGLE
      && p2tr_cluster_cw_tri_between_is_in_domain (next, current))
    {
      g_queue_push_head (&cluster->edges, p2tr_edge_ref (next));
      p2tr_edge_unref (current);
      current = next;
      next = p2tr_point_edge_ccw (P, current);
      cluster->min_angle = MIN(cluster->min_angle, temp_angle);
    }
  p2tr_edge_unref (current);
  p2tr_edge_unref (next);

  return cluster;
}

/*     ^ e1
 *    /
 *   /_ e1.Tri (e2.Mirror.Tri)
 *  /  |
 * *---------> e2
 *
 * Check if the angle marked is a part of the triangulation
 * domain
 */
static gboolean
p2tr_cluster_cw_tri_between_is_in_domain (P2trEdge *e1, P2trEdge *e2)
{
  if (P2TR_EDGE_START(e1) != P2TR_EDGE_START(e2) || e1->tri != e2->mirror->tri)
    p2tr_exception_programmatic ("Non clockwise adjacent edges!");
  return e1->tri != NULL;
}

void
p2tr_cluster_free (P2trCluster *self)
{
  GList *iter;
  
  for (iter = self->edges.head; iter != NULL; iter = iter->next)
    p2tr_edge_unref ((P2trEdge*)iter->data);

  g_queue_clear (&self->edges);
  g_slice_free (P2trCluster, self);
}
