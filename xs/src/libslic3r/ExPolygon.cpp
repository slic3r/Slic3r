#include "BoundingBox.hpp"
#include "ExPolygon.hpp"
#include "Geometry.hpp"
#include "Polygon.hpp"
#include "Line.hpp"
#include "ClipperUtils.hpp"
#include "polypartition.h"
#include "poly2tri/poly2tri.h"
#include <algorithm>
#include <cassert>
#include <list>

namespace Slic3r {

ExPolygon::operator Points() const
{
    Points points;
    Polygons pp = *this;
    for (Polygons::const_iterator poly = pp.begin(); poly != pp.end(); ++poly) {
        for (Points::const_iterator point = poly->points.begin(); point != poly->points.end(); ++point)
            points.push_back(*point);
    }
    return points;
}

ExPolygon::operator Polygons() const
{
    Polygons polygons;
    polygons.reserve(this->holes.size() + 1);
    polygons.push_back(this->contour);
    for (Polygons::const_iterator it = this->holes.begin(); it != this->holes.end(); ++it) {
        polygons.push_back(*it);
    }
    return polygons;
}

void
ExPolygon::scale(double factor)
{
    contour.scale(factor);
    for (Polygons::iterator it = holes.begin(); it != holes.end(); ++it) {
        (*it).scale(factor);
    }
}

void
ExPolygon::translate(double x, double y)
{
    contour.translate(x, y);
    for (Polygons::iterator it = holes.begin(); it != holes.end(); ++it) {
        (*it).translate(x, y);
    }
}

void
ExPolygon::rotate(double angle)
{
    contour.rotate(angle);
    for (Polygons::iterator it = holes.begin(); it != holes.end(); ++it) {
        (*it).rotate(angle);
    }
}

void
ExPolygon::rotate(double angle, const Point &center)
{
    contour.rotate(angle, center);
    for (Polygons::iterator it = holes.begin(); it != holes.end(); ++it) {
        (*it).rotate(angle, center);
    }
}

double
ExPolygon::area() const
{
    double a = this->contour.area();
    for (Polygons::const_iterator it = this->holes.begin(); it != this->holes.end(); ++it) {
        a -= -(*it).area();  // holes have negative area
    }
    return a;
}

bool
ExPolygon::is_valid() const
{
    if (!this->contour.is_valid() || !this->contour.is_counter_clockwise()) return false;
    for (Polygons::const_iterator it = this->holes.begin(); it != this->holes.end(); ++it) {
        if (!(*it).is_valid() || (*it).is_counter_clockwise()) return false;
    }
    return true;
}

bool
ExPolygon::contains(const Line &line) const
{
    return this->contains((Polyline)line);
}

bool
ExPolygon::contains(const Polyline &polyline) const
{
    return diff_pl((Polylines)polyline, *this).empty();
}

bool
ExPolygon::contains(const Point &point) const
{
    if (!this->contour.contains(point)) return false;
    for (Polygons::const_iterator it = this->holes.begin(); it != this->holes.end(); ++it) {
        if (it->contains(point)) return false;
    }
    return true;
}

// inclusive version of contains() that also checks whether point is on boundaries
bool
ExPolygon::contains_b(const Point &point) const
{
    return this->contains(point) || this->has_boundary_point(point);
}

bool
ExPolygon::has_boundary_point(const Point &point) const
{
    if (this->contour.has_boundary_point(point)) return true;
    for (Polygons::const_iterator h = this->holes.begin(); h != this->holes.end(); ++h) {
        if (h->has_boundary_point(point)) return true;
    }
    return false;
}

void
ExPolygon::remove_colinear_points()
{
    this->contour.remove_collinear_points();
    for (Polygon &p : this->holes)
        p.remove_collinear_points();
}

void
ExPolygon::remove_vertical_collinear_points(coord_t tolerance)
{
    this->contour.remove_vertical_collinear_points(tolerance);
    for (Polygon &p : this->holes)
        p.remove_vertical_collinear_points(tolerance);
}

void
ExPolygon::simplify_p(double tolerance, Polygons* polygons) const
{
    Polygons pp = this->simplify_p(tolerance);
    polygons->insert(polygons->end(), pp.begin(), pp.end());
}

Polygons
ExPolygon::simplify_p(double tolerance) const
{
    Polygons pp;
    pp.reserve(this->holes.size() + 1);
    
    // contour
    {
        Polygon p = this->contour;
        p.points.push_back(p.points.front());
        p.points = MultiPoint::_douglas_peucker(p.points, tolerance);
        p.points.pop_back();
        pp.push_back(p);
    }
    
    // holes
    for (Polygons::const_iterator it = this->holes.begin(); it != this->holes.end(); ++it) {
        Polygon p = *it;
        p.points.push_back(p.points.front());
        p.points = MultiPoint::_douglas_peucker(p.points, tolerance);
        p.points.pop_back();
        pp.push_back(p);
    }
    return simplify_polygons(pp);
}

ExPolygons
ExPolygon::simplify(double tolerance) const
{
    Polygons pp = this->simplify_p(tolerance);
    return union_ex(pp);
}

void
ExPolygon::simplify(double tolerance, ExPolygons* expolygons) const
{
    ExPolygons ep = this->simplify(tolerance);
    expolygons->insert(expolygons->end(), ep.begin(), ep.end());
}

void
ExPolygon::medial_axis(const ExPolygon &bounds, double max_width, double min_width, ThickPolylines* polylines) const
{
    // init helper object
    Slic3r::Geometry::MedialAxis ma(max_width, min_width, this);
    ma.lines = this->lines();
    
    // compute the Voronoi diagram and extract medial axis polylines
    ThickPolylines pp;
    ma.build(&pp);
    
    /* // Commented out debug code
    SVG svg("medial_axis.svg");
    svg.draw(*this);
    svg.draw(pp);
    svg.Close();
    */
    
    // Find the maximum width returned; we're going to use this for validating and 
    // filtering the output segments.
    double max_w = 0;
    for (ThickPolylines::const_iterator it = pp.begin(); it != pp.end(); ++it)
        max_w = fmaxf(max_w, *std::max_element(it->width.begin(), it->width.end()));
    
    
    //  Aligned fusion: Fusion the bits at the end of lines by "increasing thickness"
    //  For that, we have to find other lines,
    //  and with a next point no more distant than the max width.
    //  Then, we can merge the bit from the first point to the second by following the mean.

    bool changes = true;
    while (changes) {
        changes = false;
        for (size_t i = 0; i < pp.size(); ++i) {
            ThickPolyline& polyline = pp[i];

            ThickPolyline* best_candidate = nullptr;
            float best_dot = -1;
            int best_idx = 0;

            // find another polyline starting here
            for (size_t j = i + 1; j < pp.size(); ++j) {
                ThickPolyline& other = pp[j];
                if (polyline.last_point().coincides_with(other.last_point())) {
                    polyline.reverse();
                    other.reverse();
                }
                else if (polyline.first_point().coincides_with(other.last_point())) {
                    other.reverse();
                }
                else if (polyline.first_point().coincides_with(other.first_point())) {
                }
                else if (polyline.last_point().coincides_with(other.first_point())) {
                    polyline.reverse();
                } else {
                    continue;
                }

                //only consider the other if the next point is near us
                if (polyline.points.size() < 2 && other.points.size() < 2) continue;
                if (!polyline.endpoints.second || !other.endpoints.second) continue;
                if (polyline.points.back().distance_to(other.points.back()) > max_width) continue;
                if (polyline.points.size() != other.points.size()) continue;


                Pointf v_poly(polyline.lines().front().vector().x, polyline.lines().front().vector().y);
                v_poly.scale(1 / std::sqrt(v_poly.x*v_poly.x + v_poly.y*v_poly.y));
                Pointf v_other(other.lines().front().vector().x, other.lines().front().vector().y);
                v_other.scale(1 / std::sqrt(v_other.x*v_other.x + v_other.y*v_other.y));
                float other_dot = v_poly.x*v_other.x + v_poly.y*v_other.y;
                if (other_dot > best_dot) {
                    best_candidate = &other;
                    best_idx = j;
                    best_dot = other_dot;
                }
            }
            if (best_candidate != nullptr) {

                //assert polyline.size == best_candidate->size (see selection loop, an 'if' takes care of that)

                //iterate the points
                // as voronoi should create symmetric thing, we can iterate synchonously
                unsigned int idx_point = 1;
                while (idx_point < polyline.points.size() && polyline.points[idx_point].distance_to(best_candidate->points[idx_point]) < max_width) {
                    //fusion
                    polyline.points[idx_point].x += best_candidate->points[idx_point].x;
                    polyline.points[idx_point].x /= 2;
                    polyline.points[idx_point].y += best_candidate->points[idx_point].y;
                    polyline.points[idx_point].y /= 2;
                    polyline.width[idx_point] += best_candidate->width[idx_point];
                    ++idx_point;
                }
                //select if an end occur
                polyline.endpoints.second &= best_candidate->endpoints.second;

                //remove points that are the same or too close each other, ie simplify
                for (unsigned int idx_point = 1; idx_point < polyline.points.size(); ++idx_point) {
                    //distance of 1 is on the sclaed coordinates, so it correspond to SCALE_FACTOR, so it's very small
                    if (polyline.points[idx_point - 1].distance_to(polyline.points[idx_point]) < 1) {
                        if (idx_point < polyline.points.size() -1) {
                            polyline.points.erase(polyline.points.begin() + idx_point);
                        } else {
                            polyline.points.erase(polyline.points.begin() + idx_point -1);
                        }
                        --idx_point;
                    }
                }
                //remove points that are outside of the geometry
                for (unsigned int idx_point = 0; idx_point < polyline.points.size(); ++idx_point) {
                    //distance of 1 is on the sclaed coordinates, so it correspond to SCALE_FACTOR, so it's very small
                    if (!bounds.contains_b(polyline.points[idx_point])) {
                        polyline.points.erase(polyline.points.begin() + idx_point);
                        --idx_point;
                    }
                }
                if (polyline.points.size() < 2) {
                    //remove self
                    pp.erase(pp.begin() + i);
                    --i;
                    --best_idx;
                }

                pp.erase(pp.begin() + best_idx);
                changes = true;
            }
        }
    }
    
    // Loop through all returned polylines in order to extend their endpoints to the 
    //   expolygon boundaries
    for (size_t i = 0; i < pp.size(); ++i) {
        ThickPolyline& polyline = pp[i];
        
        // extend initial and final segments of each polyline if they're actual endpoints
        // Assign new endpoints to temporary variables because in case of a single-line
        // polyline. After the start point is extended it will be caught by the intersection()
        // call, so keep the inner point until the second intersection() is performed.
        Point new_front = polyline.points.front();
        Point new_back  = polyline.points.back();
        if (polyline.endpoints.first && !bounds.has_boundary_point(new_front)) {
            Line line(polyline.points.front(), polyline.points[1]);
            
            // prevent the line from touching on the other side, otherwise intersection() might return that solution
            if (polyline.points.size() == 2) line.b = line.midpoint();
            
            line.extend_start(max_width);
            (void)bounds.contour.intersection(line, &new_front);
        }
        if (polyline.endpoints.second && !bounds.has_boundary_point(new_back)) {
            Line line(
                *(polyline.points.end() - 2),
                polyline.points.back()
            );
            
            // prevent the line from touching on the other side, otherwise intersection() might return that solution
            if (polyline.points.size() == 2) line.a = line.midpoint();
            line.extend_end(max_width);
            
            (void)bounds.contour.intersection(line, &new_back);
        }
        polyline.points.front() = new_front;
        polyline.points.back()  = new_back;
    }
    
    // If we removed any short polylines, we now try to connect consecutive polylines
    // in order to allow loop detection. Note that this algorithm is greedier than 
    // MedialAxis::process_edge_neighbors(), as it will connect random pairs of 
    // polylines even when more than two start from the same point. This has no 
    // drawbacks since we optimize later using nearest-neighbor which would do the 
    // same, but should we use a more sophisticated optimization algorithm.
    // We should not connect polylines when more than two meet. 
    // Optimisation of the old algorithm : Select the most "straight line" choice 
    // when we merge with an other line at a point with more than two meet.
        
    for (size_t i = 0; i < pp.size(); ++i) {
        ThickPolyline& polyline = pp[i];
        if (polyline.endpoints.first && polyline.endpoints.second) continue; // optimization
        
        ThickPolyline* best_candidate = nullptr;
        float best_dot = -1;
        int best_idx = 0;
        
        // find another polyline starting here
        for (size_t j = i+1; j < pp.size(); ++j) {
            ThickPolyline& other = pp[j];
            if (polyline.last_point().coincides_with(other.last_point())) {
                other.reverse();
            } else if (polyline.first_point().coincides_with(other.last_point())) {
                polyline.reverse();
                other.reverse();
            } else if (polyline.first_point().coincides_with(other.first_point())) {
                polyline.reverse();
            } else if (!polyline.last_point().coincides_with(other.first_point())) {
                continue;
            }
            
            Pointf v_poly(polyline.lines().back().vector().x, polyline.lines().back().vector().y);
            v_poly.scale(1 / std::sqrt(v_poly.x*v_poly.x + v_poly.y*v_poly.y));
            Pointf v_other(other.lines().front().vector().x, other.lines().front().vector().y);
            v_other.scale(1 / std::sqrt(v_other.x*v_other.x + v_other.y*v_other.y));
            float other_dot = v_poly.x*v_other.x + v_poly.y*v_other.y;
            if (other_dot > best_dot) {
                best_candidate = &other;
                best_idx = j;
                best_dot = other_dot;
            }
        }
        if (best_candidate != nullptr) {

            polyline.points.insert(polyline.points.end(), best_candidate->points.begin() + 1, best_candidate->points.end());
            polyline.width.insert(polyline.width.end(), best_candidate->width.begin(), best_candidate->width.end());
            polyline.endpoints.second = best_candidate->endpoints.second;
            assert(polyline.width.size() == polyline.points.size()*2 - 2);
                
            pp.erase(pp.begin() + best_idx);
        }
    }
    
    for (size_t i = 0; i < pp.size(); ++i) {
        ThickPolyline& polyline = pp[i];

        //  remove too short polylines
        // (we can't do this check before endpoints extension and clipping because we don't
        // know how long will the endpoints be extended since it depends on polygon thickness
        // which is variable - extension will be <= max_width/2 on each side)
        if ((polyline.endpoints.first || polyline.endpoints.second)
            && polyline.length() < max_w * 2) {
            pp.erase(pp.begin() + i);
            --i;
            continue;
        }

    }

    
    polylines->insert(polylines->end(), pp.begin(), pp.end());
}

void
ExPolygon::medial_axis(double max_width, double min_width, Polylines* polylines) const
{
    ThickPolylines tp;
    this->medial_axis(*this, max_width, min_width, &tp);
    polylines->insert(polylines->end(), tp.begin(), tp.end());
}

void
ExPolygon::get_trapezoids(Polygons* polygons) const
{
    ExPolygons expp;
    expp.push_back(*this);
    boost::polygon::get_trapezoids(*polygons, expp);
}

void
ExPolygon::get_trapezoids(Polygons* polygons, double angle) const
{
    ExPolygon clone = *this;
    clone.rotate(PI/2 - angle, Point(0,0));
    clone.get_trapezoids(polygons);
    for (Polygons::iterator polygon = polygons->begin(); polygon != polygons->end(); ++polygon)
        polygon->rotate(-(PI/2 - angle), Point(0,0));
}

// This algorithm may return more trapezoids than necessary
// (i.e. it may break a single trapezoid in several because
// other parts of the object have x coordinates in the middle)
void
ExPolygon::get_trapezoids2(Polygons* polygons) const
{
    // get all points of this ExPolygon
    Points pp = *this;
    
    // build our bounding box
    BoundingBox bb(pp);
    
    // get all x coordinates
    std::vector<coord_t> xx;
    xx.reserve(pp.size());
    for (Points::const_iterator p = pp.begin(); p != pp.end(); ++p)
        xx.push_back(p->x);
    std::sort(xx.begin(), xx.end());
    
    // find trapezoids by looping from first to next-to-last coordinate
    for (std::vector<coord_t>::const_iterator x = xx.begin(); x != xx.end()-1; ++x) {
        coord_t next_x = *(x + 1);
        if (*x == next_x) continue;
        
        // build rectangle
        Polygon poly;
        poly.points.resize(4);
        poly[0].x = *x;
        poly[0].y = bb.min.y;
        poly[1].x = next_x;
        poly[1].y = bb.min.y;
        poly[2].x = next_x;
        poly[2].y = bb.max.y;
        poly[3].x = *x;
        poly[3].y = bb.max.y;
        
        // intersect with this expolygon
        Polygons trapezoids = intersection(poly, *this);
        
        // append results to return value
        polygons->insert(polygons->end(), trapezoids.begin(), trapezoids.end());
    }
}

void
ExPolygon::get_trapezoids2(Polygons* polygons, double angle) const
{
    ExPolygon clone = *this;
    clone.rotate(PI/2 - angle, Point(0,0));
    clone.get_trapezoids2(polygons);
    for (Polygons::iterator polygon = polygons->begin(); polygon != polygons->end(); ++polygon)
        polygon->rotate(-(PI/2 - angle), Point(0,0));
}

// While this triangulates successfully, it's NOT a constrained triangulation
// as it will create more vertices on the boundaries than the ones supplied.
void
ExPolygon::triangulate(Polygons* polygons) const
{
    // first make trapezoids
    Polygons trapezoids;
    this->get_trapezoids2(&trapezoids);
    
    // then triangulate each trapezoid
    for (Polygons::iterator polygon = trapezoids.begin(); polygon != trapezoids.end(); ++polygon)
        polygon->triangulate_convex(polygons);
}

void
ExPolygon::triangulate_pp(Polygons* polygons) const
{
    // convert polygons
    std::list<TPPLPoly> input;
    
    ExPolygons expp = simplify_polygons_ex(*this, true);
    
    for (ExPolygons::const_iterator ex = expp.begin(); ex != expp.end(); ++ex) {
        // contour
        {
            TPPLPoly p;
            p.Init(ex->contour.points.size());
            //printf("%zu\n0\n", ex->contour.points.size());
            for (Points::const_iterator point = ex->contour.points.begin(); point != ex->contour.points.end(); ++point) {
                p[ point-ex->contour.points.begin() ].x = point->x;
                p[ point-ex->contour.points.begin() ].y = point->y;
                //printf("%ld %ld\n", point->x, point->y);
            }
            p.SetHole(false);
            input.push_back(p);
        }
    
        // holes
        for (Polygons::const_iterator hole = ex->holes.begin(); hole != ex->holes.end(); ++hole) {
            TPPLPoly p;
            p.Init(hole->points.size());
            //printf("%zu\n1\n", hole->points.size());
            for (Points::const_iterator point = hole->points.begin(); point != hole->points.end(); ++point) {
                p[ point-hole->points.begin() ].x = point->x;
                p[ point-hole->points.begin() ].y = point->y;
                //printf("%ld %ld\n", point->x, point->y);
            }
            p.SetHole(true);
            input.push_back(p);
        }
    }
    
    // perform triangulation
    std::list<TPPLPoly> output;
    int res = TPPLPartition().Triangulate_MONO(&input, &output);
    if (res != 1) CONFESS("Triangulation failed");
    
    // convert output polygons
    for (std::list<TPPLPoly>::iterator poly = output.begin(); poly != output.end(); ++poly) {
        long num_points = poly->GetNumPoints();
        Polygon p;
        p.points.resize(num_points);
        for (long i = 0; i < num_points; ++i) {
            p.points[i].x = (*poly)[i].x;
            p.points[i].y = (*poly)[i].y;
        }
        polygons->push_back(p);
    }
}

void
ExPolygon::triangulate_p2t(Polygons* polygons) const
{
    for (const ExPolygon &ex : simplify_polygons_ex(*this, true)) {
        // contour
        std::vector<p2t::Point*> ContourPoints;
        
        Polygon contour = ex.contour;
        contour.remove_duplicate_points();
        for (const Point &point : contour.points) {
            // We should delete each p2t::Point object
            ContourPoints.push_back(new p2t::Point(point.x, point.y));
        }
        p2t::CDT cdt(ContourPoints);

        // holes
        for (Polygon hole : ex.holes) {
            hole.remove_duplicate_points();
            std::vector<p2t::Point*> points;
            Point prev = hole.points.back();
            for (Point &point : hole.points) {
                // Shrink large polygons by reducing each coordinate by 1 in the
                // general direction of the last point as we wind around
                // This normally wouldn't work in every case, but our upscaled polygons
                // have little chance to create new duplicate points with this method.
                // For information on why this was needed, see:
                //    https://code.google.com/p/poly2tri/issues/detail?id=90
                //    https://github.com/raptor/clip2tri
                (point.x - prev.x) > 0 ? point.x-- : point.x++;
                (point.y - prev.y) > 0 ? point.y-- : point.y++;
                prev = point;
                
                // will be destructed in SweepContext::~SweepContext
                points.push_back(new p2t::Point(point.x, point.y));
            }
            cdt.AddHole(points);
        }
        
        // perform triangulation
        cdt.Triangulate();
        std::vector<p2t::Triangle*> triangles = cdt.GetTriangles();
        
        for (p2t::Triangle* triangle : triangles) {
            Polygon p;
            for (int i = 0; i <= 2; ++i) {
                p2t::Point* point = triangle->GetPoint(i);
                p.points.push_back(Point(point->x, point->y));
            }
            polygons->push_back(p);
        }
        
        for (p2t::Point* it : ContourPoints)
            delete it;
    }
}

Lines
ExPolygon::lines() const
{
    Lines lines = this->contour.lines();
    for (Polygons::const_iterator h = this->holes.begin(); h != this->holes.end(); ++h) {
        Lines hole_lines = h->lines();
        lines.insert(lines.end(), hole_lines.begin(), hole_lines.end());
    }
    return lines;
}

std::string
ExPolygon::dump_perl() const
{
    std::ostringstream ret;
    ret << "[" << this->contour.dump_perl();
    for (Polygons::const_iterator h = this->holes.begin(); h != this->holes.end(); ++h)
        ret << "," << h->dump_perl();
    ret << "]";
    return ret.str();
}

std::ostream&
operator <<(std::ostream &s, const ExPolygons &expolygons)
{
    for (const ExPolygon &e : expolygons)
        s << e.dump_perl() << std::endl;
    return s;
}

}
