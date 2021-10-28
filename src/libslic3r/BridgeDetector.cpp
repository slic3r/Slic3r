#include "BridgeDetector.hpp"
#include "ClipperUtils.hpp"
#include "Geometry.hpp"
#include <algorithm>

namespace Slic3r {

BridgeDetector::BridgeDetector(
    ExPolygon         _expolygon,
    const ExPolygons &_lower_slices, 
    coord_t           _spacing) :
    // The original infill polygon, not inflated.
    expolygons(expolygons_owned),
    // All surfaces of the object supporting this region.
    lower_slices(_lower_slices),
    spacing(_spacing)
{
    this->expolygons_owned.push_back(std::move(_expolygon));
    initialize();
}

BridgeDetector::BridgeDetector(
    const ExPolygons  &_expolygons,
    const ExPolygons  &_lower_slices,
    coord_t            _spacing) : 
    // The original infill polygon, not inflated.
    expolygons(_expolygons),
    // All surfaces of the object supporting this region.
    lower_slices(_lower_slices),
    spacing(_spacing)
{
    initialize();
}

void BridgeDetector::initialize()
{
    // 2 degrees stepping
    this->resolution = PI/(90); 
    // output angle not known
    this->angle = -1.;

    // Outset our bridge by an arbitrary amout; we'll use this outer margin for detecting anchors.
    Polygons grown = offset(to_polygons(this->expolygons), float(this->spacing));
    
    // Detect possible anchoring edges of this bridging region.
    // Detect what edges lie on lower slices by turning bridge contour and holes
    // into polylines and then clipping them with each lower slice's contour.
    // Currently _edges are only used to set a candidate direction of the bridge (see bridge_direction_candidates()).
    Polygons contours;
    contours.reserve(this->lower_slices.size());
    for (const ExPolygon &expoly : this->lower_slices)
        contours.push_back(expoly.contour);
    this->_edges = intersection_pl(to_polylines(grown), contours);
    
    #ifdef SLIC3R_DEBUG
    printf("  bridge has %zu support(s)\n", this->_edges.size());
    #endif
    
    // detect anchors as intersection between our bridge expolygon and the lower slices
    // safety offset required to avoid Clipper from detecting empty intersection while Boost actually found some edges
    this->_anchor_regions = intersection_ex(grown, to_polygons(this->lower_slices), true);
    
    /*
    if (0) {
        require "Slic3r/SVG.pm";
        Slic3r::SVG::output("bridge.svg",
            expolygons      => [ $self->expolygon ],
            red_expolygons  => $self->lower_slices,
            polylines       => $self->_edges,
        );
    }
    */
}

bool BridgeDetector::detect_angle(double bridge_direction_override)
{
    if (this->_edges.empty() || this->_anchor_regions.empty()) 
        // The bridging region is completely in the air, there are no anchors available at the layer below.
        return false;

    std::vector<BridgeDirection> candidates;
    if (bridge_direction_override == 0.) {
        candidates = bridge_direction_candidates();
    } else
        candidates.emplace_back(BridgeDirection(bridge_direction_override));
    
    /*  Outset the bridge expolygon by half the amount we used for detecting anchors;
        we'll use this one to clip our test lines and be sure that their endpoints
        are inside the anchors and not on their contours leading to false negatives. */
    Polygons clip_area = offset(this->expolygons, 0.5f * float(this->spacing));
    
    /*  we'll now try several directions using a rudimentary visibility check:
        bridge in several directions and then sum the length of lines having both
        endpoints within anchors */
        
    bool have_coverage = false;
    for (size_t i_angle = 0; i_angle < candidates.size(); ++ i_angle)
    {
        const double angle = candidates[i_angle].angle;

        Lines lines;
        {
            // Get an oriented bounding box around _anchor_regions.
            BoundingBox bbox = get_extents_rotated(this->_anchor_regions, - angle);
            // Cover the region with line segments.
            lines.reserve((bbox.max(1) - bbox.min(1) + this->spacing) / this->spacing);
            double s = sin(angle);
            double c = cos(angle);
            //FIXME Vojtech: The lines shall be spaced half the line width from the edge, but then 
            // some of the test cases fail. Need to adjust the test cases then?
//            for (coord_t y = bbox.min(1) + this->spacing / 2; y <= bbox.max(1); y += this->spacing)
            for (coord_t y = bbox.min(1); y <= bbox.max(1); y += this->spacing)
                lines.push_back(Line(
                    Point((coord_t)round(c * bbox.min(0) - s * y), (coord_t)round(c * y + s * bbox.min(0))),
                    Point((coord_t)round(c * bbox.max(0) - s * y), (coord_t)round(c * y + s * bbox.max(0)))));
        }

        //compute stat on line with anchors, and their lengths.
        BridgeDirection& c = candidates[i_angle];
        std::vector<coordf_t> dist_anchored;
        {
            Lines clipped_lines = intersection_ln(lines, clip_area);
            for (size_t i = 0; i < clipped_lines.size(); ++i) {
                const Line &line = clipped_lines[i];
                if (expolygons_contain(this->_anchor_regions, line.a) && expolygons_contain(this->_anchor_regions, line.b)) {
                    // This line could be anchored.
                    coordf_t len = line.length();
                    //store stats
                    c.total_length_anchored += len;
                    c.max_length_anchored = std::max(c.max_length_anchored, len);
                    c.nb_lines_anchored++;
                    dist_anchored.push_back(len);
                } else {
                    // this line could NOT be anchored.
                    coordf_t len = line.length();
                    c.total_length_free += len;
                    c.max_length_free = std::max(c.max_length_free, len);
                    c.nb_lines_free++;
                }
            }        
        }
        if (c.total_length_anchored == 0. || c.nb_lines_anchored == 0) {
            continue;
        } else {
            have_coverage = true;
            // compute median
            if (!dist_anchored.empty()) {
                std::sort(dist_anchored.begin(), dist_anchored.end());
                c.median_length_anchor = dist_anchored[dist_anchored.size() / 2];
            }


            // size is 20%
        }
    }

    // if no direction produced coverage, then there's no bridge direction
    if (! have_coverage)
        return false;


    //compute global stat (max & min median & max length)
    std::vector<coordf_t> all_median_length;
    std::vector<coordf_t> all_max_length;
    for (BridgeDirection &c : candidates) {
        all_median_length.push_back(c.median_length_anchor);
        all_max_length.push_back(c.max_length_anchored);
    }
    std::sort(all_median_length.begin(), all_median_length.end());
    std::sort(all_max_length.begin(), all_max_length.end());
    coordf_t median_max_length = all_max_length[all_max_length.size() / 2];
    coordf_t min_max_length = all_max_length.front();
    coordf_t max_max_length = all_max_length.back();
    coordf_t median_median_length = all_median_length[all_median_length.size() / 2];
    coordf_t min_median_length = all_median_length.front();
    coordf_t max_median_length = all_median_length.back();

    //compute individual score
    for (BridgeDirection& c : candidates) {
        c.coverage = 0;
        //ratio_anchored is 70% of the score
        double ratio_anchored = c.total_length_anchored / (c.total_length_anchored + c.total_length_free);
        c.coverage = 70 * ratio_anchored;
        //median is 15% (and need to invert it)
        double ratio_median = 1 - double(c.median_length_anchor - min_median_length) / (double)std::max(1., max_median_length - min_median_length);
        c.coverage += 15 * ratio_median;
        //max is 15 % (and need to invert it)
        double ratio_max = 1 - double(c.max_length_anchored - min_max_length) / (double)std::max(1., max_max_length - min_max_length);
        c.coverage += 15 * ratio_max;
        //bonus for perimeter dir
        if (c.along_perimeter)
            c.coverage += 0.05;

    }
    
    // if any other direction is within extrusion width of coverage, prefer it if shorter
    // shorter = shorter max length, or if in espilon (10) range, the shorter mean length.
    // TODO: There are two options here - within width of the angle with most coverage, or within width of the currently perferred?
    size_t i_best = 0;
    for (size_t i = 1; i < candidates.size(); ++ i)
        if (candidates[i].coverage > candidates[i_best].coverage)
            i_best = i;

    this->angle = candidates[i_best].angle;
    if (this->angle >= PI)
        this->angle -= PI;
    
    #ifdef SLIC3R_DEBUG
    printf("  Optimal infill angle is %d degrees\n", (int)Slic3r::Geometry::rad2deg(this->angle));
    #endif

    return true;
}

std::vector<BridgeDetector::BridgeDirection> BridgeDetector::bridge_direction_candidates() const
{
    // we test angles according to configured resolution
    std::vector<BridgeDirection> angles;
    for (int i = 0; i <= PI/this->resolution; ++i)
        angles.emplace_back(i * this->resolution);
    
    // we also test angles of each bridge contour
    {
        Lines lines = to_lines(this->expolygons);
        for (Lines::const_iterator line = lines.begin(); line != lines.end(); ++line)
            angles.emplace_back(line->direction(), true);
    }
    
    /*  we also test angles of each open supporting edge
        (this finds the optimal angle for C-shaped supports) */
    for (const Polyline &edge : this->_edges)
        if (edge.first_point() != edge.last_point())
            angles.emplace_back(Line(edge.first_point(), edge.last_point()).direction());
    
    // remove duplicates
    double min_resolution = PI/(4*180.0);  // /180 = 1 degree
    std::sort(angles.begin(), angles.end());
    for (size_t i = 1; i < angles.size(); ++i) {
        if (Slic3r::Geometry::directions_parallel(angles[i].angle, angles[i-1].angle, min_resolution)) {
            angles.erase(angles.begin() + i);
            --i;
        }
    }
    /*  compare first value with last one and remove the greatest one (PI) 
        in case they are parallel (PI, 0) */
    if (Slic3r::Geometry::directions_parallel(angles.front().angle, angles.back().angle, min_resolution))
        angles.pop_back();

    return angles;
}

Polygons BridgeDetector::coverage(double angle, bool precise) const {

    if (angle == -1)
        angle = this->angle;

    Polygons covered;

    if (angle != -1) {
        // Get anchors, convert them to Polygons and rotate them.
        Polygons anchors = to_polygons(this->_anchor_regions);
        polygons_rotate(anchors, PI / 2.0 - angle);
        //same for region which do not need bridging
        //Polygons supported_area = diff(this->lower_slices.expolygons, this->_anchor_regions, true);
        //polygons_rotate(anchors, PI / 2.0 - angle);

        for (ExPolygon expolygon : this->expolygons) {
            // Clone our expolygon and rotate it so that we work with vertical lines.
            expolygon.rotate(PI / 2.0 - angle);
            // Outset the bridge expolygon by half the amount we used for detecting anchors;
            // we'll use this one to generate our trapezoids and be sure that their vertices
            // are inside the anchors and not on their contours leading to false negatives.
            for (ExPolygon &expoly : offset_ex(expolygon, 0.5f * float(this->spacing))) {
                // Compute trapezoids according to a vertical orientation
                Polygons trapezoids;
                if (!precise) expoly.get_trapezoids2(&trapezoids, PI / 2);
                else expoly.get_trapezoids3_half(&trapezoids, float(this->spacing));
                for (Polygon &trapezoid : trapezoids) {
                    size_t n_supported = 0;
                    if (!precise) {
                        // not nice, we need a more robust non-numeric check
                        // imporvment 1: take into account when we go in the supported area.
                        for (const Line &supported_line : intersection_ln(trapezoid.lines(), anchors))
                            if (supported_line.length() >= this->spacing)
                                ++n_supported;
                    } else {
                        Polygons intersects = intersection(Polygons{trapezoid}, anchors);
                        n_supported = intersects.size();

                        if (n_supported >= 2) {
                            // trim it to not allow to go outside of the intersections
                            BoundingBox center_bound = intersects[0].bounding_box();
                            coord_t min_y = center_bound.center()(1), max_y = center_bound.center()(1);
                            for (Polygon &poly_bound : intersects) {
                                center_bound = poly_bound.bounding_box();
                                if (min_y > center_bound.center()(1)) min_y = center_bound.center()(1);
                                if (max_y < center_bound.center()(1)) max_y = center_bound.center()(1);
                            }
                            coord_t min_x = trapezoid[0](0), max_x = trapezoid[0](0);
                            for (Point &p : trapezoid.points) {
                                if (min_x > p(0)) min_x = p(0);
                                if (max_x < p(0)) max_x = p(0);
                            }
                            //add what get_trapezoids3 has removed (+EPSILON)
                            min_x -= (this->spacing / 4 + 1);
                            max_x += (this->spacing / 4 + 1);
                            coord_t mid_x = (min_x + max_x) / 2;
                            for (Point &p : trapezoid.points) {
                                if (p(1) < min_y) p(1) = min_y;
                                if (p(1) > max_y) p(1) = max_y;
                                if (p(0) > min_x && p(0) < mid_x) p(0) = min_x;
                                if (p(0) < max_x && p(0) > mid_x) p(0) = max_x;
                            }
                        }
                    }

                    if (n_supported >= 2) {
                        //add it
                        covered.push_back(std::move(trapezoid));
                    }
                }
            }
        }

        // Unite the trapezoids before rotation, as the rotation creates tiny gaps and intersections between the trapezoids
        // instead of exact overlaps.
        covered = union_(covered);
        // Intersect trapezoids with actual bridge area to remove extra margins and append it to result.
        polygons_rotate(covered, -(PI/2.0 - angle));
        //covered = intersection(covered, to_polygons(this->expolygons));
#if 0
        {
            my @lines = map @{$_->lines}, @$trapezoids;
            $_->rotate(-(PI/2 - $angle), [0,0]) for @lines;
            
            require "Slic3r/SVG.pm";
            Slic3r::SVG::output(
                "coverage_" . rad2deg($angle) . ".svg",
                expolygons          => [$self->expolygon],
                green_expolygons    => $self->_anchor_regions,
                red_expolygons      => $coverage,
                lines               => \@lines,
            );
        }
#endif
    }
    return covered;
}

/*  This method returns the bridge edges (as polylines) that are not supported
    but would allow the entire bridge area to be bridged with detected angle
    if supported too */
void
BridgeDetector::unsupported_edges(double angle, Polylines* unsupported) const
{
    if (angle == -1) angle = this->angle;
    if (angle == -1) return;

    Polygons grown_lower = offset(this->lower_slices, float(this->spacing));

    for (ExPolygons::const_iterator it_expoly = this->expolygons.begin(); it_expoly != this->expolygons.end(); ++ it_expoly) {    
        // get unsupported bridge edges (both contour and holes)
        Lines unsupported_lines = to_lines(diff_pl(to_polylines(*it_expoly), grown_lower));
        /*  Split into individual segments and filter out edges parallel to the bridging angle
            TODO: angle tolerance should probably be based on segment length and flow width,
            so that we build supports whenever there's a chance that at least one or two bridge
            extrusions would be anchored within such length (i.e. a slightly non-parallel bridging
            direction might still benefit from anchors if long enough)
            double angle_tolerance = PI / 180.0 * 5.0; */
        for (const Line &line : unsupported_lines)
            if (! Slic3r::Geometry::directions_parallel(line.direction(), angle)) {
                unsupported->emplace_back(Polyline());
                unsupported->back().points.emplace_back(line.a);
                unsupported->back().points.emplace_back(line.b);
            }
    }
    
    /*
    if (0) {
        require "Slic3r/SVG.pm";
        Slic3r::SVG::output(
            "unsupported_" . rad2deg($angle) . ".svg",
            expolygons          => [$self->expolygon],
            green_expolygons    => $self->_anchor_regions,
            red_expolygons      => union_ex($grown_lower),
            no_arrows           => 1,
            polylines           => \@bridge_edges,
            red_polylines       => $unsupported,
        );
    }
    */
}

Polylines
BridgeDetector::unsupported_edges(double angle) const
{
    Polylines pp;
    this->unsupported_edges(angle, &pp);
    return pp;
}

}
