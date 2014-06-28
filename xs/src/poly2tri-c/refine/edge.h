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

#ifndef __P2TC_REFINE_EDGE_H__
#define __P2TC_REFINE_EDGE_H__

#include <glib.h>
#include "circle.h"
#include "triangulation.h"

/**
 * @struct P2trEdge_
 * A struct for an edge in a triangular mesh
 */
struct P2trEdge_
{
  /** The end point of this mesh */
  P2trPoint    *end;

  /** The edge going in the opposite direction from this edge */
  P2trEdge     *mirror;
  
  /** Is this a constrained edge? */
  gboolean      constrained;
  
  /** The triangle where this edge goes clockwise along its outline */
  P2trTriangle *tri;

  /**
   * The angle of the direction of this edge. Although it can be
   * computed anytime using atan2 on the vector of this edge, we cache
   * it here since it's heavily used and the computation is expensive.
   * The angle increases as we go CCW, and it's in the range [-PI,+PI]
   */
  gdouble       angle;
  
  /**
   * Is this edge a delaunay edge? This field is used by the refinement
   * algorithm and should not be used elsewhere!
   */
  gboolean      delaunay;

  /** A count of references to the edge */
  guint         refcount;
};

#define P2TR_EDGE_START(E) ((E)->mirror->end)

P2trEdge*   p2tr_edge_new                  (P2trPoint *start,
                                            P2trPoint *end,
                                            gboolean   constrained);

P2trEdge*   p2tr_edge_ref                  (P2trEdge *self);

void        p2tr_edge_unref                (P2trEdge *self);

void        p2tr_edge_free                 (P2trEdge *self);

void        p2tr_edge_remove               (P2trEdge *self);

P2trMesh*   p2tr_edge_get_mesh             (P2trEdge *self);

gboolean    p2tr_edge_is_removed           (P2trEdge *self);

gdouble     p2tr_edge_get_length           (P2trEdge* self);

gdouble     p2tr_edge_get_length_squared   (P2trEdge* self);

gdouble     p2tr_edge_angle_between        (P2trEdge *e1,
                                            P2trEdge *e2);

gdouble     p2tr_edge_angle_between_positive (P2trEdge *e1,
                                              P2trEdge *e2);

#endif
