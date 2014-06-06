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
#include "vector2.h"

gdouble
p2tr_vector2_dot (const P2trVector2 *a,
                  const P2trVector2 *b)
{
  return P2TR_VECTOR2_DOT (a, b);
}

gboolean
p2tr_vector2_is_same (const P2trVector2 *a,
                      const P2trVector2 *b)
{
  if (a == NULL || b == NULL)
    return ! ((a == NULL) ^ (b == NULL));
  else
    return a->x == b->x && a->y == b->y;
}

void
p2tr_vector2_sub (const P2trVector2 *a,
                  const P2trVector2 *b,
                  P2trVector2       *dest)
{
  dest->x = a->x - b->x;
  dest->y = a->y - b->y;
}

void
p2tr_vector2_center (const P2trVector2 *a,
                     const P2trVector2 *b,
                     P2trVector2       *dest)
{
  dest->x = (a->x + b->x) * 0.5;
  dest->y = (a->y + b->y) * 0.5;
}

gdouble
p2tr_vector2_norm (const P2trVector2 *v)
{
  return sqrt (P2TR_VECTOR2_LEN_SQ (v));
}

void
p2tr_vector2_copy (P2trVector2       *dest,
                   const P2trVector2 *src)
{
  dest->x = src->x;
  dest->y = src->y;
}
