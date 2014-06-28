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

#ifndef __P2TC_REFINE_POINT_H__
#define __P2TC_REFINE_POINT_H__

#include <glib.h>
#include "vector2.h"
#include "triangulation.h"

/**
 * @struct P2trPoint_
 * A struct for a point in a triangular mesh
 */
struct P2trPoint_
{
  /** The 2D coordinates of the point */
  P2trVector2  c;

  /**
   * A list of edges (@ref P2trEdge) which go out of this point (i.e.
   * the point is their start point). The edges are sorted by ASCENDING
   * angle, meaning they are sorted Counter Clockwise */
  GList       *outgoing_edges;

  /** A count of references to the point */
  guint        refcount;
  
  /** The triangular mesh containing this point */
  P2trMesh    *mesh;
};

P2trPoint*  p2tr_point_new                  (const P2trVector2 *c);

P2trPoint*  p2tr_point_new2                 (gdouble x, gdouble y);

P2trPoint*  p2tr_point_ref                  (P2trPoint *self);

void        p2tr_point_unref                (P2trPoint *self);

void        p2tr_point_free                 (P2trPoint *self);

void        p2tr_point_remove               (P2trPoint *self);

P2trEdge*   p2tr_point_has_edge_to          (P2trPoint *start,
                                             P2trPoint *end);

P2trEdge*   p2tr_point_get_edge_to          (P2trPoint *start,
                                             P2trPoint *end,
                                             gboolean   do_ref);

void        _p2tr_point_insert_edge         (P2trPoint *self,
                                             P2trEdge  *e);

void        _p2tr_point_remove_edge         (P2trPoint *self,
                                             P2trEdge  *e);

P2trEdge*   p2tr_point_edge_ccw             (P2trPoint *self,
                                             P2trEdge  *e);

P2trEdge*   p2tr_point_edge_cw              (P2trPoint *self,
                                             P2trEdge  *e);

gboolean    p2tr_point_is_fully_in_domain   (P2trPoint *self);

gboolean    p2tr_point_has_constrained_edge (P2trPoint *self);

P2trMesh*   p2tr_point_get_mesh             (P2trPoint *self);

#endif
