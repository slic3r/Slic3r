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

#ifndef __P2TC_REFINE_DELAUNAY_TERMINATOR_H__
#define __P2TC_REFINE_DELAUNAY_TERMINATOR_H__

#include <glib.h>
#include "cdt.h"
#include "refiner.h"
#include "vedge.h"

typedef struct
{
  P2trCDT            *cdt;
  GQueue              Qs;
  GSequence          *Qt;
  gdouble             theta;
  P2trTriangleTooBig  delta;
} P2trDelaunayTerminator;

gboolean  p2tr_cdt_test_encroachment_ignore_visibility (const P2trVector2 *w,
                                                        P2trEdge          *e);

gboolean  p2tr_cdt_is_encroached_by (P2trCDT     *self,
                                     P2trEdge    *e,
                                     P2trVector2 *p);

P2trVEdgeSet*  p2tr_cdt_get_segments_encroached_by (P2trCDT   *self,
                                                    P2trPoint *v);

gboolean      p2tr_cdt_is_encroached (P2trEdge *E);

P2trDelaunayTerminator*
p2tr_dt_new (gdouble theta, P2trTriangleTooBig delta, P2trCDT *cdt);

void p2tr_dt_free (P2trDelaunayTerminator *self);

void p2tr_dt_refine (P2trDelaunayTerminator   *self,
                     gint                      max_steps,
                     P2trRefineProgressNotify  on_progress);

#endif
