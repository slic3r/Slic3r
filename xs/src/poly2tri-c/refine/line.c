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
#include "line.h"

void
p2tr_line_init (P2trLine    *line,
                gdouble      a,
                gdouble      b,
                gdouble      c)
{
  line->a = a;
  line->b = b;
  line->c = c;
}

gboolean
p2tr_line_different_sides (const P2trLine    *line,
                           const P2trVector2 *pt1,
                           const P2trVector2 *pt2)
{
  gdouble side1 = line->a * pt1->x + line->b * pt1->y + line->c;
  gdouble side2 = line->a * pt2->x + line->b * pt2->y + line->c;

  /* Signs are different if the product is negative */
  return side1 * side2 < 0;
}

P2trLineRelation
p2tr_line_intersection (const P2trLine    *l1,
                        const P2trLine    *l2,
                        P2trVector2       *out_intersection)
{
  /* In order to find the intersection, we intend to solve
   * the following set of equations:
   *
   *   ( A1 B1 ) ( x ) = ( -C1 )
   *   ( A2 B2 ) ( y ) = ( -C2 )
   *
   * We can simplify the solution using Cramers Rule which
   * gives the following results:
   *
   *   x = (-C1 * B2) - (-C2 * B1) / (A1 * B2 - A2 * B1)
   *   y = (A1 * -C2) - (A2 * -C1) / (A1 * B2 - A2 * B1)
   */
  double d = l1->a * l2->b - l2->a * l1->b;

  /* If the denominator in the result of applying Cramers rule
   * is zero, then the lines have exactly the same slope, meaning
   * they are either exactly the same or they are parallel and
   * never intersect */
  if (d == 0)
    {
      /* We want to check if the offsets of boths the lines are the
       * same, i.e. whether:  C1 / A1 = C2 / A2
       * This test can be done without zero division errors if we do
       * it in like this:     C1 * A2 = C2 * A1
       */
      if (l1->c * l2->a == l1->a * l2->c)
        return P2TR_LINE_RELATION_SAME;
      else
        return P2TR_LINE_RELATION_PARALLEL;
    }

  if (out_intersection != NULL)
    {
      out_intersection->x = (-l1->c * l2->b + l2->c * l1->b) / d;
      out_intersection->y = (l1->a * -l2->c + l2->a * l1->c) / d;
    }

  return P2TR_LINE_RELATION_INTERSECTING;
}
