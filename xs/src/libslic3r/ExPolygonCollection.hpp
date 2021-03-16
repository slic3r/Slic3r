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

    bool empty() const { return expolygons.empty(); }
    size_t size() const { return expolygons.size(); }
    ExPolygons::iterator begin() { return expolygons.begin(); }
    ExPolygons::iterator end() { return expolygons.end(); }
    const ExPolygons::const_iterator begin() const { return expolygons.cbegin(); }
    const ExPolygons::const_iterator end() const { return expolygons.cend(); }
    ExPolygons::const_iterator cbegin() const { return expolygons.cbegin();}
    ExPolygons::const_iterator cend() const { return expolygons.cend();}
    ExPolygon& at(size_t i) { return expolygons.at(i); }
    const ExPolygon& at(size_t i) const { return expolygons.at(i); }

};

inline ExPolygonCollection&
operator <<(ExPolygonCollection &coll, const ExPolygons &expolygons) {
    coll.append(expolygons);
    return coll;
};

}

#endif
