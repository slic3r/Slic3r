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


/**
   heap.hh was originally written by Jefferson Provost.
   
   Defines a minheap template class for priority queues, etc.
   Insertions and deletions are O(log(N)).
   
   heap<T,Compare> on any comparible class T.  Compare should be a
   binary comparison functor implementing the comparison function for
   ordering the heap.  It defaults to greater<T> (which implements a
   minheap.  (yes, greater<T> => minheap.  -jp)
   
   Example usage:
   
   heap<int> minh;  -- creates a minheap of integers
   
   heap<float, less<float> > -- creates a maxheap of floats
   
   heap<foo> -- creates a minheap of elements of type foo, foo must
   have > and = operators defined.
   
   
   See the class definition below for the public operations on heaps.
**/


#ifndef _HEAP_HH_
#define _HEAP_HH_

#include <vector>
#include <functional>
#include <algorithm>
using namespace std;

template <typename T, typename Compare = greater<T> >
class heap 
{

public:

  const T& first(void) const;    // returns the first (min) item in the heap
  const T& nth(int n) const;    // returns the nth item in the heap
  void push(const T& item);      // adds an item to the heap
  void pop(void);                // removes the first item from the heap
  int size(void) const;          // returns the number of items in the heap
  bool empty(void) const;        // returns true if the heap is empty
  void clear(void);              // removes all elements

protected:
  Compare _comp;
  vector<T> _store;

};


template <class T,class Compare>
bool heap<T,Compare>::empty(void) const
{
  return this->size() == 0;
}

template <class T,class Compare>
int heap<T,Compare>::size(void) const
{
  return _store.size();
}

template <class T,class Compare>
void heap<T,Compare>::push(const T& item)
{
  _store.push_back(item);

  push_heap(_store.begin(), _store.end(), _comp);

}

template <class T,class Compare>
void heap<T,Compare>::pop(void)
{
  if (!empty()) {
    pop_heap(_store.begin(), _store.end(), _comp);
    _store.pop_back();
  }
}

template <class T,class Compare>
const T& heap<T,Compare>::first(void) const
{
  return _store[0];
}

template <class T,class Compare>
const T& heap<T,Compare>::nth(int n) const
{
  if (n >= size())
    n=size()-1;
  return _store[n];
}

template<class T, class Compare>
void heap<T,Compare>::clear(void)
{
  _store.clear();
}

#endif // _HEAP_HH_  
