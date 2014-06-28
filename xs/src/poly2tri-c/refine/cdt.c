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

#include <stdarg.h>
#include <glib.h>

#include "point.h"
#include "edge.h"
#include "triangle.h"

#include "cdt.h"
#include "visibility.h"
#include "cdt-flipfix.h"

static gboolean  p2tr_cdt_visible_from_tri        (P2trCDT      *self,
                                                   P2trTriangle *tri,
                                                   P2trVector2  *p);

static gboolean  p2tr_cdt_has_empty_circum_circle (P2trCDT      *self,
                                                   P2trTriangle *tri);

static P2trHashSet* p2tr_cdt_triangulate_fan         (P2trCDT   *self,
                                                      P2trPoint *center,
                                                      GList     *edge_pts);

void
p2tr_cdt_validate_unused (P2trCDT* self)
{
  P2trEdge *ed;
  P2trTriangle *tri;
  P2trHashSetIter iter;

  p2tr_hash_set_iter_init (&iter, self->mesh->edges);
  while (p2tr_hash_set_iter_next (&iter, (gpointer*)&ed))
    {
      g_assert (ed->mirror != NULL);
      g_assert (! p2tr_edge_is_removed (ed));
    }

  p2tr_hash_set_iter_init (&iter, self->mesh->triangles);
  while (p2tr_hash_set_iter_next (&iter, (gpointer*)&tri))
    g_assert (! p2tr_triangle_is_removed (tri));
}

P2trCDT*
p2tr_cdt_new (P2tCDT *cdt)
{
  P2tTrianglePtrArray cdt_tris = p2t_cdt_get_triangles (cdt);
  GHashTable *point_map = g_hash_table_new (g_direct_hash, g_direct_equal);
  P2trCDT *rmesh = g_slice_new (P2trCDT);
  GHashTableIter iter;
  P2trPoint *pt_iter = NULL;

  P2trVEdgeSet *new_edges = p2tr_vedge_set_new ();

  gint i, j;

  rmesh->mesh = p2tr_mesh_new ();
  rmesh->outline = p2tr_pslg_new ();

  /* First iteration over the CDT - create all the points */
  for (i = 0; i < cdt_tris->len; i++)
  {
    P2tTriangle *cdt_tri = triangle_index (cdt_tris, i);
    for (j = 0; j < 3; j++)
      {
        P2tPoint *cdt_pt = p2t_triangle_get_point(cdt_tri, j);
        P2trPoint *new_pt = (P2trPoint*) g_hash_table_lookup (point_map, cdt_pt);

        if (new_pt == NULL)
          {
            new_pt = p2tr_mesh_new_point2 (rmesh->mesh, cdt_pt->x, cdt_pt->y);
            g_hash_table_insert (point_map, cdt_pt, new_pt);
          }
      }
  }

  /* Second iteration over the CDT - create all the edges and find the
   * outline */
  for (i = 0; i < cdt_tris->len; i++)
  {
    P2tTriangle *cdt_tri = triangle_index (cdt_tris, i);

    for (j = 0; j < 3; j++)
      {
        P2tPoint *start = p2t_triangle_get_point (cdt_tri, j);
        P2tPoint *end = p2t_triangle_get_point (cdt_tri, (j + 1) % 3);
        int edge_index = p2t_triangle_edge_index (cdt_tri, start, end);

        P2trPoint *start_new = (P2trPoint*) g_hash_table_lookup (point_map, start);
        P2trPoint *end_new = (P2trPoint*) g_hash_table_lookup (point_map, end);

        if (! p2tr_point_has_edge_to (start_new, end_new))
          {
            gboolean constrained = cdt_tri->constrained_edge[edge_index]
            || cdt_tri->neighbors_[edge_index] == NULL;
            P2trEdge *edge = p2tr_mesh_new_edge (rmesh->mesh, start_new, end_new, constrained);

            /* If the edge is constrained, we should add it to the
             * outline */
            if (constrained)
              p2tr_pslg_add_new_line(rmesh->outline, &start_new->c,
                  &end_new->c);

            /* We only wanted to create the edge now. We will use it
             * later */
            p2tr_vedge_set_add (new_edges, edge);
          }
      }
  }

  /* Third iteration over the CDT - create all the triangles */
  for (i = 0; i < cdt_tris->len; i++)
  {
    P2tTriangle *cdt_tri = triangle_index (cdt_tris, i);

    P2trPoint *pt1 = (P2trPoint*) g_hash_table_lookup (point_map, p2t_triangle_get_point (cdt_tri, 0));
    P2trPoint *pt2 = (P2trPoint*) g_hash_table_lookup (point_map, p2t_triangle_get_point (cdt_tri, 1));
    P2trPoint *pt3 = (P2trPoint*) g_hash_table_lookup (point_map, p2t_triangle_get_point (cdt_tri, 2));

    P2trTriangle *new_tri = p2tr_mesh_new_triangle (rmesh->mesh,
        p2tr_point_get_edge_to(pt1, pt2, FALSE),
        p2tr_point_get_edge_to(pt2, pt3, FALSE),
        p2tr_point_get_edge_to(pt3, pt1, FALSE));

    /* We won't do any usage of the triangle, so just unref it */
    p2tr_triangle_unref (new_tri);
  }

  /* And do an extra flip fix */
  p2tr_cdt_flip_fix (rmesh, new_edges);

  p2tr_vedge_set_free (new_edges);

  /* Now finally unref the points we added into the map */
  g_hash_table_iter_init (&iter, point_map);
  while (g_hash_table_iter_next (&iter, NULL, (gpointer*)&pt_iter))
    p2tr_point_unref (pt_iter);
  g_hash_table_destroy (point_map);

  return rmesh;
}

void
p2tr_cdt_free (P2trCDT *self)
{
  p2tr_cdt_free_full (self, TRUE);
}

void
p2tr_cdt_free_full (P2trCDT* self, gboolean clear_mesh)
{
  p2tr_pslg_free (self->outline);
  if (clear_mesh)
    p2tr_mesh_clear (self->mesh);
  p2tr_mesh_unref (self->mesh);

  g_slice_free (P2trCDT, self);
}

void
p2tr_cdt_validate_edges (P2trCDT *self)
{
  P2trHashSetIter iter;
  P2trEdge *e;

  p2tr_hash_set_iter_init (&iter, self->mesh->edges);
  while (p2tr_hash_set_iter_next (&iter, (gpointer*)&e))
    {
      if (! e->constrained && e->tri == NULL)
        p2tr_exception_geometric ("Found a non constrained edge without a triangle");

      if (e->tri != NULL)
        {
          gboolean found = FALSE;
          gint i = 0;

          for (i = 0; i < 3; i++)
            if (e->tri->edges[i] == e)
              {
                found = TRUE;
                break;
              }

          if (! found)
              p2tr_exception_geometric ("An edge has a triangle to which it does not belong!");
        }
    }
}

gboolean
p2tr_cdt_visible_from_edge (P2trCDT     *self,
                            P2trEdge    *e,
                            P2trVector2 *p)
{
  P2trBoundedLine line;

  p2tr_bounded_line_init (&line, &P2TR_EDGE_START(e)->c, &e->end->c);

  return p2tr_visibility_is_visible_from_edges (self->outline, p, &line, 1);
}

static gboolean
p2tr_cdt_visible_from_tri (P2trCDT      *self,
                           P2trTriangle *tri,
                           P2trVector2  *p)
{
  P2trBoundedLine lines[3];
  gint i;

  for (i = 0; i < 3; i++)
    p2tr_bounded_line_init (&lines[i],
        &P2TR_EDGE_START(tri->edges[i])->c,
        &tri->edges[i]->end->c);

  return p2tr_visibility_is_visible_from_edges (self->outline, p, lines, 3);
}

static gboolean
p2tr_cdt_has_empty_circum_circle (P2trCDT      *self,
                                  P2trTriangle *tri)
{
  P2trCircle circum;
  P2trPoint *p;
  P2trHashSetIter iter;

  p2tr_triangle_get_circum_circle (tri, &circum);

  p2tr_hash_set_iter_init (&iter, self->mesh->points);
  while (p2tr_hash_set_iter_next (&iter, (gpointer*)&p))
    {
      /** TODO: FIXME - is a point on a constrained edge really not a
       * problem?! */
      if (p2tr_point_has_constrained_edge (p)
          /* The points of a triangle can't violate its own empty
           * circumcircle property */
          || p == tri->edges[0]->end
          || p == tri->edges[1]->end
          || p == tri->edges[2]->end)
          continue;

      if (! p2tr_circle_test_point_outside(&circum, &p->c)
          && p2tr_cdt_visible_from_tri (self, tri, &p->c))
          return FALSE;
    }
  return TRUE;
}

void
p2tr_cdt_validate_cdt (P2trCDT *self)
{
  P2trHashSetIter iter;
  P2trTriangle *tri;

  p2tr_hash_set_iter_init (&iter, self->mesh->triangles);
  while (p2tr_hash_set_iter_next (&iter, (gpointer*)&tri))
    if (! p2tr_cdt_has_empty_circum_circle(self, tri))
      p2tr_exception_geometric ("Not a CDT!");
}

P2trPoint*
p2tr_cdt_insert_point (P2trCDT           *self,
                       const P2trVector2 *pc,
                       P2trTriangle      *point_location_guess)
{
  P2trTriangle *tri;
  P2trPoint    *pt;
  gboolean      inserted = FALSE;
  gint          i;

  P2TR_CDT_VALIDATE_UNUSED (self);

  if (point_location_guess == NULL)
    tri = p2tr_mesh_find_point (self->mesh, pc);
  else
    tri = p2tr_mesh_find_point_local (self->mesh, pc, point_location_guess);

  if (tri == NULL)
    p2tr_exception_geometric ("Tried to add point outside of domain!");

  pt = p2tr_mesh_new_point (self->mesh, pc);

  /* If the point falls on a line, we should split the line */
  for (i = 0; i < 3; i++)
    {
      P2trEdge *edge = tri->edges[i];
      if (p2tr_math_orient2d (& P2TR_EDGE_START(edge)->c,
              &edge->end->c, pc) == P2TR_ORIENTATION_LINEAR)
        {
          GList *parts = p2tr_cdt_split_edge (self, edge, pt), *eIter;
          for (eIter = parts; eIter != NULL; eIter = eIter->next)
            p2tr_edge_unref ((P2trEdge*)eIter->data);
          g_list_free(parts);

          inserted = TRUE;
          break;
        }
    }

  if (! inserted)
    /* If we reached this line, then the point is inside the triangle */
    p2tr_cdt_insert_point_into_triangle (self, pt, tri);

  /* We no longer need the triangle */
  p2tr_triangle_unref (tri);

  P2TR_CDT_VALIDATE_UNUSED (self);
  return pt;
}

/** Insert a point into a triangle. This function assumes the point is
 * inside the triangle - not on one of its edges and not outside of it.
 */
void
p2tr_cdt_insert_point_into_triangle (P2trCDT      *self,
                                     P2trPoint    *P,
                                     P2trTriangle *tri)
{
  P2trVEdgeSet *flip_candidates = p2tr_vedge_set_new ();

  P2trPoint *A = tri->edges[0]->end;
  P2trPoint *B = tri->edges[1]->end;
  P2trPoint *C = tri->edges[2]->end;

  P2trEdge *CA = tri->edges[0];
  P2trEdge *AB = tri->edges[1];
  P2trEdge *BC = tri->edges[2];

  P2trEdge *AP, *BP, *CP;

  p2tr_triangle_remove (tri);

  AP = p2tr_mesh_new_edge (self->mesh, A, P, FALSE);
  BP = p2tr_mesh_new_edge (self->mesh, B, P, FALSE);
  CP = p2tr_mesh_new_edge (self->mesh, C, P, FALSE);

  p2tr_triangle_unref (p2tr_mesh_new_triangle (self->mesh, AB, BP, AP->mirror));
  p2tr_triangle_unref (p2tr_mesh_new_triangle (self->mesh, BC, CP, BP->mirror));
  p2tr_triangle_unref (p2tr_mesh_new_triangle (self->mesh, CA, AP, CP->mirror));

  p2tr_vedge_set_add (flip_candidates, CP);
  p2tr_vedge_set_add (flip_candidates, AP);
  p2tr_vedge_set_add (flip_candidates, BP);

  p2tr_vedge_set_add (flip_candidates, p2tr_edge_ref (CA));
  p2tr_vedge_set_add (flip_candidates, p2tr_edge_ref (AB));
  p2tr_vedge_set_add (flip_candidates, p2tr_edge_ref (BC));

  /* Flip fix the newly created triangles to preserve the the
   * constrained delaunay property. The flip-fix function will unref the
   * new triangles for us! */
  p2tr_cdt_flip_fix (self, flip_candidates);

  p2tr_vedge_set_free (flip_candidates);
}

/**
 * Triangulate a polygon by creating edges to a center point.
 * 1. If there is a NULL point in the polygon, two triangles are not
 *    created (these are the two that would have used it)
 * 2. THE RETURNED EDGES MUST BE UNREFFED!
 */
static P2trVEdgeSet*
p2tr_cdt_triangulate_fan (P2trCDT   *self,
                          P2trPoint *center,
                          GList     *edge_pts)
{
  P2trVEdgeSet* fan_edges = p2tr_vedge_set_new ();
  GList *iter;

  /* We can not triangulate unless at least two points are given */
  if (edge_pts == NULL || edge_pts->next == NULL)
    {
      p2tr_exception_programmatic ("Not enough points to triangulate as"
          " a star!");
    }

  for (iter = edge_pts; iter != NULL; iter = iter->next)
    {
      P2trPoint *A = (P2trPoint*) iter->data;
      P2trPoint *B = (P2trPoint*) g_list_cyclic_next (edge_pts, iter)->data;
      P2trEdge *AB, *BC, *CA;

      if (A == NULL || B == NULL)
        continue;

      AB = p2tr_point_get_edge_to (A, B, TRUE);
      BC = p2tr_mesh_new_or_existing_edge (self->mesh, B, center, FALSE);
      CA = p2tr_mesh_new_or_existing_edge (self->mesh, center, A, FALSE);

      p2tr_triangle_unref (p2tr_mesh_new_triangle (self->mesh, AB, BC, CA));

      p2tr_vedge_set_add (fan_edges, CA);
      p2tr_vedge_set_add (fan_edges, BC);
      p2tr_vedge_set_add (fan_edges, AB);
    }

  return fan_edges;
}

GList*
p2tr_cdt_split_edge (P2trCDT   *self,
                     P2trEdge  *e,
                     P2trPoint *C)
{
  /*      W
   *     /|\
   *    / | \
   *   /  |  \      E.Mirror.Tri: YXW
   * X*---*---*Y    E: X->Y
   *   \  |C /      E.Tri: XYV
   *    \ | /
   *     \|/
   *      V
   */
  P2trPoint *X = P2TR_EDGE_START (e), *Y = e->end;
  P2trPoint *V = (e->tri != NULL) ? p2tr_triangle_get_opposite_point(e->tri, e, FALSE) : NULL;
  P2trPoint *W = (e->mirror->tri != NULL) ? p2tr_triangle_get_opposite_point (e->mirror->tri, e->mirror, FALSE) : NULL;
  gboolean   constrained = e->constrained;
  P2trEdge  *XC, *CY;
  GList     *fan = NULL, *new_edges = NULL;
  P2trHashSet *fan_edges;

  P2TR_CDT_VALIDATE_UNUSED (self);

  p2tr_edge_remove (e);

  XC = p2tr_mesh_new_edge (self->mesh, X, C, constrained);
  CY = p2tr_mesh_new_edge (self->mesh, C, Y, constrained);

  fan = p2tr_utils_new_reversed_pointer_list (4, W, X, V, Y);
  fan_edges = p2tr_cdt_triangulate_fan (self, C, fan);
  g_list_free (fan);

  /* Now make this a CDT again
   * The new triangles will be unreffed by the flip_fix function, which
   * is good since we receive them with an extra reference!
   */
  p2tr_cdt_flip_fix (self, fan_edges);
  p2tr_hash_set_free (fan_edges);

  if (constrained)
    {
      /* If this was a subsegment, then both parts of the subsegment
       * should exist */
      if (p2tr_edge_is_removed (XC) || p2tr_edge_is_removed (CY))
        p2tr_exception_geometric ("Subsegments gone!");
      else
        {
          new_edges = g_list_prepend (new_edges, CY);
          new_edges = g_list_prepend (new_edges, XC);
        }
    }
  else
    {
      p2tr_edge_unref (XC);
      p2tr_edge_unref (CY);
    }

  P2TR_CDT_VALIDATE_UNUSED (self);

  return new_edges;
}

