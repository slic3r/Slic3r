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

#ifndef __P2TC_P2T_SHAPES_H__
#define __P2TC_P2T_SHAPES_H__

#include <stddef.h>
#include <assert.h>
#include <math.h>
#include "poly2tri-private.h"
#include "cutils.h"

/**
 * P2tPoint:
 * @x: The x coordinate of the point
 * @y: The y coordinate of the point
 * @edge_list: The edges this point constitutes an upper ending point
 *
 * A struct to represent 2D points with double precision, and to keep track
 * of the edges this point constitutes an upper ending point
 */
struct _P2tPoint
{
  /*< public >*/
  P2tEdgePtrArray edge_list;
  double x, y;
};

/**
 * p2t_point_init_dd:
 * @THIS: The #P2tPoint to initialize
 * @x: The desired x coordinate of the point
 * @y: The desired y coordinate of the point
 *
 * A function to initialize a #P2tPoint struct with the given coordinates. The
 * struct must later be finalized by a call to #p2t_point_destroy
 */
void p2t_point_init_dd (P2tPoint* THIS, double x, double y);

/**
 * p2t_point_new_dd:
 * @x: The desired x coordinate of the point
 * @y: The desired y coordinate of the point
 *
 * A utility function to alloacte and initialize a #P2tPoint.
 * See #p2t_point_init_dd. Note that when finished using the point, it must be
 * freed by a call to #p2t_point_free and can not be freed like regular memory.
 *
 * Returns: The allocated and initialized point
 */
P2tPoint* p2t_point_new_dd (double x, double y);

/**
 * p2t_point_init:
 * @THIS: The #P2tPoint to initialize
 *
 * A function to initialize a #P2tPoint struct to (0,0). The struct must later
 * be finalized by a call to #p2t_point_destroy
 */
void p2t_point_init (P2tPoint* THIS);

/**
 * p2t_point_new:
 *
 * A utility function to alloacte and initialize a #P2tPoint.
 * See #p2t_point_init. Note that when finished using the point, it must be
 * freed by a call to #p2t_point_free and can not be freed like regular memory.
 */
P2tPoint* p2t_point_new ();

/**
 * p2t_point_destroy:
 * @THIS: The #P2tPoint whose resources should be freed
 *
 * This function will free all the resources allocated by a #P2tPoint, without
 * freeing the #P2tPoint pointed by @THIS
 */
void p2t_point_destroy (P2tPoint* THIS);

/**
 * p2t_point_free:
 * @THIS: The #P2tPoint to free
 *
 * This function will free all the resources allocated by a #P2tPoint, and will
 * also free the #P2tPoint pointed by @THIS
 */
void p2t_point_free (P2tPoint* THIS);

/**
 * P2tEdge:
 * @p: The top right point of the edge
 * @q: The bottom left point of the edge
 *
 * Represents a simple polygon's edge
 */
struct _P2tEdge
{
  P2tPoint *p, *q;
};

/**
 * p2t_edge_init:
 * @THIS: The #P2tEdge to initialize
 * @p1: One of the two points that form the edge
 * @p2: The other point of the two points that form the edge
 *
 * A function to initialize a #P2tEdge struct from the given points. The
 * struct must later be finalized by a call to #p2t_point_destroy.
 *
 * Warning: The points must be geometrically not-equal! This means that they
 * must differ by at least one of their coordinates. Otherwise, a runtime error
 * would be raised!
 */
void p2t_edge_init (P2tEdge* THIS, P2tPoint* p1, P2tPoint* p2);

/**
 * p2t_edge_new:
 *
 * A utility function to alloacte and initialize a #P2tEdge.
 * See #p2t_edge_init. Note that when finished using the point, it must be freed
 * by a call to #p2t_point_free and can not be freed like regular memory.
 *
 * Returns: The allocated and initialized edge
 */
P2tEdge* p2t_edge_new (P2tPoint* p1, P2tPoint* p2);

/**
 * p2t_edge_destroy:
 * @THIS: The #P2tEdge whose resources should be freed
 *
 * This function will free all the resources allocated by a #P2tEdge, without
 * freeing the #P2tPoint pointed by @THIS
 */
void p2t_edge_destroy (P2tEdge* THIS);

/**
 * p2t_edge_free:
 * @THIS: The #P2tEdge to free
 *
 * This function will free all the resources allocated by a #P2tEdge, and will
 * also free the #P2tEdge pointed by @THIS
 */
void p2t_edge_free (P2tEdge* THIS);


/* Triangle-based data structures are know to have better performance than quad-edge structures
 * See: J. Shewchuk, "Triangle: Engineering a 2D Quality Mesh Generator and Delaunay Triangulator"
 *      "Triangulations in CGAL"
 */

/**
 * P2tTriangle:
 * @constrained_edge: Flags to determine if an edge is a Constrained edge
 * @delaunay_edg: Flags to determine if an edge is a Delauney edge
 * @points_: Triangle points
 * @neighbors_: Neighbor list
 * @interior_: Has this triangle been marked as an interior triangle?
 *
 * A data structure for representing a triangle, while keeping information about
 * neighbor triangles, etc.
 */
struct _P2tTriangle
{
  /*< public >*/
  gboolean constrained_edge[3];
  gboolean delaunay_edge[3];

  /*< private >*/
  P2tPoint * points_[3];
  struct _P2tTriangle * neighbors_[3];
  gboolean interior_;
};

P2tTriangle* p2t_triangle_new (P2tPoint* a, P2tPoint* b, P2tPoint* c);
void p2t_triangle_init (P2tTriangle* THIS, P2tPoint* a, P2tPoint* b, P2tPoint* c);
P2tPoint* p2t_triangle_get_point (P2tTriangle* THIS, const int index);
P2tPoint* p2t_triangle_point_cw (P2tTriangle* THIS, P2tPoint* point);
P2tPoint* p2t_triangle_point_ccw (P2tTriangle* THIS, P2tPoint* point);
P2tPoint* p2t_triangle_opposite_point (P2tTriangle* THIS, P2tTriangle* t, P2tPoint* p);

P2tTriangle* p2t_triangle_get_neighbor (P2tTriangle* THIS, const int index);
void p2t_triangle_mark_neighbor_pt_pt_tr (P2tTriangle* THIS, P2tPoint* p1, P2tPoint* p2, P2tTriangle* t);
void p2t_triangle_mark_neighbor_tr (P2tTriangle* THIS, P2tTriangle *t);

void p2t_triangle_mark_constrained_edge_i (P2tTriangle* THIS, const int index);
void p2t_triangle_mark_constrained_edge_ed (P2tTriangle* THIS, P2tEdge* edge);
void p2t_triangle_mark_constrained_edge_pt_pt (P2tTriangle* THIS, P2tPoint* p, P2tPoint* q);

int p2t_triangle_index (P2tTriangle* THIS, const P2tPoint* p);
int p2t_triangle_edge_index (P2tTriangle* THIS, const P2tPoint* p1, const P2tPoint* p2);

P2tTriangle* p2t_triangle_neighbor_cw (P2tTriangle* THIS, P2tPoint* point);
P2tTriangle* p2t_triangle_neighbor_ccw (P2tTriangle* THIS, P2tPoint* point);
gboolean p2t_triangle_get_constrained_edge_ccw (P2tTriangle* THIS, P2tPoint* p);
gboolean p2t_triangle_get_constrained_edge_cw (P2tTriangle* THIS, P2tPoint* p);
void p2t_triangle_set_constrained_edge_ccw (P2tTriangle* THIS, P2tPoint* p, gboolean ce);
void p2t_triangle_set_constrained_edge_cw (P2tTriangle* THIS, P2tPoint* p, gboolean ce);
gboolean p2t_triangle_get_delunay_edge_ccw (P2tTriangle* THIS, P2tPoint* p);
gboolean p2t_triangle_get_delunay_edge_cw (P2tTriangle* THIS, P2tPoint* p);
void p2t_triangle_set_delunay_edge_ccw (P2tTriangle* THIS, P2tPoint* p, gboolean e);
void p2t_triangle_set_delunay_edge_cw (P2tTriangle* THIS, P2tPoint* p, gboolean e);

gboolean p2t_triangle_contains_pt (P2tTriangle* THIS, P2tPoint* p);
gboolean p2t_triangle_contains_ed (P2tTriangle* THIS, const P2tEdge* e);
gboolean p2t_triangle_contains_pt_pt (P2tTriangle* THIS, P2tPoint* p, P2tPoint* q);
void p2t_triangle_legalize_pt (P2tTriangle* THIS, P2tPoint* point);
void p2t_triangle_legalize_pt_pt (P2tTriangle* THIS, P2tPoint* opoint, P2tPoint* npoint);
/**
 * Clears all references to all other triangles and points
 */
void p2t_triangle_clear (P2tTriangle* THIS);
void p2t_triangle_clear_neighbor_tr (P2tTriangle* THIS, P2tTriangle *triangle);
void p2t_triangle_clear_neighbors (P2tTriangle* THIS);
void p2t_triangle_clear_delunay_edges (P2tTriangle* THIS);

gboolean p2t_triangle_is_interior (P2tTriangle* THIS);
void p2t_triangle_is_interior_b (P2tTriangle* THIS, gboolean b);

P2tTriangle* p2t_triangle_neighbor_across (P2tTriangle* THIS, P2tPoint* opoint);

void p2t_triangle_debug_print (P2tTriangle* THIS);

gint p2t_point_cmp (gconstpointer a, gconstpointer b);

/*  gboolean operator == (const Point& a, const Point& b); */
gboolean p2t_point_equals (const P2tPoint* a, const P2tPoint* b);

#endif


