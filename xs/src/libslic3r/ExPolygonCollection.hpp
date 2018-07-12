#ifndef slic3r_ExPolygonCollection_hpp_
#define slic3r_ExPolygonCollection_hpp_

#include "libslic3r.h"
#include "ExPolygon.hpp"
#include "Line.hpp"
#include "Polyline.hpp"

namespace Slic3r {

class ExPolygonCollection;
typedef std::vector<ExPolygonCollection> ExPolygonCollections;

class ExPolygonCollection
{
    public:
    ExPolygons expolygons;
    
    ExPolygonCollection() {};
    ExPolygonCollection(const ExPolygon &expolygon);
    ExPolygonCollection(const ExPolygons &expolygons) : expolygons(expolygons) {};
    operator Points() const;
    operator Polygons() const;
    operator ExPolygons&();
    void scale(double factor);
    void translate(double x, double y);
    void translate(const Point offset) { translate(static_cast<coordf_t>(offset.x), static_cast<coordf_t>(offset.y)); }
    void translate(const Pointf offset) { translate(offset.x, offset.y); }
    void rotate(double angle, const Point &center);
    template <class T> bool contains(const T &item) const;
    bool contains_b(const Point &point) const;
    void simplify(double tolerance);
    Polygon convex_hull() const;
    Lines lines() const;
    Polygons contours() const;
    Polygons holes() const;
    void append(const ExPolygons &expolygons);
    void append(const ExPolygon &expolygons);

    /// Convenience function to iterate through all of the owned 
    /// ExPolygons and check if at least one contains the point.
    bool contains(const Point &point) const;

};

inline ExPolygonCollection&
operator <<(ExPolygonCollection &coll, const ExPolygons &expolygons) {
    coll.append(expolygons);
    return coll;
};

}

#endif
