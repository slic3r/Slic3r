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

#ifndef __P2TC_REFINE_CDT_H__
#define __P2TC_REFINE_CDT_H__

#include <poly2tri-c/p2t/poly2tri.h>
#include "mesh.h"
#include "pslg.h"

typedef struct
{
  P2trMesh *mesh;
  P2trPSLG *outline;
} P2trCDT;

/**
 * Create a new P2trCDT from an existing P2tCDT. The resulting P2trCDT
 * does not depend on the original P2tCDT which can be freed
 * @param cdt A P2tCDT Constrained Delaunay Triangulation
 * @return A P2trCDT Constrained Delaunay Triangulation
 */
P2trCDT*    p2tr_cdt_new       (P2tCDT *cdt);

void        p2tr_cdt_free      (P2trCDT *cdt);

void        p2tr_cdt_free_full (P2trCDT *cdt, gboolean clear_mesh);

/**
 * Test whether there is a path from the point @ref p to the edge @e
 * so that the path does not cross any segment of the CDT
 */
gboolean    p2tr_cdt_visible_from_edge (P2trCDT     *self,
                                        P2trEdge    *e,
                                        P2trVector2 *p);

/**
 * Make sure that all edges either have two triangles (one on each side)
 * or that they are constrained. The reason for that is that the
 * triangulation domain of the CDT is defined by a closed PSLG.
 */
void        p2tr_cdt_validate_edges    (P2trCDT *self);

void        p2tr_cdt_validate_unused   (P2trCDT* self);

/**
 * Make sure the constrained empty circum-circle property holds,
 * meaning that each triangles circum-scribing circle is either empty
 * or only contains points which are not "visible" from the edges of
 * the triangle.
 */
void        p2tr_cdt_validate_cdt      (P2trCDT *self);

#if P2TR_CDT_VALIDATE
#define P2TR_CDT_VALIDATE_EDGES(CDT)  p2tr_cdt_validate_edges(CDT)
#define P2TR_CDT_VALIDATE_UNUSED(CDT) p2tr_cdt_validate_unused(CDT)
#define P2TR_CDT_VALIDATE_CDT(CDT)    p2tr_cdt_validate_cdt(CDT)
#else
#define P2TR_CDT_VALIDATE_EDGES(CDT)  G_STMT_START { } G_STMT_END
#define P2TR_CDT_VALIDATE_UNUSED(CDT) G_STMT_START { } G_STMT_END
#define P2TR_CDT_VALIDATE_CDT(CDT)    G_STMT_START { } G_STMT_END
#endif

/**
 * Insert a point into the triangulation while preserving the
 * constrained delaunay property
 * @param self The CDT into which the point should be inserted
 * @param pc The point to insert
 * @param point_location_guess A triangle which may be near the
 *        area containing the point (or maybe even contains the point).
 *        The better the guess is, the faster the insertion will be. If
 *        such a triangle is unknown, NULL may be passed.
 */
P2trPoint*  p2tr_cdt_insert_point      (P2trCDT           *self,
                                        const P2trVector2 *pc,
                                        P2trTriangle      *point_location_guess);

/**
 * Similar to @ref p2tr_cdt_insert_point, but assumes that the point to
 * insert is located inside the area of the given triangle
 */
void        p2tr_cdt_insert_point_into_triangle (P2trCDT      *self,
                                                 P2trPoint    *pt,
                                                 P2trTriangle *tri);

/**
 * Insert a point so that is splits an existing edge, while preserving
 * the constrained delaunay property. This function assumes that the
 * point is on the edge itself and between its end-points.
 * If the edge being split is constrained, then the function returns a
 * list containing both parts resulted from the splitting. In that case,
 * THE RETURNED EDGES MUST BE UNREFERENCED!
 */
GList*      p2tr_cdt_split_edge (P2trCDT   *self,
                                 P2trEdge  *e,
                                 P2trPoint *C);

#endif
