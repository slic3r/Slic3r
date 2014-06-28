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

#ifndef __P2TC_REFINE_MESH_ACTION_H__
#define __P2TC_REFINE_MESH_ACTION_H__

#include <glib.h>

/**
 * \defgroup P2trMeshAction P2trMeshAction - Mesh Action Recording
 * Action recording state objects for mesh objects. These may be
 * inspected by code outside of the library, but objects of this type
 * should not be created or manipulated by external code!
 * @{
 */

/**
 * The type of the geometric primitive affected by a mesh action
 */
typedef enum
{
  P2TR_MESH_ACTION_POINT,
  P2TR_MESH_ACTION_EDGE,
  P2TR_MESH_ACTION_TRIANGLE
} P2trMeshActionType;

/**
 * A struct representing any single action on a mesh. A single atomic
 * action may be the insertion/removal of a point/edge/triangle.
 *
 * Note that such an action only treats the direct geometric operation
 * related to the specific geometric primitve, without any of its
 * dependencies.
 *
 * For example, if removing a point requires the removal of several
 * edges and triangles, then the removal of each one of those should be
 * recorded in its own action object.
 */
typedef struct P2trMeshAction_
{
  /** The type of geometric primitive affected by the action */
  P2trMeshActionType  type;
  /** A flag specifying whether the primitive was added or removed */
  gboolean            added;
  /** A reference count to the action object */
  gint                refcount;
  /** Specific additional information which is needed for each type
   *  of action */
  union {
    /** Information required to undo a point action */
    struct {
      /** The point that was added/deleted */
      P2trPoint *point;
    } action_point;

    /** Information required to undo an edge action */
    struct {
      /** A virtual edge representing the added/deleted edge */
      P2trVEdge *vedge;
      /** A flag specifying whether the edge is constrained */
      gboolean   constrained;
    } action_edge;

    /** Information required to undo a triangle action */
    struct {
      /** A virtual triangle representing the added/deleted triangle */
      P2trVTriangle *vtri;
    } action_tri;
  } action;
} P2trMeshAction;

/**
 * Create a new mesh action describing the addition of a new point
 * @param point The point that is added to the mesh
 * @return An object representing the point addition action
 */
P2trMeshAction*  p2tr_mesh_action_new_point      (P2trPoint      *point);

/**
 * Create a new mesh action describing the deletion of an existing point
 * @param point The point that is deleted from the mesh
 * @return An object representing the point deletion action
 */
P2trMeshAction*  p2tr_mesh_action_del_point      (P2trPoint      *point);

/**
 * Create a new mesh action describing the addition of a new edge
 * @param edge The edge that is added to the mesh
 * @return An object representing the edge addition action
 */
P2trMeshAction*  p2tr_mesh_action_new_edge       (P2trEdge       *edge);

/**
 * Create a new mesh action describing the deletion of an existing edge
 * @param edge The edge that is deleted from the mesh
 * @return An object representing the edge deletion action
 */
P2trMeshAction*  p2tr_mesh_action_del_edge       (P2trEdge       *edge);

/**
 * Create a new mesh action describing the addition of a triangle
 * @param tri The triangle that is added to the mesh
 * @return An object representing the triangle addition action
 */
P2trMeshAction*  p2tr_mesh_action_new_triangle   (P2trTriangle   *tri);

/**
 * Create a new mesh action describing the deletion of an existing
 * triangle
 * @param tri The triangle that is deleted from the mesh
 * @return An object representing the triangle deletion action
 */
P2trMeshAction*  p2tr_mesh_action_del_triangle   (P2trTriangle   *tri);

/**
 * Increase the reference count to this mesh action by 1
 * @param self The mesh action whose reference count should be increased
 */
P2trMeshAction*  p2tr_mesh_action_ref            (P2trMeshAction *self);

/**
 * Decrease the reference count to this mesh action by 1
 * @param self The mesh action whose reference count should be decreased
 */
void             p2tr_mesh_action_unref          (P2trMeshAction *self);

/**
 * Free the memory used by a mesh action data structure
 * @param self The mesh action whose memory should be freed
 */
void             p2tr_mesh_action_free           (P2trMeshAction *self);

/**
 * Undo the action described by the given mesh action object
 * @param self The mesh action to undo
 * @param mesh The mesh on which this action was applied
 */
void             p2tr_mesh_action_undo           (P2trMeshAction *self,
                                                  P2trMesh       *mesh);

#endif
