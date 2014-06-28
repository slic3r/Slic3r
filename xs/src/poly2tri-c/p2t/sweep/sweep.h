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

/**
 * Sweep-line, Constrained Delauney Triangulation (CDT) See: Domiter, V. and
 * Zalik, B.(2008)'Sweep-line algorithm for constrained Delaunay triangulation',
 * International Journal of Geographical Information Science
 *
 * "FlipScan" Constrained Edge Algorithm invented by Thomas �hl�n, thahlen@gmail.com
 */

#ifndef __P2TC_P2T_SWEEP_H__
#define __P2TC_P2T_SWEEP_H__

#include "../common/poly2tri-private.h"
#include "../common/shapes.h"

struct Sweep_
{
/* private: */
P2tNodePtrArray nodes_;

};

void p2t_sweep_init (P2tSweep* THIS);
P2tSweep* p2t_sweep_new ();

/**
 * Destructor - clean up memory
 */
void p2t_sweep_destroy (P2tSweep* THIS);
void p2t_sweep_free (P2tSweep* THIS);

/**
 * Triangulate
 *
 * @param tcx
 */
void p2t_sweep_triangulate (P2tSweep *THIS, P2tSweepContext *tcx);

/**
 * Start sweeping the Y-sorted point set from bottom to top
 *
 * @param tcx
 */
void p2t_sweep_sweep_points (P2tSweep *THIS, P2tSweepContext *tcx);

/**
 * Find closes node to the left of the new point and
 * create a new triangle. If needed new holes and basins
 * will be filled to.
 *
 * @param tcx
 * @param point
 * @return
 */
P2tNode* p2t_sweep_point_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tPoint* point);

/**
 *
 *
 * @param tcx
 * @param edge
 * @param node
 */
void p2t_sweep_edge_event_ed_n (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node);

void p2t_sweep_edge_event_pt_pt_tr_pt (P2tSweep *THIS, P2tSweepContext *tcx, P2tPoint* ep, P2tPoint* eq, P2tTriangle* triangle, P2tPoint* point);

/**
 * Creates a new front triangle and legalize it
 *
 * @param tcx
 * @param point
 * @param node
 * @return
 */
P2tNode* p2t_sweep_new_front_triangle (P2tSweep *THIS, P2tSweepContext *tcx, P2tPoint* point, P2tNode* node);

/**
 * Adds a triangle to the advancing front to fill a hole.
 * @param tcx
 * @param node - middle node, that is the bottom of the hole
 */
void p2t_sweep_fill (P2tSweep *THIS, P2tSweepContext *tcx, P2tNode* node);

/**
 * Returns true if triangle was legalized
 */
gboolean p2t_sweep_legalize (P2tSweep *THIS, P2tSweepContext *tcx, P2tTriangle *t);

/**
 * <b>Requirement</b>:<br>
 * 1. a,b and c form a triangle.<br>
 * 2. a and d is know to be on opposite side of bc<br>
 * <pre>
 *                a
 *                +
 *               / \
 *              /   \
 *            b/     \c
 *            +-------+
 *           /    d    \
 *          /           \
 * </pre>
 * <b>Fact</b>: d has to be in area B to have a chance to be inside the circle formed by
 *  a,b and c<br>
 *  d is outside B if orient2d(a,b,d) or orient2d(c,a,d) is CW<br>
 *  This preknowledge gives us a way to optimize the incircle test
 * @param a - triangle point, opposite d
 * @param b - triangle point
 * @param c - triangle point
 * @param d - point opposite a
 * @return true if d is inside circle, false if on circle edge
 */
gboolean p2t_sweep_incircle (P2tSweep *THIS, P2tPoint* pa, P2tPoint* pb, P2tPoint* pc, P2tPoint* pd);

/**
 * Rotates a triangle pair one vertex CW
 *<pre>
 *       n2                    n2
 *  P +-----+             P +-----+
 *    | t  /|               |\  t |
 *    |   / |               | \   |
 *  n1|  /  |n3           n1|  \  |n3
 *    | /   |    after CW   |   \ |
 *    |/ oT |               | oT \|
 *    +-----+ oP            +-----+
 *       n4                    n4
 * </pre>
 */
void p2t_sweep_rotate_triangle_pair (P2tSweep *THIS, P2tTriangle *t, P2tPoint* p, P2tTriangle *ot, P2tPoint* op);

/**
 * Fills holes in the Advancing Front
 *
 *
 * @param tcx
 * @param n
 */
void p2t_sweep_fill_advancingfront (P2tSweep *THIS, P2tSweepContext *tcx, P2tNode* n);

/* Decision-making about when to Fill hole.
 * Contributed by ToolmakerSteve2 */
gboolean p2t_sweep_large_hole_dont_fill (P2tSweep* THIS, P2tNode* node);
gboolean p2t_sweep_angle_exceeds_90_degrees (P2tSweep* THIS, P2tPoint* origin, P2tPoint* pa, P2tPoint* pb);
gboolean p2t_sweep_angle_exceeds_plus_90_degrees_or_is_negative (P2tSweep* THIS, P2tPoint* origin, P2tPoint* pa, P2tPoint* pb);
gdouble  p2t_sweep_angle (P2tSweep* THIS, P2tPoint* origin, P2tPoint* pa, P2tPoint* pb);

/**
 *
 * @param node - middle node
 * @return the angle between 3 front nodes
 */
double p2t_sweep_hole_angle (P2tSweep *THIS, P2tNode* node);

/**
 * The basin angle is decided against the horizontal line [1,0]
 */
double p2t_sweep_basin_angle (P2tSweep *THIS, P2tNode* node);

/**
 * Fills a basin that has formed on the Advancing Front to the right
 * of given node.<br>
 * First we decide a left,bottom and right node that forms the
 * boundaries of the basin. Then we do a reqursive fill.
 *
 * @param tcx
 * @param node - starting node, this or next node will be left node
 */
void p2t_sweep_fill_basin (P2tSweep *THIS, P2tSweepContext *tcx, P2tNode* node);

/**
 * Recursive algorithm to fill a Basin with triangles
 *
 * @param tcx
 * @param node - bottom_node
 * @param cnt - counter used to alternate on even and odd numbers
 */
void p2t_sweep_fill_basin_req (P2tSweep *THIS, P2tSweepContext *tcx, P2tNode* node);

gboolean p2t_sweep_is_shallow (P2tSweep *THIS, P2tSweepContext *tcx, P2tNode* node);

gboolean p2t_sweep_is_edge_side_of_triangle (P2tSweep *THIS, P2tTriangle *triangle, P2tPoint* ep, P2tPoint* eq);

void p2t_sweep_fill_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node);

void p2t_sweep_fill_right_above_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node);

void p2t_sweep_fill_right_below_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node);

void p2t_sweep_fill_right_concave_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node);

void p2t_sweep_fill_right_convex_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node);

void p2t_sweep_fill_left_above_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node);

void p2t_sweep_fill_left_below_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node);

void p2t_sweep_fill_left_concave_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node);

void p2t_sweep_fill_left_convex_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tEdge* edge, P2tNode* node);

void p2t_sweep_flip_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tPoint* ep, P2tPoint* eq, P2tTriangle* t, P2tPoint* p);

/**
 * After a flip we have two triangles and know that only one will still be
 * intersecting the edge. So decide which to contiune with and legalize the other
 *
 * @param tcx
 * @param o - should be the result of an orient2d( eq, op, ep )
 * @param t - triangle 1
 * @param ot - triangle 2
 * @param p - a point shared by both triangles
 * @param op - another point shared by both triangles
 * @return returns the triangle still intersecting the edge
 */
P2tTriangle* p2t_sweep_next_flip_triangle (P2tSweep *THIS, P2tSweepContext *tcx, int o, P2tTriangle *t, P2tTriangle *ot, P2tPoint* p, P2tPoint* op);

/**
 * When we need to traverse from one triangle to the next we need
 * the point in current triangle that is the opposite point to the next
 * triangle.
 *
 * @param ep
 * @param eq
 * @param ot
 * @param op
 * @return
 */
P2tPoint* p2t_sweep_next_flip_point (P2tSweep *THIS, P2tPoint* ep, P2tPoint* eq, P2tTriangle *ot, P2tPoint* op);

/**
 * Scan part of the FlipScan algorithm<br>
 * When a triangle pair isn't flippable we will scan for the next
 * point that is inside the flip triangle scan area. When found
 * we generate a new flipEdgeEvent
 *
 * @param tcx
 * @param ep - last point on the edge we are traversing
 * @param eq - first point on the edge we are traversing
 * @param flipTriangle - the current triangle sharing the point eq with edge
 * @param t
 * @param p
 */
void p2t_sweep_flip_scan_edge_event (P2tSweep *THIS, P2tSweepContext *tcx, P2tPoint* ep, P2tPoint* eq, P2tTriangle *flip_triangle, P2tTriangle *t, P2tPoint* p);

void p2t_sweep_finalization_polygon (P2tSweep *THIS, P2tSweepContext *tcx);

#endif
