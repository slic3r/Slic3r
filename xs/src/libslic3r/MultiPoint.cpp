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

bool
MultiPoint::first_intersection(const Line& line, Point* intersection) const
{
    bool   found = false;
    double dmin  = 0.;
    for (const Line &l : this->lines()) {
        Point ip;
        if (l.intersection(line, &ip)) {
            if (! found) {
                found = true;
                dmin = ip.distance_to(line.a);
                *intersection = ip;
            } else {
                double d = ip.distance_to(line.a);
                if (d < dmin) {
                    dmin = d;
                    *intersection = ip;
                }
            }
        }
    }
    return found;
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

}
