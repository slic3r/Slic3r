#ifndef slic3r_Point_hpp_
#define slic3r_Point_hpp_

#include "libslic3r.h"
#include <math.h>
#include <sstream>
#include <string>
#include <vector>

namespace Slic3r {

class Line;
class Linef;
class MultiPoint;
class Point;
class Point3;
class Pointf;
class Pointf3;
typedef Point Vector;
typedef Pointf Vectorf;
typedef Pointf3 Vectorf3;
using Vector3 = Point3;
typedef std::vector<Point> Points;
typedef std::vector<Point*> PointPtrs;
typedef std::vector<const Point*> PointConstPtrs;
typedef std::vector<Pointf> Pointfs;
typedef std::vector<Pointf3> Pointf3s;

using Point3s = std::vector<Point3>;

class Point
{
    public:
    coord_t x;
    coord_t y;
    constexpr Point(coord_t _x = 0, coord_t _y = 0): x(_x), y(_y) {};
    constexpr Point(int _x, int _y): x(_x), y(_y) {};
    #ifndef _WIN32
    constexpr Point(long long _x, long long _y): x(_x), y(_y) {};  // for Clipper
    #endif 
    Point(double x, double y);
    static constexpr Point new_scale(coordf_t x, coordf_t y) {
        return Point(scale_(x), scale_(y));
    };

    /// Scale and create a Point from a Pointf.
    static Point new_scale(Pointf p);
    bool operator==(const Point& rhs) const;
    bool operator!=(const Point& rhs) const { return !(*this == rhs); }
    Point& operator+=(const Point& rhs) { this->x += rhs.x; this->y += rhs.y; return *this; }
    Point& operator-=(const Point& rhs) { this->x -= rhs.x; this->y -= rhs.y; return *this; }
    std::string wkt() const;
    std::string dump_perl() const;
    void scale(double factor);
    void translate(double x, double y);
    void translate(const Vector &vector);
    void rotate(double angle);
    void rotate(double angle, const Point &center);
    Point rotated(double angle) const {
        Point p(*this);
        p.rotate(angle);
        return p;
    }
    Point rotated(double angle, const Point &center) const {
        Point p(*this);
        p.rotate(angle, center);
        return p;
    }
    bool coincides_with(const Point &point) const { return this->x == point.x && this->y == point.y; }
    bool coincides_with_epsilon(const Point &point) const;
    int nearest_point_index(const Points &points) const;
    int nearest_point_index(const PointConstPtrs &points) const;
    int nearest_point_index(const PointPtrs &points) const;
    size_t nearest_waypoint_index(const Points &points, const Point &point) const;
    bool nearest_point(const Points &points, Point* point) const;
    bool nearest_waypoint(const Points &points, const Point &dest, Point* point) const;
    double distance_to(const Point &point) const;
    double distance_to(const Line &line) const;
    double perp_distance_to(const Line &line) const;
    double ccw(const Point &p1, const Point &p2) const;
    double ccw(const Line &line) const;
    double ccw_angle(const Point &p1, const Point &p2) const;
    Point projection_onto(const MultiPoint &poly) const;
    Point projection_onto(const Line &line) const;
    Point negative() const;
    Vector vector_to(const Point &point) const;
    void align_to_grid(const Point &spacing, const Point &base = Point(0,0));
};

std::ostream& operator<<(std::ostream &stm, const Point &point);
Point operator+(const Point& point1, const Point& point2);
Point operator-(const Point& point1, const Point& point2);
Point operator*(double scalar, const Point& point2);

inline Points&
operator+=(Points &dst, const Points &src) {
    append_to(dst, src);
    return dst;
};

inline Points&
operator+=(Points &dst, const Point &p) {
    dst.push_back(p);
    return dst;
};

class Point3 : public Point
{
    public:
    coord_t z;
    explicit constexpr Point3(coord_t _x = 0, coord_t _y = 0, coord_t _z = 0): Point(_x, _y), z(_z) {};
    bool constexpr coincides_with(const Point3 &point3) const { return this->x == point3.x && this->y == point3.y && this->z == point3.z; }
};

std::ostream& operator<<(std::ostream &stm, const Pointf &pointf);

class Pointf
{
    public:
    coordf_t x;
    coordf_t y;
    explicit constexpr Pointf(coordf_t _x = 0, coordf_t _y = 0): x(_x), y(_y) {};
    static constexpr Pointf new_unscale(coord_t x, coord_t y) {
        return Pointf(unscale(x), unscale(y));
    };
    static constexpr Pointf new_unscale(const Point &p) {
        return Pointf(unscale(p.x), unscale(p.y));
    };

    // equality operator based on coincides_with_epsilon
    bool operator==(const Pointf& rhs) const;
    bool coincides_with_epsilon(const Pointf& rhs) const;
    Pointf& operator/=(const double& scalar); 

    std::string wkt() const;
    std::string dump_perl() const;
    void scale(double factor);
    void translate(double x, double y);
    void translate(const Vectorf &vector);
    void rotate(double angle);
    void rotate(double angle, const Pointf &center);
    Pointf negative() const;
    Vectorf vector_to(const Pointf &point) const;
};

Pointf operator+(const Pointf& point1, const Pointf& point2);
Pointf operator/(const Pointf& point1, const double& scalar);
inline coordf_t dot(const Pointf &v1, const Pointf &v2) { return v1.x * v2.x + v1.y * v2.y; }
inline coordf_t dot(const Pointf &v) { return v.x * v.x + v.y * v.y; }

std::ostream& operator<<(std::ostream &stm, const Pointf3 &pointf3);

class Pointf3 : public Pointf
{
    public:
    coordf_t z;
    explicit constexpr Pointf3(coordf_t _x = 0, coordf_t _y = 0, coordf_t _z = 0): Pointf(_x, _y), z(_z) {};
    static constexpr Pointf3 new_unscale(coord_t x, coord_t y, coord_t z) {
        return Pointf3(unscale(x), unscale(y), unscale(z));
    };
    void scale(double factor);
    void translate(const Vectorf3 &vector);
    void translate(double x, double y, double z);
    double distance_to(const Pointf3 &point) const;
    Pointf3 negative() const;
    Vectorf3 vector_to(const Pointf3 &point) const;
};

template <class T>
inline Points
to_points(const std::vector<T> &items)
{
    Points pp;
    for (typename std::vector<T>::const_iterator it = items.begin(); it != items.end(); ++it)
        append_to(pp, (Points)*it);
    return pp;
}

inline Points
scale(const std::vector<Pointf>&in ) {
    Points out; 
    for (const auto& p : in) {out.push_back(Point(scale_(p.x), scale_(p.y))); }
    return out;
}



}

// start Boost
#include <boost/version.hpp>
#include <boost/polygon/polygon.hpp>
namespace boost { namespace polygon {
#ifndef _WIN32
    template <>
    struct geometry_concept<coord_t> { typedef coordinate_concept type; };
#endif     
/* Boost.Polygon already defines a specialization for coordinate_traits<long> as of 1.60:
   https://github.com/boostorg/polygon/commit/0ac7230dd1f8f34cb12b86c8bb121ae86d3d9b97 */
#if BOOST_VERSION < 106000
    template <>
    struct coordinate_traits<coord_t> {
        typedef coord_t coordinate_type;
        typedef long double area_type;
        typedef long long manhattan_area_type;
        typedef unsigned long long unsigned_area_type;
        typedef long long coordinate_difference;
        typedef long double coordinate_distance;
    };
#endif

    template <>
    struct geometry_concept<Point> { typedef point_concept type; };
   
    template <>
    struct point_traits<Point> {
        typedef coord_t coordinate_type;
    
        static inline coordinate_type get(const Point& point, orientation_2d orient) {
            return (orient == HORIZONTAL) ? point.x : point.y;
        }
    };
    
    template <>
    struct point_mutable_traits<Point> {
        typedef coord_t coordinate_type;
        static inline void set(Point& point, orientation_2d orient, coord_t value) {
            if (orient == HORIZONTAL)
                point.x = value;
            else
                point.y = value;
        }
        static inline Point construct(coord_t x_value, coord_t y_value) {
            Point retval;
            retval.x = x_value;
            retval.y = y_value; 
            return retval;
        }
    };
} }
// end Boost

#endif
