#include "../ClipperUtils.hpp"
#include "../ExPolygon.hpp"
#include "../Flow.hpp"
#include "../Surface.hpp"

#include "FillConcentric.hpp"

namespace Slic3r {

void
FillConcentric::_fill_surface_single(
    unsigned int                     thickness_layers,
    const direction_t               &direction, 
    ExPolygon                       &expolygon, 
    Polylines*                      polylines_out)
{
    // no rotation is supported for this infill pattern
    
    const coord_t min_spacing = scale_(this->min_spacing);
    coord_t distance = coord_t(min_spacing / this->density);
    
    if (this->density > 0.9999f && !this->dont_adjust) {
        BoundingBox bounding_box = expolygon.contour.bounding_box();
        distance = Flow::solid_spacing(bounding_box.size().x, distance);
        this->_spacing = unscale(distance);
    }

    Polygons loops = (Polygons)expolygon;
    Polygons last  = loops;
    while (!last.empty()) {
        last = offset2(last, -(distance + min_spacing/2), +min_spacing/2);
        append_to(loops, last);
    }

    // generate paths from the outermost to the innermost, to avoid
    // adhesion problems of the first central tiny loops
    loops = union_pt_chained(loops, false);
    
    // split paths using a nearest neighbor search
    const size_t iPathFirst = polylines_out->size();
    Point last_pos(0, 0);
    for (Polygons::const_iterator it_loop = loops.begin(); it_loop != loops.end(); ++ it_loop) {
        polylines_out->push_back(it_loop->split_at_index(last_pos.nearest_point_index(*it_loop)));
        last_pos = polylines_out->back().last_point();
    }

    // clip the paths to prevent the extruder from getting exactly on the first point of the loop
    // Keep valid paths only.
    size_t j = iPathFirst;
    for (size_t i = iPathFirst; i < polylines_out->size(); ++i) {
        (*polylines_out)[i].clip_end(this->loop_clipping);
        if ((*polylines_out)[i].is_valid()) {
            if (j < i)
                std::swap((*polylines_out)[j], (*polylines_out)[i]);
            ++j;
        }
    }
    if (j < polylines_out->size())
        polylines_out->erase(polylines_out->begin() + j, polylines_out->end());
    // TODO: return ExtrusionLoop objects to get better chained paths
}

} // namespace Slic3r
