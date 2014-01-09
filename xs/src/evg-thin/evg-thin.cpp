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

#include "evg-thin.hpp"
#include <iostream>
#include <float.h>
#include <math.h>

namespace EVG_THIN {

/**
   Euclidean distance between two points.
**/
static inline float dist(float ax, float ay, float bx, float by) {
  return sqrt(pow(ax-bx,2)+pow(ay-by,2));
}

evg_thin::evg_thin(const grid_type& curr_grid,
		   float distance_min, float distance_max, 
		   bool pruning, bool robot_dependent,
		   int loc_x, int loc_y) {

  original_grid=curr_grid;
  coastal_dist=distance_max;
  prune_dist=distance_min;
  prune=pruning;
  location=robot_dependent;


  grid_size_x=original_grid.size();
  grid_size_y=original_grid[0].size();

  robot_loc_x=loc_x;
  robot_loc_y=loc_y;


  vector <State> tmp(grid_size_y);
  _step1_grid.resize(grid_size_x,tmp);

  dist_col tmpcol(grid_size_y);
  distance_grid.resize(grid_size_x,tmpcol);
}


/**
   Resets data structures after a skeleton is found.  
**/
void evg_thin::reset() {
  // Use this before calling generate_skeleton() if you are looping
  // over a changing grid data structure.

  // Assumes the grid size is constant, if not, add code to resize
  // (and clear) _step1_grid and distance_grid based on new grid size.

  curr_skel.clear();
  _step1_queue.clear();
  _step2_queue.clear();
}


/**
   Public method that returns the skeleton of a grid
**/
skeleton_type evg_thin::generate_skeleton() {

  calculate_distances();
  find_skel();
  
  return curr_skel;
}


bool evg_thin::on_grid(int x, int y) {
  return ((x >=0 && x < grid_size_x) &&
	  (y >=0 && y < grid_size_y));
}



/**
   Calculate the distance of each free cell from the closest occupied
   cell.  Stores these (along with the location of the closest
   obstacle) in distance_grid.
**/
void evg_thin::calculate_distances() {

  heap<dist_cell> hdist;
  dist_cell current_cell;  

  for (int x=0;x<grid_size_x;x++)
    for (int y=0;y<grid_size_y;y++)
      if (original_grid[x][y]==Occupied) {
	distance_grid[x][y].distance=0;
	distance_grid[x][y].x=x;
	distance_grid[x][y].y=y;

	current_cell.x=x;
	current_cell.y=y;
	current_cell.distance=0;
	hdist.push(current_cell);
      }
      else {
	distance_grid[x][y].distance=FLT_MAX;
	distance_grid[x][y].x=-1;
	distance_grid[x][y].y=-1;
      }


  heap<dist_cell> hdist2;
  
  while (!hdist.empty()) {
    while (!hdist.empty()) {
      current_cell=hdist.first();
      hdist.pop();
      
      // Look at neighbors to find a new free cell that needs its
      // distance updated
      for (int i=-1; i<=1; i++) {
	int nx=current_cell.x+i;
	for (int j=-1; j<=1; j++) {
	  int ny=current_cell.y+j;
	  if ((i!=0 || j!=0) &&
	      on_grid(nx,ny)) {
	    if (original_grid[nx][ny] != Occupied &&
		distance_grid[nx][ny].distance==FLT_MAX) {
	      float min_distance=FLT_MAX;
	      
	      //look at neighbors of freecell to find cells whose
	      //distance has already been found
	      for (int k=-1;k<=1;k++) {
		int nxk=nx+k;
		for (int l=-1;l<=1;l++) {
		  int nyl=ny+l;
		  if ((k!=0 || l!=0) &&
		      on_grid(nxk,nyl)) {
		    if (distance_grid[nxk][nyl].x >=0) {
		      //find distance to neighbor's closest cell
		      float d=dist(nx,ny,
				   distance_grid[nxk][nyl].x,
				   distance_grid[nxk][nyl].y);
		      if (d < min_distance) {
			min_distance=d;
			distance_grid[nx][ny].distance=min_distance;
			distance_grid[nx][ny].x=distance_grid[nxk][nyl].x;
			distance_grid[nx][ny].y=distance_grid[nxk][nyl].y;
		      }
		    }
		  }
		}
	      }
	      if (min_distance<coastal_dist) {
		dist_cell me_cell;
		me_cell.x=nx;
		me_cell.y=ny;
		me_cell.distance=min_distance;
		hdist2.push(me_cell);
	      }
	    }
	  }
	}
      }
    }
    hdist=hdist2;
    hdist2.clear();
  }
}


/**
   Basically, calls the procedures that build the skeleton once the
   distances to obstacles have been calcuated in calculate_distances()
**/
void evg_thin::find_skel() {
  
  //First initialize the grid by labeling cells
  initialize();
  
  //Then "thin" the grid by flipping free cells that border occupied
  //cells to occupied (when applicaple).
  thin();

  //Search for the actual skeleton cells after then thinning is done.
  find_skel_edge();

  // Convert from a grid to a skeleton_type data structure.
  build_skel();
}



/**
   Initialize _step1_grid to be "occupied" for all occupied grid
   cells.  Any free grid cells become either "free" (most free cells)
   or "processing" if they are next to obstacles.  Also, put any
   "processing" cells in the _step1_queue.
**/
void evg_thin::initialize() {  
  
  // All cells in _step1_grid need to be labeled based on occ grid.
  for (int i=0;i<grid_size_x;i++)
    for (int j=0;j<grid_size_y;j++) {
      if (original_grid[i][j]==Occupied) {
	_step1_grid[i][j]=occupied;
      }
      else
	// "Bleed" obstacles out by the safety_radius amount
	if (distance_grid[i][j].distance <=
	    prune_dist)
	  _step1_grid[i][j]=occupied;
	else
	  // If free
	  if (original_grid[i][j]!=Occupied) {
	    bool border=false;
	    for (int x=-1;x<=1;x++)
	      for (int y=-1;y<=1;y++)
		if (x!=0 || y!=0)
		  //if it's next to a "bleeded" obstacle
		  if (on_grid(x+i,y+j) &&
		      distance_grid[x+i][y+j].distance <=
		      prune_dist) {
		    border=true;
		    break;
		  }
	    //if bordering occupied, put on queue to be examined.
	    if (border) {
	      _step1_grid[i][j]=processing;
	      edge tmp(i,j);
	      _step1_queue.push_back(tmp);
	    }
	    else 
	      _step1_grid[i][j]=free;
	  }
      //unknown cells are not "fuel" like occupied cells and free
      //cells next to occupied cells.
	  else _step1_grid[i][j]=unknown;
    }
}


/**
   Alternate between step1 and step2 until _step1_grid==_step2_grid
   (i.e. until _step1_queue and _step2_queue no longer have any more
   "fuel" for the brush fire algorithm).
**/
void evg_thin::thin() {
  
  bool changing=true;

  while (changing) {
    changing=false;
    
    _step2_grid=_step1_grid;
    
    
    // Keep _step1_grid constant, burning cells (when applicable) in
    // _step2_grid.  Add neighbors to burned cells to _step2_queue.
    while (!_step1_queue.empty()) {
      edge current=_step1_queue.front();
      _step1_queue.pop_front();
      
      if (_step1_grid[current.x][current.y]==processing || 
	  _step1_grid[current.x][current.y]==processed) { 
	//Not occupied && not skel.  Shouldn't be "free" or "unknown"
	//and on the queue.
	
	State status=step(_step1_grid,current,true);
	//status should be processing, processed, skel, or occupied.
	
	_step2_grid[current.x][current.y]=status;
	
	if (status==processing) {
	  //If already on the heap, switch to "processed" so that it
	  //will only be looked at once more (by the next step),
	  //assuming a neighbor cell doesn't switch to occupied.
	  _step2_grid[current.x][current.y]=processed;
	  _step2_queue.push_back(current);
	}
	else
	  if (status==occupied) {
	    changing=true;
	    // Find neighboring cells to examine on the next step.
	    for (int i=-1; i<=1; i++)
	      for (int j=-1; j<=1; j++)
		if (i!=0 || j!=0) {
		  edge tmp(current.x+i,current.y+j);
		  if (on_grid(tmp.x,tmp.y) &&
		      (_step2_grid[tmp.x][tmp.y]==free ||
		       _step2_grid[tmp.x][tmp.y]==processed)) {
		    //If it is free or already processed, look at it
		    //(again)
		    if (_step2_grid[tmp.x][tmp.y]!=processed ||
			_step1_grid[tmp.x][tmp.y]!=processing)
		      //If it just turned to processed, it's already
		      //on the queue
		      _step2_queue.push_back(tmp);
		    _step2_grid[tmp.x][tmp.y]=processing;
		  }
		}
	  }
      }
    }
    
    _step1_grid=_step2_grid;
    
    // Keep _step2_grid constant, burning cells (when applicable) in
    // _step1_grid.  Add neighbors to burned cells to _step1_queue.
    while (!_step2_queue.empty()) {
      edge current=_step2_queue.front();
      _step2_queue.pop_front();
      
      if (_step2_grid[current.x][current.y]==processing || 
	  _step2_grid[current.x][current.y]==processed) {
	//Not occupied && not skel.  Shouldn't be "free" or "unknown"
	//and on the queue.
	
	State status=step(_step2_grid,current,false);
	//status should be processing, processed, skel, or occupied.
	
	_step1_grid[current.x][current.y]=status;
	
	if (status==processing) {
	  //If already on the heap, switch to "processed" so that it
	  //will only be looked at once more (by the next step),
	  //assuming a neighbor cell doesn't switch to occupied.
	  _step1_grid[current.x][current.y]=processed;	  
	  _step1_queue.push_back(current);
	}
	else
	  if (status==occupied) {
	    changing=true;
	    // Find neighboring cells to examine on the next step.
	    for (int i=-1; i<=1; i++)
	      for (int j=-1; j<=1; j++)
		if (i!=0 || j!=0) {
		  edge tmp(current.x+i,current.y+j);
		  if (on_grid(tmp.x,tmp.y) &&
		      (_step1_grid[tmp.x][tmp.y]==free ||
		       _step1_grid[tmp.x][tmp.y]==processed)) {
		    //If it is free or already processed, look at it
		    //(again)
		    if (_step1_grid[tmp.x][tmp.y]!=processed ||
			_step2_grid[tmp.x][tmp.y]!=processing)
		      //If it just turned to processed, it's already
		      //on the queue
		    _step1_queue.push_back(tmp);
		    _step1_grid[tmp.x][tmp.y]=processing;
		  }
		}
	  }
      }
      
    }
  }
}

/**
   Given a free cell in the grid, deterimine whether it can be
   switched to an occupied cell by looking at its' neighbors.  If not,
   it may be part of the skeleton.  Check for that.
**/

evg_thin::State evg_thin::step(gridtype& grid, 
			       edge& current, bool step1) {

  // If there is a bound on the maximum distance (for coastal
  // navigation) stop if we reached that distance.
  if (distance_grid[current.x][current.y].distance >= coastal_dist)
    return skel;

  
  bool freecell[8];

  int nx,ny,np;
  
  int i,j;

  np=0;

  //Marked state of all neighbors (occupied or !occupied).
  for (i=-1;i<=1;i++) {
    nx=current.x+i;
    for (j=-1;j<=1;j++)
      if (i!=0 || j!=0) {
	ny=current.y+j;
	if (on_grid(nx,ny) &&
	    grid[nx][ny]==occupied)
	  freecell[np]=false;
	else freecell[np]=true;
	np++;
      }
  }
  

  int N=0;
  for (i=0;i<8;i++)
    if (freecell[i])
      N++;

  //if more tha n6 neighbors are occupied, this cell is definately
  //part of the skeleton.
  if (N < 2)
    return skel;
  
  if (N <= 6) {
    int count=0;
    if (!freecell[0] && freecell[1])
          count++;
    if (!freecell[1] && freecell[2])
      count++;
    if (!freecell[2] && freecell[4])
      count++;
    if (!freecell[4] && freecell[7])
      count++;
    if (!freecell[7] && freecell[6])
      count++;
    if (!freecell[6] && freecell[5])
      count++;
    if (!freecell[5] && freecell[3])
      count++;
    if (!freecell[3] && freecell[0])
      count++;
    
    if (count == 1) 
      if ((step1  && (!freecell[1] || !freecell[4] || !freecell[6]) &&
	   (!freecell[4] || !freecell[6] || !freecell[3])) ||
	  (!step1 && (!freecell[1] || !freecell[4] || !freecell[3]) &&
	   (!freecell[1] || !freecell[6] || !freecell[3])))
	return occupied;
      
  }


  return grid[current.x][current.y];
}

/**
   After thin() is run, _step1_grid has a bunch of cells marked
   occupied, free, processed, and skel.  Walk through the grid finding
   skel or processed cells that border occupied cells.  Those are part
   of the skeleton, otherwise, mark them occupied in _step2_grid.

   Also, find the closest skeleton point to the robot's current
   location.
**/
void evg_thin::find_skel_edge() {
  // Don't worry about making _step1_grid and _step2 equal.  After
  // thin(), if there is any difference it would only be that
  // _step1_grid had some cells marked "skel" that are marked
  // "processed" in _step2_grid.  In this function, all cells that are
  // "skel" or "procssed" in _step1_grid become either skel or
  // occupied in _step2_grid.  Then _step2 grid can be used for
  // pruning and making the skeleton data structure.

  
  float rdist=FLT_MAX;
  _closestx=-1;
  _closesty=-1;
  _distance=-1;

  for (int i=0;i<grid_size_x;i++)
    for (int j=0;j<grid_size_y;j++)
      if (_step1_grid[i][j]==free)
	_step2_grid[i][j]=occupied;
      else
	//state should never be "processing" at this point
	if (_step1_grid[i][j]==processed || 
	    _step1_grid[i][j]==skel) {
	  //We only need cells that have an occupied cell above,
	  //below, left, or right (no diagonals).
	  bool edge=false;
	  for (int x=-1;x<=1;x++)
	    for (int y=-1;y<=1;y++)
	      if (x!=y &&
		  (x==0 || y==0))
		if (on_grid(i+x,j+y) &&
		    _step1_grid[i+x][j+y]==occupied)
		  edge=true;
	  if (!edge) 
	    _step2_grid[i][j]=occupied;
	  else {
	    _step2_grid[i][j]=skel;
	    // Now find closest point to robot.  Make sure the robot
	    // is within the radius of that point.
	    float d=dist(robot_loc_x,robot_loc_y,i,j);
	    float d2=distance_grid[i][j].distance;
	    if (d < rdist && (!location || d<=d2)){
	      rdist=d;
	      _closestx=i;
	      _closesty=j;
	      _distance=d2;
	    }
	    else if (d==rdist && 
		     (!location || d<=d2) &&
		     (d2>_distance)){	    
	      _closestx=i;
	      _closesty=j;
	      _distance=d2;
	    }
	    
	  }
	}
}


/** 
    Build the final data structure of the skeleton from the grid with
    cells marked as skeleton or occupied.
**/
void evg_thin::build_skel() {
  // Use _step2_grid because after find_skel_edge(), it will have only
  // skel, occupied, and unknown cells.  prune_skel just changes
  // things in _step2_grid.


  // Walk along the skeleton away from the closest point to the robot.
  // Walk is best-first (using total distance from start).  If a
  // branch terminates, but is not next to an unknown cell or the
  // edge, that branch must be pruned.
  if (_distance>=0) {
    //if _distance==-1, then there was no skeleton found near the
    //robot.
    crawl_grid();

    remove_final_spur();

    best_to_depth_first();
  }  
}


/**
   Starting with the closest skelton point to the robot, walk along
   the skeleton in the grid, building an intermediate skeleton data
   structure.  Do this in a best-first fashion, using cell distance as
   the cost function.  Also, if a branch ends but is not an exit (next
   to the edge or next to unknown cells in the occupancy grid), prune
   that branch back (basically, make the distance field of nodes in
   that branch equal to -1).
**/
void evg_thin::crawl_grid() {

  _tmp_skel.clear();
  
  node new_node;
  new_node.x=_closestx;
  new_node.y=_closesty;
  new_node.distance=0;
  new_node.parent=-1;
  
  heap<node> open_list;
  //Closest point is root of tree.
  open_list.push(new_node);

  _num_exits=0;
  _root_index=0;
  
  bool cont;
  vector<node> children;
  
  while (!open_list.empty()) {
    cont=true;
    node curr_node=open_list.first();
    open_list.pop();
    // If the current node has not yet been processed
    if (_step2_grid[curr_node.x][curr_node.y]==skel) {
      //Mark the node as begin processed.
      _step2_grid[curr_node.x][curr_node.y]=occupied;
      //If exit (and not the tree root) stop searching this branch
      if (is_exit(curr_node) &&
	  curr_node.parent>=0) {
	_num_exits++;
	curr_node.num_children=0;
	curr_node.radius=distance_grid[curr_node.x][curr_node.y].distance;
	//update parent with the index of this node in _tmp_skel
	_tmp_skel[curr_node.parent].children.push_back(_tmp_skel.size());
	//update _tmp_skel with this node
	_tmp_skel.push_back(curr_node);
	cont=false;
      }
      //For non-exits
      if (cont) {
	//Find neighbors
	children=find_neighbors(curr_node.x,curr_node.y);
	curr_node.num_children=children.size();
	//If there are no children (no neighbors left to process),
	//this branch is a "spur" and needs to be removed.  Remove
	//from it's parent's children.
	if (curr_node.num_children==0) {
	  //Called on parent because this node was never added to
	  //_tmp_skel.
	  remove_branch(curr_node.parent);
	}
	//For non-exits with children
	else {
	  curr_node.radius=distance_grid[curr_node.x][curr_node.y].distance;
	  int curr_index=_tmp_skel.size();
	  //If not the root, update the parent to know the index of
	  //this node.
	  if (curr_node.parent>=0)
	    _tmp_skel[curr_node.parent].children.push_back(curr_index);
	  //Add this node to the tree
	  _tmp_skel.push_back(curr_node);
	  //Now add children to the heap
	  for (uint i=0;i<curr_node.num_children;i++) {
	    new_node.x=children[i].x;
	    new_node.y=children[i].y;
	    //cost is distance (in skeleton nodes) to the root.
	    new_node.distance=
	      dist(new_node.x,new_node.y, curr_node.x,curr_node.y) +
	      curr_node.distance;
	    new_node.parent=curr_index;
	    open_list.push(new_node);
	  }
	}
      }
    }
    //If no longer available for processing, remove this node from
    //it's parent's children.
    else 
      remove_branch(curr_node.parent);
  }
}


/**
   If a node is removed, because it terminates and is not an exit,
   mark it's distance = -1, and recurse on its parent, which may now
   need to be removed as well.
**/
void evg_thin::remove_branch(int index, int child_index) {
  if (prune) 
    if (index >=0) {
      //Remove unneccessary child from list of children.
      if (child_index>=0) {
	vector<uint> new_children;
	for (uint i=0;i<_tmp_skel[index].children.size();i++)
	  if (_tmp_skel[index].children[i]!=uint(child_index))
	    new_children.push_back(_tmp_skel[index].children[i]);
	_tmp_skel[index].children=new_children;
      }
      //Reduce number of children.  Different from children.size()
      //because not all children may have been processed (thus added to
      //the tree and updated their parent's children list).
      _tmp_skel[index].num_children--;
      
      //If no more children, prune this node from it's parent's child
      //list.
      if (_tmp_skel[index].num_children==0) {
	//Set distance to -1 so that later, we'll know this is a pruned
	//node (instead of having to take it out of the tree, which may
	//be hairy---requires updating indexes of all parents and
	//chilren which _tmp_skel is rearranged).
	_tmp_skel[index].distance=-1;
	remove_branch(_tmp_skel[index].parent,index);
      }
    }
}

/**
   If a node is next to unknown nodes or an edge of the grid, it is an
   exit.
**/
bool evg_thin::is_exit(const node& curr_node) {

  int i,j,nx,ny;

  for (i=-1;i<=1;i++) {
    nx=curr_node.x+i;
    for (j=-1;j<=1;j++)
      if (i!=0 || j!=0) {
	ny=curr_node.y+j;
	if (!on_grid(nx,ny) ||
	    original_grid[nx][ny]==Unknown)
	  return true;
      }
  }
  
  return false;
}

/**
   Find the immediate neighbors, save their grid coordinates, and push
   them all into a list.
 **/
vector<node> evg_thin::find_neighbors(int x, int y) {
  vector<node> neighbors;
  
  node c1;
  int i,j,newx,newy;
  
  for (i=-1;i<=1;i++) {
    newx=x+i;
    for (j=-1;j<=1;j++) 
      if (i!=0 || j!=0) {
	newy=y+j;
	if (on_grid(newx,newy) &&
	    _step2_grid[newx][newy]==skel) {
	  c1.x=newx;
	  c1.y=newy;
	  neighbors.push_back(c1);
	}
      }
  }
  return neighbors;
}


/**
   We used the closest skeleton point to the robot to know which
   skeleton (thinning may find many) is the correct one to use.
   Sometimes, the branch that includes the closest point to the robot
   needs to be pruned.  This does that (essentially reordering the
   tree to have a new root).
 **/
void evg_thin::remove_final_spur() {
  if (prune)
    if (!_tmp_skel.empty())
      if (_num_exits > 1 && !is_exit(_tmp_skel[0]) && _tmp_skel[0].num_children==1) {
	//If not an exit and only 1 child (terminal node) and more than
	//1 exit exists (this isn't the only branch) then prune the
	//branch.
	_tmp_skel[0].distance=-1;
	remove_branch2(_tmp_skel[0].children[0]);
      }
}


/**
   Similar to remove_branch, but going down the branch instead of up
   (to prune the root of the skeleton tree).  

   If a node is removed, because it terminates and is not an exit,
   mark it's distance = -1, and recurse on its child, which may now
   need to be removed as well.
**/
void evg_thin::remove_branch2(int index) {
  if (_tmp_skel[index].num_children==1) {
    _tmp_skel[index].distance=-1;
    remove_branch2(_tmp_skel[index].children[0]);
  }
  else {
    _tmp_skel[index].parent=-1;
    _root_index=index;
  }
}

/**
   Go from the intermediate skeleton (from crawl_grid) to a fully
   pruned data structure.  While doing this, its convient (for other
   modules that may use the skeleton) to make this depth first.  Also,
   when we see exits, add them to the internal or external exists list
   (to be saved in the local topology data structure).
 **/
void evg_thin::best_to_depth_first() {
  if (!_tmp_skel.empty())
    best_to_depth_first_helper(_root_index,-1);
}


/**
   The recursive function that does the actual work of best_to_depth_first().
 **/
void evg_thin::best_to_depth_first_helper(int myindex, int parent_index){

  // Check distance (it's set to -1 for pruned nodes).  Only add
  // non-pruned cells to the final skeleton.

  if (_tmp_skel[myindex].distance>=0) {
    //Curr node is somewhere (given a depth first search) in the _tmp_skel
    node curr_node;
    curr_node.x=_tmp_skel[myindex].x;
    curr_node.y=_tmp_skel[myindex].y;
    curr_node.radius=_tmp_skel[myindex].radius;
    curr_node.distance=_tmp_skel[myindex].distance;
    curr_node.num_children=_tmp_skel[myindex].children.size();

    //parent has a new index in the depth first tree.
    curr_node.parent=parent_index;
    // Delete children indexes.  They will recreate this list when
    // they get added.
    
    uint new_index=curr_skel.size();

    //Update parents list of children to include the new index of the
    //node.
    if (parent_index>=0)
      curr_skel[parent_index].children.push_back(new_index);

    //Add to the new, depth-first tree.
    curr_skel.push_back(curr_node);

    //Do a depth first traversal of the skeleton.
    for (uint i=0;i<curr_node.num_children;i++) 
      best_to_depth_first_helper(_tmp_skel[myindex].children[i],new_index);
    
    
  }

} // end namespace EVG_THIN

}

