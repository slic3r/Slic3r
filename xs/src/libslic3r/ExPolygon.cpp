#include "BoundingBox.hpp"
#include "ExPolygon.hpp"
#include "Geometry.hpp"
#include "Polygon.hpp"
#include "Line.hpp"
#include "ClipperUtils.hpp"
#include "SVG.hpp"
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
    return to_polygons(*this);
}

ExPolygon::operator Polylines() const
{
    return to_polylines(*this);
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
ExPolygon::contains(const Polylines &polylines) const
{
    #if 0
    BoundingBox bbox = get_extents(polylines);
    bbox.merge(get_extents(*this));
    SVG svg(debug_out_path("ExPolygon_contains.svg"), bbox);
    svg.draw(*this);
    svg.draw_outline(*this);
    svg.draw(polylines, "blue");
    #endif
    Polylines pl_out = diff_pl(polylines, *this);
    #if 0
    svg.draw(pl_out, "red");
    #endif
    return pl_out.empty();
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

bool
ExPolygon::overlaps(const ExPolygon &other) const
{
    #if 0
    BoundingBox bbox = get_extents(other);
    bbox.merge(get_extents(*this));
    static int iRun = 0;
    SVG svg(debug_out_path("ExPolygon_overlaps-%d.svg", iRun ++), bbox);
    svg.draw(*this);
    svg.draw_outline(*this);
    svg.draw_outline(other, "blue");
    #endif
    Polylines pl_out = intersection_pl((Polylines)other, *this);
    #if 0
    svg.draw(pl_out, "red");
    #endif
    if (! pl_out.empty())
        return true; 
    return ! other.contour.points.empty() && this->contains_b(other.contour.points.front());
}

void ExPolygon::simplify_p(double tolerance, Polygons* polygons) const
{
    Polygons pp = this->simplify_p(tolerance);
    polygons->insert(polygons->end(), pp.begin(), pp.end());
}

Polygons ExPolygon::simplify_p(double tolerance) const
{
    Polygons pp;
    pp.reserve(this->holes.size() + 1);
    // contour
    {
        Polygon p = this->contour;
        p.points.push_back(p.points.front());
        p.points = MultiPoint::_douglas_peucker(p.points, tolerance);
        p.points.pop_back();
        pp.emplace_back(std::move(p));
    }
    // holes
    for (Polygon p : this->holes) {
        p.points.push_back(p.points.front());
        p.points = MultiPoint::_douglas_peucker(p.points, tolerance);
        p.points.pop_back();
        pp.emplace_back(std::move(p));
    }
    return simplify_polygons(pp);
}

ExPolygons ExPolygon::simplify(double tolerance) const
{
    return union_ex(this->simplify_p(tolerance));
}

void ExPolygon::simplify(double tolerance, ExPolygons* expolygons) const
{
    append(*expolygons, this->simplify(tolerance));
}

/// remove point that are at SCALED_EPSILON * 2 distance.
void remove_point_too_near(ThickPolyline* to_reduce) {
        const int32_t smallest = SCALED_EPSILON * 2;
        uint32_t id = 1;
        while (id < to_reduce->points.size() - 2) {
            uint32_t newdist = min(to_reduce->points[id].distance_to(to_reduce->points[id - 1])
                , to_reduce->points[id].distance_to(to_reduce->points[id + 1]));
            if (newdist < smallest) {
                to_reduce->points.erase(to_reduce->points.begin() + id);
                to_reduce->width.erase(to_reduce->width.begin() + id);
            } else {
                ++id;
            }
        }
}

/// add points  from pattern to to_modify at the same % of the length
/// so not add if an other point is present at the correct position
void add_point_same_percent(ThickPolyline* pattern, ThickPolyline* to_modify) {
    const double to_modify_length = to_modify->length();
    const double percent_epsilon = SCALED_EPSILON / to_modify_length;
    const double pattern_length = pattern->length();

    double percent_length = 0;
    for (uint32_t idx_point = 1; idx_point < pattern->points.size() - 1; ++idx_point) {
        percent_length += pattern->points[idx_point-1].distance_to(pattern->points[idx_point]) / pattern_length;
        //find position 
        uint32_t idx_other = 1;
        double percent_length_other_before = 0;
        double percent_length_other = 0;
        while (idx_other < to_modify->points.size()) {
            percent_length_other_before = percent_length_other;
            percent_length_other += to_modify->points[idx_other-1].distance_to(to_modify->points[idx_other])
                / to_modify_length;
            if (percent_length_other > percent_length - percent_epsilon) {
                //if higher (we have gone over it)
                break;
            }
            ++idx_other;
        }
        if (percent_length_other > percent_length + percent_epsilon) {
            //insert a new point before the position
            double percent_dist = (percent_length - percent_length_other_before) / (percent_length_other - percent_length_other_before);
            coordf_t new_width = to_modify->width[idx_other - 1] * (1 - percent_dist);
            new_width += to_modify->width[idx_other] * (percent_dist);
            Point new_point;
            new_point.x = (coord_t)((double)(to_modify->points[idx_other - 1].x) * (1 - percent_dist));
            new_point.x += (coord_t)((double)(to_modify->points[idx_other].x) * (percent_dist));
            new_point.y = (coord_t)((double)(to_modify->points[idx_other - 1].y) * (1 - percent_dist));
            new_point.y += (coord_t)((double)(to_modify->points[idx_other].y) * (percent_dist));
            to_modify->width.insert(to_modify->width.begin() + idx_other, new_width);
            to_modify->points.insert(to_modify->points.begin() + idx_other, new_point);
        }
    }
}

/// find the nearest angle in the contour (or 2 nearest if it's difficult to choose) 
/// return 1 for an angle of 90° and 0 for an angle of 0° or 180°
double get_coeff_from_angle_countour(Point &point, const ExPolygon &contour) {
    double nearestDist = point.distance_to(contour.contour.points.front());
    Point nearest = contour.contour.points.front();
    uint32_t id_nearest = 0;
    double nearDist = nearestDist;
    Point near = nearest;
    uint32_t id_near=0;
    for (uint32_t id_point = 1; id_point < contour.contour.points.size(); ++id_point) {
        if (nearestDist > point.distance_to(contour.contour.points[id_point])) {
            nearestDist = point.distance_to(contour.contour.points[id_point]);
            near = nearest;
            nearest = contour.contour.points[id_point];
            id_near = id_nearest;
            id_nearest = id_point;
        }
    }
    double angle = 0;
    Point point_before = id_nearest == 0 ? contour.contour.points.back() : contour.contour.points[id_nearest - 1];
    Point point_after = id_nearest == contour.contour.points.size()-1 ? contour.contour.points.front() : contour.contour.points[id_nearest + 1];
    //compute angle
    angle = min(nearest.ccw_angle(point_before, point_after), nearest.ccw_angle(point_after, point_before));
    //compute the diff from 90°
    angle = abs(angle - PI / 2);
    if (near != nearest && max(nearestDist, nearDist) + SCALED_EPSILON < nearest.distance_to(near)) {
        //not only nearest
        Point point_before = id_near == 0 ? contour.contour.points.back() : contour.contour.points[id_near - 1];
        Point point_after = id_near == contour.contour.points.size() - 1 ? contour.contour.points.front() : contour.contour.points[id_near + 1];
        double angle2 = min(nearest.ccw_angle(point_before, point_after), nearest.ccw_angle(point_after, point_before));
        angle2 = abs(angle - PI / 2);
        angle = (angle + angle2) / 2;
    }

    return 1-(angle/(PI/2));
}

void
ExPolygon::medial_axis(const ExPolygon &bounds, double max_width, double min_width, ThickPolylines* polylines, double height) const
{
    // init helper object
    Slic3r::Geometry::MedialAxis ma(max_width, min_width, this);
    ma.lines = this->lines();
    
    // compute the Voronoi diagram and extract medial axis polylines
    ThickPolylines pp;
    ma.build(&pp);
    

    //{
    //    stringstream stri;
    //    stri << "medial_axis" << id << ".svg";
    //    SVG svg(stri.str());
    //    svg.draw(bounds);
    //    svg.draw(*this);
    //    svg.draw(pp);
    //    svg.Close();
    //}
    
    /* Find the maximum width returned; we're going to use this for validating and 
       filtering the output segments. */
    double max_w = 0;
    for (ThickPolylines::const_iterator it = pp.begin(); it != pp.end(); ++it)
        max_w = fmaxf(max_w, *std::max_element(it->width.begin(), it->width.end()));

    concatThickPolylines(pp);
    //reoder pp by length (ascending) It's really important to do that to avoid building the line from the width insteand of the length
    std::sort(pp.begin(), pp.end(), [](const ThickPolyline & a, const ThickPolyline & b) { return a.length() < b.length(); });

    // Aligned fusion: Fusion the bits at the end of lines by "increasing thickness"
    // For that, we have to find other lines,
    // and with a next point no more distant than the max width.
    // Then, we can merge the bit from the first point to the second by following the mean.
    //
    int id_f = 0;
    bool changes = true;

    
    while (changes) {
        changes = false;
        for (size_t i = 0; i < pp.size(); ++i) {
            ThickPolyline& polyline = pp[i];
            
            //simple check to see if i can be fusionned
            if (!polyline.endpoints.first && !polyline.endpoints.second) continue;
            

            ThickPolyline* best_candidate = nullptr;
            float best_dot = -1;
            int best_idx = 0;
            double dot_poly_branch = 0;
            double dot_candidate_branch = 0;
            
            // find another polyline starting here
            for (size_t j = i + 1; j < pp.size(); ++j) {
                ThickPolyline& other = pp[j];
                if (polyline.last_point().coincides_with(other.last_point())) {
                    polyline.reverse();
                    other.reverse();
                } else if (polyline.first_point().coincides_with(other.last_point())) {
                    other.reverse();
                } else if (polyline.first_point().coincides_with(other.first_point())) {
                } else if (polyline.last_point().coincides_with(other.first_point())) {
                    polyline.reverse();
                } else {
                    continue;
                }
                //std::cout << " try : " << i << ":" << j << " : " << 
                //    (polyline.points.size() < 2 && other.points.size() < 2) <<
                //    (!polyline.endpoints.second || !other.endpoints.second) <<
                //    ((polyline.points.back().distance_to(other.points.back())
                //    + (polyline.width.back() + other.width.back()) / 4)
                //    > max_width*1.05) <<
                //    (abs(polyline.length() - other.length()) > max_width / 2) << "\n";

                //// mergeable tests
                if (polyline.points.size() < 2 && other.points.size() < 2) continue;
                if (!polyline.endpoints.second || !other.endpoints.second) continue;
                // test if the new width will not be too big if a fusion occur
                //note that this isn't the real calcul. It's just to avoid merging lines too far apart.
                if (
                    ((polyline.points.back().distance_to(other.points.back())
                        + (polyline.width.back() + other.width.back()) / 4)
                        > max_width*1.05))
                        continue;
                // test if the lines are not too different in length.
                if (abs(polyline.length() - other.length()) > max_width / 2) continue;


                //test if we don't merge with something too different and without any relevance.
                double coeffSizePolyI = 1;
                if (polyline.width.back() == 0) {
                    coeffSizePolyI = 0.1 + 0.9*get_coeff_from_angle_countour(polyline.points.back(), *this);
                }
                double coeffSizeOtherJ = 1;
                if (other.width.back() == 0) {
                    coeffSizeOtherJ = 0.1+0.9*get_coeff_from_angle_countour(other.points.back(), *this);
                }
                if (abs(polyline.length()*coeffSizePolyI - other.length()*coeffSizeOtherJ) > max_width / 2) continue;

                //compute angle to see if it's better than previous ones (straighter = better).
                Pointf v_poly(polyline.lines().front().vector().x, polyline.lines().front().vector().y);
                v_poly.scale(1 / std::sqrt(v_poly.x*v_poly.x + v_poly.y*v_poly.y));
                Pointf v_other(other.lines().front().vector().x, other.lines().front().vector().y);
                v_other.scale(1 / std::sqrt(v_other.x*v_other.x + v_other.y*v_other.y));
                float other_dot = v_poly.x*v_other.x + v_poly.y*v_other.y;

                // Get the branch/line in wich we may merge, if possible
                // with that, we can decide what is important, and how we can merge that.
                // angle_poly - angle_candi =90° => one is useless
                // both angle are equal => both are useful with same strength
                // ex: Y => | both are useful to crete a nice line
                // ex2: TTTTT => -----  these 90° useless lines should be discarded
                bool find_main_branch = false;
                int biggest_main_branch_id = 0;
                int biggest_main_branch_length = 0;
                for (size_t k = 0; k < pp.size(); ++k) {
                    //std::cout << "try to find main : " << k << " ? " << i << " " << j << " ";
                    if (k == i | k == j) continue;
                    ThickPolyline& main = pp[k];
                    if (polyline.first_point().coincides_with(main.last_point())) {
                        main.reverse();
                        if (!main.endpoints.second)
                            find_main_branch = true;
                        else if (biggest_main_branch_length < main.length()) {
                            biggest_main_branch_id = k;
                            biggest_main_branch_length = main.length();
                        }
                    } else if (polyline.first_point().coincides_with(main.first_point())) {
                        if (!main.endpoints.second)
                            find_main_branch = true;
                        else if (biggest_main_branch_length < main.length()) {
                            biggest_main_branch_id = k;
                            biggest_main_branch_length = main.length();
                        }
                    }
                    if (find_main_branch) {
                        //use this variable to store the good index and break to compute it
                        biggest_main_branch_id = k;
                        break;
                    }
                }
                if (!find_main_branch && biggest_main_branch_length == 0) {
                    // nothing -> it's impossible!
                    dot_poly_branch = 0.707;
                    dot_candidate_branch = 0.707;
                    //std::cout << "no main branch... impossible!!\n";
                } else if (!find_main_branch && 
                    (pp[biggest_main_branch_id].length() < polyline.length() || pp[biggest_main_branch_id].length() < other.length()) ){
                    //the main branch should have no endpoint or be bigger!
                    //here, it have an endpoint, and is not the biggest -> bad!
                    continue;
                } else {
                    //compute the dot (biggest_main_branch_id)
                    Pointf v_poly(polyline.lines().front().vector().x, polyline.lines().front().vector().y);
                    v_poly.scale(1 / std::sqrt(v_poly.x*v_poly.x + v_poly.y*v_poly.y));
                    Pointf v_candid(other.lines().front().vector().x, other.lines().front().vector().y);
                    v_candid.scale(1 / std::sqrt(v_candid.x*v_candid.x + v_candid.y*v_candid.y));
                    Pointf v_branch(-pp[biggest_main_branch_id].lines().front().vector().x, -pp[biggest_main_branch_id].lines().front().vector().y);
                    v_branch.scale(1 / std::sqrt(v_branch.x*v_branch.x + v_branch.y*v_branch.y));
                    dot_poly_branch = v_poly.x*v_branch.x + v_poly.y*v_branch.y;
                    dot_candidate_branch = v_candid.x*v_branch.x + v_candid.y*v_branch.y;
                    if (dot_poly_branch < 0) dot_poly_branch = 0;
                    if (dot_candidate_branch < 0) dot_candidate_branch = 0;
                }
                //test if it's useful to merge or not
                //ie, don't merge  'T' but ok for 'Y', merge only lines of not disproportionate different length (ratio max: 4)
                if (dot_poly_branch < 0.1 || dot_candidate_branch < 0.1 ||
                    (polyline.length()>other.length() ? polyline.length() / other.length() : other.length() / polyline.length()) > 4) {
                    continue;
                }
                if (other_dot > best_dot) {
                    best_candidate = &other;
                    best_idx = j;
                    best_dot = other_dot;
                }
            }
            if (best_candidate != nullptr) {
                // delete very near points
                remove_point_too_near(&polyline);
                remove_point_too_near(best_candidate);

                // add point at the same pos than the other line to have a nicer fusion
                add_point_same_percent(&polyline, best_candidate);
                add_point_same_percent(best_candidate, &polyline);

                //get the angle of the nearest points of the contour to see : _| (good) \_ (average) __(bad)
                //sqrt because the result are nicer this way: don't over-penalize /_ angles
                //TODO: try if we can achieve a better result if we use a different algo if the angle is <90°
                const double coeff_angle_poly = (get_coeff_from_angle_countour(polyline.points.back(), *this));
                const double coeff_angle_candi = (get_coeff_from_angle_countour(best_candidate->points.back(), *this));

                //this will encourage to follow the curve, a little, because it's shorter near the center
                //without that, it tends to go to the outter rim.
                double weight_poly = 2 - polyline.length() / max(polyline.length(), best_candidate->length());
                double weight_candi = 2 - best_candidate->length() / max(polyline.length(), best_candidate->length());
                weight_poly *= coeff_angle_poly;
                weight_candi *= coeff_angle_candi;
                const double coeff_poly = (dot_poly_branch * weight_poly) / (dot_poly_branch * weight_poly + dot_candidate_branch * weight_candi);
                const double coeff_candi = 1.0 - coeff_poly;
                //iterate the points
                // as voronoi should create symetric thing, we can iterate synchonously
                unsigned int idx_point = 1;
                while (idx_point < min(polyline.points.size(), best_candidate->points.size())) {
                    //fusion
                    polyline.points[idx_point].x = polyline.points[idx_point].x * coeff_poly + best_candidate->points[idx_point].x * coeff_candi;
                    polyline.points[idx_point].y = polyline.points[idx_point].y * coeff_poly + best_candidate->points[idx_point].y * coeff_candi;
                   
                    // The width decrease with distance from the centerline.
                    // This formula is what works the best, even if it's not perfect (created empirically).  0->3% error on a gap fill on some tests.
                    //If someone find  an other formula based on the properties of the voronoi algorithm used here, and it works better, please use it.
                    //or maybe just use the distance to nearest edge in bounds...
                    double value_from_current_width = 0.5*polyline.width[idx_point] * dot_poly_branch / max(dot_poly_branch, dot_candidate_branch);
                    value_from_current_width += 0.5*best_candidate->width[idx_point] * dot_candidate_branch / max(dot_poly_branch, dot_candidate_branch);
                    double value_from_dist = 2 * polyline.points[idx_point].distance_to(best_candidate->points[idx_point]);
                    value_from_dist *= sqrt(min(dot_poly_branch, dot_candidate_branch) / max(dot_poly_branch, dot_candidate_branch));
                    polyline.width[idx_point] = value_from_current_width + value_from_dist;
                    //failsafe
                    if (polyline.width[idx_point] > max_width) polyline.width[idx_point] = max_width;

                    ++idx_point;
                }
                if (idx_point < best_candidate->points.size()) {
                    if (idx_point + 1 < best_candidate->points.size()) {
                        //create a new polyline
                        pp.emplace_back();
                        pp.back().endpoints.first = true;
                        pp.back().endpoints.second = best_candidate->endpoints.second;
                        for (int idx_point_new_line = idx_point; idx_point_new_line < best_candidate->points.size(); ++idx_point_new_line) {
                            pp.back().points.push_back(best_candidate->points[idx_point_new_line]);
                            pp.back().width.push_back(best_candidate->width[idx_point_new_line]);
                        }
                    } else {
                        //Add last point
                        polyline.points.push_back(best_candidate->points[idx_point]);
                        polyline.width.push_back(best_candidate->width[idx_point]);
                        //select if an end opccur
                        polyline.endpoints.second &= best_candidate->endpoints.second;
                    }

                } else {
                    //select if an end opccur
                    polyline.endpoints.second &= best_candidate->endpoints.second;
                }

                //remove points that are the same or too close each other, ie simplify
                for (unsigned int idx_point = 1; idx_point < polyline.points.size(); ++idx_point) {
                    if (polyline.points[idx_point - 1].distance_to(polyline.points[idx_point]) < SCALED_EPSILON) {
                        if (idx_point < polyline.points.size() -1) {
                            polyline.points.erase(polyline.points.begin() + idx_point);
                            polyline.width.erase(polyline.width.begin() + idx_point);
                        } else {
                            polyline.points.erase(polyline.points.begin() + idx_point - 1);
                            polyline.width.erase(polyline.width.begin() + idx_point - 1);
                        }
                        --idx_point;
                    }
                }
                //remove points that are outside of the geometry
                for (unsigned int idx_point = 0; idx_point < polyline.points.size(); ++idx_point) {
                    if (!bounds.contains_b(polyline.points[idx_point])) {
                        polyline.points.erase(polyline.points.begin() + idx_point);
                        polyline.width.erase(polyline.width.begin() + idx_point);
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
                break;
            }
        }
        if (changes) {
            concatThickPolylines(pp);
            ///reorder, in case of change
            std::sort(pp.begin(), pp.end(), [](const ThickPolyline & a, const ThickPolyline & b) { return a.length() < b.length(); });
        }
    }

    // remove too small extrusion at start & end of polylines
    changes = false;
    for (size_t i = 0; i < pp.size(); ++i) {
        ThickPolyline& polyline = pp[i];
        // remove bits with too small extrusion
        while (polyline.points.size() > 1 && polyline.width.front() < min_width && polyline.endpoints.first) {
            //try to split if possible
            if (polyline.width[1] > min_width) {
                double percent_can_keep = (min_width - polyline.width[0]) / (polyline.width[1] - polyline.width[0]);
                if (polyline.points.front().distance_to(polyline.points[1]) * percent_can_keep > max_width / 2
                    && polyline.points.front().distance_to(polyline.points[1])* (1 - percent_can_keep) > max_width / 2) {
                    //Can split => move the first point and assign a new weight.
                    //the update of endpoints wil be performed in concatThickPolylines
                    polyline.points.front().x = polyline.points.front().x + 
                        (coord_t)((polyline.points[1].x - polyline.points.front().x) * percent_can_keep);
                    polyline.points.front().y = polyline.points.front().y + 
                        (coord_t)((polyline.points[1].y - polyline.points.front().y) * percent_can_keep);
                    polyline.width.front() = min_width;
                    changes = true;
                    break;
                }
            }
            polyline.points.erase(polyline.points.begin());
            polyline.width.erase(polyline.width.begin());
            changes = true;
        }
        while (polyline.points.size() > 1 && polyline.width.back() < min_width && polyline.endpoints.second) {
            //try to split if possible
            if (polyline.width[polyline.points.size()-2] > min_width) {
                double percent_can_keep = (min_width - polyline.width.back()) / (polyline.width[polyline.points.size() - 2] - polyline.width.back());
                if (polyline.points.back().distance_to(polyline.points[polyline.points.size() - 2]) * percent_can_keep > max_width / 2
                    && polyline.points.back().distance_to(polyline.points[polyline.points.size() - 2]) * (1-percent_can_keep) > max_width / 2) {
                    //Can split => move the first point and assign a new weight.
                    //the update of endpoints wil be performed in concatThickPolylines
                    polyline.points.back().x = polyline.points.back().x + 
                        (coord_t)((polyline.points[polyline.points.size() - 2].x - polyline.points.back().x) * percent_can_keep);
                    polyline.points.back().y = polyline.points.back().y + 
                        (coord_t)((polyline.points[polyline.points.size() - 2].y - polyline.points.back().y) * percent_can_keep);
                    polyline.width.back() = min_width;
                    changes = true;
                    break;
                }
            }
            polyline.points.erase(polyline.points.end()-1);
            polyline.width.erase(polyline.width.end() - 1);
            changes = true;
        }
        if (polyline.points.size() < 2) {
            //remove self if too small
            pp.erase(pp.begin() + i);
            --i;
        }
    }
    if (changes) concatThickPolylines(pp);

    // Loop through all returned polylines in order to extend their endpoints to the 
    //   expolygon boundaries
    for (size_t i = 0; i < pp.size(); ++i) {
        ThickPolyline& polyline = pp[i];

        // extend initial and final segments of each polyline if they're actual endpoints
        // We assign new endpoints to temporary variables because in case of a single-line
        // polyline, after we extend the start point it will be caught by the intersection()
        // call, so we keep the inner point until we perform the second intersection() as well
        Point new_front = polyline.points.front();
        Point new_back = polyline.points.back();
        if (polyline.endpoints.first && !bounds.has_boundary_point(new_front)) {
            Line line(polyline.points[1], polyline.points.front());

            // prevent the line from touching on the other side, otherwise intersection() might return that solution
            if (polyline.points.size() == 2) line.a = line.midpoint();

            line.extend_end(max_width);
            (void)bounds.contour.first_intersection(line, &new_front);
        }
        if (polyline.endpoints.second && !bounds.has_boundary_point(new_back)) {
            Line line(
                *(polyline.points.end() - 2),
                polyline.points.back()
                );

            // prevent the line from touching on the other side, otherwise intersection() might return that solution
            if (polyline.points.size() == 2) line.a = line.midpoint();
            line.extend_end(max_width);

            (void)bounds.contour.first_intersection(line, &new_back);
        }
        polyline.points.front() = new_front;
        polyline.points.back() = new_back;

    }


    // concatenate, but even where multiple thickpolyline join, to create nice long strait polylines
    /*  If we removed any short polylines we now try to connect consecutive polylines
        in order to allow loop detection. Note that this algorithm is greedier than 
        MedialAxis::process_edge_neighbors() as it will connect random pairs of 
        polylines even when more than two start from the same point. This has no 
        drawbacks since we optimize later using nearest-neighbor which would do the 
        same, but should we use a more sophisticated optimization algorithm we should
        not connect polylines when more than two meet. 
        Optimisation of the old algorithm : now we select the most "strait line" choice 
        when we merge with an other line at a point with more than two meet.
        */
    changes = false;
    for (size_t i = 0; i < pp.size(); ++i) {
        ThickPolyline& polyline = pp[i];
        if (polyline.endpoints.first && polyline.endpoints.second) continue; // optimization
            
        ThickPolyline* best_candidate = nullptr;
        float best_dot = -1;
        int best_idx = 0;

        // find another polyline starting here
        for (size_t j = i + 1; j < pp.size(); ++j) {
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
            polyline.width.insert(polyline.width.end(), best_candidate->width.begin() + 1, best_candidate->width.end());
            polyline.endpoints.second = best_candidate->endpoints.second;
            assert(polyline.width.size() == polyline.points.size());
            changes = true;
            pp.erase(pp.begin() + best_idx);
        }
    }
    if (changes) concatThickPolylines(pp);

    //remove too thin polylines points (inside a polyline : split it)
    for (size_t i = 0; i < pp.size(); ++i) {
        ThickPolyline& polyline = pp[i];

        // remove bits with too small extrusion
        size_t idx_point = 0;
        while (idx_point<polyline.points.size()) {
            if (polyline.width[idx_point] < min_width) {
                if (idx_point == 0) {
                    //too thin at start
                    polyline.points.erase(polyline.points.begin());
                    polyline.width.erase(polyline.width.begin());
                    idx_point = 0;
                } else if (idx_point == 1) {
                    //too thin at start
                    polyline.points.erase(polyline.points.begin());
                    polyline.width.erase(polyline.width.begin());
                    polyline.points.erase(polyline.points.begin());
                    polyline.width.erase(polyline.width.begin());
                    idx_point = 0;
                } else if (idx_point == polyline.points.size() - 2) {
                    //too thin at (near) end
                    polyline.points.erase(polyline.points.end() - 1);
                    polyline.width.erase(polyline.width.end() - 1);
                    polyline.points.erase(polyline.points.end() - 1);
                    polyline.width.erase(polyline.width.end() - 1);
                } else if (idx_point == polyline.points.size() - 1) {
                    //too thin at end
                    polyline.points.erase(polyline.points.end() - 1);
                    polyline.width.erase(polyline.width.end() - 1);
                } else {
                    //too thin in middle : split
                    pp.emplace_back();
                    ThickPolyline &newone = pp.back();
                    newone.points.insert(newone.points.begin(), polyline.points.begin() + idx_point + 1, polyline.points.end());
                    newone.width.insert(newone.width.begin(), polyline.width.begin() + idx_point + 1, polyline.width.end());
                    polyline.points.erase(polyline.points.begin() + idx_point, polyline.points.end());
                    polyline.width.erase(polyline.width.begin() + idx_point, polyline.width.end());
                }
            } else idx_point++;

            if (polyline.points.size() < 2) {
                //remove self if too small
                pp.erase(pp.begin() + i);
                --i;
                break;
            }
        }
    }

    //remove too short polyline
    changes = true;
    while (changes) {
        changes = false;
        
        double shortest_size = max_w * 2;
        int32_t shortest_idx = -1;
        for (size_t i = 0; i < pp.size(); ++i) {
            ThickPolyline& polyline = pp[i];
            // Remove the shortest polylines : polyline that are shorter than wider
            // (we can't do this check before endpoints extension and clipping because we don't
            // know how long will the endpoints be extended since it depends on polygon thickness
            // which is variable - extension will be <= max_width/2 on each side) 
            if ((polyline.endpoints.first || polyline.endpoints.second)
                && polyline.length() < max_width / 2) {
               if (shortest_size > polyline.length()) {
                    shortest_size = polyline.length();
                    shortest_idx = i;
                }

            }
        }
        if (shortest_idx >= 0 && shortest_idx < pp.size()) {
            pp.erase(pp.begin() + shortest_idx);
            changes = true;
        }
        if (changes) concatThickPolylines(pp);
    }

    //TODO: reduce the flow at the intersection ( + ) points ?

    //ensure the volume extruded is correct for what we have been asked
    // => don't over-extrude
    double surface = 0;
    double volume = 0;
    for (ThickPolyline& polyline : pp) {
        for (ThickLine l : polyline.thicklines()) {
            surface += l.length() * (l.a_width + l.b_width) / 2;
            double width_mean = (l.a_width + l.b_width) / 2;
            volume += height * (width_mean - height * (1. - 0.25 * PI)) * l.length();
        }
    }

    // compute bounds volume
    double boundsVolume = 0;
    boundsVolume += height*bounds.area();
    // add external "perimeter gap"
    double perimeterRoundGap = bounds.contour.length() * height * (1 - 0.25*PI) * 0.5;
    // add holes "perimeter gaps"
    double holesGaps = 0;
    for (auto hole = bounds.holes.begin(); hole != bounds.holes.end(); ++hole) {
        holesGaps += hole->length() * height * (1 - 0.25*PI) * 0.5;
    }
    boundsVolume += perimeterRoundGap + holesGaps;
    
    if (boundsVolume < volume) {
        //reduce width
        double reduce_by = boundsVolume / volume;
        for (ThickPolyline& polyline : pp) {
            for (ThickLine l : polyline.thicklines()) {
                l.a_width *= reduce_by;
                l.b_width *= reduce_by;
            }
        }
    }
    polylines->insert(polylines->end(), pp.begin(), pp.end());
}

void
ExPolygon::medial_axis(double max_width, double min_width, Polylines* polylines) const
{
    ThickPolylines tp;
    this->medial_axis(*this, max_width, min_width, &tp, max_width/2.0);
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
        // append results to return value
        polygons_append(*polygons, intersection(poly, to_polygons(*this)));
    }
}

void
ExPolygon::get_trapezoids2(Polygons* polygons, double angle) const {
    ExPolygon clone = *this;
    clone.rotate(PI / 2 - angle, Point(0, 0));
    clone.get_trapezoids2(polygons);
    for (Polygons::iterator polygon = polygons->begin(); polygon != polygons->end(); ++polygon)
        polygon->rotate(-(PI / 2 - angle), Point(0, 0));
}

void
ExPolygon::get_trapezoids3_half(Polygons* polygons, float spacing) const {

    // get all points of this ExPolygon
    Points pp = *this;

    if (pp.empty()) return;

    // build our bounding box
    BoundingBox bb(pp);

    // get all x coordinates
    int min_x = pp[0].x, max_x = pp[0].x;
    std::vector<coord_t> xx;
    for (Points::const_iterator p = pp.begin(); p != pp.end(); ++p) {
        if (min_x > p->x) min_x = p->x;
        if (max_x < p->x) max_x = p->x;
    }
    for (int x = min_x; x < max_x-spacing/2; x += spacing) {
        xx.push_back(x);
    }
    xx.push_back(max_x);
    //std::sort(xx.begin(), xx.end());

    // find trapezoids by looping from first to next-to-last coordinate
    for (std::vector<coord_t>::const_iterator x = xx.begin(); x != xx.end() - 1; ++x) {
        coord_t next_x = *(x + 1);
        if (*x == next_x) continue;

        // build rectangle
        Polygon poly;
        poly.points.resize(4);
        poly[0].x = *x +spacing / 4;
        poly[0].y = bb.min.y;
        poly[1].x = next_x -spacing / 4;
        poly[1].y = bb.min.y;
        poly[2].x = next_x -spacing / 4;
        poly[2].y = bb.max.y;
        poly[3].x = *x +spacing / 4;
        poly[3].y = bb.max.y;

        // intersect with this expolygon
        // append results to return value
        polygons_append(*polygons, intersection(poly, to_polygons(*this)));
    }
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
    
    ExPolygons expp = union_ex(simplify_polygons(to_polygons(*this), true));
    
    for (ExPolygons::const_iterator ex = expp.begin(); ex != expp.end(); ++ex) {
        // contour
        {
            TPPLPoly p;
            p.Init(int(ex->contour.points.size()));
            //printf(PRINTF_ZU "\n0\n", ex->contour.points.size());
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
            //printf(PRINTF_ZU "\n1\n", hole->points.size());
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
            p.points[i].x = coord_t((*poly)[i].x);
            p.points[i].y = coord_t((*poly)[i].y);
        }
        polygons->push_back(p);
    }
}

void
ExPolygon::triangulate_p2t(Polygons* polygons) const
{
    ExPolygons expp = simplify_polygons_ex(*this, true);
    
    for (ExPolygons::const_iterator ex = expp.begin(); ex != expp.end(); ++ex) {
        // TODO: prevent duplicate points

        // contour
        std::vector<p2t::Point*> ContourPoints;
        for (Points::const_iterator point = ex->contour.points.begin(); point != ex->contour.points.end(); ++point) {
            // We should delete each p2t::Point object
            ContourPoints.push_back(new p2t::Point(point->x, point->y));
        }
        p2t::CDT cdt(ContourPoints);

        // holes
        for (Polygons::const_iterator hole = ex->holes.begin(); hole != ex->holes.end(); ++hole) {
            std::vector<p2t::Point*> points;
            for (Points::const_iterator point = hole->points.begin(); point != hole->points.end(); ++point) {
                // will be destructed in SweepContext::~SweepContext
                points.push_back(new p2t::Point(point->x, point->y));
            }
            cdt.AddHole(points);
        }
        
        // perform triangulation
        cdt.Triangulate();
        std::vector<p2t::Triangle*> triangles = cdt.GetTriangles();
        
        for (std::vector<p2t::Triangle*>::const_iterator triangle = triangles.begin(); triangle != triangles.end(); ++triangle) {
            Polygon p;
            for (int i = 0; i <= 2; ++i) {
                p2t::Point* point = (*triangle)->GetPoint(i);
                p.points.push_back(Point(point->x, point->y));
            }
            polygons->push_back(p);
        }

        for(std::vector<p2t::Point*>::iterator it = ContourPoints.begin(); it != ContourPoints.end(); ++it) {
            delete *it;
        }
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

BoundingBox get_extents(const ExPolygon &expolygon)
{
    return get_extents(expolygon.contour);
}

BoundingBox get_extents(const ExPolygons &expolygons)
{
    BoundingBox bbox;
    if (! expolygons.empty()) {
        for (size_t i = 0; i < expolygons.size(); ++ i)
			if (! expolygons[i].contour.points.empty())
				bbox.merge(get_extents(expolygons[i]));
    }
    return bbox;
}

BoundingBox get_extents_rotated(const ExPolygon &expolygon, double angle)
{
    return get_extents_rotated(expolygon.contour, angle);
}

BoundingBox get_extents_rotated(const ExPolygons &expolygons, double angle)
{
    BoundingBox bbox;
    if (! expolygons.empty()) {
        bbox = get_extents_rotated(expolygons.front().contour, angle);
        for (size_t i = 1; i < expolygons.size(); ++ i)
            bbox.merge(get_extents_rotated(expolygons[i].contour, angle));
    }
    return bbox;
}

extern std::vector<BoundingBox> get_extents_vector(const ExPolygons &polygons)
{
    std::vector<BoundingBox> out;
    out.reserve(polygons.size());
    for (ExPolygons::const_iterator it = polygons.begin(); it != polygons.end(); ++ it)
        out.push_back(get_extents(*it));
    return out;
}

bool remove_sticks(ExPolygon &poly)
{
    return remove_sticks(poly.contour) || remove_sticks(poly.holes);
}

} // namespace Slic3r
