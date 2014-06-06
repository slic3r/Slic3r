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

#ifndef __P2TC_REFINE_PSLG_H__
#define __P2TC_REFINE_PSLG_H__

#include "utils.h"
#include "vector2.h"
#include "line.h"
#include "bounded-line.h"

typedef P2trHashSet     P2trPSLG;
typedef P2trHashSetIter P2trPSLGIter;

/**
 * Create a new PSLG. After finishing to use this PSLG, it should be
 * freed by calling @ref p2tr_pslg_free
 * @return A new empty PSLG
 */
P2trPSLG* p2tr_pslg_new               (void);

/**
 * Add a new line to the PSLG, where the line is defined by two given
 * points.
 * @param[in] pslg The PSLG
 * @param[in] start The first edge-point of the new line to add
 * @param[in] end The second edge-point of the new line to add
 */
void      p2tr_pslg_add_new_line      (P2trPSLG          *pslg,
                                       const P2trVector2 *start,
                                       const P2trVector2 *end);

/**
 * Add an existing P2trBoundedLine to the PSLG, so that the line will
 * not be freed when the PSLG is freed. This line must not be freed
 * before the PSLG is freed!
 * @param[in] pslg The PSLG
 * @param[in] line The existing line to add
 */
void      p2tr_pslg_add_existing_line (P2trPSLG              *pslg,
                                       const P2trBoundedLine *line);

/**
 * Count how many lines are there in the PSLG
 * @param[in] pslg The PSLG
 * @return The amount of lines in the PSLG
 */
guint     p2tr_pslg_size              (P2trPSLG *pslg);

/**
 * Initialize an iterator to iterate over all the lines of the PSLG. The
 * iterator will remain valid as long as the PSLG is not modified.
 * @param[out] iter The iterator for this PSLG
 * @param[in] pslg The PSLG
 */
void      p2tr_pslg_iter_init         (P2trPSLGIter *iter,
                                       P2trPSLG     *pslg);

/**
 * Advance the iterator to the next line of the PSLG
 * @param[in] iter The PSLG iterator
 * @param[out] line The next line of the PSLG
 * @return TRUE if there was another line, FALSE if the iteration over
 *         all the lines was finished
 */
gboolean  p2tr_pslg_iter_next         (P2trPSLGIter           *iter,
                                       const P2trBoundedLine **line);

/**
 * Test whether the PSLG contains this line. The line comparision is
 * done *by refrence* and not by value, so this function only test if
 * the line at the given memory location is inside the PSLG.
 * @param[in] pslg The PSLG
 * @param[in] line The line to search for
 * @return TRUE if the line was found in the PSLG, FALSE otherwise
 */
gboolean  p2tr_pslg_contains_line     (P2trPSLG              *pslg,
                                       const P2trBoundedLine *line);

/**
 * Free a PSLG and all of the resources allocated for it
 * @param[in] pslg The PSLG to free
 */
void      p2tr_pslg_free              (P2trPSLG *pslg);

#endif
