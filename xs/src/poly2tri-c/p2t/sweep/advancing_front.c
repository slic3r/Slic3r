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

#include "advancing_front.h"

void
p2t_node_init_pt (P2tNode* THIS, P2tPoint* p)
{
  THIS->point = p;
  THIS->triangle = NULL;
  THIS->value = p->x;
  THIS->next = NULL;
  THIS->prev = NULL;
}

P2tNode*
p2t_node_new_pt (P2tPoint* p)
{
  P2tNode* THIS = g_slice_new (P2tNode);
  p2t_node_init_pt (THIS, p);
  return THIS;
}

void
p2t_node_init_pt_tr (P2tNode* THIS, P2tPoint* p, P2tTriangle* t)
{
  THIS->point = p;
  THIS->triangle = t;
  THIS->value = p->x;
  THIS->next = NULL;
  THIS->prev = NULL;
}

P2tNode*
p2t_node_new_pt_tr (P2tPoint* p, P2tTriangle* t)
{
  P2tNode* THIS = g_slice_new (P2tNode);
  p2t_node_init_pt_tr (THIS, p, t);
  return THIS;
}

void p2t_node_destroy (P2tNode* THIS)
{
}
void p2t_node_free (P2tNode* THIS)
{
  p2t_node_destroy (THIS);
  g_slice_free (P2tNode, THIS);
}

void
p2t_advancingfront_init (P2tAdvancingFront* THIS, P2tNode* head, P2tNode* tail)
{
  THIS->head_ = head;
  THIS->tail_ = tail;
  THIS->search_node_ = head;
}

P2tAdvancingFront*
p2t_advancingfront_new (P2tNode* head, P2tNode* tail)
{
  P2tAdvancingFront* THIS = g_slice_new (P2tAdvancingFront);
  p2t_advancingfront_init (THIS, head, tail);
  return THIS;
}

void
p2t_advancingfront_destroy (P2tAdvancingFront* THIS) { }

void
p2t_advancingfront_free (P2tAdvancingFront* THIS)
{
  p2t_advancingfront_destroy (THIS);
  g_slice_free (P2tAdvancingFront, THIS);
}

P2tNode*
p2t_advancingfront_locate_node (P2tAdvancingFront *THIS, const double x)
{
  P2tNode* node = THIS->search_node_;

  if (x < node->value)
    {
      while ((node = node->prev) != NULL)
        {
          if (x >= node->value)
            {
              THIS->search_node_ = node;
              return node;
            }
        }
    }
  else
    {
      while ((node = node->next) != NULL)
        {
          if (x < node->value)
            {
              THIS->search_node_ = node->prev;
              return node->prev;
            }
        }
    }
  return NULL;
}

P2tNode*
p2t_advancingfront_find_search_node (P2tAdvancingFront *THIS, const double x)
{
  /* TODO: implement BST index */
  return THIS->search_node_;
}

P2tNode*
p2t_advancingfront_locate_point (P2tAdvancingFront *THIS, const P2tPoint* point)
{
  const double px = point->x;
  P2tNode* node = p2t_advancingfront_find_search_node (THIS, px);
  const double nx = node->point->x;

  if (px == nx)
    {
      if (point != node->point)
        {
          /* We might have two nodes with same x value for a short time */
          if (point == node->prev->point)
            {
              node = node->prev;
            }
          else if (point == node->next->point)
            {
              node = node->next;
            }
          else
            {
              assert (0);
            }
        }
    }
  else if (px < nx)
    {
      while ((node = node->prev) != NULL)
        {
          if (point == node->point)
            {
              break;
            }
        }
    }
  else
    {
      while ((node = node->next) != NULL)
        {
          if (point == node->point)
            break;
        }
    }
  if (node) THIS->search_node_ = node;
  return node;
}

P2tNode*
p2t_advancingfront_head (P2tAdvancingFront *THIS)
{
  return THIS->head_;
}

void
AdvancingFront_set_head (P2tAdvancingFront *THIS, P2tNode* node)
{
  THIS->head_ = node;
}

P2tNode*
p2t_advancingfront_tail (P2tAdvancingFront *THIS)
{
  return THIS->tail_;
}

void
p2t_advancingfront_set_tail (P2tAdvancingFront *THIS, P2tNode* node)
{
  THIS->tail_ = node;
}

P2tNode*
p2t_advancingfront_search (P2tAdvancingFront *THIS)
{
  return THIS->search_node_;
}

void
p2t_advancingfront_set_search (P2tAdvancingFront *THIS, P2tNode* node)
{
  THIS->search_node_ = node;
}

