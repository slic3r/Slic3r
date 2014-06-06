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
ConfSpace::add_Polygon(const Polygon poly, int r, int l) {
    tri_polygons.push_back(tri_polygon_type(poly, r,l));
}

void
ConfSpace::triangulate() {
    P2tPointPtrArray pts=g_ptr_array_new();
    int required=0;
    for(std::vector<tri_polygon_type>::const_iterator it=tri_polygons.begin();it!=tri_polygons.end(); ++it)
        required+=it->poly.points.size();
    std::vector< ::P2tPoint*> storage;
    storage.reserve(required);


    p2t_polygon(tri_polygons.front().poly, pts, &storage);
    P2tCDT* cdt=p2t_cdt_new(pts);

    for(std::vector<tri_polygon_type>::const_iterator it=tri_polygons.begin()+1;it!=tri_polygons.end(); ++it) {
        g_ptr_array_set_size(pts, 0);
        p2t_polygon(it->poly, pts, &storage);
        p2t_cdt_add_hole(cdt, pts);
    }

    g_ptr_array_free(pts, TRUE);

    p2t_cdt_triangulate(cdt);

    P2trCDT *rcdt=p2tr_cdt_new (cdt);
    p2t_cdt_free (cdt);
    
    P2trRefiner *refiner;
    refiner = p2tr_refiner_new (G_PI / 6, p2tr_refiner_false_too_big, rcdt);
    p2tr_refiner_refine (refiner, storage.size()*20, NULL);
    p2tr_refiner_free (refiner);

    P2trHashSetIter iter;
    p2tr_hash_set_iter_init(&iter, rcdt->mesh->triangles);

    P2trTriangle* tri;
    while (p2tr_hash_set_iter_next (&iter, (gpointer*)&tri)) {
        Poly poly;
        idx_type tv[3];
        poly.vertCount=3;
        for(int i=0;i<3;i++) {
            ::P2trPoint* pt=P2TR_TRIANGLE_GET_POINT(tri,2-i);     // reverse points
            tv[i]=insert(Point(pt->c.x, pt->c.y)).first;          // find triangle vertex
            poly.verts[i]=tv[i];                                  // store point as polygon vertex
        }
        // fill neighbor (list)
        for(int i=0;i<3;i++) {
            idx_type v1i=tv[i];
            Vertex& v1=vertices[v1i];            
            idx_type v2i=tv[(i+1)%3];
            poly.neis[i]=-1; // will be updated below
            poly.constrained[i]=tri->edges[(4-i)%3]->constrained;
            for(std::vector<idx_type>::const_iterator neib=v1.polys.begin();neib!=v1.polys.end();++neib) {
                for(int j1=0;j1<polys[*neib].vertCount;++j1) {
                    idx_type j2=(j1+1)%polys[*neib].vertCount;
                    if(polys[*neib].verts[j2]==v1i && polys[*neib].verts[j1]==v2i) {  // edge must have opposite orientation in neighbor polygon (both have same direction)
                        poly.neis[i]=*neib;
                        polys[*neib].neis[j1]=polys.size();
                        // TODO - we are done, only one neighbour possible
                    }
                }
            }
            v1.polys.push_back(polys.size());
        }
        polys.push_back(poly);
    }
    // mark polygons
    for(std::vector<tri_polygon_type>::const_iterator it=tri_polygons.begin()+1;it!=tri_polygons.end(); ++it) {
        if(it->left>=0) {
            idx_type tri=poly_find_left(it->poly.points[0], it->poly.points[1]);
            if(tri>0) polys_set_class(tri, it->left);
        }
        if(it->right>=0) {
            idx_type tri=poly_find_left(it->poly.points[1], it->poly.points[0]);
            if(tri>0) polys_set_class(tri, it->right);
        }
    }
    
    p2tr_cdt_free (rcdt);
    storage.clear();
    tri_polygons.clear();
}

ConfSpace::idx_type
ConfSpace::poly_find_left(idx_type v1, idx_type v2) {
    const Line edge(vertices[v1].point, vertices[v2].point);
    
    for(std::vector<idx_type>::iterator it=vertices[v1].polys.begin();it!=vertices[v1].polys.end();++it) {
        Poly& poly(polys[*it]);
        for(int i=0;i<poly.vertCount;i++)
            if(poly.verts[i]==v1) {
                Point pp2(vertices[poly.verts[(i+1)%poly.vertCount]].point);                
                // look for edge colinear with [v1,v2] starting at v1
                if( pp2.projection_onto(edge).coincides_with(pp2) )  // next point is on search edge
                    return &poly-&polys.front();
                else 
                    break; // not looking for this polygon
            }
    }
    return -1;
}


ConfSpace::idx_type 
ConfSpace::poly_find_left(const Point& p1, const Point& p2) {
    idx_type v1=find(p1); 
    if(v1<0) return -1;
    idx_type v2=find(p2);
    if(v2<0) return -1;
    return poly_find_left(v1,v2);
}

void 
ConfSpace::polys_set_class(idx_type first, int type) {
    std::stack<idx_type> stack;
    stack.push(first);
    while(!stack.empty()) {
        Poly &p(polys[stack.top()]); stack.pop();
        if(p.type==type) continue;
        p.type=type;
        for(int i=0;i<p.vertCount;i++)
            if(!p.constrained[i]) stack.push(p.neis[i]);
    }
}

void
ConfSpace::points(Points* p) {
    p->reserve(p->size()+vertices.size());
    for(std::vector<Vertex>::const_iterator it = vertices.begin(); it != vertices.end(); ++it)
        p->push_back(it->point);
}

void
ConfSpace::edge_lines(Lines* l) {
    for(std::vector<Poly>::const_iterator poly = polys.begin(); poly != polys.end(); ++poly) 
        for(int i=0;i<poly->vertCount;i++) 
            if(poly->neis[i]<poly-polys.begin())  // emit only one edge from bigger oly, no neighbor case also hadled here
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
    if(polys.empty()) return -1;
    Poly* poly=&polys[0];
new_poly:
    for(int i=0;i<poly->vertCount;i++) {
        Point& p1=vertices[poly->verts[i]].point;
        Point& p2=vertices[poly->verts[(i+1)%poly->vertCount]].point;
        if((p2.x - p1.x)*(p.y - p1.y) - (p2.y - p1.y)*(p.x - p1.x) < 0 ) {  // point is on other side of this edge, walk
            idx_type nxt=poly->neis[i];
            if(nxt<0) 
                return -1; // no polygon behind this edge
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
        it->color=Poly::Open;
    }
}

// compare distance of gray verices, in ascending order. If distance is equal, compare pointers
struct CostCompare { bool operator ()(const ConfSpace::Poly* a, const ConfSpace::Poly* b) { return (a->total==b->total)?(a<b):(a->total < b->total); } };

// this code is heavily inspired by Recast/Detour (https://github.com/memononen/recastnavigation/blob/master/Detour/Source/DetourNavMeshQuery.cpp)

bool
ConfSpace::path_dijkstra(const Point& from, const Point& to, std::vector<idx_type>* ret) {
    path_init();  // reset all distances
    std::set<Poly*,CostCompare> queue;   

    idx_type pfrom=find_poly(from);
    idx_type pto=find_poly(to);

    if (pfrom<0 || pto<0) return false;
    {
        Poly* first=&polys[pfrom];
        first->color=Poly::Visited;
        first->entry_point=from;
        first->cost=0;
        first->total=from.distance_to(to);
    }

    queue.insert(&polys[pfrom]);
    while(!queue.empty()) {
        Poly* best=*queue.begin(); queue.erase(best);
        idx_type besti=best-&polys.front();
        best->color=Poly::Closed;
        if(best==&polys[pto]) { // path found to target polygon, do last step TODO
            best->color=Poly::Closed;
            break;
        }
        Poly* parent=best->parent;
 
        for(int i=0;i<best->vertCount;i++) {
            idx_type nexti=best->neis[i];
            if(nexti<0) continue; // no polygon behind this edge
            Poly* next=&polys[nexti];
            if(next==parent) continue;  // don't go back
            Point mid;
            if(next->color==Poly::Open || next->color==Poly::Visited) {
                int j;
                for(j=0;j<next->vertCount;j++) 
                    if(next->neis[j]==besti) break;
                mid=0.5*(vertices[next->verts[j]].point+vertices[next->verts[(j+1)%best->vertCount]].point);
                if(next->color==Poly::Open) {  // new polygon discovered, compute it's position
                    next->entry_point=mid;
                } 
            } else {
                mid=next->entry_point;
            }
            weight_type cost,heuristic;
            if(nexti==pto) { // in target polygon, use different heuristic
                cost=best->cost+(best->entry_point.distance_to(mid)+mid.distance_to(to))*std::max(best->type,1);
                heuristic=0;
            } else {
                cost=best->cost+best->entry_point.distance_to(mid)*std::max(best->type,1);
                heuristic=next->entry_point.distance_to(to);
            }
            weight_type total=cost+heuristic;
            
            if(next->color==Poly::Visited && total>=next->total) continue; // better path already exists
            if(next->color==Poly::Closed && total>=next->total) continue; // closed node and closer anyway
            
            // update node
            if(next->color==Poly::Visited) {  // remove from queue first
                queue.erase(next);
                next->entry_point=mid;
            }
            next->parent=best;
            next->cost=cost;
            next->total=total;
            next->color=Poly::Visited;
            queue.insert(next);
        }
    }
    if(polys[pto].color==Poly::Closed) { // path found
        for(Poly* p=&polys[pto];p!=NULL;p=p->parent) {
            p->color=Poly::Path;
            ret->push_back(p-&polys.front());
        }
        std::reverse(ret->begin(), ret->end());
        return true;
    } else {
        return false;
    }
}



void
ConfSpace::poly_get_portal(idx_type from, idx_type to, Point* left, Point* right) {
    Poly& p=polys[from];
    // TODO - there may be more edges between two polygons
    for(int i=0;i<p.vertCount;++i) {
        if(p.neis[i]==to) {
            *left=vertices[p.verts[i]].point;
            *right=vertices[p.verts[(i+1)%p.vertCount]].point;
            return;
        }
    }
    throw "Passed polygons are not neighbors";
}

// this code is basically stripped version of dtNavMeshQuery::findStraightPath from Recast/Detour (https://github.com/memononen/recastnavigation/blob/master/Detour/Source/DetourNavMeshQuery.cpp)

void
ConfSpace::path_straight(const Point& from, const Point& to, const std::vector<idx_type>& path, Polyline* ret) {
    ret->points.push_back(from);
    if(path.size()>1) {
        Point portalApex(from), portalLeft(from), portalRight(from);
        std::vector<idx_type>::const_iterator apexIndex = path.begin();
        std::vector<idx_type>::const_iterator leftIndex = path.begin();
        std::vector<idx_type>::const_iterator rightIndex = path.begin();
        for(std::vector<idx_type>::const_iterator poly=path.begin();poly!=path.end();++poly) {            
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
            if(right.ccw(portalApex, portalRight)<=0) {
                if ((portalApex==portalRight) || right.ccw(portalApex, portalLeft) > 0) {
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
            if(left.ccw(portalApex, portalLeft)>=0) {
                if ((portalApex==portalLeft) || left.ccw(portalApex, portalRight) < 0) {
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
    std::vector<idx_type> path_polys;
    bool found=path_dijkstra(from, to, &path_polys);
    path_straight(from, to, path_polys, ret);
    return found;
}

void
ConfSpace::SVG_dump_path(const char* fname, Point& from, Point& to, const Polyline& straight_path) {
    SVG svg(fname);
    for(std::vector<Poly>::const_iterator p=polys.begin();p!=polys.end();++p) {
        Polygon pol;
        pol.points.reserve(p->vertCount);
        for(int i=0;i<p->vertCount;++i)
            pol.points.push_back(vertices[p->verts[i]].point);
        char buff[256]="";
        sprintf(buff, "id=%d", p-polys.begin());
        if(p->color!=Poly::Open)
            sprintf(buff+strlen(buff)," c=%.3f t=%.3f", unscale(p->cost), unscale(p->total));
        svg.AddPolygon(pol, "", (p->type>1)?"red":"green", buff);
        svg.arrows=false;
        for(int i=0;i<p->vertCount;++i)
            if(p->constrained[i])
                svg.AddLine(Line(vertices[p->verts[i]].point, vertices[p->verts[(i+1)%p->vertCount]].point), "Yellow", 1);
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
    idx_type pto=find_poly(to);
    if(pto>0) {
        Polyline pl;
        pl.points.push_back(to);
        for(Poly* p=&polys[pto];p!=NULL;p=p->parent) {
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
