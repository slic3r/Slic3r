#include <limits>
#include <queue>

#include "Polyline.hpp"
#include "ConfSpace.hpp"

namespace Slic3r {

bool zorder_compare(const Point& r, const Point& l) { return r.x<l.x || (r.x==l.x && r.y<l.y); }

bool
ConfSpace::Vertex::add_edge(idx_type to, weight_type w) {
    std::pair<edges_type::iterator,bool> i;
    i=edges.insert(edges_type::value_type(to, w));
    
    if(i.second) return true;
    if(i.first->second>w) {   // decrease edge value
        i.first->second=w;
        return true;
    }
    return false;
}

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

bool
ConfSpace::insert_edge(idx_type from, idx_type to, weight_type weight) {
    bool ret=false;
    ret|=vertices[from].add_edge(to, weight);
    ret|=vertices[to].add_edge(from, weight);
}

bool
ConfSpace::insert_edge(Point& from, Point& to, weight_type weight, bool create) {
    idx_type p1=create?(insert(from).first):find(from);
    if(p1<0) return false;
    idx_type p2=create?(insert(to).first):find(to);
    if(p2<0) return false;
    return insert_edge(p1,p2,weight);
}

void
ConfSpace::points(Points* p) {
    p->reserve(p->size()+vertices.size());
    for(std::vector<Vertex>::const_iterator it = vertices.begin(); it != vertices.end(); ++it)
        p->push_back(it->point);
}

void
ConfSpace::edge_lines(Lines* l) {
    for(std::vector<Vertex>::const_iterator vertex = vertices.begin(); vertex != vertices.end(); ++vertex) {
        idx_type vertex_idx=vertex-vertices.begin();
        for(Vertex::edges_type::const_iterator it = vertex->edges.begin(); it != vertex->edges.end(); ++it)
            if(it->first > vertex_idx)
                l->push_back(Line(vertex->point, vertices[it->first].point));
    }
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


void
ConfSpace::path_init() {
    for(std::vector<Vertex>::iterator it = vertices.begin(); it != vertices.end(); ++it) {
        it->distance=infinity;
        it->parent=-1;
        it->color=Vertex::White;
    }
}

// compare distance of gray verices, in ascending order. If distance is equal, compare pointers
struct DistanceCompare { bool operator ()(const ConfSpace::Vertex* a, const ConfSpace::Vertex* b) { return (a->distance==b->distance)?(a<b):(a->distance < b->distance); } };

bool
ConfSpace::path_dijkstra(idx_type from, idx_type to, Polyline* ret) {
    path_init();  // reset all distances
    std::set<Vertex*,DistanceCompare> queue;   

    queue.insert(&vertices[from]);
    while(!queue.empty()) {
        Vertex* vmin=*queue.begin();
        queue.erase(queue.begin());
        vmin->color=Vertex::Black;
        if(vmin==&vertices[to]) { // path found, stop here
            break;
        }
        for(Vertex::edges_type::const_iterator e = vmin->edges.begin(); e != vmin->edges.end(); ++e) {
            Vertex* v=&vertices[e->first];
            weight_type vdist=vmin->distance+e->second;
            if(v->color==Vertex::White) {  // vertex is discovered first time
                v->color=Vertex::Gray;
                v->distance=vdist;
                v->parent=vmin-&*vertices.begin();
                queue.insert(v);
            } else if(v->color==Vertex::Gray && v->distance>vdist) { // found better edge, update vertex
                queue.erase(v);
                v->distance=vdist;
                v->parent=vmin-&*vertices.begin();
                queue.insert(v);
            }
                
        }
       
    }
    if(vertices[to].color==Vertex::Black) { // path found
        for(idx_type vi=to;vi>=0;vi=vertices[vi].parent) 
            ret->points.push_back(vertices[vi].point);
        ret->reverse();
        return true;
    } else {
        return false;
    }
}

bool
ConfSpace::path_dijkstra(const Point& from, const Point& to, Polyline* ret) {
    idx_type p1=find(from);
    if(p1<0) return false;
    idx_type p2=find(to);
    if(p2<0) return false;
    return path_dijkstra(p1, p2, ret);
}

#ifdef SLIC3RXS
REGISTER_CLASS(ConfSpace, "ConfSpace");
#endif

}
