#include "ExPolygon.hpp"
#include "Polygon.hpp"
#include "ClipperUtils.hpp"
#include "evg-thin/evg-thin.hpp"
#include <stdlib.h>

namespace Slic3r {

ExPolygon::operator Polygons() const
{
    Polygons polygons;
    polygons.push_back(this->contour);
    for (Polygons::const_iterator it = this->holes.begin(); it != this->holes.end(); ++it) {
        polygons.push_back(*it);
    }
    return polygons;
}

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
ExPolygon::rotate(double angle, Point* center)
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
ExPolygon::contains_line(const Line* line) const
{
    Polylines pl(1);
    pl.push_back(*line);
    
    Polylines pl_out;
    diff(pl, *this, pl_out);
    return pl_out.empty();
}

bool
ExPolygon::contains_point(const Point* point) const
{
    if (!this->contour.contains_point(point)) return false;
    for (Polygons::const_iterator it = this->holes.begin(); it != this->holes.end(); ++it) {
        if (it->contains_point(point)) return false;
    }
    return true;
}

Polygons
ExPolygon::simplify_p(double tolerance) const
{
    Polygons pp(this->holes.size() + 1);
    
    // contour
    Polygon p = this->contour;
    p.points = MultiPoint::_douglas_peucker(p.points, tolerance);
    pp.push_back(p);
    
    // holes
    for (Polygons::const_iterator it = this->holes.begin(); it != this->holes.end(); ++it) {
        p = *it;
        p.points = MultiPoint::_douglas_peucker(p.points, tolerance);
        pp.push_back(p);
    }
    simplify_polygons(pp, pp);
    return pp;
}

ExPolygons
ExPolygon::simplify(double tolerance) const
{
    Polygons pp = this->simplify_p(tolerance);
    ExPolygons expp;
    union_(pp, expp);
    return expp;
}

void
ExPolygon::simplify(double tolerance, ExPolygons &expolygons) const
{
    ExPolygons ep = this->simplify(tolerance);
    expolygons.reserve(expolygons.size() + ep.size());
    expolygons.insert(expolygons.end(), ep.begin(), ep.end());
}

void
ExPolygon::medial_axis(Polylines* polylines) const
{
    // clone this expolygon and scale it down
    ExPolygon scaled_this = *this;
    #define MEDIAL_AXIS_SCALE 0.0001
    scaled_this.scale(MEDIAL_AXIS_SCALE);
    
    // get our bounding box and compute our size
    BoundingBox bb(scaled_this);
    Size size = bb.size();
    
    // init grid to be as large as our size
    EVG_THIN::grid_type grid;
    grid.resize(size.x + 1);
    for (coord_t x=0; x <= size.x; x++) {
        grid[x].resize(size.y + 1);
        for (coord_t y=0; y <= size.y; y++)
            grid[x][y] = EVG_THIN::Free;
    }
    
    // draw polygons
    Polygons pp = scaled_this;
    for (Polygons::const_iterator p = pp.begin(); p != pp.end(); ++p) {
        Lines lines = p->lines();
        for (Lines::iterator line = lines.begin(); line != lines.end(); ++line) {
            coord_t x1 = line->a.x - bb.min.x;
            coord_t y1 = line->a.y - bb.min.y;
            coord_t x2 = line->b.x - bb.min.x;
            coord_t y2 = line->b.y - bb.min.y;
            
            // Bresenham's line algorithm
            const bool steep = (abs(y2 - y1) > abs(x2 - x1));
            if (steep) {
                std::swap(x1, y1);
                std::swap(x2, y2);
            }
            if (x1 > x2) {
                std::swap(x1, x2);
                std::swap(y1, y2);
            }

            const double dx = x2 - x1;
            const double dy = abs(y2 - y1);
            double error = dx / 2.0f;
            const coord_t ystep = (y1 < y2) ? 1 : -1;
            coord_t y = (coord_t)y1;
            const coord_t maxX = (coord_t)x2;

            for (coord_t x = (coord_t)x1;  x < maxX; x++) {
                if (steep) {
                    grid[y][x] = EVG_THIN::Occupied;
                } else {
                    grid[x][y] = EVG_THIN::Occupied;
                }

                error -= dy;
                if (error < 0) {
                    y += ystep;
                    error += dx;
                }
            }
        }
    }
    printf("Drawn\n");
    // compute skeleton
    EVG_THIN::evg_thin thin(grid, 0.0, FLT_MAX, false, false, -1, -1);
    EVG_THIN::skeleton_type skel = thin.generate_skeleton();
    
    // emulate a lambda function to allow recursion
    struct PolyBuilder {
        EVG_THIN::skeleton_type skel;
        Polylines* polylines;
        BoundingBox bb;
        
        void follow (size_t node_id) {
            EVG_THIN::node* node = &this->skel.at(node_id);
            Polyline polyline;
            polyline.points.push_back(Point(node->x + this->bb.min.x, node->y + this->bb.min.y));
            while (node->children.size() == 1) {
                node = &this->skel.at(node->children.front());
                polyline.points.push_back(Point(node->x + this->bb.min.x, node->y + this->bb.min.y));
            }
            if (polyline.is_valid()) this->polylines->push_back(polyline);
            if (node->children.size() > 1) {
                for (std::vector<unsigned int>::const_iterator child_id = node->children.begin(); child_id != node->children.end(); ++child_id)
                    this->follow(*child_id);
            }
        }
    } pb;
    pb.skel      = skel;
    pb.polylines = polylines;
    pb.bb        = bb;
    
    // build polylines
    for (EVG_THIN::skeleton_type::const_iterator node = skel.begin(); node != skel.end(); ++node) {
        printf("[%zu] x = %d, y = %d, parent = %d, children count = %zu\n", node - skel.begin(), node->x, node->y, node->parent, node->children.size());
        if (node->parent == -1)
            pb.follow(node - skel.begin());
    }
    for (Polylines::iterator it = polylines->begin(); it != polylines->end(); ++it)
        it->scale(1/MEDIAL_AXIS_SCALE);
}

BoundingBox
ExPolygon::bounding_box() const
{
    return BoundingBox(*this);
}

#ifdef SLIC3RXS
SV*
ExPolygon::to_AV() {
    const unsigned int num_holes = this->holes.size();
    AV* av = newAV();
    av_extend(av, num_holes);  // -1 +1
    
    av_store(av, 0, this->contour.to_SV_ref());
    
    for (unsigned int i = 0; i < num_holes; i++) {
        av_store(av, i+1, this->holes[i].to_SV_ref());
    }
    return newRV_noinc((SV*)av);
}

SV*
ExPolygon::to_SV_ref() {
    SV* sv = newSV(0);
    sv_setref_pv( sv, "Slic3r::ExPolygon::Ref", this );
    return sv;
}

SV*
ExPolygon::to_SV_clone_ref() const {
    SV* sv = newSV(0);
    sv_setref_pv( sv, "Slic3r::ExPolygon", new ExPolygon(*this) );
    return sv;
}

SV*
ExPolygon::to_SV_pureperl() const
{
    const unsigned int num_holes = this->holes.size();
    AV* av = newAV();
    av_extend(av, num_holes);  // -1 +1
    av_store(av, 0, this->contour.to_SV_pureperl());
    for (unsigned int i = 0; i < num_holes; i++) {
        av_store(av, i+1, this->holes[i].to_SV_pureperl());
    }
    return newRV_noinc((SV*)av);
}

void
ExPolygon::from_SV(SV* expoly_sv)
{
    AV* expoly_av = (AV*)SvRV(expoly_sv);
    const unsigned int num_polygons = av_len(expoly_av)+1;
    this->holes.resize(num_polygons-1);
    
    SV** polygon_sv = av_fetch(expoly_av, 0, 0);
    this->contour.from_SV(*polygon_sv);
    for (unsigned int i = 0; i < num_polygons-1; i++) {
        polygon_sv = av_fetch(expoly_av, i+1, 0);
        this->holes[i].from_SV(*polygon_sv);
    }
}

void
ExPolygon::from_SV_check(SV* expoly_sv)
{
    if (sv_isobject(expoly_sv) && (SvTYPE(SvRV(expoly_sv)) == SVt_PVMG)) {
        // a XS ExPolygon was supplied
        *this = *(ExPolygon *)SvIV((SV*)SvRV( expoly_sv ));
    } else {
        // a Perl arrayref was supplied
        this->from_SV(expoly_sv);
    }
}
#endif

}
