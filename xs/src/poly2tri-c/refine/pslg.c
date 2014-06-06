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

#include <glib.h>
#include "pslg.h"

static void
free_bounded_line_if_not_null (gpointer line)
{
  p2tr_bounded_line_free ((P2trBoundedLine*)line);
}

/* A PSLG which is essentially a set of lines, will be represented by a
 * hash table. When it will have an entry where both key and value are
 * a line, it means that this is a new line and therefore should be
 * freed. When it has an entry where the key is a line and the value is
 * NULL, it means this is a line that should not be freed.
 * This behaiour will be acheived by a NULL key free function and a
 * real free function for values */
P2trPSLG*
p2tr_pslg_new (void)
{
  return g_hash_table_new_full (NULL, NULL, NULL, free_bounded_line_if_not_null);
}

void
p2tr_pslg_add_new_line (P2trPSLG          *pslg,
                        const P2trVector2 *start,
                        const P2trVector2 *end)
{
  P2trBoundedLine *line = p2tr_bounded_line_new (start, end);
  /* We would like to free this line, so also add it as a value */
  g_hash_table_insert (pslg, line, line);
}

/* Add a line that needs not to be freed */
void
p2tr_pslg_add_existing_line (P2trPSLG              *pslg,
                             const P2trBoundedLine *line)
{
  g_hash_table_insert (pslg, (P2trBoundedLine*) line, NULL);
}

guint
p2tr_pslg_size (P2trPSLG *pslg)
{
  return g_hash_table_size (pslg);
}

void
p2tr_pslg_iter_init (P2trPSLGIter *iter,
                     P2trPSLG     *pslg)
{
  g_hash_table_iter_init (iter, pslg);
}

gboolean
p2tr_pslg_iter_next (P2trPSLGIter           *iter,
                     const P2trBoundedLine **line)
{
  /* The values are always stored in the key */
  return g_hash_table_iter_next (iter, (gpointer*)line, NULL);
}

gboolean
p2tr_pslg_contains_line (P2trPSLG              *pslg,
                         const P2trBoundedLine *line)
{
  return g_hash_table_lookup_extended (pslg, line, NULL, NULL);
}

void
p2tr_pslg_free (P2trPSLG *pslg)
{
  g_hash_table_destroy (pslg);
}
