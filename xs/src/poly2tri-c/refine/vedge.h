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

#ifndef __P2TC_REFINE_VEDGE_H__
#define __P2TC_REFINE_VEDGE_H__

#include <glib.h>
#include "utils.h"
#include "triangulation.h"

/**
 * @struct P2trVEdge_
 * A struct for representing a potential ("virtual") edge in a
 * triangular mesh
 */
struct P2trVEdge_
{
  /** The start point of this virtual edge */
  P2trPoint *start;
  /** The end point of this virtual edge */
  P2trPoint *end;
  /** A flag to make whether this is a constrained edge */
  gboolean   constrained;
  /** A count of references to the virtual edge */
  guint      refcount;
};

P2trVEdge*  p2tr_vedge_new       (P2trPoint *start,
                                  P2trPoint *end,
                                  gboolean   constrained);

P2trVEdge*  p2tr_vedge_new2      (P2trEdge  *real);

P2trVEdge*  p2tr_vedge_ref       (P2trVEdge *self);

void        p2tr_vedge_unref     (P2trVEdge *self);

void        p2tr_vedge_free      (P2trVEdge *self);

P2trMesh*   p2tr_vedge_get_mesh  (P2trVEdge *self);

P2trEdge*   p2tr_vedge_is_real   (P2trVEdge *self);

void        p2tr_vedge_create    (P2trVEdge *self);

void        p2tr_vedge_remove    (P2trVEdge *self);

P2trEdge*   p2tr_vedge_get       (P2trVEdge *self);

/**
 * Try get a real edge from a virtual edge, and then
 * unref the virtual edge. IF A MATCHING REAL EDGE IS
 * RETURNED, IT MUST BE UNREFFED BY THE CALLER!
 * @param self The virtual edge to test
 * @param real The place to store the matching real edge
 * @return TRUE if a real edge was returned, FALSE if no
 *         matching edge exists.
 */
gboolean    p2tr_vedge_try_get_and_unref (P2trVEdge  *self,
                                          P2trEdge  **real);

/**
 * A set of edges to flip is basically a hash set, with unique equality
 * and hashing function to prevent the same edge from appearing twice
 * in different directions
 */
typedef P2trHashSet P2trVEdgeSet;

/**
 * Create a new flip-set - a set of virtual edges that should possibly
 * be flipped to restore the Constrained Delaunay property to a
 * triangulation
*/
P2trVEdgeSet* p2tr_vedge_set_new  ();

/**
 * Add the given edge to the flip set. THE EDGE MUST HAVE BEEN REFFED
 * BEFORE THE CALL TO THIS FUNCTION!
 */
void         p2tr_vedge_set_add  (P2trVEdgeSet  *self,
                                  P2trEdge      *to_flip);

/**
 * Add the given virtual edge to the flip set. THE VIRTUAL EDGE MUST
 * HAVE BEEN REFFED BEFORE THE CALL TO THIS FUNCTION!
 */
void         p2tr_vedge_set_add2 (P2trVEdgeSet  *self,
                                  P2trVEdge     *to_flip);

/**
 * Try popping a virtual edge from the set. If succeeds, THE RETURNED
 * VIRTUAL EDGE MUST BE UNREFFED!
 */
gboolean     p2tr_vedge_set_pop  (P2trVEdgeSet  *self,
                                  P2trVEdge    **value);

/**
 * Returns the amount of virtual edges currently in the set
 */
guint        p2tr_vedge_set_size (P2trVEdgeSet  *self);

/**
 * Free the flip set. IT IS THE REPONSIBILITY OF THE CALLER TO MAKE
 * SURE NO VIRTUAL EDGES WERE LEFT IN THE SET!
 */
void         p2tr_vedge_set_free (P2trVEdgeSet  *self);

gboolean     p2tr_vedge_undirected_equals (const P2trVEdge *e1,
                                           const P2trVEdge *e2);

guint        p2tr_vedge_undirected_hash   (const P2trVEdge *edge);


#endif
