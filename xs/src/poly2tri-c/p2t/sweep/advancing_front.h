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

#ifndef __P2TC_P2T_ADVANCING_FRONT_H__
#define __P2TC_P2T_ADVANCING_FRONT_H__

#include "../common/poly2tri-private.h"
#include "../common/shapes.h"

/* Advancing front node */

struct _P2tNode
{
  P2tPoint* point;
  P2tTriangle* triangle;

  struct _P2tNode* next;
  struct _P2tNode* prev;

  double value;
};

void p2t_node_init_pt (P2tNode* THIS, P2tPoint* p);
P2tNode* p2t_node_new_pt (P2tPoint* p);
void p2t_node_init_pt_tr (P2tNode* THIS, P2tPoint* p, P2tTriangle* t);
P2tNode* p2t_node_new_pt_tr (P2tPoint* p, P2tTriangle* t);
void p2t_node_destroy (P2tNode* THIS);
void p2t_node_free (P2tNode* THIS);

/* Advancing front */

struct AdvancingFront_
{
  /* private: */

  P2tNode* head_, *tail_, *search_node_;

};

void p2t_advancingfront_init (P2tAdvancingFront* THIS, P2tNode* head, P2tNode* tail);
P2tAdvancingFront* p2t_advancingfront_new (P2tNode* head, P2tNode* tail);

void p2t_advancingfront_destroy (P2tAdvancingFront* THIS);
void p2t_advancingfront_free (P2tAdvancingFront* THIS);

P2tNode* p2t_advancingfront_head (P2tAdvancingFront *THIS);
void AdvancingFront_set_head (P2tAdvancingFront *THIS, P2tNode* node);
P2tNode* p2t_advancingfront_tail (P2tAdvancingFront *THIS);
void p2t_advancingfront_set_tail (P2tAdvancingFront *THIS, P2tNode* node);
P2tNode* p2t_advancingfront_search (P2tAdvancingFront *THIS);
void p2t_advancingfront_set_search (P2tAdvancingFront *THIS, P2tNode* node);

/** Locate insertion point along advancing front */
P2tNode* p2t_advancingfront_locate_node (P2tAdvancingFront *THIS, const double x);

P2tNode* p2t_advancingfront_locate_point (P2tAdvancingFront *THIS, const P2tPoint* point);

P2tNode* p2t_advancingfront_find_search_node (P2tAdvancingFront *THIS, const double x);

#endif
