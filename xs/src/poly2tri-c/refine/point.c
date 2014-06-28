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
#include "mesh.h"

P2trPoint*
p2tr_point_new (const P2trVector2 *c)
{
  return p2tr_point_new2 (c->x, c->y);
}

P2trPoint*
p2tr_point_new2 (gdouble x, gdouble y)
{
  P2trPoint *self = g_slice_new (P2trPoint);
  
  self->c.x = x;
  self->c.y = y;
  self->mesh = NULL;
  self->outgoing_edges = NULL;
  self->refcount = 1;

  return self;
}

void
p2tr_point_remove (P2trPoint *self)
{
  /* We can not iterate over the list of edges while removing the edges,
   * because the removal action will modify the list. Instead we will
   * simply look at the first edge untill the list is emptied. */
  while (self->outgoing_edges != NULL)
    p2tr_edge_remove ((P2trEdge*) self->outgoing_edges->data);

  if (self->mesh != NULL)
    p2tr_mesh_on_point_removed (self->mesh, self);
}

void
p2tr_point_free (P2trPoint *self)
{
  p2tr_point_remove (self);
  g_slice_free (P2trPoint, self);
}

P2trEdge*
p2tr_point_has_edge_to (P2trPoint *start,
                        P2trPoint *end)
{
  GList *iter;
  
  for (iter = start->outgoing_edges; iter != NULL; iter = iter->next)
    {
      P2trEdge *edge = (P2trEdge*) iter->data;
      if (edge->end == end)
        return edge;
    }
  
  return NULL;
}

P2trEdge*
p2tr_point_get_edge_to (P2trPoint *start,
                        P2trPoint *end,
                        gboolean   do_ref)
{
    P2trEdge* result = p2tr_point_has_edge_to (start, end);
    if (result == NULL)
      p2tr_exception_programmatic ("Tried to get an edge that doesn't exist!");
    else
      return do_ref ? p2tr_edge_ref (result) : result;
}

void
_p2tr_point_insert_edge (P2trPoint *self, P2trEdge *e)
{
  GList *iter = self->outgoing_edges;

  /* Remember: Edges are sorted in ASCENDING angle! */
  while (iter != NULL && ((P2trEdge*)iter->data)->angle < e->angle)
    iter = iter->next;

  self->outgoing_edges =
      g_list_insert_before (self->outgoing_edges, iter, e);

  p2tr_edge_ref (e);
}

void
_p2tr_point_remove_edge (P2trPoint *self, P2trEdge* e)
{
  GList *node;
  
  if (P2TR_EDGE_START(e) != self)
    p2tr_exception_programmatic ("Could not remove the given outgoing "
        "edge because doesn't start on this point!");

  node = g_list_find (self->outgoing_edges, e);
  if (node == NULL)
    p2tr_exception_programmatic ("Could not remove the given outgoing "
        "edge because it's not present in the outgoing-edges list!");

  self->outgoing_edges = g_list_delete_link (self->outgoing_edges, node);

  p2tr_edge_unref (e);
}

P2trEdge*
p2tr_point_edge_ccw (P2trPoint *self,
                     P2trEdge  *e)
{
  GList *node;
  P2trEdge *result;

  if (P2TR_EDGE_START(e) != self)
      p2tr_exception_programmatic ("Not an edge of this point!");

  node = g_list_find (self->outgoing_edges, e);
  if (node == NULL)
    p2tr_exception_programmatic ("Could not find the CCW sibling edge"
        "because the edge is not present in the outgoing-edges list!");

  result = (P2trEdge*) g_list_cyclic_next (self->outgoing_edges, node)->data;
  return p2tr_edge_ref (result);
}

P2trEdge*
p2tr_point_edge_cw (P2trPoint* self,
                    P2trEdge *e)
{
  GList *node;
  P2trEdge *result;

  if (P2TR_EDGE_START(e) != self)
      p2tr_exception_programmatic ("Not an edge of this point!");

  node = g_list_find (self->outgoing_edges, e);
  if (node == NULL)
    p2tr_exception_programmatic ("Could not find the CW sibling edge"
        "because the edge is not present in the outgoing-edges list!");

  result = (P2trEdge*) g_list_cyclic_prev (self->outgoing_edges, node)->data;
  return p2tr_edge_ref (result);
}

gboolean
p2tr_point_is_fully_in_domain (P2trPoint *self)
{
  GList *iter;
  for (iter = self->outgoing_edges; iter != NULL; iter = iter->next)
    if (((P2trEdge*) iter->data)->tri == NULL)
      return FALSE;
      
  return TRUE;
}

gboolean
p2tr_point_has_constrained_edge (P2trPoint *self)
{
  GList *iter;
  for (iter = self->outgoing_edges; iter != NULL; iter = iter->next)
    if (((P2trEdge*) iter->data)->constrained)
      return TRUE;
      
  return FALSE;
}

/**
 * Increase the reference count of the given input point
 * @param self - The point to ref
 * @return The point given
 */
P2trPoint*
p2tr_point_ref (P2trPoint *self)
{
  ++self->refcount;
  return self;
}

void
p2tr_point_unref (P2trPoint *self)
{
  g_assert (self->refcount > 0);
  if (--self->refcount == 0)
    p2tr_point_free (self);
}

P2trMesh*
p2tr_point_get_mesh (P2trPoint *self)
{
  if (self->mesh)
    return p2tr_mesh_ref (self->mesh);
  else
    return NULL;
}
