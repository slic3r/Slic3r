/*
 * This file is a part of the C port of the Poly2Tri library
 * Porting to C done by (c) Barak Itkin <lightningismyname@gmail.com>
 * http://code.google.com/p/poly2tri-c/
 *
 * Poly2Tri Copyright (c) 2009-2010, Poly2Tri Contributors
 * http://code.google.com/p/poly2tri/
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

#ifndef __P2TC_P2T_SWEEP_CONTEXT_H__
#define __P2TC_P2T_SWEEP_CONTEXT_H__

#include "../common/poly2tri-private.h"
#include "../common/shapes.h"
#include "advancing_front.h"

/* Inital triangle factor, seed triangle will extend 30% of
 * PointSet width to both left and right. */
#define kAlpha 0.3

struct P2tSweepContextBasin_
{
  P2tNode* left_node;
  P2tNode* bottom_node;
  P2tNode* right_node;
  double width;
  gboolean left_highest;
};

void p2t_sweepcontext_basin_init (P2tSweepContextBasin* THIS);
void p2t_sweepcontext_basin_clear (P2tSweepContextBasin* THIS);

struct P2tSweepContextEdgeEvent_
{
  P2tEdge* constrained_edge;
  gboolean right;
};

void p2t_sweepcontext_edgeevent_init (P2tSweepContextEdgeEvent* THIS);

struct SweepContext_
{
  P2tEdgePtrArray edge_list;

  P2tSweepContextBasin basin;
  P2tSweepContextEdgeEvent edge_event;

  P2tTrianglePtrArray triangles_;
  P2tTrianglePtrList map_;
  P2tPointPtrArray points_;

  /** Advancing front */
  P2tAdvancingFront* front_;
  /** head point used with advancing front */
  P2tPoint* head_;
  /** tail point used with advancing front */
  P2tPoint* tail_;

  P2tNode *af_head_, *af_middle_, *af_tail_;
};

/** Constructor */
void p2t_sweepcontext_init (P2tSweepContext* THIS, P2tPointPtrArray polyline);
P2tSweepContext* p2t_sweepcontext_new (P2tPointPtrArray polyline);

/** Destructor */
void p2t_sweepcontext_destroy (P2tSweepContext* THIS);
void p2t_sweepcontext_delete (P2tSweepContext* THIS);

void p2t_sweepcontext_set_head (P2tSweepContext *THIS, P2tPoint* p1);

P2tPoint* p2t_sweepcontext_head (P2tSweepContext *THIS);

void p2t_sweepcontext_set_tail (P2tSweepContext *THIS, P2tPoint* p1);

P2tPoint* p2t_sweepcontext_tail (P2tSweepContext *THIS);

int p2t_sweepcontext_point_count (P2tSweepContext *THIS);

P2tNode* p2t_sweepcontext_locate_node (P2tSweepContext *THIS, P2tPoint* point);

void p2t_sweepcontext_remove_node (P2tSweepContext *THIS, P2tNode* node);

void p2t_sweepcontext_create_advancingfront (P2tSweepContext *THIS, P2tNodePtrArray nodes);

/** Try to map a node to all sides of this triangle that don't have a neighbor */
void p2t_sweepcontext_map_triangle_to_nodes (P2tSweepContext *THIS, P2tTriangle* t);

void p2t_sweepcontext_add_to_map (P2tSweepContext *THIS, P2tTriangle* triangle);

P2tPoint* p2t_sweepcontext_get_point (P2tSweepContext *THIS, const int index);

P2tPoint* SweepContext_GetPoints (P2tSweepContext *THIS);

void p2t_sweepcontext_remove_from_map (P2tSweepContext *THIS, P2tTriangle* triangle);

void p2t_sweepcontext_add_hole (P2tSweepContext *THIS, P2tPointPtrArray polyline);

void p2t_sweepcontext_add_point (P2tSweepContext *THIS, P2tPoint* point);

P2tAdvancingFront* p2t_sweepcontext_front (P2tSweepContext *THIS);

void p2t_sweepcontext_mesh_clean (P2tSweepContext *THIS, P2tTriangle* triangle);

P2tTrianglePtrArray p2t_sweepcontext_get_triangles (P2tSweepContext *THIS);
P2tTrianglePtrList p2t_sweepcontext_get_map (P2tSweepContext *THIS);

void p2t_sweepcontext_init_triangulation (P2tSweepContext *THIS);
void p2t_sweepcontext_init_edges (P2tSweepContext *THIS, P2tPointPtrArray polyline);

#endif
