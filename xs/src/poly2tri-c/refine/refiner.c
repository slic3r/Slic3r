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
#include "cdt.h"
#include "delaunay-terminator.h"
#include "refiner.h"

#define P2TR_REFINER_REAL_TYPE P2trDelaunayTerminator

#define P2T_REFINER_TO_IMP(X) ((P2TR_REFINER_REAL_TYPE*)(X))
#define P2T_IMP_TO_REFINER(X) ((P2trRefiner*)(X))

gboolean
p2tr_refiner_false_too_big (P2trTriangle *tri)
{
  return FALSE;
}

P2trRefiner*
p2tr_refiner_new (gdouble             min_angle,
                  P2trTriangleTooBig  size_control,
                  P2trCDT            *cdt)
{
  return P2T_IMP_TO_REFINER (p2tr_dt_new (min_angle, size_control, cdt));
}

void
p2tr_refiner_free (P2trRefiner *self)
{
  p2tr_dt_free (P2T_REFINER_TO_IMP (self));
}

void
p2tr_refiner_refine (P2trRefiner             *self,
                     gint                     max_steps,
                     P2trRefineProgressNotify on_progress)
{
  p2tr_dt_refine (P2T_REFINER_TO_IMP (self), max_steps, on_progress);
}

