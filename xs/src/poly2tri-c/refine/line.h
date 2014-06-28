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

#ifndef __P2TC_REFINE_LINE_H__
#define __P2TC_REFINE_LINE_H__

#include <glib.h>
#include "vector2.h"

/* A line is the equation of the following form:
 *   a * X + b * Y + c = 0
 */
typedef struct {
  gdouble a, b, c;
} P2trLine;

typedef enum
{
  P2TR_LINE_RELATION_INTERSECTING = 0,
  P2TR_LINE_RELATION_PARALLEL = 1,
  P2TR_LINE_RELATION_SAME = 2
} P2trLineRelation;

void              p2tr_line_init            (P2trLine    *line,
                                             gdouble      a,
                                             gdouble      b,
                                             gdouble      c);

gboolean          p2tr_line_different_sides (const P2trLine    *line,
                                             const P2trVector2 *pt1,
                                             const P2trVector2 *pt2);

P2trLineRelation  p2tr_line_intersection    (const P2trLine    *l1,
                                             const P2trLine    *l2,
                                             P2trVector2       *out_intersection);

#endif
