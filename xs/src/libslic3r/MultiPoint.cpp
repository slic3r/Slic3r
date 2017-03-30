#include "MultiPoint.hpp"
#include "BoundingBox.hpp"

namespace Slic3r {

MultiPoint::operator Points() const
{
    return this->points;
}

void
MultiPoint::scale(double factor)
{
    for (Points::iterator it = points.begin(); it != points.end(); ++it) {
        (*it).scale(factor);
    }
}

void
MultiPoint::translate(double x, double y)
{
    for (Points::iterator it = points.begin(); it != points.end(); ++it) {
        (*it).translate(x, y);
    }
}

void
MultiPoint::translate(const Point &vector)
{
    this->translate(vector.x, vector.y);
}

void
MultiPoint::rotate(double angle)
{
    double s = sin(angle);
    double c = cos(angle);
    for (Points::iterator it = points.begin(); it != points.end(); ++it) {
        double cur_x = (double)it->x;
        double cur_y = (double)it->y;
        it->x = (coord_t)round(c * cur_x - s * cur_y);
        it->y = (coord_t)round(c * cur_y + s * cur_x);
    }
}

void
MultiPoint::rotate(double angle, const Point &center)
{
    double s = sin(angle);
    double c = cos(angle);
    for (Points::iterator it = points.begin(); it != points.end(); ++it) {
        double dx = double(it->x - center.x);
        double dy = double(it->y - center.y);
        it->x = (coord_t)round(double(center.x) + c * dx - s * dy);
        it->y = (coord_t)round(double(center.y) + c * dy + s * dx);
    }
}

void
MultiPoint::reverse()
{
    std::reverse(this->points.begin(), this->points.end());
}

Point
MultiPoint::first_point() const
{
    return this->points.front();
}

double
MultiPoint::length() const
{
    Lines lines = this->lines();
    double len = 0;
    for (Lines::iterator it = lines.begin(); it != lines.end(); ++it) {
        len += it->length();
    }
    return len;
}

int
MultiPoint::find_point(const Point &point) const
{
    for (Points::const_iterator it = this->points.begin(); it != this->points.end(); ++it) {
        if (it->coincides_with(point)) return it - this->points.begin();
    }
    return -1;  // not found
}

bool
MultiPoint::has_boundary_point(const Point &point) const
{
    double dist = point.distance_to(point.projection_onto(*this));
    return dist < SCALED_EPSILON;
}

BoundingBox
MultiPoint::bounding_box() const
{
    return BoundingBox(this->points);
}

bool 
MultiPoint::has_duplicate_points() const
{
    for (size_t i = 1; i < points.size(); ++i)
        if (points[i-1].coincides_with(points[i]))
            return true;
    return false;
}

bool
MultiPoint::remove_duplicate_points()
{
    size_t j = 0;
    for (size_t i = 1; i < points.size(); ++i) {
        if (points[j].coincides_with(points[i])) {
            // Just increase index i.
        } else {
            ++ j;
            if (j < i)
                points[j] = points[i];
        }
    }
    if (++ j < points.size()) {
        points.erase(points.begin() + j, points.end());
        return true;
    }
    return false;
}

void
MultiPoint::append(const Point &point)
{
    this->points.push_back(point);
}

void
MultiPoint::append(const Points &points)
{
    this->append(points.begin(), points.end());
}

void
MultiPoint::append(const Points::const_iterator &begin, const Points::const_iterator &end)
{
    this->points.insert(this->points.end(), begin, end);
}

bool
MultiPoint::intersection(const Line& line, Point* intersection) const
{
    Lines lines = this->lines();
    for (Lines::const_iterator it = lines.begin(); it != lines.end(); ++it) {
        if (it->intersection(line, intersection)) return true;
    }
    return false;
}

std::string
MultiPoint::dump_perl() const
{
    std::ostringstream ret;
    ret << "[";
    for (Points::const_iterator p = this->points.begin(); p != this->points.end(); ++p) {
        ret << p->dump_perl();
        if (p != this->points.end()-1) ret << ",";
    }
    ret << "]";
    return ret.str();
}

//FIXME This is very inefficient in term of memory use.
// The recursive algorithm shall run in place, not allocating temporary data in each recursion.
Points
MultiPoint::_douglas_peucker(const Points &points, const double tolerance)
{
    assert(points.size() >= 2);
    Points results;
    double dmax = 0;
    size_t index = 0;
    Line full(points.front(), points.back());
    for (Points::const_iterator it = points.begin() + 1; it != points.end(); ++it) {
        // we use shortest distance, not perpendicular distance
        double d = it->distance_to(full);
        if (d > dmax) {
            index = it - points.begin();
            dmax = d;
        }
    }
    if (dmax >= tolerance) {
        Points dp0;
        dp0.reserve(index + 1);
        dp0.insert(dp0.end(), points.begin(), points.begin() + index + 1);
        // Recursive call.
        Points dp1 = MultiPoint::_douglas_peucker(dp0, tolerance);
        results.reserve(results.size() + dp1.size() - 1);
        results.insert(results.end(), dp1.begin(), dp1.end() - 1);
        
        dp0.clear();
        dp0.reserve(points.size() - index);
        dp0.insert(dp0.end(), points.begin() + index, points.end());
        // Recursive call.
        dp1 = MultiPoint::_douglas_peucker(dp0, tolerance);
        results.reserve(results.size() + dp1.size());
        results.insert(results.end(), dp1.begin(), dp1.end());
    } else {
        results.push_back(points.front());
        results.push_back(points.back());
    }
    return results;
}

/*

    struct - vis_node

    Used with the visivalignam simplification algorithm, which needs to be able to find a points
    successors and predecessors to operate succesfully. Since this struct is only used in one
    location, it could probably be dropped into a namespace to avoid polluting the slic3r namespace.

    Source: https://github.com/shortsleeves/visvalingam_simplify

    ^ Provided original algorithm implementation. I've only changed things a bit to "clean" them up
    (i.e be more like my personal style), and managed to do this without requiring a binheap implementation

*/

struct vis_node{
    vis_node(const size_t& idx, const size_t& _prev_idx, const size_t& _next_idx, const double& _area) : pt_idx(idx), prev_idx(_prev_idx), next_idx(_next_idx), area(_area) {}
    // Indices into a Points container, from which this object was constructed
    size_t pt_idx, prev_idx, next_idx;
    // Effective area of this "node"
    double area;
    // Overloaded operator used to sort the binheap
    // Greater area = "more important" node. So, this node is less than the 
    // other node if it's area is less than the other node's area
    bool operator<(const vis_node& other){
        return (this->area < other.area);
    }
};

Points
MultiPoint::visivalingam(const Points& pts, const double& tolerance)
{
    // Make sure there's enough points in "pts" to bother with simplification.
    assert(pts.size() >= 2);

    // Result object
    Points results;

    // Lambda to calculate effective area spanned by a point and its immediate 
    // successor + predecessor.
    auto effective_area = [pts](const size_t& curr_pt_idx, const size_t& prev_pt_idx, const size_t& next_pt_idx)->coordf_t{
        const Point& curr = line[curr_pt_idx];
        const Point& prev = line[prev_pt_idx];
        const Point& next = line[next_pt_idx];
        // Use point objects as vector-distances
        const Point curr_to_next = next - curr;
        const Point prev_to_next = prev - curr;
        // Take cross product of these two vector distances
        const coordf_t cross_product = static_cast<coordf_t>(curr_to_next.x * prev_to_next.y) - static_cast<coordf_t>(curr_to_next.y * prev_to_next.x);
        return 0.50 * abs(cross_product);
    };

    // We store the effective areas for each node
    std::vector<coordf_t> areas;
    areas.reserve(pts.size());

    // Construct the initial set of nodes. We will make a heap out of the "heap" vector using 
    // std::make_heap. node_list is used later.
    std::vector<vis_node*> node_list;
    node_list.resize(pts.size());
    std::vector<vis_node*> heap;
    heap.reserve(pts.size());
    for(size_t i = 1; i < pts.size() - 1; ++i){
        // Get effective area of current node.
        coordf_t area = effective_area(i, i - 1, i + 1);
        // If area is greater than some arbitrarily small value, use it.
        node_list[i] = new vis_node(i, i - 1, i + 1, area);
        heap.push_back(node_list[i]);
    }

    // Call std::make_heap, which uses the < operator by default to make "heap" into 
    // a binheap, sorted by the < operator we defind in the vis_node struct
    std::make_heap(heap.begin(), heap.end());

    // Start comparing areas. Set min_area to an outrageous value initially.
    double min_area = -std::numeric_limits<double>::max();
    while(!heap.empty()){

        // Get current node.
        vis_node* curr = heap.front();

        // Pop node we just retrieved off the heap. pop_heap moves front element in vector
        // to the back, so we can call pop_back()
        std::pop_heap(heap.begin(), heap.end());
        heap.pop_back();

        // Sanity assert check
        assert(curr == node_list[curr->pt_idx]);

        // If the current pt'ss area is less than that of the previous pt's area
        // use the last pt's area instead. This ensures we don't elimate the current
        // point without eliminating the previous 
        min_area = std::max(min_area, curr->area);

        // Update prev
        vis_node* prev = node_list[curr->prev_idx];
        if(prev != nullptr){
            prev->next_idx = curr->next_idx;
            prev->area = effective_area(prev->pt_idx, prev->prev_idx, prev->next_idx);
            // For some reason, std::make_heap() is the fastest way to resort the heap. Probably needs testing.
            std::make_heap(heap.begin(), heap.end());
        }

        // Update next
        vis_node* next = node_list[curr->next_idx];
        if(next != nullptr){
            next->prev_idx = curr->prev_idx;
            next->area = effective_area(next->pt_idx, next->prev_idx, next->next_idx);
            std::make_heap(heap.begin(), heap.end());
        }

        areas[curr->pt_idx] = min_area;
        node_list[curr->pt_idx] = nullptr;
        delete curr;

    }

    // Clear node list, we're done with it
    node_list.clear();

    // This lambda is how we test whether or not to keep a point.
    auto use_point = [areas, pt, tolerance](const size_t& idx)->bool{
        assert(idx < areas.size());
        // Return true at front/back of path/areas
        if(idx == 0 || idx == areas.size() - 1){
            return true;
        }
        else{
            return areas[idx] > tolerance;
        }
    };

    // Use previously defined lambda to build results.
    for(size_t i = 0; i < pts.size(); ++i) {
        if(use_point(i)){
            results.push_back(pts[i]);
        }
    }

    // Check that results has at least two points
    assert(results.size() >= 2);

    // Return simplified vector of points
    return results;

}

}
