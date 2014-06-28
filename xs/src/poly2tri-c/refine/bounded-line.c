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
#include "bounded-line.h"

P2trBoundedLine*
p2tr_bounded_line_new (const P2trVector2 *start,
                       const P2trVector2 *end)
{
  P2trBoundedLine* line = g_slice_new (P2trBoundedLine);
  p2tr_bounded_line_init (line, start, end);
  return line;
}

void
p2tr_bounded_line_init (P2trBoundedLine   *line,
                        const P2trVector2 *start,
                        const P2trVector2 *end)
{
  /* Traditional line Equation:
   * y - mx - n = 0   <==>   y = mx + n
   * Slope Equation:
   * m = dy / dx
   * Slope + Traditional:
   * dx * y - dy * x - dx * n = 0
   * And the remaining part can be found as
   * dx * y0 - dy * x0 = dx * n
   * So the final equation is:
   * dx * y - dy * x - (dx * y0 - dy * x0) = 0
   */
  gdouble dx = end->x - start->x;
  gdouble dy = end->y - start->y;

  gdouble dxXn = start->y * dx - start->x * dy;

  p2tr_line_init(&line->infinite, -dy, dx, -dxXn);

  p2tr_vector2_copy(&line->start, start);
  p2tr_vector2_copy(&line->end, end);
}

gboolean
p2tr_bounded_line_intersect (const P2trBoundedLine *l1,
                             const P2trBoundedLine *l2)
{
  return p2tr_line_different_sides (&l1->infinite, &l2->start, &l2->end)
    && p2tr_line_different_sides (&l2->infinite, &l1->start, &l1->end);
}

void
p2tr_bounded_line_free (P2trBoundedLine *line)
{
  g_slice_free (P2trBoundedLine, line);
}
