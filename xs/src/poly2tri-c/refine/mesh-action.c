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
#include "vedge.h"
#include "vtriangle.h"
#include "mesh-action.h"

static P2trMeshAction*
p2tr_mesh_action_point (P2trPoint *point,
                        gboolean   added)
{
  P2trMeshAction *self = g_slice_new (P2trMeshAction);
  self->type = P2TR_MESH_ACTION_POINT;
  self->added = added;
  self->refcount = 1;
  self->action.action_point.point = p2tr_point_ref (point);
  return self;
}

static void
p2tr_mesh_action_point_free (P2trMeshAction *self)
{
  g_assert (self->type == P2TR_MESH_ACTION_POINT);
  p2tr_point_unref (self->action.action_point.point);
  g_slice_free (P2trMeshAction, self);
}

static void
p2tr_mesh_action_point_undo (P2trMeshAction *self,
                             P2trMesh       *mesh)
{
  g_assert (self->type == P2TR_MESH_ACTION_POINT);
  if (self->added)
    p2tr_point_remove (self->action.action_point.point);
  else
    p2tr_mesh_add_point (mesh, self->action.action_point.point);
}

P2trMeshAction*
p2tr_mesh_action_new_point (P2trPoint *point)
{
  return p2tr_mesh_action_point (point, TRUE);
}

P2trMeshAction*
p2tr_mesh_action_del_point (P2trPoint *point)
{
  return p2tr_mesh_action_point (point, FALSE);
}

static P2trMeshAction*
p2tr_mesh_action_edge (P2trEdge *edge,
                       gboolean  added)
{
  P2trMeshAction *self = g_slice_new (P2trMeshAction);
  self->type = P2TR_MESH_ACTION_EDGE;
  self->added = added;
  self->refcount = 1;
  self->action.action_edge.vedge = p2tr_vedge_new2 (edge);
  self->action.action_edge.constrained = edge->constrained;
  return self;
}

static void
p2tr_mesh_action_edge_free (P2trMeshAction *self)
{
  g_assert (self->type == P2TR_MESH_ACTION_EDGE);
  p2tr_vedge_unref (self->action.action_edge.vedge);
  g_slice_free (P2trMeshAction, self);
}

static void
p2tr_mesh_action_edge_undo (P2trMeshAction *self,
                            P2trMesh       *mesh)
{
  g_assert (self->type == P2TR_MESH_ACTION_EDGE);

  if (self->added)
    p2tr_vedge_remove (self->action.action_edge.vedge);
  else
    p2tr_vedge_create (self->action.action_edge.vedge);
}

P2trMeshAction*
p2tr_mesh_action_new_edge (P2trEdge *edge)
{
  return p2tr_mesh_action_edge (edge, TRUE);
}

P2trMeshAction*
p2tr_mesh_action_del_edge (P2trEdge *edge)
{
  return p2tr_mesh_action_edge (edge, FALSE);
}

static P2trMeshAction*
p2tr_mesh_action_triangle (P2trTriangle *tri,
                           gboolean      added)
{
  P2trMeshAction *self = g_slice_new (P2trMeshAction);
  self->type = P2TR_MESH_ACTION_TRIANGLE;
  self->added = added;
  self->refcount = 1;
  self->action.action_tri.vtri = p2tr_vtriangle_new (tri);
  return self;
}

static void
p2tr_mesh_action_triangle_free (P2trMeshAction *self)
{
  g_assert (self->type == P2TR_MESH_ACTION_TRIANGLE);
  p2tr_vtriangle_unref (self->action.action_tri.vtri);
  g_slice_free (P2trMeshAction, self);
}

static void
p2tr_mesh_action_triangle_undo (P2trMeshAction *self,
                                P2trMesh       *mesh)
{
  g_assert (self->type == P2TR_MESH_ACTION_TRIANGLE);

  if (self->added)
    p2tr_vtriangle_remove (self->action.action_tri.vtri);
  else
    p2tr_vtriangle_create (self->action.action_tri.vtri);
}

P2trMeshAction*
p2tr_mesh_action_new_triangle (P2trTriangle *tri)
{
  return p2tr_mesh_action_triangle (tri, TRUE);
}

P2trMeshAction*
p2tr_mesh_action_del_triangle (P2trTriangle *tri)
{
  return p2tr_mesh_action_triangle (tri, FALSE);
}

P2trMeshAction*
p2tr_mesh_action_ref (P2trMeshAction *self)
{
  ++self->refcount;
  return self;
}

void
p2tr_mesh_action_free (P2trMeshAction *self)
{
  switch (self->type)
    {
      case P2TR_MESH_ACTION_POINT:
        p2tr_mesh_action_point_free (self);
        break;
      case P2TR_MESH_ACTION_EDGE:
        p2tr_mesh_action_edge_free (self);
        break;
      case P2TR_MESH_ACTION_TRIANGLE:
        p2tr_mesh_action_triangle_free (self);
        break;
      default:
        g_assert_not_reached ();
        break;
    }
}

void
p2tr_mesh_action_unref (P2trMeshAction *self)
{
  g_assert (self->refcount > 0);
  if (--self->refcount == 0)
    p2tr_mesh_action_free (self);
}

void
p2tr_mesh_action_undo (P2trMeshAction *self,
                       P2trMesh       *mesh)
{
  switch (self->type)
    {
      case P2TR_MESH_ACTION_POINT:
        p2tr_mesh_action_point_undo (self, mesh);
        break;
      case P2TR_MESH_ACTION_EDGE:
        p2tr_mesh_action_edge_undo (self, mesh);
        break;
      case P2TR_MESH_ACTION_TRIANGLE:
        p2tr_mesh_action_triangle_undo (self, mesh);
        break;
      default:
        g_assert_not_reached ();
        break;
    }
}

