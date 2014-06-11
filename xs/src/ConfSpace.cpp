#include <limits>
#include <queue>
#include <stack>

#include <poly2tri-c/p2t/poly2tri.h>
#include <poly2tri-c/refine/refine.h>

#include "Polyline.hpp"
#include "ExPolygon.hpp"
#include "Line.hpp"
#include "ConfSpace.hpp"
#include "SVG.hpp"

namespace Slic3r {

// TODO - implement Z-order compare
bool 
ConfSpace::VertexComparator::operator ()(const Point& r, const Point& l) { return r.x<l.x || (r.x==l.x && r.y<l.y); }

// find previous edge around vertex (pointing into same vertex)
ConfSpace::Edge*
ConfSpace::Edge::prev() 
{
    Edge* e=this->mirror;
    while(e->end!=end) e=e->next;  // follow the polygon
    return e;
}

void
ConfSpace::Edge::free_loop(Edge* start) 
{
    if(!start) return;
    Edge* e=start;
    do {
        Edge* enext=e->next;
        delete e;
        e=enext;
    } while(e!=start);
}

ConfSpace::ConfSpace() : infinity(std::numeric_limits<float>::max())  {
}

ConfSpace::~ConfSpace() {
    // delete all allocated data. State WILL be inconsistent during deletion.
    while(!polys.empty()) {
        Poly* p=polys.back(); polys.pop_back();
        Edge::free_loop(p->edge);
        delete p;
    }
    Edge::free_loop(outside_loop);
    outside_loop=NULL;
    
    for (vertices_index_type::const_iterator vi=vertices.begin(); vi!=vertices.end(); ++vi)
        delete vi->second;
    
    vertices.clear();
}

std::pair<ConfSpace::Vertex*,bool> 
ConfSpace::point_insert (const Point& val) {
    Vertex* v;
    if((v=point_find(val))!=NULL)
        return std::make_pair(v,false);
    v=new Vertex(val);
    vertices.insert(vertices_index_type::value_type(val, v));
    return std::make_pair(v,true);
}
       
ConfSpace::Vertex*
ConfSpace::point_find(const Point& val) {
    vertices_index_type::const_iterator it=vertices.find(val);
    if(it==vertices.end()) return NULL;    
    return it->second;
}

ConfSpace::Edge*
ConfSpace::edge_find(const Vertex* from, const Vertex* to) {
    if(!from->edge) return NULL;
    Edge* e=from->edge;
    do {
        if(e->end==to) return e;
        e=e->mirror->next;
    } while(e!=from->edge);
    return NULL;
}


// v is vertex insterted edge points to
void
ConfSpace::vertex_insert_edge(Edge* edge) {
    Vertex* v=edge->end;
    if(!v->edge) {  // first edge, just link to mirror
        v->edge=edge->mirror;
        edge->next=edge->mirror;
        return;
    }
    
    Edge* e=v->edge;                  // first edge from this vertex
    Edge* eprev;
    double a=edge->mirror->angle;     // angle of inserted edge pointing out of this vertex

    if(e->angle<a) {                  // we are the first edge
        do {
            eprev=e;
            e=e->mirror->next;
        } while(e!=v->edge);
        v->edge=edge->mirror;
    } else {                          // find last larger edge
        do {
            if(e->angle>a)
                eprev=e;          // last larger edge
            else 
                break;           // found, do not continue
            e=e->mirror->next;
        } while(e!=v->edge);
    }
    edge->next=eprev->mirror->next;
    eprev->mirror->next=edge->mirror;
}

std::pair<ConfSpace::Edge*, bool>
ConfSpace::edge_insert(Vertex* from, Vertex* to) {
    Edge* e;
    if((e=edge_find(from, to))!=NULL) 
        return std::make_pair(e, false);
    
    double a1=atan2(to->point.y-from->point.y, to->point.x-from->point.x);
    double a2=(a1<0)?(a1+PI):(a1-PI);
    Edge* e1=new Edge(to, a1);
    Edge* e2=new Edge(from, a2);
    e1->mirror=e2;
    e2->mirror=e1;
    vertex_insert_edge(e1);
    vertex_insert_edge(e2);
    return std::make_pair(e1, true);
}

void 
ConfSpace::SVG_dump(const char * fname) {
    SVG svg(fname);
    for(vertices_index_type::const_iterator it = vertices.begin(); it != vertices.end(); ++it) {
        Vertex* v=it->second;
        Edge* e=v->edge;
        if (e) {
            do {
                Point p1(e->start()->point), p2(e->end->point);
                double a=e->angle;
                p1.translate(-sin(a)*2e5, cos(a)*2e5);
                p2.translate(-sin(a)*2e5, cos(a)*2e5);
                char clr[32]; sprintf(clr, "#00%02x80", (int)floor((e->angle+PI)/(2*PI)*255));
                svg.AddLine(Line(p1, p2), clr, e==v->edge?1.5:1);
                Edge* ne=e->mirror->next;
                svg.AddLine(Line(0.7*e->start()->point+0.3*e->end->point, 0.7*ne->start()->point+0.3*ne->end->point), "Purple", 1);
                ne=e->next;
                svg.AddLine(Line(0.6*e->start()->point+0.4*e->end->point, 0.6*ne->start()->point+0.4*ne->end->point), "DarkBlue", 1);
                e=e->mirror->next;
            } while(e!=v->edge);
        } else {
            svg.AddPoint(v->point, "red",1);
        }
    }
    svg.Close();
}

void
ConfSpace::add_Polygon(const Polygon poly, int l, int r) {
    input_polygons.push_back(input_polygon_type(poly, l, r));
}

// create vertices from polygon points and append p2t adresses to dst vector
static void p2t_polygon(const MultiPoint& src, P2tPointPtrArray dst, std::vector<_P2tPoint*> *storage) {
    ::P2tPoint* p;
    for(Points::const_iterator it=src.points.begin(); it!=src.points.end();++it) {
        p=p2t_point_new_dd(it->x,it->y);
        g_ptr_array_add(dst, p);
        storage->push_back(p);
    }
}

void
ConfSpace::triangulate() {
    // preallocate storage space
    int required=0;
    for(std::vector<input_polygon_type>::const_iterator it=input_polygons.begin();it!=input_polygons.end(); ++it)
        required+=it->poly.points.size();
    std::vector< ::P2tPoint* > storage;
    storage.reserve(required);

    P2tPointPtrArray pts=g_ptr_array_new();
    
    // contour polygon must be passed to constructor
    p2t_polygon(input_polygons.front().poly, pts, &storage);
    P2tCDT* cdt=p2t_cdt_new(pts);
    
    // now add all other polygons as holes. poly2tri is modified to use them only as constrained edges, not real holes
    for(std::vector<input_polygon_type>::const_iterator it=input_polygons.begin()+1;it!=input_polygons.end(); ++it) {
        g_ptr_array_set_size(pts, 0);
        p2t_polygon(it->poly, pts, &storage);
        p2t_cdt_add_hole(cdt, pts);
    }

    g_ptr_array_free(pts, TRUE);

    p2t_cdt_triangulate(cdt);

    // now pass initial CDT to refiner
    
    P2trCDT *rcdt=p2tr_cdt_new (cdt);
    // all data was copied to cdtr, it's OK to free cdt here
    p2t_cdt_free (cdt);
    
    P2trRefiner *refiner;
    refiner = p2tr_refiner_new (G_PI / 6, p2tr_refiner_false_too_big, rcdt);
    p2tr_refiner_refine (refiner, storage.size()*20, NULL);
    p2tr_refiner_free (refiner);

    // iterate over all refined CDT triangles and add them as polygons
    P2trHashSetIter iter;
    p2tr_hash_set_iter_init(&iter, rcdt->mesh->triangles);

    P2trTriangle* tri;
    while (p2tr_hash_set_iter_next (&iter, (gpointer*)&tri)) {
        // find or create triangle points
        Vertex* tv[3];
        for(int i=0;i<3;i++) {
            ::P2trPoint* pt=P2TR_TRIANGLE_GET_POINT(tri,2-i);     // reverse points
            tv[i]=point_insert(Point(pt->c.x, pt->c.y)).first;          // find triangle vertex
        }
        // find or create triange edges
        Edge* te[3];
        for(int i=0;i<3;i++) 
            te[i]=edge_insert(tv[i],tv[(i+1)%3]).first;          // create or find edge. Both edge and mirror is created if neccesary and links are created.
        // create polygon and link it's edges together
        Poly* poly=new Poly;
        for(int i=0;i<3;i++) {
            assert(te[i]->next==te[(i+1)%3]);
            te[i]->poly=poly;
            if(i==0)
                poly->edge=te[i];                                 // link to some polygon edge
            
            te[i]->constrained=tri->edges[(4-i)%3]->constrained;
        }
        polys.push_back(poly);
    }
    // store external loop 
    // go through edges in positive X direction until we find boundary 
    Edge* e=polys.front()->edge;
    while(e->poly) {
        if(e->end->point.y>e->start()->point.y)
            e=e->mirror->next;  // go through edge and skip edge going backwards
        else
            e=e->next;          // try another edge of polygon
    }
    outside_loop=e;
    
    // mark polygons
    for(std::vector<input_polygon_type>::const_iterator it=input_polygons.begin()+1;it!=input_polygons.end(); ++it) {
        if(it->left>=0) {
            Poly* p=poly_find_left(it->poly.points[0], it->poly.points[1]);
            if(p) poly_fill_type(p, it->left);
        }
        if(it->right>=0) {
            Poly* p=poly_find_left(it->poly.points[1], it->poly.points[0]);
            if(p) poly_fill_type(p, it->right);
        }
    }
    
    p2tr_cdt_free (rcdt);
    storage.clear();
    input_polygons.clear();
    SVG_dump("tri.svg");
}

ConfSpace::Poly*
ConfSpace::poly_find_left(const Vertex* v1, const Point& v2) {
    const Line line(v1->point, v2);
    
    Edge* e=v1->edge;
    do {
        // look for edge colinear with [v1,v2] starting at v1
        if( e->end->point.projection_onto(line).coincides_with(e->end->point) )  // edge end lies on original boundary (TODO - test for colinearity only, but direction is important)
            return e->poly;
        e=e->mirror->next;   // next edge from this vertex, clockwise
    } while(e!=v1->edge);
    return NULL;
}

ConfSpace::Poly*
ConfSpace::poly_find_left(const Point& p1, const Point& p2) {
    Vertex* v1=point_find(p1); 
    if(!v1) return NULL;
    return poly_find_left(v1,p2);
}

void 
ConfSpace::poly_fill_type(Poly* first, int type) {
    std::stack<Poly*> stack;
    stack.push(first);
    while(!stack.empty()) {
        Poly *p=stack.top(); stack.pop();
        p->type=type;                            // only different type polygons are pushed except first one
        Edge* e=p->edge;
        do {
            if(!e->constrained && e->mirror->poly && e->mirror->poly->type!=type)
                stack.push(e->mirror->poly);
            e=e->next;
        } while(e!=p->edge);
    }
}

void
ConfSpace::points(Points* p) {
    p->reserve(p->size()+vertices.size());
    for(vertices_index_type::const_iterator it = vertices.begin(); it != vertices.end(); ++it)
        p->push_back(it->second->point);
}

void
ConfSpace::edge_lines(Lines* l) {
    for(std::vector<Poly*>::const_iterator it = polys.begin(); it != polys.end(); ++it) {
        Edge *e=(*it)->edge;
        do {
            if(e->poly>e->mirror->poly) // only push edges in polygon with higher address (this handles boundary edges too)
                l->push_back(Line(e->start()->point, e->end->point));
            e=e->next;
        } while(e!=(*it)->edge);
    }
}

#if 0
void 
ConfSpace::points_in_range(Point& from, float range, Points* p) {
    for(vertices_index_type::const_iterator vi = vertices.begin(); vi != vertices.end(); ++vi) 
        if(from.distance_to((*vi)->point)<range)
            p->push_back((*vi)->point);
}
#endif

ConfSpace::Vertex*
ConfSpace::vertex_nearest(const Point& from) {
    float distance=std::numeric_limits<float>::max();
    Vertex* best=NULL;
    for(vertices_index_type::const_iterator vi = vertices.begin(); vi != vertices.end(); ++vi) {
        float d=from.distance_to(vi->second->point);
        if(d<distance) {
            distance=d;
            best=vi->second;
        }
    }
    return best;
}

ConfSpace::Vertex*
ConfSpace::vertex_near(const Point& from) {
    // TODO - optimize Z-index search
    vertices_index_type::const_iterator vi=vertices.lower_bound(from);
    if(vi!=vertices.end()) 
        return vi->second;
    if(!vertices.empty())
        return vertices.begin()->second; 
    return NULL;
}


// triangle walk algorithm
// TODO - this is guarantied to work for Delaunay trianulation, but not for triangulation in general
// with randomized edge test it should converge eventually for all triangles
// check that this walk will work for polygons

ConfSpace::Poly*
ConfSpace::poly_find(const Point& p)
{
    Vertex* v=vertex_near(p);
    if(!v) return NULL;
    Edge* e=v->edge;
    Edge* estart=e;
    do {
        Point& p1(e->start()->point);
        Point& p2(e->end->point);
        if((p2.x - p1.x)*(p.y - p1.y) - (p2.y - p1.y)*(p.x - p1.x) < 0 ) {  // point is on other side of this edge, walk
            e=e->mirror;
            if(!e->poly) 
                return NULL;             // crossing convex boundary, no polygon can be there
            estart=e;
            e=e->next;                   // skip edge going back
        } else { 
            e=e->next; 
        }
    } while(e!=estart);
    // point is inside or on edge
    return e->poly;
}



void
ConfSpace::path_init() {
    for(std::vector<Poly*>::iterator it = polys.begin(); it != polys.end(); ++it) {
        (*it)->cost=(*it)->total=infinity;
        (*it)->parent=NULL;
        (*it)->color=Poly::Open;
    }
}

// compare distance of gray verices, in ascending order. If distance is equal, compare pointers
struct CostCompare { bool operator ()(const ConfSpace::Poly* a, const ConfSpace::Poly* b) { return (a->total==b->total)?(a<b):(a->total < b->total); } };

// this code is heavily inspired by Recast/Detour (https://github.com/memononen/recastnavigation/blob/master/Detour/Source/DetourNavMeshQuery.cpp)

bool
ConfSpace::path_dijkstra(const Point& from, const Point& to, std::vector<Poly*>* ret) {
    path_init();  // reset all distances
    std::set<Poly*,CostCompare> queue;   

    Poly* pfrom=poly_find(from);
    Poly* pto=poly_find(to);

    if (!pfrom || !pto) return false;

    pfrom->color=Poly::Visited;
    pfrom->entry_point=from;
    pfrom->cost=0;
    pfrom->total=from.distance_to(to);
    
    queue.insert(pfrom);
    while(!queue.empty()) {
        Poly* best=*queue.begin(); queue.erase(best);
        best->color=Poly::Closed;
        if(best==pto) {              // path found to target polygon, do last step TODO
            best->color=Poly::Closed;
            break;
        }
        Edge* edge=best->edge; 
        do {
            Poly* next=edge->mirror->poly;

            if(!next || next==best->parent) 
                continue; // don't scan out of CDT or back
            
            Point mid;
            if(next->color==Poly::Open || next->color==Poly::Visited) {
                mid=0.5*(edge->start()->point+edge->end->point);
                if(next->color==Poly::Open) {  // new polygon discovered, set it'  entry position
                    next->entry_point=mid;
                }
            } else {
                mid=next->entry_point;
            }
            cost_t cost,heuristic,total;
            if(next==pto) {  // in target polygon, use different heuristic
                cost=best->cost+(best->entry_point.distance_to(mid)+mid.distance_to(to))*std::max(best->type,1);
                heuristic=0;
            } else {
                cost=best->cost+best->entry_point.distance_to(mid)*std::max(best->type,1);
                heuristic=next->entry_point.distance_to(to);
            }
            total=cost+heuristic;
            
            if(next->color==Poly::Visited && total>=next->total) continue; // better path already exists
            if(next->color==Poly::Closed && total>=next->total) continue;  // closed node and closer anyway
            
            // update node
            if(next->color==Poly::Visited) {  // remove from queue first
                queue.erase(next);
                next->entry_point=mid;        // update entry point
            }
            next->parent=best;
            next->cost=cost;
            next->total=total;
            next->color=Poly::Visited;
            queue.insert(next);
        } while((edge=edge->next)!=best->edge);
    }
    if(pto->color==Poly::Closed) { // path found
        for(Poly* p=pto;p!=NULL;p=p->parent) {
            p->color=Poly::Path;
            ret->push_back(p);
        }
        std::reverse(ret->begin(), ret->end());
        return true;
    } else {
        return false;
    }
}



void
ConfSpace::poly_get_portal(Poly* from, Poly* to, Point* left, Point* right) {
    // TODO - there may be more edges between two polygons
    Edge* e=from->edge;
    do {
        if(e->mirror->poly==to) {
            *left=e->end->point;
            *right=e->start()->point;
            return;
        }
    } while((e=e->next)!=from->edge);
    throw "Passed polygons are not neighbors";
}

// this code is basically stripped version of dtNavMeshQuery::findStraightPath from Recast/Detour (https://github.com/memononen/recastnavigation/blob/master/Detour/Source/DetourNavMeshQuery.cpp)

void
ConfSpace::path_straight(const Point& from, const Point& to, const std::vector<Poly*>& path, Polyline* ret) {
    ret->points.push_back(from);
    if(path.size()>1) {
        Point portalApex(from), portalLeft(from), portalRight(from);
        std::vector<Poly*>::const_iterator apexIndex = path.begin();
        std::vector<Poly*>::const_iterator leftIndex = path.begin();
        std::vector<Poly*>::const_iterator rightIndex = path.begin();
        for(std::vector<Poly*>::const_iterator poly=path.begin();poly!=path.end();++poly) {            
            Point left,right;
            if(poly+1<path.end()) {
                poly_get_portal(*poly, *(poly+1), &left, &right);
                if(poly==path.begin()) {
                    // If starting really close the portal, advance
                    if(portalApex.distance_to(Line(left,right))<SCALED_EPSILON)
                        continue;
                }
            } else {
                left=right=to;
            }
            if(right.ccw(portalApex, portalRight)>=0) {
                if ((portalApex==portalRight) || right.ccw(portalApex, portalLeft) < 0) {
                    portalRight=right;
                    rightIndex = poly;
                } else {
                    portalApex=portalLeft;
                    apexIndex = leftIndex;
                    if(ret->points.back()!=portalApex)
                        ret->points.push_back(portalApex);
                    portalLeft=portalApex;
                    portalRight=portalApex;
                    leftIndex = apexIndex;
                    rightIndex = apexIndex;

                    // Restart
                    poly = apexIndex;
                    
                    continue;
                }
            }
            if(left.ccw(portalApex, portalLeft)<=0) {
                if ((portalApex==portalLeft) || left.ccw(portalApex, portalRight) > 0) {
                    portalLeft=left;
                    leftIndex = poly;
                } else {
                    portalApex=portalRight;
                    apexIndex = rightIndex;
                    if(ret->points.back()!=portalApex)
                        ret->points.push_back(portalApex);
                    portalLeft=portalApex;
                    portalRight=portalApex;
                    leftIndex = apexIndex;
                    rightIndex = apexIndex;

                    // Restart
                    poly = apexIndex;
                    
                    continue;
                }
            }
        }
    }
    if(ret->points.back()!=to)
        ret->points.push_back(to);
}

bool
ConfSpace::path(const Point& from, const Point& to, Polyline*  ret) {
    std::vector<Poly*> path_polys;
    bool found=path_dijkstra(from, to, &path_polys);
    path_straight(from, to, path_polys, ret);
    return found;
}

void
ConfSpace::SVG_dump_path(const char* fname, Point& from, Point& to, const Polyline& straight_path) {
    SVG svg(fname);
    for(std::vector<Poly*>::const_iterator pi=polys.begin();pi!=polys.end();++pi) {
        Poly* p=*pi;
        Polygon pol;
        Edge* e=p->edge;
        do {
            pol.points.push_back(e->end->point);
            e=e->next;
        } while(e!=p->edge);
        char buff[256]="";
        sprintf(buff, "id=%i", (int)(pi-polys.begin()));
        if(p->color!=Poly::Open)
            sprintf(buff+strlen(buff)," c=%.3f t=%.3f", unscale(p->cost), unscale(p->total));
        svg.AddPolygon(pol, "", (p->type>1)?"red":"green", buff);
        svg.arrows=false;
        e=p->edge;
        do {
            if(e->constrained)
                svg.AddLine(Line(e->start()->point, e->end->point), "Gold", 1);
            e=e->next;
        } while(e!=p->edge);
        std::string color;
        switch(p->color) {
        case Poly::Open: color="white"; break;
        case Poly::Visited: color="gray"; break;
        case Poly::Closed: color="black"; break;
        case Poly::Path: color="yellow"; break;
        }
        svg.AddPoint(pol.centroid(), color, 1.5, buff);
        if(p->parent) {
            svg.AddLine(Line(p->entry_point, p->parent->entry_point), "DarkBlue");
        }
    }
    svg.AddPoint(from, "DarkGreen", 3);
    svg.AddPoint(to, "DarkGreen", 3);
    Poly* pto=poly_find(to);
    if(pto) {
        Polyline pl;
        pl.points.push_back(to);
        for(Poly* p=pto;p!=NULL;p=p->parent) {
            pl.points.push_back(p->entry_point); 
        }
        svg.AddPolyline(pl, "red", .5);
    }
    svg.AddPolyline(straight_path, "DarkGreen", 3);
    svg.Close();
}

#ifdef SLIC3RXS
REGISTER_CLASS(ConfSpace, "ConfSpace");
#endif

}
