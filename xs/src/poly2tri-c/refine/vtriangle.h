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

#ifndef __P2TC_REFINE_VTRIANGLE_H__
#define __P2TC_REFINE_VTRIANGLE_H__

#include <glib.h>
#include "rmath.h"
#include "triangulation.h"

/**
 * @struct P2trVTriangle_
 * A struct for representing a potential ("virtual") triangle
 * in a triangular mesh
 */
struct P2trVTriangle_
{
  P2trPoint* points[3];
  
  guint refcount;
};

P2trVTriangle*   p2tr_vtriangle_new          (P2trTriangle  *tri);

P2trVTriangle*   p2tr_vtriangle_ref          (P2trVTriangle *self);

void             p2tr_vtriangle_unref        (P2trVTriangle *self);

void             p2tr_vtriangle_free         (P2trVTriangle *self);

P2trMesh*        p2tr_vtriangle_get_mesh     (P2trVTriangle *self);

P2trTriangle*    p2tr_vtriangle_is_real      (P2trVTriangle *self);

void             p2tr_vtriangle_create       (P2trVTriangle *self);

void             p2tr_vtriangle_remove       (P2trVTriangle *self);

P2trTriangle*    p2tr_vtriangle_get          (P2trVTriangle *self);

#endif
