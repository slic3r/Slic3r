/*********************************************************************

  EVG-THIN v1.1: A Thinning Approximation to the Extended Voronoi Graph
  
  Copyright (C) 2006 - Patrick Beeson  (pbeeson@cs.utexas.edu)


  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
  USA

*********************************************************************/


#ifndef datatypes_hh
#define datatypes_hh 

#include <vector>

using namespace std;

namespace EVG_THIN {

typedef unsigned int uint;

// GRID DATA TYPES

// Each cell belongs to one of three states.
typedef enum {Occupied, Unknown, Free} cell_type;

typedef vector<cell_type> column_type;

/**
   The cells of a grid basically have three possible states: occupied,
   free, or unknown.  Free cells are light cells in a greyscale image
   (129-255), occupied cells are dark (0-126), and (by default)
   unknown cells are light grey (127&128).  These ranges can be
   changed at the command line.
**/
typedef vector<column_type> grid_type;




// SKELETON DATA TYPES

/**
   This data type holds the data for a single node in a skeleton graph
   (e.g. Voronoi graph of free space).  Graphs are meant to be non-cyclic
   graphs represented as trees.  Each node has a location, a radius
   (distance to nearest obstacle), a parent, some children, a tag of
   whether it touches the edge of the LPM, and a distance to the root
   node.  Coordinates and distances are in occ. grid coordinates.
**/
class node {
public:
  int x,y; //!< location in grid_coords
  float radius; //!< distance to nearest obstacle (in number of cells)
  float distance; //!< shortest depth in graph to graph root
  int parent; //!< index of parant in list
  /** 
      Basically, exactly equal to children.size() by the time the
      skeleton is made. However, when making the skeleton, there may
      times when the number of estimated children is known, but
      children hasn't been fully filled out (e.g. doing a depth first
      crawl over the raw skeleton to get a pruned skeleton).
  **/
  unsigned int num_children;  
  vector<unsigned int> children; //!< if # children > 1, graph branches
  
  friend bool operator>(const node& n1, const node& n2) {
    return n1.distance>n2.distance;
  }
};

typedef vector<node> skeleton_type;

} // end namespace EVG_THIN

#endif
