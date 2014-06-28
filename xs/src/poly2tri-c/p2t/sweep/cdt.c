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

#include "cdt.h"

void
p2t_cdt_init (P2tCDT* THIS, P2tPointPtrArray polyline)
{
  THIS->sweep_context_ = p2t_sweepcontext_new (polyline);
  THIS->sweep_ = p2t_sweep_new ();
}

P2tCDT*
p2t_cdt_new (P2tPointPtrArray polyline)
{
  P2tCDT* THIS = g_slice_new (P2tCDT);
  p2t_cdt_init (THIS, polyline);
  return THIS;
}

void
p2t_cdt_destroy (P2tCDT* THIS)
{
  p2t_sweepcontext_delete (THIS->sweep_context_);
  p2t_sweep_free (THIS->sweep_);
}

void
p2t_cdt_free (P2tCDT* THIS)
{
  p2t_cdt_destroy (THIS);
  g_slice_free (P2tCDT, THIS);
}

void
p2t_cdt_add_hole (P2tCDT *THIS, P2tPointPtrArray polyline)
{
  p2t_sweepcontext_add_hole (THIS->sweep_context_, polyline);
}

void
p2t_cdt_add_point (P2tCDT *THIS, P2tPoint* point)
{
  p2t_sweepcontext_add_point (THIS->sweep_context_, point);
}

void
p2t_cdt_triangulate (P2tCDT *THIS)
{
  p2t_sweep_triangulate (THIS->sweep_, THIS->sweep_context_);
}

P2tTrianglePtrArray
p2t_cdt_get_triangles (P2tCDT *THIS)
{
  return p2t_sweepcontext_get_triangles (THIS->sweep_context_);
}

P2tTrianglePtrList
p2t_cdt_get_map (P2tCDT *THIS)
{
  return p2t_sweepcontext_get_map (THIS->sweep_context_);
}
