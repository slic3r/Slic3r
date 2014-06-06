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

/* Given a polygon "Poly" (a list of bounded lines), a point "P" and a
 * a planar straight line graph (pslg) "PSLG", the question is if
 * there is a straight line from "P" to some point in/on "Poly" so that
 * the line does not intersect any of the lines of "PSLG".
 *
 * In this file is an algorithm for answering the above question, and
 * it's pseudo-code is hereby presented:
 *
 * IsVisible(G, Poly, P):
 * ----------------------
 * { G = (V,E) - The PSLG where E is the set of edges in the graph   }
 * { Poly      - A polygon (a list of edges)                         }
 * { P         - The point we are checking whether is "visible" from }
 * {             Poly                                                }
 *
 * W <- Some point in T (for example, center of weight)
 *
 * # A set of edges from @PSLG known to intersect some potential lines
 * # from @P to @Poly
 * KnownBlocks <- {}
 *
 * # A set of points on @Poly which should be checked whether a line
 * # from them to @P is not obstructed by any of the edges of @PSLG
 * SecondPoint <- {W}
 *
 * WHILE SecondPoint is not empty:
 *
 *   # Pick some point to check
 *   S <- Some point from SecondPoint
 *   SecondPoint <- SecondPoint \ {S}
 *
 *   PS <- The infinite line going through P and S
 *
 *   IF PS intersects @Poly:
 *
 *     IF there is an edge B=(u,v) (from E) that intersects PS so that
 *        the intersection is between @Poly and @P:
 *
 *       IF B is not in KnownBlocks:
 *         # Try new lines going through the end points of this edge
 *         SecondPoint <- SecondPoint + {u,v}
 *         KnownBlocks <- KnownBlocks + {B}
 *
 *       ELSE:
 *         # Nothing to do - we already tried/are trying to handle this
 *         # blocking edge by going around it
 *       
 *     ELSE:
 *       RETURN "Visible"
 *
 *   ELSE:
 *     # PS doesn't help anyway, no matter if it's blocked or not,
 *     # since it does not reach the polygon
 *
 * RETURN "Ocluded"
 */

#include <glib.h>
#include "bounded-line.h"
#include "pslg.h"


static gboolean
find_closest_intersection (P2trPSLG    *pslg,
                           P2trLine    *line,
                           P2trVector2 *close_to,
                           P2trVector2 *dest)
{
    gdouble distance_sq = G_MAXDOUBLE;
    gboolean found = FALSE;

    const P2trBoundedLine *pslg_line = NULL;
    P2trPSLGIter pslg_iter;
    P2trVector2 temp;

    p2tr_pslg_iter_init (&pslg_iter, pslg);

    while (p2tr_pslg_iter_next (&pslg_iter, &pslg_line))
    {
        if (p2tr_line_intersection (&pslg_line->infinite, line, &temp)
            == P2TR_LINE_RELATION_INTERSECTING)
        {
            gdouble test_distance_sq = P2TR_VECTOR2_DISTANCE_SQ (&temp, close_to);

            found = TRUE;

            if (test_distance_sq < distance_sq)
            {
                distance_sq = test_distance_sq;
            }
        }
    }

    if (found && dest != NULL)
        p2tr_vector2_copy (dest, &temp);

    return found;
}

static void
find_point_in_polygon (P2trPSLG    *polygon,
                       P2trVector2 *out_point)
{
    P2trPSLGIter iter;
    const P2trBoundedLine *line = NULL;

    g_assert (p2tr_pslg_size (polygon) >= 1);

    p2tr_pslg_iter_init (&iter, polygon);
    p2tr_pslg_iter_next (&iter, &line);

    out_point->x = (line->start.x + line->end.x) / 2;
    out_point->y = (line->start.y + line->end.y) / 2;
}

#ifdef EXPERIMENTAL_VISIBILITY
static P2trBoundedLine*
pslg_line_intersection (P2trPSLG        *pslg,
                        P2trBoundedLine *line)
{
    P2trPSLGIter iter;
    P2trBoundedLine *pslg_line = NULL;

    p2tr_pslg_iter_init (&iter, pslg);
    while (p2tr_pslg_iter_next (&iter, &pslg_line))
        if (p2tr_bounded_line_intersect (line, pslg_line))
            return pslg_line;

    return NULL;
}

gboolean
p2tr_pslg_visibility_check (P2trPSLG    *pslg,
                            P2trVector2 *point,
                            P2trPSLG    *polygon)
{
    P2trPSLG *known_blocks;
    GArray   *second_points;
    gboolean  found_visibility_path = FALSE;

    /* W <- Some point in T (for example, center of weight) */
    P2trVector2 W;
    find_point_in_polygon (polygon, &W);

    /* KnownBlocks <- {} */
    known_blocks = p2tr_pslg_new ();

    /* SecondPoint <- {W} */
    second_points   = g_array_new (FALSE, FALSE, sizeof(P2trVector2));
    g_array_append_val (second_points, W);

    while ((! found_visibility_path) && second_points->len > 0)
    {
        P2trVector2 S;
        P2trBoundedLine PS;
        P2trVector2 poly_intersection;

        /* S <- Some point from SecondPoint */
        p2tr_vector2_copy (&S, &g_array_index(second_points, P2trVector2, 0));
        /* SecondPoint <- SecondPoint \ {S} */
        g_array_remove_index_fast (second_points, 0);

        /* PS <- The infinite line going through P and S */
        p2tr_bounded_line_init (&PS, &S, point);

        /* IF PS intersects @Poly */
        if (find_closest_intersection (polygon, &PS.infinite, point, &poly_intersection))
        {
            P2trBoundedLine PS_exact, *B;

            /* IF there is an edge B=(u,v) (from E) that intersects PS */
            p2tr_bounded_line_init (&PS_exact, point, &poly_intersection);
            B = pslg_line_intersection (pslg, &PS_exact);

            if (B != NULL)
            {
                /* IF B is not in KnownBlocks: */
                if (! p2tr_pslg_contains_line (known_blocks, B))
                {
                    /* SecondPoint <- SecondPoint + {u,v} */
                    g_array_append_val (second_points, B->start);
                    g_array_append_val (second_points, B->end);
                    /* KnownBlocks <- KnownBlocks + {B} */
                    p2tr_pslg_add_existing_line (known_blocks, B);
                }
            }
            else
            {
                found_visibility_path = TRUE;
            }
        }
    }

    g_array_free (second_points, TRUE);
    p2tr_pslg_free (known_blocks);

    return found_visibility_path;
}
#else

/* Refer to the "Ray casting algorithm":
 *   http://en.wikipedia.org/wiki/Point_in_polygon#Ray_casting_algorithm
 */
static gboolean
PointIsInsidePolygon (P2trVector2 *vec,
		      P2trPSLG    *polygon)
{
  P2trHashSetIter iter;
  const P2trBoundedLine *polyline = NULL;
  int count = 0;

  /* So, if we work on the X axis, what we want to check is how many
   * segments of the polygon cross the line (-inf,y)->(x,y) */
  p2tr_pslg_iter_init (&iter, polygon);
  while (p2tr_pslg_iter_next (&iter, &polyline))
    {
      if ((polyline->start.y - vec->y) * (polyline->end.y - vec->y) >= 0)
        continue; /* The line doesn't cross the horizontal line Y = vec->y */
      if (MIN (polyline->start.x, polyline->end.x) <= vec->x)
        count++;
    }
  return (count % 2) == 1;
}

/* If the bounded line intersects the polygon in more than two places,
 * then the answer is simply NO - the line is not completly outside the
 * polygon.
 * Else, if the bounded line intersects the polygon twice, we analyze
 * its end points and middle point:
 * - If both end-points are inside/on:
 *   - Test the midpoint - it it's inside then the line goes inside the
 *     polygon, otherwise it goes outside (remember, we said it
 *     intersects the polygon exactly twice!)
 * - Otherwise, there must be a subsegment of the poly-line which is
 *   partially inside!
 * Else, if it intersects exactly once:
 * - If both end-points are inside/on then it's partially inside.
 * - If exactly one end-points is inside/on then decide by the middle
 *   point (if it's partially inside then so is the line)
 * - If both end-points are outside - CAN'T HAPPEN WITH EXACTLY ONE INTERSECTION!
 * Else, if it does not intersect:
 * - Test any point inside the bounded line (end-points, middle points,
 *   etc.). If it's inside then the bounded line is inside. Otherwise
 *   it is outside
 */
static gboolean
LineIsOutsidePolygon (P2trBoundedLine *line,
                      P2trPSLG        *polygon)
{
  P2trHashSetIter iter;
  const P2trBoundedLine *polyline = NULL;
  P2trVector2 middle;
  gint intersection_count = 0, inside_count = 0;

  p2tr_pslg_iter_init (&iter, polygon);
  while (p2tr_pslg_iter_next (&iter, &polyline))
    {
      if (p2tr_bounded_line_intersect (polyline, line))
        if (++intersection_count > 2)
          return FALSE;
    }

  inside_count += (PointIsInsidePolygon (&line->start, polygon) ? 1 : 0); 
  inside_count += (PointIsInsidePolygon (&line->end, polygon) ? 1 : 0); 
  
  /* To decrease the chance of numeric errors when working on the edge points of
   * the line, the point we will test is the middle of the input line */
  middle.x = (line->start.x + line->end.x) / 2;
  middle.y = (line->start.y + line->end.y) / 2;

  if (intersection_count == 2)
    {
      if (inside_count == 2) return PointIsInsidePolygon (&middle, polygon);
      else return TRUE;
    }
  else if (intersection_count == 1)
    {
      if (inside_count == 2) return TRUE;
      else return PointIsInsidePolygon (&middle, polygon);
    }
  else return inside_count > 0;
}

static gboolean
TryVisibilityAroundBlock(P2trPSLG        *PSLG,
                         P2trVector2     *P,
                         P2trPSLG        *ToSee,
                         P2trPSLG        *KnownBlocks,
                         GQueue          *BlocksForTest,
                         /* Try on the edges of this block */
                         const P2trBoundedLine *BlockBeingTested,
                         const P2trVector2     *SideOfBlock)
{
  const P2trVector2 *S = SideOfBlock;
  P2trVector2 ClosestIntersection;
  P2trBoundedLine PS;
  
  p2tr_bounded_line_init (&PS, P, S);

  if (find_closest_intersection (ToSee, &PS.infinite, P, &ClosestIntersection))
    {
      P2trPSLGIter iter;
      P2trBoundedLine PK;
      const P2trBoundedLine *Segment = NULL;
      p2tr_bounded_line_init (&PK, P, &ClosestIntersection);

      /* Now we must make sure that the bounded line PK is inside
       * the polygon, because otherwise it is not considered as a
       * valid visibility path */

      p2tr_pslg_iter_init (&iter, PSLG);
      while (p2tr_pslg_iter_next (&iter, &Segment))
        {
          if (Segment == BlockBeingTested)
              continue;

          /* If we have two segments with a shared point,
           * the point should not be blocked by any of them
           */
          if (p2tr_vector2_is_same (SideOfBlock, &(Segment->start))
              || p2tr_vector2_is_same (SideOfBlock, &(Segment->end)))
              continue;

          if (p2tr_bounded_line_intersect (Segment, &PK))
            {
              if (g_queue_find (BlocksForTest, Segment))
                {
                  g_queue_push_tail (BlocksForTest, (P2trBoundedLine*)Segment);
                }
              /* obstruction found! */
              return FALSE;
            }
        }

      if (LineIsOutsidePolygon (&PK, PSLG))
        return FALSE;

      /* No obstruction! */
      return TRUE;
    }
  /* No intersection for this attempt, continue */
  return FALSE;
}

/**
 * Check if a point is "visible" from any one or more of the edges in a
 * given group.
 * Formally: Check if there is a line from @ref P to any of the edges in
 * @ref Edges so that the line does not cross any of the lines of the
 * PSLG @ref PSLG
 */
gboolean
IsVisibleFromEdges (P2trPSLG    *PSLG,
                   P2trVector2 *P,
                   P2trPSLG    *Edges)
{
    gboolean  found_visibility_path = FALSE;
    P2trPSLG *KnownBlocks = p2tr_pslg_new ();
    GQueue   *BlocksForTest = g_queue_new ();

    P2trVector2 W;
    find_point_in_polygon (Edges, &W);

    if (TryVisibilityAroundBlock(PSLG, P, Edges, KnownBlocks, BlocksForTest, NULL, &W))
        found_visibility_path = TRUE;

    while (! g_queue_is_empty (BlocksForTest) && ! found_visibility_path)
      {
        const P2trBoundedLine *Block = (P2trBoundedLine*)g_queue_pop_head (BlocksForTest);

        if (p2tr_pslg_contains_line (KnownBlocks, Block))
            continue;
        else if (TryVisibilityAroundBlock(PSLG, P, Edges, KnownBlocks, BlocksForTest, Block, &Block->start)
            || TryVisibilityAroundBlock(PSLG, P, Edges, KnownBlocks, BlocksForTest, Block, &Block->end))
          {
            found_visibility_path = TRUE;
          }
        else
            p2tr_pslg_add_existing_line (KnownBlocks, Block);
      }

    p2tr_pslg_free (KnownBlocks);
    g_queue_free (BlocksForTest);

    return found_visibility_path;
}
#endif

gboolean
p2tr_visibility_is_visible_from_edges (P2trPSLG              *pslg,
                                       P2trVector2           *p,
                                       const P2trBoundedLine *lines,
                                       guint                  line_count)
{
  P2trPSLG *edges = p2tr_pslg_new ();
  gint i;
  gboolean result;
  
  for (i = 0; i < line_count; i++)
    p2tr_pslg_add_existing_line (edges, &lines[i]);
  
  result = IsVisibleFromEdges (pslg, p, edges);
  
  p2tr_pslg_free (edges);
  return result;
}
