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


#ifndef evg_thin_hh 
#define evg_thin_hh 

#include <deque>
#include "datatypes.hpp"
#include "heap.hpp"

using namespace std;

namespace EVG_THIN {

/**
   The thinning algorithm can be thought of as a "brush fire" where a
   fire starts at every obstacle and burns into freespace.  When two
   fronts of the fire meet, they define a skeleton.  
   
   This particular algorithm has special purpose machinery to crete a
   1 cell wide skeleton (stops a cell from "burning" if it is next to
   two or more "fronts" of the fire.
   
   I made a linear algorthim from the quadradic algorithm discussed in
   the original thinning publication (see README).

   I also added max and min parameters and a pruning operator to
   remove spurs.
**/
class evg_thin {
public:
  evg_thin(const grid_type&, float, float, bool, bool, int, int);
  skeleton_type generate_skeleton();
  void reset();

private:
  grid_type original_grid;

  int grid_size_x,grid_size_y;
  
  float coastal_dist, prune_dist;
  bool prune,location;

  // Keeps track of distance to closest obstacle.
  class dist_cell {
  public:
    int x,y;
    float distance;
    friend bool operator> (const dist_cell& d1, const dist_cell& d2)  {
      return d1.distance>d2.distance;
    }
  };

  typedef vector <dist_cell> dist_col;
  typedef vector <dist_col> dist_grid;
  dist_grid distance_grid;
  
  void calculate_distances();


  /** 
      occupied: not-part of skeleton (either an obstacle or a "burned"
      free cell.  free: a free cell that has not yet been
      examined. processing: a free cell that is in queue to be
      examined (prevents multiple copies of a cell in a queue).
      processed: a free cell that has been determined will never
      "burn".  unknown : neither occupied nor free (grey cells inthe
      lpm).
  **/
  typedef enum {occupied, free, skel, processing, processed, unknown} State;

  typedef vector < vector <State> > gridtype;  

  gridtype _step1_grid; //!< holds current state of thinning algorithm
 			//!< before step1
  gridtype _step2_grid; //!< holds current state of thinning algorithm
 			//!< before step2


  int robot_loc_x, robot_loc_y;
  int _num_exits;

  bool on_grid(int,int);

  skeleton_type curr_skel; //!< final skeleton 
  skeleton_type _tmp_skel; //!< intermediate skeleton
  uint _root_index; //!< index of skeleton tree root in _tmp_skel;


  //////// Responsible for actual thinning algorithm.
  
  int _closestx, _closesty; //!< closest point on skeleton to robot
  float _distance; //!< distance of robot's closest skeletal point to
 		   //!< nearest obstacle


  void find_skel();
  void initialize();
  void thin();
  
  class edge {
  public:
    edge(int i, int j) {
      x=i;
      y=j;
    }
    int x,y;
  };
  
  typedef deque<edge> queuetype;
  
  queuetype _step1_queue;  //!< holds cells to process in step1
  queuetype _step2_queue;  //!< holds cells to process in step2
  
  State step(gridtype&, edge&, bool);
  

  /////////////////////////
  ///////Responsible for  building the data structure from the grid
  
  void find_skel_edge();
  void build_skel();
  
  void crawl_grid();
  void remove_final_spur();
  void best_to_depth_first();
  void best_to_depth_first_helper(int myindex, int parent_index);
  
  void remove_branch(int index, int child_index=-1);
  void remove_branch2(int index);
  bool is_exit(const node& curr_node);
  vector<node> find_neighbors(int x, int y);

};  

} // end namespace EVG_THIN

#endif //evg_thin_hh 

