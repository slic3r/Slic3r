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

#ifndef __P2TC_P2T_CDT_H__
#define __P2TC_P2T_CDT_H__

#include "../common/poly2tri-private.h"
#include "advancing_front.h"
#include "sweep_context.h"
#include "sweep.h"

/**
 * 
 * @author Mason Green <mason.green@gmail.com>
 *
 */

struct CDT_
{
  /*private: */

  /**
   * Internals
   */

  P2tSweepContext* sweep_context_;
  P2tSweep* sweep_;

};
/**
 * Constructor - add polyline with non repeating points
 *
 * @param polyline
 */
void p2t_cdt_init (P2tCDT* THIS, P2tPointPtrArray polyline);
P2tCDT* p2t_cdt_new (P2tPointPtrArray polyline);

/**
 * Destructor - clean up memory
 */
void p2t_cdt_destroy (P2tCDT* THIS);
void p2t_cdt_free (P2tCDT* THIS);

/**
 * Add a hole
 *
 * @param polyline
 */
void p2t_cdt_add_hole (P2tCDT *THIS, P2tPointPtrArray polyline);

/**
 * Add a steiner point
 *
 * @param point
 */
void p2t_cdt_add_point (P2tCDT *THIS, P2tPoint* point);

/**
 * Triangulate - do this AFTER you've added the polyline, holes, and Steiner points
 */
void p2t_cdt_triangulate (P2tCDT *THIS);

/**
 * Get CDT triangles
 */
P2tTrianglePtrArray p2t_cdt_get_triangles (P2tCDT *THIS);

/**
 * Get triangle map
 */
P2tTrianglePtrList p2t_cdt_get_map (P2tCDT *THIS);

#endif
