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

#ifndef __P2TC_REFINE_MESH_H__
#define __P2TC_REFINE_MESH_H__

#include <stdio.h>
#include <glib.h>
#include "vector2.h"
#include "utils.h"
#include "triangulation.h"

/**
 * \defgroup P2trMesh P2trMesh - Triangular Meshes
 * The library is designed to handle triangular meshes which are made
 * of one continuous region, potentially with holes
 * @{
 */

/**
 * A struct for representing a triangular mesh
 */
struct P2trMesh_
{
  /**
   * A hash set containing pointers to all the triangles
   * (\ref P2trTriangle) in the mesh
   */
  P2trHashSet *triangles;

  /**
   * A hash set containing pointers to all the edges (\ref P2trEdge) in
   * the mesh
   */
  P2trHashSet *edges;

  /**
   * A hash set containing pointers to all the points (\ref P2trPoint)
   * in the mesh
   */
  P2trHashSet *points;

  /**
   * A boolean flag specifying whether recording of actions on the
   * mesh (for allowing to undo them) is taking place right now
   */
  gboolean     record_undo;

  /**
   * A queue stroing all the actions done on the mesh since the begining
   * of the recording
   */
  GQueue       undo;

  /**
   * Counts the amount of references to the mesh. When this counter
   * reaches zero, the mesh will be freed
   */
  guint        refcount;
};

/**
 * Create a new empty mesh
 * @return The newly created mesh
 */
P2trMesh*     p2tr_mesh_new             (void);

/**
 * Add an existing point to the given mesh
 * @param self The mesh to add the point to
 * @param point The point to add
 * @return The given point
 */
P2trPoint*    p2tr_mesh_add_point       (P2trMesh  *self,
                                         P2trPoint *point);

/**
 * Create a new point and add it to the given mesh
 * @param mesh The mesh to add the point to
 * @param c The coordinates of the point to create
 * @return The newly created point
 */
P2trPoint*    p2tr_mesh_new_point       (P2trMesh          *mesh,
                                         const P2trVector2 *c);

/**
 * Create a new point and add it to the given mesh
 * @param mesh The mesh to add the point to
 * @param x The X coordinate of the point to create
 * @param y The Y coordinate of the point to create
 * @return The newly created point
 */
P2trPoint*    p2tr_mesh_new_point2      (P2trMesh  *mesh,
                                         gdouble    x,
                                         gdouble    y);

/**
 * Add an existing edge to the given mesh
 * @param self The mesh to add the edge to
 * @param edge The edge to add
 * @return The given edge
 */
P2trEdge*     p2tr_mesh_add_edge        (P2trMesh  *self,
                                         P2trEdge  *point);

/**
 * Create a new edge and add it to the given mesh
 * @param mesh The mesh to add the edge to
 * @param start The starting point of the edge
 * @param end The ending point of the edge
 * @param constrained Specify whether this edge is constrained or not
 * @return Thew newly created edge
 */
P2trEdge*     p2tr_mesh_new_edge        (P2trMesh  *mesh,
                                         P2trPoint *start,
                                         P2trPoint *end,
                                         gboolean   constrained);

/**
 * This function checks if an edge between two points exists, and if
 * not it creates it. In both cases the returned edge will be returned
 * with an extra reference, so it must be unreffed later.
 * @param self The mesh of the returned edge
 * @param start The starting point of the returned edge
 * @param end The ending point of the returned edge
 * @param constrained Specify whether this edge should be constrained
 *        or not (in case a new edge is created)
 * @return An edge between the two points
 */
P2trEdge*     p2tr_mesh_new_or_existing_edge (P2trMesh  *self,
                                              P2trPoint *start,
                                              P2trPoint *end,
                                              gboolean   constrained);

/**
 * Add an existing triangle to the given mesh
 * @param self The mesh to add the triangle to
 * @param edge The triangle to add
 * @return The given triangle
 */
P2trTriangle* p2tr_mesh_add_triangle        (P2trMesh     *self,
                                             P2trTriangle *tri);

/**
 * Create a new triangle and add it to the given mesh
 * @param mesh The mesh to add the triangle to
 * @param AB An edge from the first point of the triangle to the second
 * @param BC An edge from the second point of the triangle to the third
 * @param CA An edge from the third point of the triangle to the first
 */
P2trTriangle* p2tr_mesh_new_triangle        (P2trMesh *mesh,
                                             P2trEdge *AB,
                                             P2trEdge *BC,
                                             P2trEdge *CA);

/** \internal
 * This function should be called just before a point is removed from
 * the mesh. It is used internally to update the mesh and it should not
 * be called by any code outside of this library.
 * @param mesh The mesh from which the point is going to be removed
 * @param point The point which is going to be removed
*/
void          p2tr_mesh_on_point_removed    (P2trMesh  *mesh,
                                             P2trPoint *point);

/** \internal
 * This function should be called just before an edge is removed from
 * the mesh. It is used internally to update the mesh and it should not
 * be called by any code outside of this library.
 * @param mesh The mesh from which the edge is going to be removed
 * @param edge The edge which is going to be removed
*/
void          p2tr_mesh_on_edge_removed     (P2trMesh *mesh,
                                             P2trEdge *edge);

/** \internal
 * This function should be called just before a triangle is removed from
 * the mesh. It is used internally to update the mesh and it should not
 * be called by any code outside of this library.
 * @param mesh The mesh from which the triangle is going to be removed
 * @param triangle The triangle which is going to be removed
*/
void          p2tr_mesh_on_triangle_removed (P2trMesh     *mesh,
                                             P2trTriangle *triangle);

/**
 * Begin recording all action performed on a mesh. Recording the
 * actions performed on a mesh allows choosing later whether to commit
 * those actions or whether they should be undone.
 * \warning This function must not be called when recording is already
 *          taking place!
 * @param self The mesh whose actions should be recorded
 */
void          p2tr_mesh_action_group_begin    (P2trMesh *self);

/**
 * Terminate the current session of recording mesh actions by
 * committing all the actions to the mesh
 * \warning This function must not be called unless recording of
 *          actions is already taking place!
 * @param self The mesh whose actions were recorded
 */
void          p2tr_mesh_action_group_commit   (P2trMesh *self);

/**
 * Terminate the current session of recording mesh actions by
 * undoing all the actions done to the mesh.
 * \warning This function must not be called unless recording of
 *          actions is already taking place!
 * \warning A call to this function may invalidate all references to
 *          any non virtual geometric primitives! If you plan on using
 *          this function, consider using virtual data structures!
 * @param self The mesh whose actions were recorded
 */
void          p2tr_mesh_action_group_undo     (P2trMesh *self);

/**
 * Remove all triangles, edges and points from a mesh
 * @param mesh The mesh to clear
 */
void          p2tr_mesh_clear           (P2trMesh *mesh);

/**
 * Clear and then free the memory used by a mesh data structure
 * @param mesh The mesh whose memory should be freed
 */
void          p2tr_mesh_free            (P2trMesh *mesh);

/**
 * Decrease the reference count to this mesh by 1
 * @param mesh The mesh whose reference count should be decreased
 */
void          p2tr_mesh_unref           (P2trMesh *mesh);

/**
 * Increase the reference count to this mesh by 1
 * @param mesh The mesh whose reference count should be increased
 * @return The given mesh (\ref mesh)
 */
P2trMesh*     p2tr_mesh_ref             (P2trMesh *mesh);

/**
 * Find a triangle of the mesh, containing the point at the given
 * location
 * @param self The mesh whose triangles should be checked
 * @param pt The location of the point to test
 * @return The triangle containing the given point, or NULL if the
 *         point is outside the triangulation domain
 */
P2trTriangle* p2tr_mesh_find_point      (P2trMesh *self,
                                         const P2trVector2 *pt);

/**
 * Exactly like \ref p2tr_mesh_find_point, except for the fact that
 * this variant also returns the UV coordinates of the point inside the
 * triangle
 * @param[in] self The mesh whose triangles should be checked
 * @param[in] pt The location of the point to test
 * @param[out] u The U coordinate of the point inside the triangle
 * @param[out] v The V coordinate of the point inside the triangle
 * @return The triangle containing the given point, or NULL if the
 *         point is outside the triangulation domain
 */
P2trTriangle* p2tr_mesh_find_point2     (P2trMesh          *self,
                                         const P2trVector2 *pt,
                                         gdouble           *u,
                                         gdouble           *v);

/**
 * Another variant of \ref p2tr_mesh_find_point taking an initial
 * triangle that the search should begin from its area. The search is
 * performed first on triangles close to the given triangle, and it
 * gradually tests farther and farther triangles until it finds the one
 * containing the given point.
 *
 * This way of search should be fast when the approximate area of the
 * point to find is known, but it will be slower if initial triangle
 * is not near.
 *
 * Note that in this function, triangles are considered near depending
 * on the length of the chain of neighbor triangles needed to go from
 * one to the other.
 *
 * \warning This function may use memory which is at least linear in
 *          the amount of triangles to test. Therefor, do not use it
 *          on large mesh objects unless the initial guess is supposed
 *          to be good!
 * @param self The mesh whose triangles should be checked
 * @param pt The location of the point to test
 * @param initial_guess An initial guess for which triangle contains
 *        the point, or is at least near the triangle containing it
 * @return The triangle containing the given point, or NULL if the
 *         point is outside the triangulation domain
 */
P2trTriangle* p2tr_mesh_find_point_local (P2trMesh *self,
                                          const P2trVector2 *pt,
                                          P2trTriangle *initial_guess);

/**
 * Exactly like \ref p2tr_mesh_find_point_local, except for the fact
 * that this variant also returns the UV coordinates of the point
 * inside the triangle
 * @param[in] self The mesh whose triangles should be checked
 * @param[in] pt The location of the point to test
 * @param[in] initial_guess An initial guess for which triangle
 *            contains the point, or is at least near the triangle
 *            containing it
 * @param[out] u The U coordinate of the point inside the triangle
 * @param[out] v The V coordinate of the point inside the triangle
 * @return The triangle containing the given point, or NULL if the
 *         point is outside the triangulation domain
 */
P2trTriangle* p2tr_mesh_find_point_local2 (P2trMesh *self,
                                           const P2trVector2 *pt,
                                           P2trTriangle *initial_guess,
                                           gdouble *u,
                                           gdouble *v);

/**
 * Find the bounding rectangle containing this mesh.
 * @param[in] self The mesh whose bounding rectangle should be computed
 * @param[out] min_x The minimal X coordinate of the mesh
 * @param[out] min_y The minimal Y coordinate of the mesh
 * @param[out] max_x The maximal X coordinate of the mesh
 * @param[out] max_y The maximal Y coordinate of the mesh
 */
void          p2tr_mesh_get_bounds        (P2trMesh    *self,
                                           gdouble     *min_x,
                                           gdouble     *min_y,
                                           gdouble     *max_x,
                                           gdouble     *max_y);

/**
 * Same as p2tr_mesh_save_to_file, but also opens the file at the
 * specified path to be used as the target file
 * @param[in] self The mesh to export
 * @param[in] path The path of the file to export
 * @return TRUE if exporting the mesh succeeded, FALSE otherwise
 */
gboolean      p2tr_mesh_save              (P2trMesh    *self,
                                           const gchar *path);

/**
 * Export the mesh to a file in the Object File Format (.off),
 * with 0 as the value of the Z coordinates
 * @param[in] self The mesh to export
 * @param[in] out The file into which the mesh should be exported
 */
void          p2tr_mesh_save_to_file      (P2trMesh *self,
                                           FILE     *out);

/**
 * Load a 2D triangular mesh from an .off file (ignoring the Z
 * coordinates of the points in the file)
 * @param[in] path The file to load the mesh from
 * @return The loaded mesh, or NULL if an error occurred while parsing
 *         the file
 */
P2trMesh*     p2tr_mesh_load_from_file    (FILE     *in);

/**
 * Same as p2tr_mesh_load_from_file, but also opens the file at the
 * specified path to load the mesh from it
 * @param[in] path The path of the file to load the mesh from
 * @return The loaded mesh, or NULL if an error occurred while parsing
 *         the file
 */
P2trMesh*     p2tr_mesh_load              (const gchar *path);

/** @} */
#endif
