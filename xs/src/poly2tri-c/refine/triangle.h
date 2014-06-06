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

#ifndef __P2TC_REFINE_TRIANGLE_H__
#define __P2TC_REFINE_TRIANGLE_H__

#include <glib.h>
#include "rmath.h"
#include "triangulation.h"

/**
 * @struct P2trTriangle_
 * A struct for a triangle in a triangular mesh
 */
struct P2trTriangle_
{
  P2trEdge* edges[3];
  
  guint refcount;
};

P2trTriangle*   p2tr_triangle_new            (P2trEdge *AB,
                                              P2trEdge *BC,
                                              P2trEdge *CA);

P2trTriangle* p2tr_triangle_ref              (P2trTriangle *self);

void        p2tr_triangle_unref              (P2trTriangle *self);

void        p2tr_triangle_free               (P2trTriangle *self);

void        p2tr_triangle_remove             (P2trTriangle *self);

P2trMesh*   p2tr_triangle_get_mesh           (P2trTriangle *self);

gboolean    p2tr_triangle_is_removed         (P2trTriangle *self);

P2trPoint*  p2tr_triangle_get_opposite_point (P2trTriangle *self,
                                              P2trEdge     *e,
                                              gboolean      do_ref);

P2trEdge*   p2tr_triangle_get_opposite_edge  (P2trTriangle *self,
                                              P2trPoint    *p);

gdouble     p2tr_triangle_get_angle_at       (P2trTriangle *self,
                                              P2trPoint    *p);

gdouble     p2tr_triangle_smallest_non_constrained_angle (P2trTriangle *self);

void        p2tr_triangle_get_circum_circle (P2trTriangle *self,
                                             P2trCircle   *circle);

P2trInCircle p2tr_triangle_circumcircle_contains_point (P2trTriangle       *self,
                                                        const P2trVector2  *pt);

P2trInTriangle p2tr_triangle_contains_point  (P2trTriangle      *self,
                                              const P2trVector2 *pt);

P2trInTriangle p2tr_triangle_contains_point2 (P2trTriangle      *self,
                                              const P2trVector2 *pt,
                                              gdouble           *u,
                                              gdouble           *v);

#define P2TR_TRIANGLE_GET_POINT(tr,index) ((tr)->edges[((index)+3-1)%3]->end)
#endif
