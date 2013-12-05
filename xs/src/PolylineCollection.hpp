#ifndef slic3r_PolylineCollection_hpp_
#define slic3r_PolylineCollection_hpp_

#include <myinit.h>
#include "Polyline.hpp"

namespace Slic3r {

class PolylineCollection
{
    public:
    Polylines polylines;
    void scale(double factor);
    void translate(double x, double y);
    PolylineCollection* chained_path(bool no_reverse) const;
    PolylineCollection* chained_path_from(const Point* start_near, bool no_reverse) const;
    Point* leftmost_point() const;
};

}

#endif
