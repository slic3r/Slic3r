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

#include "point.h"
#include "edge.h"
#include "vedge.h"
#include "mesh.h"

static void
p2tr_vedge_init (P2trVEdge *self,
                 P2trPoint *start,
                 P2trPoint *end,
                 gboolean   constrained)
{
  self->start       = start;
  self->end         = end;
  self->constrained = constrained;
  self->refcount    = 0;
}

P2trVEdge*
p2tr_vedge_new (P2trPoint *start,
                P2trPoint *end,
                gboolean   constrained)
{
  P2trVEdge *self = g_slice_new (P2trVEdge);

  p2tr_vedge_init (self, start, end, constrained);

  p2tr_point_ref (start);
  p2tr_point_ref (end);
  
  ++self->refcount;
  return self;
}

P2trVEdge*
p2tr_vedge_new2 (P2trEdge *real)
{
  return p2tr_vedge_new (P2TR_EDGE_START (real), real->end,
      real->constrained);
}

P2trVEdge*
p2tr_vedge_ref (P2trVEdge *self)
{
  ++self->refcount;
  return self;
}

void
p2tr_vedge_unref (P2trVEdge *self)
{
  g_assert (self->refcount > 0);
  if (--self->refcount == 0)
    p2tr_vedge_free (self);
}

P2trEdge*
p2tr_vedge_is_real (P2trVEdge *self)
{
  return p2tr_point_has_edge_to (self->start, self->end);
}

void
p2tr_vedge_create (P2trVEdge *self)
{
  P2trMesh *mesh;
  P2trEdge *edge;

  g_assert (! p2tr_vedge_is_real (self));

  mesh = p2tr_vedge_get_mesh (self);
  if (mesh != NULL)
    {
      edge = p2tr_mesh_new_edge (mesh, self->start, self->end, self->constrained);
      p2tr_mesh_unref (mesh);
    }
  else
    edge = p2tr_edge_new (self->start, self->end, self->constrained);

  p2tr_edge_unref (edge);
}

void
p2tr_vedge_remove (P2trVEdge *self)
{
  P2trEdge *edge = p2tr_vedge_is_real (self);

  g_assert (edge != NULL);

  p2tr_edge_remove (edge);
}

void
p2tr_vedge_free (P2trVEdge *self)
{
  p2tr_point_unref (self->start);
  p2tr_point_unref (self->end);
  g_slice_free (P2trVEdge, self);
}

P2trMesh*
p2tr_vedge_get_mesh (P2trVEdge *self)
{
  return p2tr_point_get_mesh (self->end);
}

P2trEdge*
p2tr_vedge_get (P2trVEdge *self)
{
  return p2tr_point_get_edge_to (self->start, self->end, TRUE);
}

gboolean
p2tr_vedge_try_get_and_unref (P2trVEdge  *self,
                              P2trEdge  **real)
{
  P2trEdge *real_one = p2tr_vedge_is_real (self);
  if (real_one)
    p2tr_edge_ref (real_one);
  p2tr_vedge_unref (self);
  return (*real = real_one) != NULL;
}

P2trVEdgeSet*
p2tr_vedge_set_new ()
{
  return p2tr_hash_set_new (
      (GHashFunc)      p2tr_vedge_undirected_hash,
      (GEqualFunc)     p2tr_vedge_undirected_equals,
      NULL
  );
}

void
p2tr_vedge_set_add (P2trVEdgeSet *self,
                   P2trEdge    *to_flip)
{
  p2tr_vedge_set_add2 (self, p2tr_vedge_new2 (to_flip));
  p2tr_edge_unref (to_flip);
}

void
p2tr_vedge_set_add2 (P2trVEdgeSet *self,
                    P2trVEdge   *to_flip)
{
  if (p2tr_hash_set_contains (self, to_flip))
    p2tr_vedge_unref (to_flip);
  else
    p2tr_hash_set_insert (self, to_flip);
}

gboolean
p2tr_vedge_set_pop (P2trVEdgeSet  *self,
                   P2trVEdge   **value)
{
  P2trHashSetIter iter;
  p2tr_hash_set_iter_init (&iter, self);
  if (p2tr_hash_set_iter_next (&iter, (gpointer*) value))
    {
      p2tr_hash_set_remove (self, *value);
      return TRUE;
    }
  else
    return FALSE;
}

guint
p2tr_vedge_set_size (P2trVEdgeSet *self)
{
  return p2tr_hash_set_size (self);
}

void
p2tr_vedge_set_free (P2trVEdgeSet *self)
{
  g_assert (p2tr_hash_set_size (self) == 0);
  p2tr_hash_set_free (self);
}

gboolean
p2tr_vedge_undirected_equals (const P2trVEdge *e1,
                              const P2trVEdge *e2)
{
  return ((e1 == NULL) == (e2 == NULL)) &&
      (e1 == e2
      || (e1->start == e2->start && e1->end == e2->end)
      || (e1->end == e2->start && e1->start == e2->end));
}

guint
p2tr_vedge_undirected_hash (const P2trVEdge *edge)
{
  return g_direct_hash (edge->start) ^ g_direct_hash (edge->end);
}
