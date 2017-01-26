#include "../ClipperUtils.hpp"
#include "../ExPolygon.hpp"
#include "../PolylineCollection.hpp"
#include "../Surface.hpp"
#include <algorithm>
#include <cmath>

#include "FillRectilinear.hpp"

//#define DEBUG_RECTILINEAR
#ifdef DEBUG_RECTILINEAR
    #include "../SVG.hpp"
#endif

namespace Slic3r {

void
FillRectilinear::_fill_single_direction(ExPolygon expolygon,
    const direction_t &direction, coord_t x_shift, Polylines* out)
{
    // rotate polygons so that we can work with vertical lines here
    expolygon.rotate(-direction.first);
    
    assert(this->density > 0.0001f && this->density <= 1.f);
    const coord_t min_spacing   = scale_(this->min_spacing);
    coord_t line_spacing        = (double) min_spacing / this->density;
    
    // We ignore this->bounding_box because it doesn't matter; we're doing align_to_grid below.
    BoundingBox bounding_box    = expolygon.contour.bounding_box();
    
    // Due to integer rounding, rotated polygons might not preserve verticality
    // (i.e. when rotating by PI/2 two points having the same x coordinate 
    // they might get different y coordinates), thus the first line will be skipped.
    bounding_box.offset(-1);
    
    // define flow spacing according to requested density
    if (this->density > 0.9999f && !this->dont_adjust) {
        line_spacing = this->adjust_solid_spacing(bounding_box.size().x, line_spacing);
        this->_spacing = unscale(line_spacing);
    } else {
        // extend bounding box so that our pattern will be aligned with other layers
        // Transform the reference point to the rotated coordinate system.
        Point p = direction.second.rotated(-direction.first);
        p.x -= x_shift >= 0 ? x_shift : (x_shift + line_spacing);
        bounding_box.min.align_to_grid(
            Point(line_spacing, line_spacing), 
            p
        );
    }
    
    // Find all the polygons points intersecting the rectilinear vertical lines and store
    // them in an std::map<> (grid) which orders them automatically by x and y.
    // For each intersection point we store its position (upper/lower): upper means it's
    // the upper endpoint of an intersection line, and vice versa.
    // Whenever between two intersection points we find vertices of the original polygon,
    // store them in the 'skipped' member of the latter point.
    
    grid_t grid;
    {
        const Polygons polygons = expolygon;
        for (Polygons::const_iterator polygon = polygons.begin(); polygon != polygons.end(); ++polygon) {
            const Points &points = polygon->points;
            
            // This vector holds the original polygon vertices found after the last intersection
            // point. We'll flush it as soon as we find the next intersection point.
            Points skipped_points;
            
            // This vector holds the coordinates of the intersection points found while
            // looping through the polygon.
            Points ips;
            
            for (Points::const_iterator p = points.begin(); p != points.end(); ++p) {
                const Point &prev  = p == points.begin()   ? *(points.end()-1) : *(p-1);
                const Point &next  = p == points.end()-1   ? *points.begin()   : *(p+1);
                
                // Does the p-next line belong to an intersection line?
                if (p->x == next.x && ((p->x - bounding_box.min.x) % line_spacing) == 0) {
                    if (p->y == next.y) continue;  // skip coinciding points
                    vertical_t &v = grid[p->x];
                    
                    // Detect line direction.
                    IntersectionPoint::ipType p_type = IntersectionPoint::ipTypeLower;
                    IntersectionPoint::ipType n_type = IntersectionPoint::ipTypeUpper;
                    if (p->y > next.y) std::swap(p_type, n_type);  // line goes downwards
                    
                    // Do we already have 'p' in our grid?
                    vertical_t::iterator pit = v.find(p->y);
                    if (pit != v.end()) {
                        // Yes, we have it. If its not of the same type, it means it's
                        // an intermediate point of a longer line. We store this information
                        // for now and we'll remove it later.
                        if (pit->second.type != p_type)
                            pit->second.type = IntersectionPoint::ipTypeMiddle;
                    } else {
                        // Store the point.
                        IntersectionPoint ip(p->x, p->y, p_type);
                        v[p->y] = ip;
                        ips.push_back(ip);
                    }
                    
                    // Do we already have 'next' in our grid?
                    pit = v.find(next.y);
                    if (pit != v.end()) {
                        // Yes, we have it. If its not of the same type, it means it's
                        // an intermediate point of a longer line. We store this information
                        // for now and we'll remove it later.
                        if (pit->second.type != n_type)
                            pit->second.type = IntersectionPoint::ipTypeMiddle;
                    } else {
                        // Store the point.
                        IntersectionPoint ip(next.x, next.y, n_type);
                        v[next.y] = ip;
                        ips.push_back(ip);
                    }
                    continue;
                }
                
                // We're going to look for intersection points within this line.
                // First, let's sort its x coordinates regardless of the original line direction.
                const coord_t min_x = std::min(p->x, next.x);
                const coord_t max_x = std::max(p->x, next.x);
                
                // Now find the leftmost intersection point belonging to the line.
                const coord_t min_x2 = bounding_box.min.x + ceil((double) (min_x - bounding_box.min.x) / (double)line_spacing) * (double)line_spacing;
                assert(min_x2 >= min_x);
                
                // In case this coordinate does not belong to this line, we have no intersection points.
                if (min_x2 > max_x) {
                    // Store the two skipped points and move on.
                    skipped_points.push_back(*p);
                    skipped_points.push_back(next);
                    continue;
                }
                
                // Find the rightmost intersection point belonging to the line.
                const coord_t max_x2 = bounding_box.min.x + floor((double) (max_x - bounding_box.min.x) / (double) line_spacing) * (double)line_spacing;
                assert(max_x2 <= max_x);
                
                // We're now going past the first point, so save it.
                const bool line_goes_right = next.x > p->x;
                if (line_goes_right ? (p->x < min_x2) : (p->x > max_x2))
                    skipped_points.push_back(*p);
                
                // Now loop through those intersection points according the original direction
                // of the line (because we need to store them in this order).
                for (coord_t x = line_goes_right ? min_x2 : max_x2;
                    x >= min_x && x <= max_x;
                    x += line_goes_right ? +line_spacing : -line_spacing) {
                    
                    // Is this intersection an endpoint of the original line *and* is the
                    // intersection just a tangent point? If so, just skip it.
                    if (x == p->x && ((prev.x > x && next.x > x) || (prev.x < x && next.x < x))) {
                        skipped_points.push_back(*p);
                        continue;
                    }
                    if (x == next.x) {
                        const Point &next2 = p == (points.end()-2) ? *points.begin()
                                           : p == (points.end()-1) ? *(points.begin()+1) : *(p+2);
                        if ((p->x > x && next2.x > x) || (p->x < x && next2.x < x)) {
                            skipped_points.push_back(next);
                            continue;
                        }
                    }
                    
                    // Calculate the y coordinate of this intersection.
                    IntersectionPoint ip(
                        x,
                        p->y + double(next.y - p->y) * double(x - p->x) / double(next.x - p->x),
                        line_goes_right ? IntersectionPoint::ipTypeLower : IntersectionPoint::ipTypeUpper
                    );
                    vertical_t &v = grid[ip.x];
                    
                    // Did we already find this point?
                    // (We might have found it as the endpoint of a vertical line.)
                    {
                        vertical_t::iterator pit = v.find(ip.y);
                        if (pit != v.end()) {
                            // Yes, we have it. If its not of the same type, it means it's
                            // an intermediate point of a longer line. We store this information
                            // for now and we'll remove it later.
                            if (pit->second.type != ip.type)
                                pit->second.type = IntersectionPoint::ipTypeMiddle;
                            continue;
                        }
                    }
                    
                    // Store the skipped polygon vertices along with this point.
                    ip.skipped = skipped_points;
                    skipped_points.clear();
                    
                    #ifdef DEBUG_RECTILINEAR
                    printf("NEW POINT at %f,%f\n", unscale(ip.x), unscale(ip.y));
                    for (Points::const_iterator it = ip.skipped.begin(); it != ip.skipped.end(); ++it)
                        printf("  skipped: %f,%f\n", unscale(it->x), unscale(it->y));
                    #endif
                    
                    // Store the point.
                    v[ip.y] = ip;
                    ips.push_back(ip);
                }
                
                // We're now going past the final point, so save it.
                if (line_goes_right ? (next.x > max_x2) : (next.x < min_x2))
                    skipped_points.push_back(next);
            }
            
            if (!this->dont_connect) {
                // We'll now build connections between the vertical intersection lines.
                // Each intersection point will be connected to the first intersection point
                // found along the original polygon having a greater x coordinate (or the same
                // x coordinate: think about two vertical intersection lines having the same x
                // separated by a hole polygon: we'll connect them with the hole portion).
                // We will sweep only from left to right, so we only need to build connections
                // in this direction.
                for (Points::const_iterator it = ips.begin(); it != ips.end(); ++it) {
                    IntersectionPoint &ip   = grid[it->x][it->y];
                    IntersectionPoint &next = it == ips.end()-1 ? grid[ips.begin()->x][ips.begin()->y] : grid[(it+1)->x][(it+1)->y];
                    
                    #ifdef DEBUG_RECTILINEAR
                    printf("CONNECTING %f,%f to %f,%f\n",
                        unscale(ip.x), unscale(ip.y),
                        unscale(next.x), unscale(next.y)
                    );
                    #endif
                    
                    // We didn't flush the skipped_points vector after completing the loop above:
                    // it now contains the polygon vertices between the last and the first
                    // intersection points.
                    if (it == ips.begin())
                        ip.skipped.insert(ip.skipped.begin(), skipped_points.begin(), skipped_points.end());
                
                    if (ip.x <= next.x) {
                        // Link 'ip' to 'next' --->
                        if (ip.next.empty()) {
                            ip.next = next.skipped;
                            ip.next.push_back(next);
                        }
                    } else if (next.x < ip.x) {
                        // Link 'next' to 'ip' --->
                        if (next.next.empty()) {
                            next.next = next.skipped;
                            std::reverse(next.next.begin(), next.next.end());
                            next.next.push_back(ip);
                        }
                    }
                }
            }
            
            // Do some cleanup: remove the 'skipped' points we used for building 
            // connections and also remove the middle intersection points.
            for (Points::const_iterator it = ips.begin(); it != ips.end(); ++it) {
                vertical_t &v = grid[it->x];
                IntersectionPoint &ip = v[it->y];
                ip.skipped.clear();
                if (ip.type == IntersectionPoint::ipTypeMiddle)
                    v.erase(it->y);
            }
        }
    }
    
    #ifdef DEBUG_RECTILINEAR
    SVG svg("grid.svg");
    svg.draw(expolygon);
    
    printf("GRID:\n");
    for (grid_t::const_iterator it = grid.begin(); it != grid.end(); ++it) {
        printf("x = %f:\n", unscale(it->first));
        for (vertical_t::const_iterator v = it->second.begin(); v != it->second.end(); ++v) {
            const IntersectionPoint &ip = v->second;
            printf("   y = %f (%s, next = %f,%f, extra = %zu)\n", unscale(v->first),
                ip.type == IntersectionPoint::ipTypeLower ? "lower"
                : ip.type == IntersectionPoint::ipTypeMiddle ? "middle" : "upper",
                (ip.next.empty() ? -1 : unscale(ip.next.back().x)),
                (ip.next.empty() ? -1 : unscale(ip.next.back().y)),
                (ip.next.empty() ? 0  : ip.next.size()-1)
                );
            svg.draw(ip, ip.type == IntersectionPoint::ipTypeLower ? "blue"
                : ip.type == IntersectionPoint::ipTypeMiddle ? "yellow" : "red");
        }
    }
    printf("\n");
    
    svg.Close();
    #endif
    
    // Store the number of polygons already existing in the output container.
    const size_t n_polylines_out_old = out->size();
    
    // Loop until we have no more vertical lines available.
    while (!grid.empty()) {
        // Get the first x coordinate.
        vertical_t &v = grid.begin()->second;
        
        // If this x coordinate does not have any y coordinate, remove it.
        if (v.empty()) {
            grid.erase(grid.begin());
            continue;
        }
        
        // We expect every x coordinate to contain an even number of y coordinates 
        // because they are the endpoints of vertical intersection lines:
        // lower/upper, lower/upper etc.
        assert(v.size() % 2 == 0);
        
        // Get the first lower point.
        vertical_t::iterator it = v.begin();  // minimum x,y
        IntersectionPoint p = it->second;
        assert(p.type == IntersectionPoint::ipTypeLower);
        
        // Start our polyline.
        Polyline polyline;
        polyline.append(p);
        polyline.points.back().y -= this->endpoints_overlap;
        
        while (true) {
            // Complete the vertical line by finding the corresponding upper or lower point.
            if (p.type == IntersectionPoint::ipTypeUpper) {
                // find first point along c.x with y < c.y
                assert(it != grid[p.x].begin());
                --it;
            } else {
                // find first point along c.x with y > c.y
                ++it;
                assert(it != grid[p.x].end());
            }
            
            // Append the point to our polyline.
            IntersectionPoint b = it->second;
            assert(b.type != p.type);
            polyline.append(b);
            polyline.points.back().y += this->endpoints_overlap * (b.type == IntersectionPoint::ipTypeUpper ? 1 : -1);

            // Remove the two endpoints of this vertical line from the grid.
            {
                vertical_t &v = grid[p.x];
                v.erase(p.y);
                v.erase(it);
                if (v.empty()) grid.erase(p.x);
            }
            // Do we have a connection starting from here?
            // If not, stop the polyline.
            if (b.next.empty())
                break;
            
            // If we have a connection, append it to the polyline.
            // We apply the y extension to the whole connection line. This works well when
            // the connection is straight and horizontal, but doesn't work well when the
            // connection is articulated and also has vertical parts.
            {
                // TODO: here's where we should check for overextrusion. We should only add
                // connection points while they are not generating vertical lines within the
                // extrusion thickness of the main vertical lines. We should also check whether
                // a previous run of this method occupied this polygon portion (derived infill
                // patterns doing multiple runs at different angles generate overlapping connections).
                // In both cases, we should just stop the connection and break the polyline here.
                const size_t n = polyline.points.size();
                polyline.append(b.next);
                for (Points::iterator pit = polyline.points.begin()+n; pit != polyline.points.end(); ++pit)
                    pit->y += this->endpoints_overlap * (b.type == IntersectionPoint::ipTypeUpper ? 1 : -1);
            }
            
            // Is the final point still available?
            if (grid.count(b.next.back().x) == 0
                || grid[b.next.back().x].count(b.next.back().y) == 0)
                // We already used this point or we might have removed this
                // point while building the grid because it's collinear (middle); in either
                // cases the connection line from the previous one is legit and worth having.
                break;
            
            // Retrieve the intersection point. The next loop will find the correspondent
            // endpoint of the vertical line.
            it = grid[ b.next.back().x ].find(b.next.back().y);
            p  = it->second;
            
            // If the connection brought us to another x coordinate, we expect the point 
            // type to be the same.
            assert((p.type == b.type && p.x > b.x)
                || (p.type != b.type && p.x == b.x));
        }
        
        // Yay, we have a polyline!
        if (polyline.is_valid())
            out->push_back(polyline);
    }
    
    // paths must be rotated back
    for (Polylines::iterator it = out->begin() + n_polylines_out_old;
        it != out->end(); ++it)
        it->rotate(direction.first);
}

void FillRectilinear::_fill_surface_single(
    unsigned int                    thickness_layers,
    const direction_t               &direction,
    ExPolygon                       &expolygon,
    Polylines*                      out)
{
    this->_fill_single_direction(expolygon, direction, 0, out);
}

void FillGrid::_fill_surface_single(
    unsigned int                    thickness_layers,
    const direction_t               &direction,
    ExPolygon                       &expolygon,
    Polylines*                      out)
{
    FillGrid fill2 = *this;
    fill2.density /= 2.;
    
    direction_t direction2 = direction;
    direction2.first += PI/2;
    fill2._fill_single_direction(expolygon, direction,  0, out);
    fill2.dont_connect = true;
    fill2._fill_single_direction(expolygon, direction2, 0, out);
}

void FillTriangles::_fill_surface_single(
    unsigned int                    thickness_layers,
    const direction_t               &direction,
    ExPolygon                       &expolygon,
    Polylines*                      out)
{
    FillTriangles fill2 = *this;
    fill2.density /= 3.;
    direction_t direction2 = direction;
    
    fill2._fill_single_direction(expolygon, direction2, 0, out);
    
    fill2.dont_connect = true;
    direction2.first += PI/3;
    fill2._fill_single_direction(expolygon, direction2, 0, out);
    
    direction2.first += PI/3;
    fill2._fill_single_direction(expolygon, direction2, 0, out);
}

void FillStars::_fill_surface_single(
    unsigned int                    thickness_layers,
    const direction_t               &direction,
    ExPolygon                       &expolygon,
    Polylines*                      out)
{
    FillStars fill2 = *this;
    fill2.density /= 3.;
    direction_t direction2 = direction;
    
    fill2._fill_single_direction(expolygon, direction2, 0, out);
    
    fill2.dont_connect = true;
    direction2.first += PI/3;
    fill2._fill_single_direction(expolygon, direction2, 0, out);
    
    direction2.first += PI/3;
    const coord_t x_shift = 0.5 * scale_(fill2.min_spacing) / fill2.density;
    fill2._fill_single_direction(expolygon, direction2, x_shift, out);
}

void FillCubic::_fill_surface_single(
    unsigned int                    thickness_layers,
    const direction_t               &direction,
    ExPolygon                       &expolygon,
    Polylines*                      out)
{
    FillCubic fill2 = *this;
    fill2.density /= 3.;
    direction_t direction2 = direction;
    
    const coord_t range = scale_(this->min_spacing / this->density);
    const coord_t x_shift = abs(( (coord_t)(scale_(this->z) + range) % (coord_t)(range * 2)) - range);
    
    fill2._fill_single_direction(expolygon, direction2, -x_shift, out);
    
    fill2.dont_connect = true;
    direction2.first += PI/3;
    fill2._fill_single_direction(expolygon, direction2, +x_shift, out);
    
    direction2.first += PI/3;
    fill2._fill_single_direction(expolygon, direction2, -x_shift, out);
}

} // namespace Slic3r
