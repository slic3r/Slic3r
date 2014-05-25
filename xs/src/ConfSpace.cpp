#include <limits>
#include <queue>

#include "Polyline.hpp"
#include "Line.hpp"
#include "ConfSpace.hpp"

namespace Slic3r {

bool zorder_compare(const Point& r, const Point& l) { return r.x<l.x || (r.x==l.x && r.y<l.y); }

ConfSpace::ConfSpace() : infinity(std::numeric_limits<weight_type>::max())  {
}

std::pair<ConfSpace::idx_type,bool> 
ConfSpace::insert (const Point& val) {
    idx_type idx;
    if((idx=find(val))>=0)
        return std::make_pair(idx,false);
    vertices.push_back(Vertex(val));
    idx=vertices.size()-1;
    index.insert(index_type::value_type(val, idx));
    return std::make_pair(idx,true);
}
       
ConfSpace::idx_type 
ConfSpace::find(const Point& val) {
    index_type::const_iterator i;
    i=index.find(val);
    if(i==index.end()) return -1;    
    return i->second;
}

void
ConfSpace::points(Points* p) {
    p->reserve(p->size()+vertices.size());
    for(std::vector<Vertex>::const_iterator it = vertices.begin(); it != vertices.end(); ++it)
        p->push_back(it->point);
}

void
ConfSpace::edge_lines(Lines* l) {
    // TODO - remove duplicate lines
    for(std::vector<Poly>::const_iterator poly = polys.begin(); poly != polys.end(); ++poly) 
        for(int i=0;i<poly->vertCount;i++) 
            l->push_back(Line(vertices[poly->verts[i]].point, vertices[poly->verts[(i+1)%poly->vertCount]].point));
}

void 
ConfSpace::points_in_range(Point& from, float range, Points* p) {
    for(std::vector<Vertex>::const_iterator vertex = vertices.begin(); vertex != vertices.end(); ++vertex) 
        if(from.distance_to(vertex->point)<range)
            p->push_back(vertex->point);
}

ConfSpace::idx_type 
ConfSpace::nearest_point(Point& from) {
    float distance=std::numeric_limits<float>::max();
    std::vector<Vertex>::const_iterator best=vertices.end();
    for(std::vector<Vertex>::const_iterator vertex = vertices.begin(); vertex != vertices.end(); ++vertex) {
        float d=from.distance_to(vertex->point);
        if(d<distance) {
            distance=d;
            best=vertex;
        }
    }
    if(best==vertices.end()) return -1;
    return best-vertices.begin();
}

ConfSpace::idx_type 
ConfSpace::find_poly(const Point& p) 
{
    Poly* poly=&polys[0];
new_poly:
    for(int i=0;i<poly->vertCount;i++) {
        Point& p1=vertices[poly->verts[i]].point;
        Point& p2=vertices[poly->verts[(i+1)%poly->vertCount]].point;
        if((p2.x - p1.x)*(p.y - p1.y) - (p2.y - p1.y)*(p.x - p1.x) > 0 ) {  // point is on other side of this edge, walk
            idx_type nxt=poly->neis[i];
            if(nxt<0) return -1; // no polygon behind this edge
            poly=&polys[nxt];
            goto new_poly;
        }
    }
    // point is inside or on edge
    return poly-&polys[0];
}



void
ConfSpace::path_init() {
    for(std::vector<Poly>::iterator it = polys.begin(); it != polys.end(); ++it) {
        it->cost=it->total=infinity;
        it->parent=NULL;
        it->color=Poly::White;
    }
}

// compare distance of gray verices, in ascending order. If distance is equal, compare pointers
struct CostCompare { bool operator ()(const ConfSpace::Poly* a, const ConfSpace::Poly* b) { return (a->total==b->total)?(a<b):(a->total < b->total); } };

bool
ConfSpace::path_dijkstra(const Point& from, const Point& to, Polyline* ret) {
    path_init();  // reset all distances
    std::set<Poly*,CostCompare> queue;   

    idx_type pfrom=find_poly(from);
    idx_type pto=find_poly(to);

    queue.insert(&polys[pfrom]);
    while(!queue.empty()) {
        Poly* best=*queue.begin();
        queue.erase(best);
        best->color=Poly::Black;
        if(best==&polys[pto]) { // path found to target polygon, do last step TODO
            best->color=Poly::Black;
            break;
        }
        Poly* parent=best->parent;
 
        for(int i=0;i<best->vertCount;i++) {
            Poly* next=&polys[best->neis[i]];
            if(next==parent) continue;  // don't go back
            if(next->color==Poly::White) {  // new polygon discovered, compute it's position
                Point mid=0.5*(vertices[i].point+vertices[(i+1)%best->vertCount].point);
                next->entry_point=mid;
            }
            // todo - check for target polygon
            weight_type cost=best->cost+best->entry_point.distance_to(next->entry_point);
            weight_type heuristic=best->entry_point.distance_to(to);

            weight_type total=cost+heuristic;
            
            if(next->color==Poly::Gray && total>=next->total) continue; // better path already exists
            if(next->color==Poly::Black && total>=next->total) continue; // closed node and closer anyway
            
            // update node
            if(next->color==Poly::Gray) {  // remove from queue first
                queue.erase(next);
            }
            next->parent=best;
            next->cost=cost;
            next->total=total;
            next->color=Poly::Gray;
            queue.insert(next);
        }
    }
    if(polys[pto].color==Poly::Black) { // path found
        for(Poly* p=&polys[pto];p!=NULL;p=p->parent) 
            ret->points.push_back(p->entry_point);
        ret->reverse();
        return true;
    } else {
        return false;
    }
}

#ifdef SLIC3RXS
REGISTER_CLASS(ConfSpace, "ConfSpace");
#endif

}
