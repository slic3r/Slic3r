#include "../ClipperUtils.hpp"
#include "../PolylineCollection.hpp"
#include "../Surface.hpp"

#include "Fill3DHoneycomb.hpp"

namespace Slic3r {

/*
Creates a contiguous sequence of points at a specified height that make
up a horizontal slice of the edges of a space filling truncated
octahedron tesselation. The octahedrons are oriented so that the
square faces are in the horizontal plane with edges parallel to the X
and Y axes.

Credits: David Eccles (gringer).
*/

// Generate an array of points that are in the same direction as the
// basic printing line (i.e. Y points for columns, X points for rows)
// Note: a negative offset only causes a change in the perpendicular
// direction
static std::vector<coordf_t>
colinearPoints(const coordf_t offset, const size_t baseLocation, size_t gridLength)
{
    const coordf_t offset2 = std::abs(offset / coordf_t(2.));
    std::vector<coordf_t> points;
    points.push_back(baseLocation - offset2);
    for (size_t i = 0; i < gridLength; ++i) {
        points.push_back(baseLocation + i + offset2);
        points.push_back(baseLocation + i + 1 - offset2);
    }
    points.push_back(baseLocation + gridLength + offset2);
    return points;
}

// Generate an array of points for the dimension that is perpendicular to
// the basic printing line (i.e. X points for columns, Y points for rows)
static std::vector<coordf_t>
perpendPoints(const coordf_t offset, const size_t baseLocation, size_t gridLength)
{
    const coordf_t offset2 = offset / coordf_t(2.);
    coord_t  side    = 2 * (baseLocation & 1) - 1;
    std::vector<coordf_t> points;
    points.push_back(baseLocation - offset2 * side);
    for (size_t i = 0; i < gridLength; ++i) {
        side = 2*((i+baseLocation) & 1) - 1;
        points.push_back(baseLocation + offset2 * side);
        points.push_back(baseLocation + offset2 * side);
    }
    points.push_back(baseLocation - offset2 * side);
    return points;
}

template<typename T>
static inline T
clamp(T low, T high, T x)
{
    return std::max<T>(low, std::min<T>(high, x));
}

// Trims an array of points to specified rectangular limits. Point
// components that are outside these limits are set to the limits.
static inline void
trim(Pointfs &pts, coordf_t minX, coordf_t minY, coordf_t maxX, coordf_t maxY)
{
    for (Pointfs::iterator it = pts.begin(); it != pts.end(); ++ it) {
        it->x = clamp(minX, maxX, it->x);
        it->y = clamp(minY, maxY, it->y);
    }
}

static inline Pointfs
zip(const std::vector<coordf_t> &x, const std::vector<coordf_t> &y)
{
    assert(x.size() == y.size());
    Pointfs out;
    out.reserve(x.size());
    for (size_t i = 0; i < x.size(); ++ i)
        out.push_back(Pointf(x[i], y[i]));
    return out;
}

// Generate a set of curves (array of array of 2d points) that describe a
// horizontal slice of a truncated regular octahedron with edge length `gridSize`.
// curveType specifies which lines to print, 1 for vertical lines
// (columns), 2 for horizontal lines (rows), and 3 for both.
static std::vector<Pointfs>
makeNormalisedGrid(coordf_t z, coord_t gridSize, size_t gridWidth, size_t gridHeight){
  coordf_t ch = coordf_t(4*gridSize / std::sqrt(2)); // z-height of a single octahedron cycle
  coordf_t pl = coordf_t(gridSize * 4); // planar length of the printed curve unit
  // sawtooth wave function
  coordf_t dl = coordf_t((std::abs(std::fmod(z * 4, 4*ch) - (2*ch)) - ch) / 2); // diagonal length
  // work out whether to display horizontal (0) or vertical (1) pattern
  size_t dc = int(std::fmod(z*2, 2*ch) / ch); // diagonal cycle
  
  coordf_t dox = coordf_t(dl / sqrt(2)); // diagonal x offset
  coordf_t doy = coordf_t(std::abs(dox)); // diagonal y offset
  coordf_t ol = coordf_t((pl - 2*doy)/2); // orthogonal length
  
  std::vector<Pointfs> points;
  if(dc == 0){
    points.push_back(Pointfs());
    Pointfs &newPoints = points.back();
    for (coordf_t xofs = 0; xofs <= gridWidth; xofs+= gridSize) {
      for(coordf_t yofs = 0; yofs <= gridHeight; yofs += pl){
	newPoints.push_back(Pointf(coordf_t(dox/2+xofs), coordf_t(0+yofs)));
	newPoints.push_back(Pointf(coordf_t(dox/2+xofs), coordf_t(ol/2+yofs)));
	newPoints.push_back(Pointf(coordf_t(-dox/2+xofs), coordf_t(ol/2+doy+yofs)));
	newPoints.push_back(Pointf(coordf_t(-dox/2+xofs), coordf_t(ol*1.5+doy+yofs)));
	newPoints.push_back(Pointf(coordf_t(dox/2+xofs), coordf_t(ol*1.5+doy*2+yofs)));
      }
    }
    trim(newPoints, coordf_t(0.), coordf_t(0.), coordf_t(gridWidth), coordf_t(gridHeight));
  } else {
    points.push_back(Pointfs());
    Pointfs &newPoints = points.back();
    for(coordf_t yofs = 0; yofs <= gridWidth; yofs+= gridSize){
      for(coordf_t xofs = 0; xofs <= gridHeight; xofs += pl){
	newPoints.push_back(Pointf(coordf_t(0+xofs), coordf_t(dox/2+yofs)));
	newPoints.push_back(Pointf(coordf_t(ol/2+xofs), coordf_t(dox/2+yofs)));
	newPoints.push_back(Pointf(coordf_t(ol/2+doy+xofs), coordf_t(-dox/2+yofs)));
	newPoints.push_back(Pointf(coordf_t(ol*1.5+doy+xofs), coordf_t(-dox/2+yofs)));
	newPoints.push_back(Pointf(coordf_t(ol*1.5+doy*2+xofs), coordf_t(dox/2+yofs)));
      }
    }
    trim(newPoints, coordf_t(0.), coordf_t(0.), coordf_t(gridWidth), coordf_t(gridHeight));
  }
  return points;
}

// Generate a set of curves (array of array of 2d points) that describe a
// horizontal slice of a truncated regular octahedron with a specified
// grid square size.
static Polylines
makeGrid(coord_t z, coord_t gridSize, size_t gridWidth, size_t gridHeight, size_t curveType)
{
    std::vector<Pointfs> polylines = makeNormalisedGrid(z, gridSize, gridWidth, gridHeight);
    Polylines result;
    result.reserve(polylines.size());
    for (std::vector<Pointfs>::const_iterator it_polylines = polylines.begin(); it_polylines != polylines.end(); ++ it_polylines) {
        result.push_back(Polyline());
        Polyline &polyline = result.back();
        for (Pointfs::const_iterator it = it_polylines->begin(); it != it_polylines->end(); ++ it)
            polyline.points.push_back(Point(coord_t(it->x), coord_t(it->y)));
    }
    return result;
}

void
Fill3DHoneycomb::_fill_surface_single(
    unsigned int                    thickness_layers,
    const direction_t               &direction, 
    ExPolygon                       &expolygon, 
    Polylines*                      polylines_out)
{
    // no rotation is supported for this infill pattern
    BoundingBox bb = expolygon.contour.bounding_box();
    const coord_t distance = coord_t(scale_(this->min_spacing) / this->density);

    // align bounding box to a multiple of our honeycomb grid module
    // (a module is 2*$distance since one $distance half-module is 
    // growing while the other $distance half-module is shrinking)
    bb.min.align_to_grid(Point(2*distance, 2*distance));
    
    // generate pattern
    Polylines polylines = makeGrid(
        scale_(this->z),
        distance,
        ceil(bb.size().x / distance) + 1,
        ceil(bb.size().y / distance) + 1,
        ((this->layer_id/thickness_layers) % 2) + 1
    );
    
    // move pattern in place
    for (Polylines::iterator it = polylines.begin(); it != polylines.end(); ++ it)
        it->translate(bb.min.x, bb.min.y);

    // clip pattern to boundaries
    polylines = intersection_pl(polylines, (Polygons)expolygon);

    // connect lines
    if (!this->dont_connect && !polylines.empty()) { // prevent calling leftmost_point() on empty collections
        ExPolygon expolygon_off;
        {
            ExPolygons expolygons_off = offset_ex(expolygon, SCALED_EPSILON);
            if (!expolygons_off.empty()) {
                // When expanding a polygon, the number of islands could only shrink. Therefore the offset_ex shall generate exactly one expanded island for one input island.
                assert(expolygons_off.size() == 1);
                std::swap(expolygon_off, expolygons_off.front());
            }
        }
        Polylines chained = PolylineCollection::chained_path_from(
            STDMOVE(polylines),
            PolylineCollection::leftmost_point(polylines),
            false // reverse allowed
        );
        bool first = true;
        for (Polylines::iterator it_polyline = chained.begin(); it_polyline != chained.end(); ++ it_polyline) {
            if (!first) {
                // Try to connect the lines.
                Points &pts_end = polylines_out->back().points;
                const Point &first_point = it_polyline->points.front();
                const Point &last_point = pts_end.back();
                // TODO: we should also check that both points are on a fill_boundary to avoid 
                // connecting paths on the boundaries of internal regions
                if (first_point.distance_to(last_point) <= 1.5 * distance && 
                    expolygon_off.contains(Line(last_point, first_point))) {
                    // Append the polyline.
                    pts_end.insert(pts_end.end(), it_polyline->points.begin(), it_polyline->points.end());
                    continue;
                }
            }
            // The lines cannot be connected.
            #if SLIC3R_CPPVER >= 11
                polylines_out->push_back(std::move(*it_polyline));
            #else
                polylines_out->push_back(Polyline());
                std::swap(polylines_out->back(), *it_polyline);
            #endif
            first = false;
        }
    }
}

} // namespace Slic3r
